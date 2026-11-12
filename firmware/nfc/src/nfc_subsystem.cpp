#include "dbmo/nfc/nfc_subsystem.h"

#include "dbmo/core/crc.h"
#include "dbmo/core/log.h"

namespace dbmo::nfc {

namespace {
constexpr char kTag[] = "nfc";
}

const char *toString(NfcState state) {
  switch (state) {
    case NfcState::Off: return "Off";
    case NfcState::Idle: return "Idle";
    case NfcState::Scanning: return "Scanning";
    case NfcState::Fault: return "Fault";
  }
  return "?";
}

NfcSubsystem::NfcSubsystem(Pn532 &chip, EventBus &bus, ConfigStore &config, IClock &clock)
    : chip_(chip), bus_(bus), config_(config), clock_(clock) {}

Status NfcSubsystem::registerConfig(ConfigStore &config) {
  DBMO_TRY(config.define(ConfigSpec::boolean(config_key::kScanEnabled, false)));
  DBMO_TRY(config.define(ConfigSpec::integer(config_key::kScanPeriodMs, 250, 100, 5000)));
  return Status::Ok;
}

void NfcSubsystem::setState(NfcState state) {
  if (state_ == state) return;
  state_ = state;
  (void)bus_.publish(nfc_event::kStateChanged, static_cast<uint32_t>(state));
}

Status NfcSubsystem::runBlocking() {
  for (uint32_t waited = 0; waited <= kInitCommandTimeoutMs; ++waited) {
    const CommandState state = chip_.poll();
    if (state == CommandState::Done) return Status::Ok;
    if (state == CommandState::Failed) return chip_.failure();
    clock_.delayMs(1);
  }
  chip_.abort();
  return Status::Timeout;
}

Status NfcSubsystem::bringUp() {
  chip_.abort();
  DBMO_TRY(chip_.probe());

  DBMO_TRY(chip_.startGetFirmwareVersion());
  DBMO_TRY(runBlocking());
  DBMO_TRY(Pn532::parseFirmwareVersion(chip_.response(), chip_.responseLength(), &firmware_));
  if (firmware_.ic != kExpectedIc) {
    DBMO_LOGW(kTag, "chip no endereco do PN532 respondeu IC=0x%02X (esperado 0x32)",
              static_cast<unsigned>(firmware_.ic));
    return Status::HardwareAbsent;
  }

  DBMO_TRY(chip_.startSamConfiguration());
  DBMO_TRY(runBlocking());
  // Sem isto, a busca por etiqueta só terminaria quando uma etiqueta aparecesse.
  DBMO_TRY(chip_.startSetPassiveActivationRetries(1));
  DBMO_TRY(runBlocking());
  DBMO_TRY(chip_.startRfField(false));
  DBMO_TRY(runBlocking());

  errors_ = 0;
  step_ = Step::None;
  fieldOffPending_ = false;  // bringUp() termina com o campo desligado
  return Status::Ok;
}

Status NfcSubsystem::init() {
  scanPeriodMs_ = static_cast<uint32_t>(config_.getInt(config_key::kScanPeriodMs).valueOr(250));
  initialized_ = false;
  tagPresent_ = false;
  misses_ = 0;
  state_ = NfcState::Off;
  wantScan_ = config_.getBool(config_key::kScanEnabled).valueOr(false);

  const Status status = bringUp();
  if (!isOk(status)) {
    DBMO_LOGW(kTag, "PN532 nao respondeu: %s", toString(status));
    // Para o System, qualquer falha de identificação é ausência do periférico.
    return status == Status::IoError ? status : Status::HardwareAbsent;
  }
  initialized_ = true;
  lastScanMs_ = clock_.nowMs();
  DBMO_LOGI(kTag, "PN532 firmware %u.%u; busca %s", static_cast<unsigned>(firmware_.version),
            static_cast<unsigned>(firmware_.revision), wantScan_ ? "ligada" : "desligada");
  setState(wantScan_ ? NfcState::Scanning : NfcState::Idle);
  return Status::Ok;
}

Status NfcSubsystem::shutdown() {
  Status status = Status::Ok;
  if (initialized_ && state_ != NfcState::Fault) {
    chip_.abort();
    status = chip_.startRfField(false);
    if (isOk(status)) status = runBlocking();
  }
  wantScan_ = false;
  initialized_ = false;
  dropTag();
  setState(NfcState::Off);
  return status;
}

Status NfcSubsystem::startScan() {
  if (!initialized_) return Status::NotInitialized;
  wantScan_ = true;
  if (state_ == NfcState::Idle) setState(NfcState::Scanning);
  return Status::Ok;
}

Status NfcSubsystem::stopScan() {
  if (!initialized_) return Status::NotInitialized;
  wantScan_ = false;
  if (state_ != NfcState::Scanning) return Status::Ok;
  dropTag();
  setState(NfcState::Idle);
  // Desliga o campo: abandona a busca em andamento, se houver.
  chip_.abort();
  const Status status = chip_.startRfField(false);
  step_ = isOk(status) ? Step::FieldOff : Step::None;
  // Se o desligamento não saiu agora, tick() insiste até o campo estar desligado.
  fieldOffPending_ = !isOk(status);
  return Status::Ok;
}

bool NfcSubsystem::currentTag(TagInfo *out) const {
  if (!tagPresent_ || out == nullptr) return false;
  *out = tag_;
  return true;
}

void NfcSubsystem::dropTag() {
  if (!tagPresent_) return;
  tagPresent_ = false;
  misses_ = 0;
  (void)bus_.publish(nfc_event::kTagRemoved, 0, crc32(tag_.uid, tag_.uidLength));
}

void NfcSubsystem::noteError(Status status, uint32_t nowMs) {
  if (errors_ < kFaultThreshold) ++errors_;
  if (errors_ < kFaultThreshold || state_ == NfcState::Fault) return;
  DBMO_LOGE(kTag, "PN532 parou de responder: %s", toString(status));
  dropTag();
  lastRetryMs_ = nowMs;
  step_ = Step::None;
  setState(NfcState::Fault);
  (void)bus_.publish(nfc_event::kFault, static_cast<uint32_t>(status));
}

void NfcSubsystem::handleScanResult(Status status) {
  TagInfo found{};
  if (isOk(status)) status = Pn532::parseTag(chip_.response(), chip_.responseLength(), &found);

  if (isOk(status)) {
    misses_ = 0;
    if (tagPresent_ && sameTag(tag_, found)) return;  // a mesma etiqueta continua no campo
    dropTag();
    tag_ = found;
    tagPresent_ = true;
    // O UID não vai para o log: só o tamanho.
    DBMO_LOGI(kTag, "etiqueta detectada (UID de %u bytes)", static_cast<unsigned>(found.uidLength));
    (void)bus_.publish(nfc_event::kTagDetected, found.uidLength, crc32(found.uid, found.uidLength));
    return;
  }
  // NotFound (resposta "nenhuma etiqueta") e Timeout (sem resposta) são campo vazio.
  if (tagPresent_ && ++misses_ >= kMissesToRemove) dropTag();
}

void NfcSubsystem::tick(uint32_t nowMs) {
  if (!initialized_) return;

  if (state_ == NfcState::Fault) {
    if (elapsedMs(nowMs, lastRetryMs_) < kRetryPeriodMs) return;
    lastRetryMs_ = nowMs;
    if (isOk(bringUp())) {
      DBMO_LOGI(kTag, "PN532 voltou a responder");
      setState(wantScan_ ? NfcState::Scanning : NfcState::Idle);
    }
    return;
  }

  // Comando em andamento: acompanha até terminar.
  if (step_ != Step::None) {
    const CommandState command = chip_.poll();
    if (command == CommandState::WaitingAck || command == CommandState::WaitingResponse) return;
    const Step finished = step_;
    step_ = Step::None;
    const Status result = command == CommandState::Done ? Status::Ok : chip_.failure();
    if (finished == Step::Find) {
      if (isOk(result) || result == Status::Timeout) {
        errors_ = 0;
        if (state_ == NfcState::Scanning) handleScanResult(result);
      } else {
        noteError(result, nowMs);
      }
    } else if (!isOk(result)) {
      fieldOffPending_ = true;  // o campo pode ter ficado ligado: tenta de novo
      noteError(result, nowMs);
    }
    return;
  }

  if (fieldOffPending_ && state_ == NfcState::Idle) {
    if (elapsedMs(nowMs, lastScanMs_) < scanPeriodMs_) return;
    lastScanMs_ = nowMs;
    const Status started = chip_.startRfField(false);
    if (isOk(started)) {
      fieldOffPending_ = false;
      step_ = Step::FieldOff;
    } else {
      noteError(started, nowMs);
    }
    return;
  }

  if (state_ != NfcState::Scanning) return;
  if (elapsedMs(nowMs, lastScanMs_) < scanPeriodMs_) return;
  lastScanMs_ = nowMs;
  const Status started = chip_.startFindTagIso14443A();
  if (isOk(started)) {
    step_ = Step::Find;
  } else {
    noteError(started, nowMs);
  }
}

}  // namespace dbmo::nfc
