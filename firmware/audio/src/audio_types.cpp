#include "dbmo/audio/audio_types.h"

#include <cstring>

namespace dbmo::audio {

bool isSupportedSampleRate(uint32_t hz) {
  return hz == 8000 || hz == 16000 || hz == 32000 || hz == 44100 || hz == 48000;
}

size_t BufferSource::read(int16_t *out, size_t n) {
  if (samples_ == nullptr || out == nullptr) return 0;
  const size_t remaining = count_ - position_;
  const size_t count = n < remaining ? n : remaining;
  memcpy(out, samples_ + position_, count * sizeof(int16_t));
  position_ += count;
  return count;
}

uint16_t Volume::gainQ15() const {
  if (muted_) return 0;
  return static_cast<uint16_t>(static_cast<uint32_t>(percent_) * percent_ * 32768u / 10000u);
}

void Volume::apply(int16_t *samples, size_t n) const {
  const int32_t gain = gainQ15();
  if (gain == 32768) return;
  for (size_t i = 0; i < n; ++i)
    samples[i] = static_cast<int16_t>((static_cast<int32_t>(samples[i]) * gain) / 32768);
}

uint16_t peakLevel(const int16_t *samples, size_t n) {
  uint16_t peak = 0;
  for (size_t i = 0; i < n; ++i) {
    const int32_t value = samples[i];
    const uint16_t magnitude = static_cast<uint16_t>(value < 0 ? -value : value);
    if (magnitude > peak) peak = magnitude;
  }
  return peak;
}

}  // namespace dbmo::audio
