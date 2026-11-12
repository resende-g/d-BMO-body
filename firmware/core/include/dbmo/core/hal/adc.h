// Um canal de ADC e uma saída PWM.
#pragma once

#include <cstdint>

#include "dbmo/core/status.h"

namespace dbmo {

class IAdcChannel {
 public:
  virtual ~IAdcChannel() = default;
  // Leitura bruta, de 0 a maxRaw().
  virtual Result<uint16_t> readRaw() = 0;
  virtual uint16_t maxRaw() const = 0;
  // Tensão no pino, em mV, com a calibração que o porte tiver.
  virtual Result<uint16_t> readMillivolts() = 0;
};

class IPwmOutput {
 public:
  virtual ~IPwmOutput() = default;
  // Ciclo de trabalho de 0 a 1000 (por mil).
  virtual Status setDutyPermille(uint16_t permille) = 0;
};

}  // namespace dbmo
