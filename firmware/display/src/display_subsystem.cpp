#include "dbmo/display/display_subsystem.h"

#include "dbmo/core/hal/clock.h"
#include "dbmo/core/log.h"

namespace dbmo::display {

namespace {
constexpr char kTag[] = "display";
}

DisplaySubsystem::DisplaySubsystem(IDisplay &display, EventBus &bus, ConfigStore &config)
    : display_(display), bus_(bus), config_(config) {}

Status DisplaySubsystem::registerConfig(ConfigStore &config) {
  DBMO_TRY(config.define(ConfigSpec::integer(config_key::kBrightness, 80, 0, 100)));
  DBMO_TRY(config.define(ConfigSpec::integer(config_key::kDimAfterS, 30, 0, 3600)));
  return config.define(ConfigSpec::integer(config_key::kOffAfterS, 120, 0, 3600));
}

uint8_t DisplaySubsystem::configuredBrightness() const {
  return static_cast<uint8_t>(config_.getInt(config_key::kBrightness).valueOr(80));
}

Status DisplaySubsystem::applyBacklight(uint8_t percent) {
  if (!backlightControlled_) return Status::Ok;
  const Status status = display_.setBacklight(percent);
  if (status == Status::Unsupported) {
    // Na bancada o LED da tela está direto no 3V3: segue sem controle de brilho.
    backlightControlled_ = false;
    DBMO_LOGW(kTag, "backlight sem controle: brilho e esmaecimento desativados");
    return Status::Ok;
  }
  return status;
}

Status DisplaySubsystem::checkController() {
  const Result<uint16_t> id = display_.controllerId();
  if (id.status() == Status::Unsupported) return Status::Ok;  // a tela não informa o ID
  if (!id.ok()) {
    DBMO_LOGW(kTag, "ID do controlador nao lido: %s", toString(id.status()));
    return id.status();
  }
  if (id.value() == kExpectedControllerId) {
    DBMO_LOGI(kTag, "controlador 0x%04X", static_cast<unsigned>(id.value()));
    return Status::Ok;
  }
  // 0x0000 e 0xFFFF são o MISO da tela sem resposta (fio do SDO solto, por exemplo): a tela
  // desenha mesmo assim, só de escrita. Outro valor indica um controlador diferente.
  const bool silent = id.value() == 0x0000 || id.value() == 0xFFFF;
  DBMO_LOGW(kTag, "ID do controlador 0x%04X, esperado 0x%04X (%s)",
            static_cast<unsigned>(id.value()), static_cast<unsigned>(kExpectedControllerId),
            silent ? "MISO da tela sem resposta" : "controlador diferente");
  return silent ? Status::IoError : Status::Incompatible;
}

Status DisplaySubsystem::init() {
  backlightControlled_ = true;
  DBMO_TRY(display_.setSleep(false));
  DBMO_TRY(display_.fillRect(0, 0, static_cast<int16_t>(display_.width()),
                             static_cast<int16_t>(display_.height()), palette::kBlack));
  DBMO_TRY(applyBacklight(configuredBrightness()));
  controllerCheck_ = checkController();
  power_ = ScreenPower::On;
  activitySeen_ = false;
  initialized_ = true;
  return Status::Ok;
}

void DisplaySubsystem::setPower(ScreenPower power) {
  if (power == power_) return;
  const ScreenPower previous = power_;
  Status status = Status::Ok;
  switch (power) {
    case ScreenPower::On:
      if (previous == ScreenPower::Off) status = display_.setSleep(false);
      if (isOk(status)) status = applyBacklight(configuredBrightness());
      break;
    case ScreenPower::Dimmed:
      status =
          applyBacklight(static_cast<uint8_t>(configuredBrightness() * kDimFactorPercent / 100));
      break;
    case ScreenPower::Off:
      status = applyBacklight(0);
      if (isOk(status)) status = display_.setSleep(true);
      break;
  }
  if (!isOk(status)) {
    // O estado não muda: a próxima volta do tick tenta de novo.
    DBMO_LOGE(kTag, "falha ao mudar a tela de estado: %s", toString(status));
    return;
  }
  power_ = power;
  (void)bus_.publish(display_event::kPowerChanged, static_cast<uint32_t>(power),
                     static_cast<uint32_t>(previous));
}

void DisplaySubsystem::notifyActivity(uint32_t nowMs) {
  lastActivityMs_ = nowMs;
  activitySeen_ = true;
  if (initialized_) setPower(ScreenPower::On);
}

Status DisplaySubsystem::setBrightness(uint8_t percent) {
  DBMO_TRY(config_.setInt(config_key::kBrightness, percent));
  if (!initialized_ || power_ != ScreenPower::On) return Status::Ok;
  return applyBacklight(percent);
}

void DisplaySubsystem::tick(uint32_t nowMs) {
  if (!initialized_) return;
  if (!activitySeen_) {
    // A contagem de inatividade começa no primeiro tick depois do init.
    lastActivityMs_ = nowMs;
    activitySeen_ = true;
    return;
  }

  const uint32_t idleMs = elapsedMs(nowMs, lastActivityMs_);
  const uint32_t offAfterMs =
      static_cast<uint32_t>(config_.getInt(config_key::kOffAfterS).valueOr(0)) * 1000u;
  const uint32_t dimAfterMs =
      static_cast<uint32_t>(config_.getInt(config_key::kDimAfterS).valueOr(0)) * 1000u;

  if (offAfterMs != 0 && idleMs >= offAfterMs) {
    setPower(ScreenPower::Off);
  } else if (dimAfterMs != 0 && idleMs >= dimAfterMs && backlightControlled_ &&
             power_ == ScreenPower::On) {
    setPower(ScreenPower::Dimmed);
  }
}

Status DisplaySubsystem::shutdown() {
  if (!initialized_) return Status::Ok;
  initialized_ = false;
  DBMO_TRY(display_.fillRect(0, 0, static_cast<int16_t>(display_.width()),
                             static_cast<int16_t>(display_.height()), palette::kBlack));
  DBMO_TRY(applyBacklight(0));
  return display_.setSleep(true);
}

}  // namespace dbmo::display
