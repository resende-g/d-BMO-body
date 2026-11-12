#include "dbmo/infrared/infrared_subsystem.h"

#include "dbmo/core/log.h"
#include "dbmo/infrared/ir_codec.h"

namespace dbmo::infrared {

namespace {

constexpr char kTag[] = "infrared";

uint32_t packHeader(const IrCode &code) {
  return static_cast<uint32_t>(code.protocol) | (static_cast<uint32_t>(code.bits) << 8);
}
uint32_t packValue(const IrCode &code) {
  return (static_cast<uint32_t>(code.address) << 16) | code.command;
}

}  // namespace

const char *toString(IrTxDenial denial) {
  switch (denial) {
    case IrTxDenial::None: return "None";
    case IrTxDenial::Disabled: return "Disabled";
    case IrTxDenial::NoTransmitter: return "NoTransmitter";
    case IrTxDenial::Busy: return "Busy";
    case IrTxDenial::InvalidSignal: return "InvalidSignal";
    case IrTxDenial::TooLong: return "TooLong";
    case IrTxDenial::BudgetExceeded: return "BudgetExceeded";
  }
  return "?";
}

InfraredSubsystem::InfraredSubsystem(EventBus &bus, ConfigStore &config, IClock &clock)
    : bus_(bus), config_(config), clock_(clock) {}

Status InfraredSubsystem::registerConfig(ConfigStore &config) {
  DBMO_TRY(config.define(ConfigSpec::boolean(config_key::kRxEnabled, true)));
  DBMO_TRY(config.define(ConfigSpec::boolean(config_key::kTxEnabled, false)));
  // 6000 ms por minuto = 10 %: folga para uso normal de controle remoto, insuficiente
  // para emissão contínua.
  DBMO_TRY(config.define(ConfigSpec::integer(config_key::kTxMsPerMin, 6000, 0, 30000)));
  return Status::Ok;
}

void InfraredSubsystem::applyConfig() {
  rxEnabled_ = config_.getBool(config_key::kRxEnabled).valueOr(true);
  txEnabled_ = config_.getBool(config_key::kTxEnabled).valueOr(false);
  budgetMs_ = static_cast<uint32_t>(config_.getInt(config_key::kTxMsPerMin).valueOr(6000));
}

Status InfraredSubsystem::init() {
  applyConfig();
  initialized_ = false;
  hasLast_ = hasLearned_ = learning_ = transmitting_ = false;
  rxErrors_ = 0;
  rxFaulted_ = false;
  usedUs_ = 0;
  if (receiver_ == nullptr && transmitter_ == nullptr) return Status::HardwareAbsent;
  if (receiver_ != nullptr) DBMO_TRY(receiver_->begin());
  if (transmitter_ != nullptr) DBMO_TRY(transmitter_->begin());
  windowStartMs_ = clock_.nowMs();
  initialized_ = true;
  DBMO_LOGI(kTag, "receptor: %s; emissor: %s; envio %s", receiver_ != nullptr ? "sim" : "nao",
            transmitter_ != nullptr ? "sim" : "nao", txEnabled_ ? "habilitado" : "desabilitado");
  return Status::Ok;
}

Status InfraredSubsystem::shutdown() {
  learning_ = false;
  initialized_ = false;
  return Status::Ok;
}

bool InfraredSubsystem::lastReceived(StoredCode *out) const {
  if (!hasLast_ || out == nullptr) return false;
  *out = last_;
  return true;
}

bool InfraredSubsystem::learned(StoredCode *out) const {
  if (!hasLearned_ || out == nullptr) return false;
  *out = learnedCode_;
  return true;
}

Status InfraredSubsystem::startLearning(uint32_t timeoutMs) {
  if (!initialized_) return Status::NotInitialized;
  if (receiver_ == nullptr || !rxEnabled_) return Status::Unsupported;
  learning_ = true;
  hasLearned_ = false;
  learnStartMs_ = clock_.nowMs();
  learnTimeoutMs_ = timeoutMs;
  return Status::Ok;
}

void InfraredSubsystem::cancelLearning() { learning_ = false; }

void InfraredSubsystem::handleReceived(const RawTiming &timing, uint32_t nowMs) {
  if (timing.count == 0) return;

  // Repetição do NEC: só vale logo depois de um quadro NEC completo.
  if (isNecRepeat(timing)) {
    if (hasLast_ && !last_.isRaw && last_.code.protocol == IrProtocol::Nec &&
        elapsedMs(nowMs, lastCodeMs_) <= kRepeatWindowMs) {
      lastCodeMs_ = nowMs;
      (void)bus_.publish(infrared_event::kRepeat, packHeader(last_.code), packValue(last_.code));
    }
    return;
  }

  StoredCode received;
  const Status status = decode(timing, &received.code);
  if (status == Status::CrcError) return;  // quadro NEC corrompido: descarta
  received.isRaw = !isOk(status);
  if (received.isRaw) received.raw = timing;

  if (learning_) {
    learning_ = false;
    learnedCode_ = received;
    // Guarda sempre o sinal bruto junto: permite reenviar exatamente o que foi captado.
    learnedCode_.raw = timing;
    hasLearned_ = true;
    (void)bus_.publish(infrared_event::kLearned, received.isRaw ? 1 : 0);
  }

  if (received.isRaw) {
    last_ = received;
    hasLast_ = true;
    (void)bus_.publish(infrared_event::kRawReceived, timing.count);
    return;
  }

  // Controles reenviam o quadro inteiro enquanto a tecla está apertada (Sony sempre).
  const bool repeat = hasLast_ && !last_.isRaw && last_.code == received.code &&
                      elapsedMs(nowMs, lastCodeMs_) <= kRepeatWindowMs;
  last_ = received;
  hasLast_ = true;
  lastCodeMs_ = nowMs;
  (void)bus_.publish(repeat ? infrared_event::kRepeat : infrared_event::kCodeReceived,
                     packHeader(received.code), packValue(received.code));
}

void InfraredSubsystem::rollBudget(uint32_t nowMs) {
  if (elapsedMs(nowMs, windowStartMs_) >= kBudgetWindowMs) {
    windowStartMs_ = nowMs;
    usedUs_ = 0;
  }
}

Status InfraredSubsystem::deny(IrTxDenial denial) {
  lastDenial_ = denial;
  DBMO_LOGW(kTag, "envio negado: %s", toString(denial));
  (void)bus_.publish(infrared_event::kTxDenied, static_cast<uint32_t>(denial));
  return Status::NotPermitted;
}

Status InfraredSubsystem::sendRaw(const RawTiming &timing) {
  if (!initialized_) return Status::NotInitialized;
  if (!txEnabled_) return deny(IrTxDenial::Disabled);
  if (transmitter_ == nullptr) return deny(IrTxDenial::NoTransmitter);
  if (transmitting_) return deny(IrTxDenial::Busy);
  if (timing.count == 0 || timing.count > RawTiming::kMaxEntries ||
      timing.carrierHz < kMinCarrierHz || timing.carrierHz > kMaxCarrierHz)
    return deny(IrTxDenial::InvalidSignal);
  for (uint16_t i = 0; i < timing.count; ++i)
    if (timing.durationsUs[i] == 0) return deny(IrTxDenial::InvalidSignal);

  const uint32_t durationUs = timing.totalUs();
  if (durationUs > kMaxSendUs) return deny(IrTxDenial::TooLong);
  const uint32_t now = clock_.nowMs();
  rollBudget(now);
  if (static_cast<uint64_t>(usedUs_) + durationUs > static_cast<uint64_t>(budgetMs_) * 1000)
    return deny(IrTxDenial::BudgetExceeded);

  const Status status = transmitter_->send(timing);
  if (!isOk(status)) {
    (void)bus_.publish(infrared_event::kFault, static_cast<uint32_t>(status));
    return status;
  }
  usedUs_ += durationUs;
  transmitting_ = true;
  lastDenial_ = IrTxDenial::None;
  return Status::Ok;
}

Status InfraredSubsystem::send(const IrCode &code) {
  if (!initialized_) return Status::NotInitialized;
  const Status encoded = encode(code, &scratch_);
  if (!isOk(encoded)) return encoded;
  return sendRaw(scratch_);
}

Status InfraredSubsystem::send(const StoredCode &code) {
  return code.isRaw ? sendRaw(code.raw) : send(code.code);
}

void InfraredSubsystem::tick(uint32_t nowMs) {
  if (!initialized_) return;

  if (transmitting_ && !transmitter_->busy()) {
    transmitting_ = false;
    (void)bus_.publish(infrared_event::kTxDone);
  }

  if (receiver_ != nullptr && rxEnabled_) {
    const Result<bool> ready = receiver_->read(&scratch_);
    if (!ready.ok()) {
      if (rxErrors_ < kFaultThreshold) ++rxErrors_;
      if (rxErrors_ >= kFaultThreshold && !rxFaulted_) {
        rxFaulted_ = true;
        DBMO_LOGE(kTag, "receptor falhou: %s", toString(ready.status()));
        (void)bus_.publish(infrared_event::kFault, static_cast<uint32_t>(ready.status()));
      }
    } else {
      rxErrors_ = 0;
      rxFaulted_ = false;
      // O que chega durante o próprio envio é reflexo do emissor.
      if (ready.value() && !transmitting_) handleReceived(scratch_, nowMs);
    }
  }

  if (learning_ && elapsedMs(nowMs, learnStartMs_) >= learnTimeoutMs_) {
    learning_ = false;
    (void)bus_.publish(infrared_event::kLearnTimeout);
  }
}

}  // namespace dbmo::infrared
