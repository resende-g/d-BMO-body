#include "dbmo/power/battery_monitor.h"

namespace dbmo::power {

const char *toString(BatteryLevel level) {
  switch (level) {
    case BatteryLevel::Unknown: return "Unknown";
    case BatteryLevel::Critical: return "Critical";
    case BatteryLevel::Low: return "Low";
    case BatteryLevel::Normal: return "Normal";
  }
  return "?";
}

const char *toString(BatteryFault fault) {
  switch (fault) {
    case BatteryFault::None: return "None";
    case BatteryFault::Disabled: return "Disabled";
    case BatteryFault::NoSensor: return "NoSensor";
    case BatteryFault::ReadError: return "ReadError";
    case BatteryFault::Saturated: return "Saturated";
    case BatteryFault::Implausible: return "Implausible";
  }
  return "?";
}

void BatteryMonitor::setEnabled(bool enabled) {
  if (enabled_ == enabled) return;
  enabled_ = enabled;
  discard();
  fault_ = enabled ? BatteryFault::None : BatteryFault::Disabled;
}

void BatteryMonitor::discard() {
  count_ = 0;
  next_ = 0;
  invalidStreak_ = 0;
  level_ = BatteryLevel::Unknown;
}

Result<uint16_t> BatteryMonitor::millivolts() const {
  if (count_ == 0) return Status::NotInitialized;
  uint32_t sum = 0;
  for (size_t i = 0; i < count_; ++i) sum += window_[i];
  return static_cast<uint16_t>(sum / count_);
}

BatteryFault BatteryMonitor::sample() {
  BatteryFault fault = BatteryFault::None;
  uint32_t batteryMv = 0;

  if (!enabled_) {
    fault = BatteryFault::Disabled;
  } else if (adc_ == nullptr) {
    fault = BatteryFault::NoSensor;
  } else {
    const Result<uint16_t> pin = adc_->readMillivolts();
    if (!pin.ok()) {
      fault = BatteryFault::ReadError;
    } else if (pin.value() > config_.adcCeilingMv) {
      fault = BatteryFault::Saturated;
    } else {
      batteryMv = static_cast<uint32_t>(pin.value()) * config_.ratioPermille / 1000;
      if (batteryMv < config_.minPlausibleMv || batteryMv > config_.maxPlausibleMv)
        fault = BatteryFault::Implausible;
    }
  }

  fault_ = fault;
  if (fault != BatteryFault::None) {
    // Uma amostra ruim isolada não apaga a leitura; uma sequência, sim.
    if (invalidStreak_ < kFaultSamples) ++invalidStreak_;
    if (invalidStreak_ >= kFaultSamples || fault == BatteryFault::Disabled ||
        fault == BatteryFault::NoSensor)
      discard();
    return fault;
  }

  invalidStreak_ = 0;
  window_[next_] = static_cast<uint16_t>(batteryMv);
  next_ = (next_ + 1) % kWindow;
  if (count_ < kWindow) ++count_;
  classify(millivolts().value());
  return fault;
}

void BatteryMonitor::classify(uint16_t mv) {
  const uint16_t low = config_.lowMv;
  const uint16_t critical = config_.criticalMv;
  const uint16_t hysteresis = config_.hysteresisMv;
  switch (level_) {
    case BatteryLevel::Unknown:
      level_ = mv < critical ? BatteryLevel::Critical
               : mv < low    ? BatteryLevel::Low
                             : BatteryLevel::Normal;
      break;
    case BatteryLevel::Normal:
      if (mv < critical) {
        level_ = BatteryLevel::Critical;
      } else if (mv < low) {
        level_ = BatteryLevel::Low;
      }
      break;
    case BatteryLevel::Low:
      if (mv < critical) {
        level_ = BatteryLevel::Critical;
      } else if (mv >= static_cast<uint32_t>(low) + hysteresis) {
        level_ = BatteryLevel::Normal;
      }
      break;
    case BatteryLevel::Critical:
      if (mv >= static_cast<uint32_t>(low) + hysteresis) {
        level_ = BatteryLevel::Normal;
      } else if (mv >= static_cast<uint32_t>(critical) + hysteresis) {
        level_ = BatteryLevel::Low;
      }
      break;
  }
}

}  // namespace dbmo::power
