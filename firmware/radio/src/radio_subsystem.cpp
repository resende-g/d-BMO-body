#include "dbmo/radio/radio_subsystem.h"

#include "dbmo/core/log.h"
#include "dbmo/radio/cc1101_regs.h"

namespace dbmo::radio {

namespace {
constexpr char kTag[] = "radio";
}

const char *toString(RadioState state) {
  switch (state) {
    case RadioState::Off: return "Off";
    case RadioState::Idle: return "Idle";
    case RadioState::Receiving: return "Receiving";
    case RadioState::Transmitting: return "Transmitting";
    case RadioState::Fault: return "Fault";
  }
  return "?";
}

RadioSubsystem::RadioSubsystem(Cc1101 &chip, EventBus &bus, ConfigStore &config, IClock &clock)
    : chip_(chip), bus_(bus), config_(config), clock_(clock) {}

Status RadioSubsystem::registerConfig(ConfigStore &config) {
  // Transmissão desabilitada e sem faixa: o usuário habilita e informa a faixa em que
  // tem autorização para operar.
  DBMO_TRY(config.define(ConfigSpec::boolean(config_key::kTxEnabled, false)));
  DBMO_TRY(config.define(ConfigSpec::integer(config_key::kTxMinKhz, 0, 0, 928000)));
  DBMO_TRY(config.define(ConfigSpec::integer(config_key::kTxMaxKhz, 0, 0, 928000)));
  DBMO_TRY(config.define(ConfigSpec::integer(config_key::kTxMsPerMin, 600, 0, 6000)));
  // 0x12 é o menor ajuste da tabela de potência do datasheet para 433 MHz (valor não
  // reconferido nesta sessão; ver README do módulo).
  DBMO_TRY(config.define(ConfigSpec::integer(config_key::kTxPaTable, 0x12, 0, 255)));
  DBMO_TRY(config.define(ConfigSpec::integer(config_key::kXtalKhz, 26000, 26000, 27000)));
  return Status::Ok;
}

void RadioSubsystem::applyConfig() {
  TxLimits limits;
  limits.enabled = config_.getBool(config_key::kTxEnabled).valueOr(false);
  limits.minHz = static_cast<uint32_t>(config_.getInt(config_key::kTxMinKhz).valueOr(0)) * 1000u;
  limits.maxHz = static_cast<uint32_t>(config_.getInt(config_key::kTxMaxKhz).valueOr(0)) * 1000u;
  limits.maxAirtimeMsPerMinute =
      static_cast<uint32_t>(config_.getInt(config_key::kTxMsPerMin).valueOr(600));
  policy_.setLimits(limits);
  const uint8_t paTable =
      static_cast<uint8_t>(config_.getInt(config_key::kTxPaTable).valueOr(0x12));
  const uint32_t xtalHz =
      static_cast<uint32_t>(config_.getInt(config_key::kXtalKhz).valueOr(26000)) * 1000u;
  const bool changed = paTable != paTable_ || xtalHz != chip_.xtalHz();
  paTable_ = paTable;
  chip_.setXtalHz(xtalHz);
  // Potência e cristal entram nos registradores: com o rádio em uso, regrava o chip para
  // que ele não fique operando com valores diferentes dos que a política avalia.
  if (changed && initialized_ && state_ != RadioState::Transmitting) reconfigureChip();
}

void RadioSubsystem::reconfigureChip() {
  if (state_ == RadioState::Fault) return;  // bringUp() regrava tudo ao recuperar
  Status status = chip_.configure(radioConfig_, paTable_);
  if (isOk(status)) status = state_ == RadioState::Receiving ? chip_.startReceive() : chip_.sleep();
  if (!isOk(status)) failNow(status);
}

// Estado do chip desconhecido: vai para Fault, de onde bringUp() reinicia e regrava tudo.
void RadioSubsystem::failNow(Status status) {
  errors_ = kFaultThreshold - 1;
  noteError(status, clock_.nowMs());
}

void RadioSubsystem::setState(RadioState state) {
  if (state_ == state) return;
  state_ = state;
  (void)bus_.publish(radio_event::kStateChanged, static_cast<uint32_t>(state));
}

Status RadioSubsystem::bringUp() {
  DBMO_TRY(chip_.reset());
  const Result<ChipId> id = chip_.identify();
  if (!id.ok()) return id.status();
  chipId_ = id.value();
  if (chipId_.version != cc1101::kVersionCurrent && chipId_.version != cc1101::kVersionOlder)
    DBMO_LOGW(kTag, "VERSION 0x%02X fora das documentadas (0x04, 0x14)",
              static_cast<unsigned>(chipId_.version));
  DBMO_TRY(chip_.configure(radioConfig_, paTable_));
  errors_ = 0;
  if (wantReceive_) {
    DBMO_TRY(chip_.startReceive());
    setState(RadioState::Receiving);
  } else {
    DBMO_TRY(chip_.sleep());
    setState(RadioState::Idle);
  }
  return Status::Ok;
}

Status RadioSubsystem::init() {
  applyConfig();
  initialized_ = false;
  wantReceive_ = false;
  queueHead_ = queueCount_ = 0;
  state_ = RadioState::Off;
  const Status status = bringUp();
  if (!isOk(status)) {
    // Timeout esperando o "pronto" também é ausência de chip.
    const Status reported = status == Status::Timeout ? Status::HardwareAbsent : status;
    DBMO_LOGW(kTag, "CC1101 nao respondeu: %s", toString(status));
    return reported;
  }
  initialized_ = true;
  lastPollMs_ = clock_.nowMs();
  DBMO_LOGI(kTag, "CC1101 PARTNUM=0x%02X VERSION=0x%02X; transmissao %s",
            static_cast<unsigned>(chipId_.partnum), static_cast<unsigned>(chipId_.version),
            policy_.limits().enabled ? "habilitada na configuracao" : "desabilitada");
  return Status::Ok;
}

Status RadioSubsystem::shutdown() {
  wantReceive_ = false;
  Status status = Status::Ok;
  if (initialized_ && state_ != RadioState::Fault) status = chip_.sleep();
  initialized_ = false;
  setState(RadioState::Off);
  return status;
}

Status RadioSubsystem::configure(const RadioConfig &config) {
  DBMO_TRY(validate(config, chip_.xtalHz()));
  if (state_ == RadioState::Transmitting) return Status::Busy;
  if (!initialized_ || state_ == RadioState::Fault) {
    radioConfig_ = config;  // vale no próximo bringUp
    return Status::Ok;
  }
  Status status = chip_.configure(config, paTable_);
  if (isOk(status)) {
    radioConfig_ = config;  // só passa a valer depois que o chip aceitou e conferiu
    status = state_ == RadioState::Receiving ? chip_.startReceive() : chip_.sleep();
  }
  // Falha no meio da gravação: os registradores podem estar misturados. O rádio vai para
  // Fault e é reiniciado com a configuração anterior, que continua sendo a válida.
  if (!isOk(status)) failNow(status);
  return status;
}

Status RadioSubsystem::startReceive() {
  if (!initialized_) return Status::NotInitialized;
  if (state_ == RadioState::Transmitting) return Status::Busy;
  wantReceive_ = true;
  if (state_ == RadioState::Fault) return Status::Ok;  // retoma quando o chip voltar
  DBMO_TRY(chip_.startReceive());
  setState(RadioState::Receiving);
  return Status::Ok;
}

Status RadioSubsystem::stop() {
  if (!initialized_) return Status::NotInitialized;
  wantReceive_ = false;
  if (state_ == RadioState::Fault) return Status::Ok;
  DBMO_TRY(chip_.sleep());
  setState(RadioState::Idle);
  return Status::Ok;
}

bool RadioSubsystem::takePacket(RadioPacket *out) {
  if (queueCount_ == 0 || out == nullptr) return false;
  *out = queue_[queueHead_];
  queueHead_ = (queueHead_ + 1) % kQueueSize;
  --queueCount_;
  return true;
}

Status RadioSubsystem::transmit(const uint8_t *data, size_t length) {
  if (!initialized_) return Status::NotInitialized;
  if (state_ == RadioState::Fault) return Status::HardwareAbsent;
  if (state_ == RadioState::Transmitting) return Status::Busy;
  if (data == nullptr || length == 0 || length > radioConfig_.maxPayload)
    return Status::InvalidArgument;

  const uint32_t now = clock_.nowMs();
  const uint32_t airtime = airtimeMs(radioConfig_, length);
  const TxDecision decision =
      policy_.authorize(radioConfig_.frequencyHz, occupiedBandwidthHz(radioConfig_), airtime, now);
  lastDenial_ = decision.denial;
  if (!decision.allowed()) {
    ++counters_.denied;
    DBMO_LOGW(kTag, "transmissao negada: %s", toString(decision.denial));
    (void)bus_.publish(radio_event::kTxDenied, static_cast<uint32_t>(decision.denial));
    return Status::NotPermitted;
  }

  const Status status = chip_.startTransmit(decision.grant, data, length);
  if (!isOk(status)) {
    noteError(status, now);
    return status;
  }
  txStartMs_ = now;
  txTimeoutMs_ = airtime + kTxMarginMs;
  setState(RadioState::Transmitting);
  return Status::Ok;
}

void RadioSubsystem::noteError(Status status, uint32_t nowMs) {
  if (errors_ < kFaultThreshold) ++errors_;
  if (errors_ < kFaultThreshold || state_ == RadioState::Fault) return;
  DBMO_LOGE(kTag, "CC1101 parou de responder: %s", toString(status));
  lastRetryMs_ = nowMs;
  setState(RadioState::Fault);
  (void)bus_.publish(radio_event::kFault, static_cast<uint32_t>(status));
}

void RadioSubsystem::pollReceive(uint32_t nowMs) {
  RadioPacket packet;
  const Result<RxPoll> poll = chip_.pollReceive(&packet);
  if (!poll.ok()) {
    noteError(poll.status(), nowMs);
    return;
  }
  errors_ = 0;
  switch (poll.value()) {
    case RxPoll::Nothing: return;
    case RxPoll::Packet:
      ++counters_.packets;
      if (queueCount_ == kQueueSize) {
        ++counters_.droppedPackets;
      } else {
        queue_[(queueHead_ + queueCount_) % kQueueSize] = packet;
        ++queueCount_;
        (void)bus_.publish(radio_event::kPacketReceived, packet.length,
                           static_cast<uint16_t>(packet.rssiDbm));
      }
      break;
    case RxPoll::CrcError: ++counters_.crcErrors; break;
    case RxPoll::Overflow: ++counters_.overflows; break;
    case RxPoll::Malformed: ++counters_.malformed; break;
    case RxPoll::Stopped: break;
  }
  const Status restarted = chip_.startReceive();
  if (!isOk(restarted)) noteError(restarted, nowMs);
}

void RadioSubsystem::pollTransmit(uint32_t nowMs) {
  const Result<TxPoll> poll = chip_.pollTransmit();
  Status failure = Status::Ok;
  if (!poll.ok()) {
    failure = poll.status();
  } else if (poll.value() == TxPoll::Underflow) {
    failure = Status::IoError;
  } else if (poll.value() == TxPoll::Busy) {
    if (elapsedMs(nowMs, txStartMs_) < txTimeoutMs_) return;
    // Passou do tempo previsto: corta a transmissão em vez de deixá-la no ar.
    (void)chip_.idle();
    failure = Status::Timeout;
  }

  if (isOk(failure)) {
    ++counters_.transmitted;
    (void)bus_.publish(radio_event::kTxDone);
  } else {
    DBMO_LOGW(kTag, "transmissao falhou: %s", toString(failure));
    (void)bus_.publish(radio_event::kTxFailed, static_cast<uint32_t>(failure));
  }

  Status next = Status::Ok;
  if (wantReceive_) {
    next = chip_.startReceive();
    if (isOk(next)) setState(RadioState::Receiving);
  } else {
    next = chip_.sleep();
    if (isOk(next)) setState(RadioState::Idle);
  }
  if (!isOk(next)) {
    errors_ = kFaultThreshold - 1;  // sem estado conhecido do chip: trata como falha
    noteError(next, nowMs);
  }
}

void RadioSubsystem::tick(uint32_t nowMs) {
  if (!initialized_) return;

  if (state_ == RadioState::Fault) {
    if (elapsedMs(nowMs, lastRetryMs_) < kRetryPeriodMs) return;
    lastRetryMs_ = nowMs;
    if (isOk(bringUp())) DBMO_LOGI(kTag, "CC1101 voltou a responder");
    return;
  }

  if (elapsedMs(nowMs, lastPollMs_) < kPollPeriodMs) return;
  lastPollMs_ = nowMs;
  if (state_ == RadioState::Receiving) {
    pollReceive(nowMs);
  } else if (state_ == RadioState::Transmitting) {
    pollTransmit(nowMs);
  }
}

}  // namespace dbmo::radio
