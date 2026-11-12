// EEPROM I2C 24C02 (256 bytes), usada para identificar o cartucho.
//
// O tamanho de página varia entre fabricantes (8 bytes na Microchip 24LC02B e na Atmel
// AT24C02; 16 bytes na ST M24C02), então o padrão é 8, que serve para todas. O ciclo de
// escrita dura até 5 ms (datasheets da Microchip e da Atmel); durante ele o chip não
// responde, e o driver espera consultando o endereço (acknowledge polling).
// Fontes: docs/hardware/sources/PERIPHERALS.md, EE-01 a EE-11.
#pragma once

#include <cstddef>
#include <cstdint>

#include "dbmo/core/hal/clock.h"
#include "dbmo/core/hal/i2c.h"
#include "dbmo/core/status.h"

namespace dbmo::cartridges {

class Eeprom24c02 {
 public:
  static constexpr size_t kSize = 256;
  static constexpr uint8_t kDefaultPageSize = 8;
  static constexpr uint32_t kWriteCycleTimeoutMs = 10;  // o dobro do máximo de datasheet
  static constexpr size_t kReadChunk = 32;

  Eeprom24c02(II2cBus &bus, IClock &clock, uint8_t address, uint8_t pageSize = kDefaultPageSize)
      : bus_(bus), clock_(clock), address_(address), pageSize_(pageSize) {}

  // Ok se o chip responde no endereço; HardwareAbsent se não.
  Status probe();

  // OutOfRange se offset + length passa de 256.
  Status read(size_t offset, uint8_t *out, size_t length);

  // Grava por páginas e confere lendo de volta. Bloqueia até kWriteCycleTimeoutMs por
  // página: é operação de programação de cartucho, não do laço principal. IoError se o
  // dado lido de volta não confere (proteção de escrita ligada, por exemplo); Timeout se
  // o ciclo de escrita não termina.
  Status write(size_t offset, const uint8_t *data, size_t length);

  uint8_t address() const { return address_; }

 private:
  Status waitWriteCycle();

  II2cBus &bus_;
  IClock &clock_;
  uint8_t address_;
  uint8_t pageSize_;
};

}  // namespace dbmo::cartridges
