#include "dbmo/input/input_subsystem.h"

#include "dbmo/core/hal/clock.h"
#include "dbmo/core/log.h"

namespace dbmo::input {

namespace {
constexpr char kTag[] = "input";
}

InputSubsystem::InputSubsystem(EventBus &bus, ConfigStore &config) : bus_(bus), config_(config) {
  buttons_.setSinks(&InputSubsystem::onButton, &InputSubsystem::onCombo, this);
}

Status InputSubsystem::registerConfig(ConfigStore &config) {
  const ButtonTiming defaults;
  DBMO_TRY(
      config.define(ConfigSpec::integer(config_key::kDebounceMs, defaults.debounceMs, 1, 200)));
  DBMO_TRY(config.define(
      ConfigSpec::integer(config_key::kLongPressMs, defaults.longPressMs, 200, 5000)));
  DBMO_TRY(config.define(
      ConfigSpec::integer(config_key::kRepeatDelayMs, defaults.repeatDelayMs, 100, 3000)));
  DBMO_TRY(config.define(
      ConfigSpec::integer(config_key::kRepeatMs, defaults.repeatIntervalMs, 30, 1000)));
  return config.define(ConfigSpec::integer(config_key::kDeadzone, 120, 0, 900));
}

Status InputSubsystem::addButtonSource(IButtonSource &source) {
  if (initialized_) return Status::NotPermitted;
  if (sourceCount_ == kMaxSources) return Status::NoSpace;
  sources_[sourceCount_++] = Source{&source, false, 0, 0};
  return Status::Ok;
}

void InputSubsystem::setStick(Stick which, StickProcessor &stick) {
  sticks_[static_cast<uint8_t>(which)] = &stick;
}

void InputSubsystem::applyConfig() {
  const ButtonTiming defaults;
  ButtonTiming timing;
  timing.debounceMs =
      static_cast<uint16_t>(config_.getInt(config_key::kDebounceMs).valueOr(defaults.debounceMs));
  timing.longPressMs =
      static_cast<uint16_t>(config_.getInt(config_key::kLongPressMs).valueOr(defaults.longPressMs));
  timing.repeatDelayMs = static_cast<uint16_t>(
      config_.getInt(config_key::kRepeatDelayMs).valueOr(defaults.repeatDelayMs));
  timing.repeatIntervalMs = static_cast<uint16_t>(
      config_.getInt(config_key::kRepeatMs).valueOr(defaults.repeatIntervalMs));
  buttons_.setTiming(timing);

  const uint16_t deadzone =
      static_cast<uint16_t>(config_.getInt(config_key::kDeadzone).valueOr(120));
  for (StickProcessor *stick : sticks_)
    if (stick != nullptr) (void)stick->setDeadzonePermille(deadzone);
}

Status InputSubsystem::init() {
  applyConfig();
  size_t healthy = 0;
  Status firstError = Status::HardwareAbsent;
  for (size_t i = 0; i < sourceCount_; ++i) {
    const Status status = sources_[i].source->begin();
    sources_[i].healthy = isOk(status);
    sources_[i].errors = 0;
    if (isOk(status)) {
      ++healthy;
    } else {
      DBMO_LOGW(kTag, "origem de botoes %u: %s", static_cast<unsigned>(i), toString(status));
      if (status != Status::HardwareAbsent) firstError = status;
    }
  }
  if (healthy == 0) return firstError;  // nem um botão: o módulo não tem o que fazer
  initialized_ = true;
  polled_ = false;
  return Status::Ok;
}

size_t InputSubsystem::healthySources() const {
  size_t count = 0;
  for (size_t i = 0; i < sourceCount_; ++i)
    if (sources_[i].healthy) ++count;
  return count;
}

StickPosition InputSubsystem::stick(Stick which) const {
  const StickProcessor *processor = sticks_[static_cast<uint8_t>(which)];
  return processor != nullptr ? processor->position() : StickPosition{};
}

void InputSubsystem::onButton(const ButtonEvent &event, void *context) {
  auto *self = static_cast<InputSubsystem *>(context);
  (void)self->bus_.publish(input_event::kButton, static_cast<uint32_t>(event.button),
                           static_cast<uint32_t>(event.action));
}

void InputSubsystem::onCombo(uint8_t comboId, void *context) {
  auto *self = static_cast<InputSubsystem *>(context);
  (void)self->bus_.publish(input_event::kCombo, comboId);
}

void InputSubsystem::tick(uint32_t nowMs) {
  if (!initialized_) return;
  if (polled_ && elapsedMs(nowMs, lastPollMs_) < kPollPeriodMs) return;
  polled_ = true;
  lastPollMs_ = nowMs;

  ButtonMask raw = 0;
  bool anyHealthy = false;
  for (size_t i = 0; i < sourceCount_; ++i) {
    Source &entry = sources_[i];
    if (!entry.healthy) {
      // Origem em falha: tenta reiniciar de tempos em tempos (módulo replugado).
      if (elapsedMs(nowMs, entry.lastRetryMs) < kRetryPeriodMs) continue;
      entry.lastRetryMs = nowMs;
      if (!isOk(entry.source->begin())) continue;
      // Só volta a valer depois de uma leitura boa: reiniciar sem erro não basta, ou a
      // origem ficaria alternando entre "voltou" e "em falha" a cada tentativa.
      const Result<ButtonMask> probe = entry.source->read();
      if (!probe.ok()) continue;
      entry.healthy = true;
      entry.errors = 0;
      DBMO_LOGI(kTag, "origem de botoes %u voltou", static_cast<unsigned>(i));
      (void)bus_.publish(input_event::kSourceRecovered, static_cast<uint32_t>(i));
      raw |= probe.value();
      anyHealthy = true;
      continue;
    }

    const Result<ButtonMask> mask = entry.source->read();
    if (mask.ok()) {
      entry.errors = 0;
      raw |= mask.value();
      anyHealthy = true;
      continue;
    }
    if (++entry.errors < kFaultThreshold) {
      anyHealthy = true;  // falha passageira: mantém o estado desta volta
      raw |= buttons_.pressed();
      continue;
    }
    entry.healthy = false;
    entry.lastRetryMs = nowMs;
    DBMO_LOGE(kTag, "origem de botoes %u em falha: %s", static_cast<unsigned>(i),
              toString(mask.status()));
    (void)bus_.publish(input_event::kSourceFault, static_cast<uint32_t>(i),
                       static_cast<uint32_t>(mask.status()));
  }

  if (anyHealthy) {
    buttons_.update(raw, nowMs);
  } else if (buttons_.pressed() != 0) {
    buttons_.releaseAll(nowMs);  // nada responde: nenhuma tecla pode ficar presa
  }

  for (uint8_t i = 0; i < 2; ++i) {
    StickProcessor *processor = sticks_[i];
    if (processor == nullptr) continue;
    const Result<bool> changed = processor->update();
    if (changed.ok() && changed.value())
      (void)bus_.publish(input_event::kStick, i, packStick(processor->position()));
  }
}

Status InputSubsystem::shutdown() {
  if (initialized_ && buttons_.pressed() != 0) buttons_.releaseAll(lastPollMs_);
  initialized_ = false;
  return Status::Ok;
}

}  // namespace dbmo::input
