#include "dbmo/audio/tone.h"

namespace dbmo::audio {

int16_t approximateSine(uint16_t phase) {
  const uint32_t half = phase & 0x7FFFu;            // posição dentro da meia volta
  uint32_t value = (half * (32768u - half)) >> 13;  // parábola com pico em 32768
  if (value > 32767u) value = 32767u;
  return (phase & 0x8000u) != 0 ? static_cast<int16_t>(-static_cast<int32_t>(value))
                                : static_cast<int16_t>(value);
}

Status ToneSource::start(uint32_t frequencyHz, uint32_t durationMs, uint32_t sampleRateHz,
                         uint16_t amplitude) {
  if (frequencyHz == 0 || durationMs == 0 || sampleRateHz == 0 || amplitude == 0 ||
      amplitude > 32767)
    return Status::InvalidArgument;
  if (frequencyHz > sampleRateHz / 2) return Status::InvalidArgument;
  increment_ = static_cast<uint32_t>((static_cast<uint64_t>(frequencyHz) << 32) / sampleRateHz);
  total_ = static_cast<uint32_t>(static_cast<uint64_t>(durationMs) * sampleRateHz / 1000);
  fade_ = kFadeMs * sampleRateHz / 1000;
  if (fade_ > total_ / 2) fade_ = total_ / 2;
  position_ = 0;
  phase_ = 0;
  amplitude_ = amplitude;
  return Status::Ok;
}

size_t ToneSource::read(int16_t *out, size_t n) {
  size_t produced = 0;
  while (produced < n && position_ < total_) {
    int32_t sample = approximateSine(static_cast<uint16_t>(phase_ >> 16));
    sample = sample * amplitude_ / 32768;
    if (fade_ > 0) {
      const uint32_t fromEnd = total_ - position_;
      if (position_ < fade_) {
        sample = sample * static_cast<int32_t>(position_) / static_cast<int32_t>(fade_);
      } else if (fromEnd <= fade_) {
        sample = sample * static_cast<int32_t>(fromEnd - 1) / static_cast<int32_t>(fade_);
      }
    }
    out[produced++] = static_cast<int16_t>(sample);
    phase_ += increment_;
    ++position_;
  }
  return produced;
}

}  // namespace dbmo::audio
