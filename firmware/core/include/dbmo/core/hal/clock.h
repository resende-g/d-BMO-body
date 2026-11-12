// Relógio monotônico e esperas. Único ponto do firmware que conhece o tempo.
#pragma once

#include <cstdint>

namespace dbmo {

class IClock {
 public:
  virtual ~IClock() = default;
  // Milissegundos desde o boot. Estoura a cada ~49,7 dias: comparar sempre por
  // diferença (elapsedMs), nunca por valor absoluto.
  virtual uint32_t nowMs() = 0;
  virtual uint64_t nowUs() = 0;
  virtual void delayMs(uint32_t ms) = 0;
  virtual void delayUs(uint32_t us) = 0;
};

// Tempo decorrido, correto mesmo quando nowMs() estoura.
constexpr uint32_t elapsedMs(uint32_t nowMs, uint32_t sinceMs) { return nowMs - sinceMs; }

}  // namespace dbmo
