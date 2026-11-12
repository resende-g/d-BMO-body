// Barramento I2C em modo mestre. Endereços de 7 bits.
#pragma once

#include <cstddef>
#include <cstdint>

#include "dbmo/core/status.h"

namespace dbmo {

class II2cBus {
 public:
  virtual ~II2cBus() = default;
  // HardwareAbsent quando o endereço não responde (NACK); IoError em falha do barramento.
  virtual Status write(uint8_t address, const uint8_t *data, size_t n) = 0;
  virtual Status read(uint8_t address, uint8_t *data, size_t n) = 0;
  // Escrita seguida de leitura com repeated start (sem STOP no meio).
  virtual Status writeRead(uint8_t address, const uint8_t *tx, size_t txLen, uint8_t *rx,
                           size_t rxLen) = 0;
  // Ok se o endereço responde com ACK; HardwareAbsent se não.
  virtual Status probe(uint8_t address) = 0;
};

}  // namespace dbmo
