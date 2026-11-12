// Contratos de hardware de áudio e utilitários de amostras. Formato único do módulo:
// PCM de 16 bits com sinal, mono.
#pragma once

#include <cstddef>
#include <cstdint>

#include "dbmo/core/status.h"

namespace dbmo::audio {

// Taxas que servem ao mesmo tempo ao amplificador MAX98357A (8, 16, 32, 44,1, 48, 88,2 e
// 96 kHz; AMP-08) e ao microfone INMP441 (7,8 a 50 kHz; MIC-05), que dividem os clocks.
// 11,025, 12, 22,05 e 24 kHz NÃO são aceitas pelo amplificador.
bool isSupportedSampleRate(uint32_t hz);

// Saída I2S + amplificador. write() não bloqueia.
class IAudioOutput {
 public:
  virtual ~IAudioOutput() = default;
  virtual Status begin(uint32_t sampleRateHz) = 0;
  // Liga ou desliga o amplificador (pino SD_MODE).
  virtual Status setAmplifier(bool on) = 0;
  // Aceita até n amostras; devolve quantas couberam agora (pode ser 0).
  virtual Result<size_t> write(const int16_t *samples, size_t n) = 0;
  virtual Status end() = 0;
};

// Entrada I2S (microfone). read() não bloqueia.
class IAudioInput {
 public:
  virtual ~IAudioInput() = default;
  virtual Status begin(uint32_t sampleRateHz) = 0;
  // Devolve quantas amostras havia (pode ser 0).
  virtual Result<size_t> read(int16_t *samples, size_t n) = 0;
  virtual Status end() = 0;
};

// Origem de amostras para reprodução.
class IAudioSource {
 public:
  virtual ~IAudioSource() = default;
  // Preenche até n amostras; 0 significa que acabou.
  virtual size_t read(int16_t *out, size_t n) = 0;
};

// Reproduz amostras que já estão na memória (não copia).
class BufferSource final : public IAudioSource {
 public:
  void set(const int16_t *samples, size_t count) {
    samples_ = samples;
    count_ = count;
    position_ = 0;
  }
  size_t read(int16_t *out, size_t n) override;

 private:
  const int16_t *samples_ = nullptr;
  size_t count_ = 0;
  size_t position_ = 0;
};

// Volume de 0 a 100 com curva quadrática (ganho = (v/100)²), mais próxima da percepção
// do que a linear. 100 = ganho unitário: o volume nunca amplifica, então não satura.
class Volume {
 public:
  void setPercent(uint8_t percent) { percent_ = percent > 100 ? 100 : percent; }
  uint8_t percent() const { return percent_; }
  void setMuted(bool muted) { muted_ = muted; }
  bool muted() const { return muted_; }
  // Ganho em Q15 (32768 = 1,0). Zero quando mudo.
  uint16_t gainQ15() const;
  bool silent() const { return gainQ15() == 0; }
  void apply(int16_t *samples, size_t n) const;

 private:
  uint8_t percent_ = 60;
  bool muted_ = false;
};

// Maior valor absoluto de um bloco (0 a 32768).
uint16_t peakLevel(const int16_t *samples, size_t n);

}  // namespace dbmo::audio
