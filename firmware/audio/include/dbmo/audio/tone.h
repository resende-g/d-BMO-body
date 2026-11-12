// Gerador de tons para os sons de interface. Usa aritmética inteira e uma aproximação
// parabólica do seno (distorção de alguns por cento, irrelevante para um bipe).
#pragma once

#include <cstddef>
#include <cstdint>

#include "dbmo/audio/audio_types.h"
#include "dbmo/core/status.h"

namespace dbmo::audio {

class ToneSource final : public IAudioSource {
 public:
  // Rampa de entrada e de saída, para o tom não começar nem terminar com estalo.
  static constexpr uint32_t kFadeMs = 5;

  // InvalidArgument se a frequência não é reproduzível na taxa (acima de taxa/2) ou se
  // algum parâmetro é zero.
  Status start(uint32_t frequencyHz, uint32_t durationMs, uint32_t sampleRateHz,
               uint16_t amplitude = 16000);
  size_t read(int16_t *out, size_t n) override;
  bool finished() const { return position_ >= total_; }

 private:
  uint32_t phase_ = 0;
  uint32_t increment_ = 0;
  uint32_t total_ = 0;
  uint32_t position_ = 0;
  uint32_t fade_ = 0;
  uint16_t amplitude_ = 0;
};

// Seno aproximado: fase de 0 a 65535 (uma volta), saída de −32767 a +32767.
int16_t approximateSine(uint16_t phase);

}  // namespace dbmo::audio
