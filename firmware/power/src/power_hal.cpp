#include "dbmo/power/power_hal.h"

namespace dbmo::power {

const char *toString(ChargeState state) {
  switch (state) {
    case ChargeState::Unknown: return "Unknown";
    case ChargeState::NotCharging: return "NotCharging";
    case ChargeState::Charging: return "Charging";
    case ChargeState::Full: return "Full";
  }
  return "?";
}

Status GpioChargerStatus::begin() {
  const PinMode mode = activeLow_ ? PinMode::InputPullUp : PinMode::InputPullDown;
  DBMO_TRY(gpio_.configure(chargingPin_, mode));
  if (fullPin_ >= 0) DBMO_TRY(gpio_.configure(fullPin_, mode));
  return Status::Ok;
}

Result<ChargeState> GpioChargerStatus::read() {
  const Result<bool> charging = gpio_.read(chargingPin_);
  if (!charging.ok()) return charging.status();
  if (charging.value() != activeLow_) return ChargeState::Charging;
  if (fullPin_ >= 0) {
    const Result<bool> full = gpio_.read(fullPin_);
    if (!full.ok()) return full.status();
    if (full.value() != activeLow_) return ChargeState::Full;
  }
  return ChargeState::NotCharging;
}

Status GpioPowerLatch::hold() {
  // Escreve o nível antes de virar saída não é possível pelo contrato de IGpio; a ordem
  // configure → write deixa o pino no nível padrão por alguns microssegundos.
  DBMO_TRY(gpio_.configure(pin_, PinMode::Output));
  return gpio_.write(pin_, activeHigh_);
}

Status GpioPowerLatch::release() { return gpio_.write(pin_, !activeHigh_); }

}  // namespace dbmo::power
