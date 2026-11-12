#include "dbmo/power/power_subsystem.h"

#include "dbmo/config/pins.h"
#include "dbmo/core/log.h"

namespace dbmo::power {

namespace {
constexpr char kTag[] = "power";
}

const char *toString(ShutdownReason reason) {
  switch (reason) {
    case ShutdownReason::None: return "None";
    case ShutdownReason::UserButton: return "UserButton";
    case ShutdownReason::BatteryCritical: return "BatteryCritical";
    case ShutdownReason::Command: return "Command";
  }
  return "?";
}

PowerSubsystem::PowerSubsystem(EventBus &bus, ConfigStore &config) : bus_(bus), config_(config) {}

Status PowerSubsystem::registerConfig(ConfigStore &config) {
  // Desabilitada por padrão: o divisor da bateria ainda não existe no hardware. Ligar a
  // leitura sem o divisor produziria números sem significado.
  DBMO_TRY(config.define(ConfigSpec::boolean(config_key::kVbatEnabled, false)));
  DBMO_TRY(config.define(ConfigSpec::integer(config_key::kVbatRatio, 2000, 1000, 10000)));
  DBMO_TRY(config.define(ConfigSpec::integer(config_key::kLowMv, 3500, 3000, 4200)));
  DBMO_TRY(config.define(ConfigSpec::integer(config_key::kCriticalMv, 3300, 2800, 4000)));
  DBMO_TRY(config.define(ConfigSpec::integer(config_key::kHysteresisMv, 100, 0, 500)));
  DBMO_TRY(config.define(ConfigSpec::integer(config_key::kOffHoldMs, 1500, 500, 10000)));
  DBMO_TRY(config.define(ConfigSpec::boolean(config_key::kAutoOffCritical, true)));
  DBMO_TRY(config.define(ConfigSpec::integer(config_key::kSleepAfterS, 0, 0, 86400)));
  return Status::Ok;
}

void PowerSubsystem::setPowerButton(IGpio &gpio, int pin, bool activeLow) {
  buttonGpio_ = &gpio;
  buttonPin_ = pin;
  buttonActiveLow_ = activeLow;
}

void PowerSubsystem::applyConfig() {
  BatteryConfig battery = battery_.config();
  battery.ratioPermille =
      static_cast<uint16_t>(config_.getInt(config_key::kVbatRatio).valueOr(2000));
  battery.lowMv = static_cast<uint16_t>(config_.getInt(config_key::kLowMv).valueOr(3500));
  battery.criticalMv = static_cast<uint16_t>(config_.getInt(config_key::kCriticalMv).valueOr(3300));
  battery.hysteresisMv =
      static_cast<uint16_t>(config_.getInt(config_key::kHysteresisMv).valueOr(100));
  // Limiar crítico acima do baixo não faz sentido: o crítico acompanha o baixo.
  if (battery.criticalMv > battery.lowMv) {
    DBMO_LOGW(kTag, "power.critical_mv (%u) acima de power.low_mv (%u): usando %u",
              static_cast<unsigned>(battery.criticalMv), static_cast<unsigned>(battery.lowMv),
              static_cast<unsigned>(battery.lowMv));
    battery.criticalMv = battery.lowMv;
  }
  battery_.configure(battery);
  battery_.setEnabled(config_.getBool(config_key::kVbatEnabled).valueOr(false));
  offHoldMs_ = static_cast<uint32_t>(config_.getInt(config_key::kOffHoldMs).valueOr(1500));
  autoOffCritical_ = config_.getBool(config_key::kAutoOffCritical).valueOr(true);
  sleepAfterMs_ = static_cast<uint32_t>(config_.getInt(config_key::kSleepAfterS).valueOr(0)) * 1000;
}

Status PowerSubsystem::init() {
  // Primeiro a trava: enquanto ela não está segura, soltar o botão desliga o aparelho.
  if (switch_ != nullptr) {
    const Status status = switch_->hold();
    if (!isOk(status)) {
      DBMO_LOGE(kTag, "trava de alimentacao nao respondeu: %s", toString(status));
      return status;
    }
  }
  if (buttonGpio_ != nullptr) {
    DBMO_TRY(buttonGpio_->configure(
        buttonPin_, buttonActiveLow_ ? PinMode::InputPullUp : PinMode::InputPullDown));
  }
  applyConfig();

  phase_ = PowerPhase::Running;
  reason_ = ShutdownReason::None;
  chargeState_ = ChargeState::Unknown;
  reportedLevel_ = BatteryLevel::Unknown;
  reportedFault_ = battery_.enabled() ? BatteryFault::None : BatteryFault::Disabled;
  criticalStreak_ = 0;
  buttonArmed_ = false;
  buttonStable_ = false;
  buttonCounter_ = 0;
  longFired_ = false;
  polledOnce_ = false;
  idleReported_ = false;
  initialized_ = true;

  DBMO_LOGI(kTag, "bateria: %s; carregador: %s; trava: %s; botao: %s",
            battery_.enabled() ? "leitura habilitada" : "leitura desabilitada",
            charger_ != nullptr ? "sim" : "nao", switch_ != nullptr ? "sim" : "nao",
            buttonGpio_ != nullptr ? "sim" : "nao");
  return Status::Ok;
}

void PowerSubsystem::notifyActivity(uint32_t nowMs) {
  lastActivityMs_ = nowMs;
  idleReported_ = false;
}

void PowerSubsystem::requestShutdown(ShutdownReason reason) {
  if (phase_ != PowerPhase::Running) return;
  phase_ = PowerPhase::ShutdownRequested;
  reason_ = reason;
  DBMO_LOGI(kTag, "desligamento pedido: %s", toString(reason));
  (void)bus_.publish(power_event::kShutdownRequested, static_cast<uint32_t>(reason));
}

void PowerSubsystem::cancelShutdown() {
  if (phase_ != PowerPhase::ShutdownRequested) return;
  phase_ = PowerPhase::Running;
  reason_ = ShutdownReason::None;
  criticalStreak_ = 0;
}

Status PowerSubsystem::completeShutdown() {
  if (phase_ == PowerPhase::Off) return Status::Ok;
  if (phase_ != PowerPhase::ShutdownRequested) return Status::NotPermitted;
  if (buttonGpio_ != nullptr) {
    const Result<bool> level = buttonGpio_->read(buttonPin_);
    if (level.ok() && level.value() != buttonActiveLow_) return Status::Busy;
  }
  if (switch_ != nullptr) {
    DBMO_TRY(switch_->release());
    phase_ = PowerPhase::Off;
    return Status::Ok;
  }
  if (sleep_ != nullptr) {
    WakeSource wake;
    wake.pin = buttonPin_;
    wake.activeLow = buttonActiveLow_;
    DBMO_TRY(sleep_->deepSleep(wake));
    phase_ = PowerPhase::Off;
    return Status::Ok;
  }
  return Status::Unsupported;
}

void PowerSubsystem::tick(uint32_t nowMs) {
  if (!initialized_ || phase_ == PowerPhase::Off) return;

  if (!polledOnce_) {
    polledOnce_ = true;
    lastButtonPollMs_ = nowMs;
    lastBatteryMs_ = nowMs - kBatteryPeriodMs;  // primeira amostra já neste tick
    lastActivityMs_ = nowMs;
  }

  if (buttonGpio_ != nullptr && elapsedMs(nowMs, lastButtonPollMs_) >= kButtonPollMs) {
    lastButtonPollMs_ = nowMs;
    pollButton(nowMs);
  }

  if (elapsedMs(nowMs, lastBatteryMs_) >= kBatteryPeriodMs) {
    lastBatteryMs_ = nowMs;
    pollBattery();
  }

  if (sleepAfterMs_ != 0 && !idleReported_ && phase_ == PowerPhase::Running &&
      elapsedMs(nowMs, lastActivityMs_) >= sleepAfterMs_) {
    idleReported_ = true;
    (void)bus_.publish(power_event::kIdleTimeout);
  }
}

void PowerSubsystem::pollButton(uint32_t nowMs) {
  const Result<bool> level = buttonGpio_->read(buttonPin_);
  if (!level.ok()) return;
  const bool pressed = level.value() != buttonActiveLow_;

  if (pressed == buttonStable_) {
    buttonCounter_ = 0;
  } else if (++buttonCounter_ >= kButtonDebounceSamples) {
    buttonCounter_ = 0;
    buttonStable_ = pressed;
    if (pressed) {
      pressStartMs_ = nowMs;
      longFired_ = false;
    } else {
      if (buttonArmed_ && !longFired_ && phase_ == PowerPhase::Running) {
        notifyActivity(nowMs);
        (void)bus_.publish(power_event::kButtonPress);
      }
      buttonArmed_ = true;
    }
  }

  // Aparelho recém-ligado com o botão já solto: arma depois da primeira leitura estável.
  if (!buttonStable_ && !pressed) buttonArmed_ = true;

  if (buttonStable_ && buttonArmed_ && !longFired_ &&
      elapsedMs(nowMs, pressStartMs_) >= offHoldMs_) {
    longFired_ = true;
    requestShutdown(ShutdownReason::UserButton);
  }
}

void PowerSubsystem::pollBattery() {
  if (charger_ != nullptr) {
    const Result<ChargeState> state = charger_->read();
    const ChargeState now = state.ok() ? state.value() : ChargeState::Unknown;
    if (now != chargeState_) {
      chargeState_ = now;
      (void)bus_.publish(power_event::kChargeState, static_cast<uint32_t>(now));
    }
  }

  const BatteryFault fault = battery_.sample();
  if (fault != reportedFault_) {
    reportedFault_ = fault;
    if (fault != BatteryFault::None && fault != BatteryFault::Disabled)
      DBMO_LOGW(kTag, "leitura da bateria invalida: %s", toString(fault));
    (void)bus_.publish(power_event::kBatteryFault, static_cast<uint32_t>(fault));
  }

  const BatteryLevel level = battery_.level();
  if (level != reportedLevel_) {
    reportedLevel_ = level;
    (void)bus_.publish(power_event::kBatteryLevel, static_cast<uint32_t>(level),
                       battery_.millivolts().valueOr(0));
  }

  // Bateria crítica confirmada e sem carregador ligado: pede o desligamento ordenado.
  if (level == BatteryLevel::Critical && fault == BatteryFault::None &&
      chargeState_ != ChargeState::Charging && chargeState_ != ChargeState::Full) {
    if (criticalStreak_ < kCriticalConfirmSamples) ++criticalStreak_;
    if (criticalStreak_ >= kCriticalConfirmSamples && autoOffCritical_)
      requestShutdown(ShutdownReason::BatteryCritical);
  } else {
    criticalStreak_ = 0;
  }
}

}  // namespace dbmo::power
