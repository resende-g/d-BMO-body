#include "dbmo/input/stick_processor.h"

namespace dbmo::input {

uint32_t integerSqrt(uint32_t value) {
  uint32_t result = 0;
  uint32_t bit = 1u << 30;
  while (bit > value) bit >>= 2;
  while (bit != 0) {
    if (value >= result + bit) {
      value -= result + bit;
      result = (result >> 1) + bit;
    } else {
      result >>= 1;
    }
    bit >>= 2;
  }
  return result;
}

StickProcessor::StickProcessor(IAdcChannel &x, IAdcChannel &y) : x_(x), y_(y) {
  // Até haver calibração: centro no meio da escala e curso inteiro do ADC.
  calX_ = AxisCalibration{0, static_cast<uint16_t>(x.maxRaw() / 2), x.maxRaw()};
  calY_ = AxisCalibration{0, static_cast<uint16_t>(y.maxRaw() / 2), y.maxRaw()};
}

bool StickProcessor::validCalibration(const AxisCalibration &calibration, uint16_t maxRaw) const {
  if (calibration.max > maxRaw) return false;
  if (!(calibration.min < calibration.center && calibration.center < calibration.max)) return false;
  const uint16_t minimumSpan = static_cast<uint16_t>(maxRaw / 20);  // 5 % da escala
  return calibration.center - calibration.min >= minimumSpan &&
         calibration.max - calibration.center >= minimumSpan;
}

Status StickProcessor::setCalibration(const AxisCalibration &x, const AxisCalibration &y) {
  if (!validCalibration(x, x_.maxRaw()) || !validCalibration(y, y_.maxRaw()))
    return Status::InvalidArgument;
  calX_ = x;
  calY_ = y;
  return Status::Ok;
}

Status StickProcessor::calibrateCenter() {
  uint32_t sumX = 0;
  uint32_t sumY = 0;
  for (uint8_t i = 0; i < kCenterSamples; ++i) {
    const Result<uint16_t> x = x_.readRaw();
    if (!x.ok()) return x.status();
    const Result<uint16_t> y = y_.readRaw();
    if (!y.ok()) return y.status();
    sumX += x.value();
    sumY += y.value();
  }
  AxisCalibration x = calX_;
  AxisCalibration y = calY_;
  x.center = static_cast<uint16_t>(sumX / kCenterSamples);
  y.center = static_cast<uint16_t>(sumY / kCenterSamples);
  if (!validCalibration(x, x_.maxRaw()) || !validCalibration(y, y_.maxRaw()))
    return Status::Corrupt;
  calX_ = x;
  calY_ = y;
  return Status::Ok;
}

void StickProcessor::beginRangeCalibration() {
  learning_ = true;
  learnedX_ = AxisCalibration{calX_.center, calX_.center, calX_.center};
  learnedY_ = AxisCalibration{calY_.center, calY_.center, calY_.center};
}

Status StickProcessor::endRangeCalibration() {
  if (!learning_) return Status::NotInitialized;
  learning_ = false;
  if (!validCalibration(learnedX_, x_.maxRaw()) || !validCalibration(learnedY_, y_.maxRaw()))
    return Status::Corrupt;
  calX_ = learnedX_;
  calY_ = learnedY_;
  return Status::Ok;
}

Status StickProcessor::setDeadzonePermille(uint16_t permille) {
  if (permille > 900) return Status::OutOfRange;
  deadzone_ = permille;
  return Status::Ok;
}

void StickProcessor::setInverted(bool invertX, bool invertY) {
  invertX_ = invertX;
  invertY_ = invertY;
}

int16_t StickProcessor::normalize(uint16_t raw, const AxisCalibration &calibration) {
  int32_t value;
  if (raw >= calibration.center) {
    value = static_cast<int32_t>(raw - calibration.center) * 1000 /
            (calibration.max - calibration.center);
  } else {
    value = -(static_cast<int32_t>(calibration.center - raw) * 1000 /
              (calibration.center - calibration.min));
  }
  if (value > 1000) value = 1000;
  if (value < -1000) value = -1000;
  return static_cast<int16_t>(value);
}

Result<bool> StickProcessor::update() {
  const Result<uint16_t> rawX = x_.readRaw();
  const Result<uint16_t> rawY = rawX.ok() ? y_.readRaw() : Result<uint16_t>(rawX.status());
  if (!rawX.ok() || !rawY.ok()) {
    if (errors_ < UINT16_MAX) ++errors_;
    return rawX.ok() ? rawY.status() : rawX.status();
  }
  errors_ = 0;

  if (learning_) {
    if (rawX.value() < learnedX_.min) learnedX_.min = rawX.value();
    if (rawX.value() > learnedX_.max) learnedX_.max = rawX.value();
    if (rawY.value() < learnedY_.min) learnedY_.min = rawY.value();
    if (rawY.value() > learnedY_.max) learnedY_.max = rawY.value();
  }

  int32_t x = normalize(rawX.value(), calX_);
  int32_t y = normalize(rawY.value(), calY_);
  if (invertX_) x = -x;
  if (invertY_) y = -y;

  // Zona morta radial: dentro do raio vale zero; fora, o curso é reescalado para que a
  // saída comece em zero na borda da zona morta e ainda chegue a 1000 no fim do curso.
  const uint32_t magnitude = integerSqrt(static_cast<uint32_t>(x * x + y * y));
  if (magnitude <= deadzone_) {
    x = 0;
    y = 0;
  } else {
    const int64_t numerator = static_cast<int64_t>(magnitude - deadzone_) * 1000;
    const int64_t denominator = static_cast<int64_t>(1000 - deadzone_) * magnitude;
    x = static_cast<int32_t>(x * numerator / denominator);
    y = static_cast<int32_t>(y * numerator / denominator);
    if (x > 1000) x = 1000;
    if (x < -1000) x = -1000;
    if (y > 1000) y = 1000;
    if (y < -1000) y = -1000;
  }

  const int32_t dx = x - position_.x;
  const int32_t dy = y - position_.y;
  const bool wasCentered = position_.x == 0 && position_.y == 0;
  const bool isCentered = x == 0 && y == 0;
  const bool changed = dx >= kChangeThreshold || dx <= -kChangeThreshold ||
                       dy >= kChangeThreshold || dy <= -kChangeThreshold ||
                       (wasCentered != isCentered);
  if (changed) position_ = StickPosition{static_cast<int16_t>(x), static_cast<int16_t>(y)};
  return changed;
}

}  // namespace dbmo::input
