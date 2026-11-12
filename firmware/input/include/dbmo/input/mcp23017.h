// Driver mínimo do expansor de I/O MCP23017 (Microchip DS20001952), em I2C.
//
// Supõe IOCON.BANK = 0, que é o estado depois do reset do chip. Cada registrador é
// acessado em uma transação própria, sem depender do avanço automático de endereço.
//
// Revisão D do datasheet: GPA7 e GPB7 são somente saída ("IO7 must be set via I2C
// interface to 0 (output)"). begin() recusa uma máscara de entradas que os inclua.
#pragma once

#include <cstdint>

#include "dbmo/core/hal/i2c.h"
#include "dbmo/core/status.h"

namespace dbmo::input {

class Mcp23017 {
 public:
  // Bit 0–7 = GPA0–7, bit 8–15 = GPB0–7.
  static constexpr uint16_t kOutputOnlyPins = 0x8080;  // GPA7 e GPB7

  // Registradores com IOCON.BANK = 0 (porta A; a porta B é o endereço seguinte).
  static constexpr uint8_t kRegIodir = 0x00;  // 1 = entrada
  static constexpr uint8_t kRegIpol = 0x02;   // 1 = leitura invertida
  static constexpr uint8_t kRegIocon = 0x0A;
  static constexpr uint8_t kRegGppu = 0x0C;  // 1 = pull-up interno de 100 kΩ
  static constexpr uint8_t kRegGpio = 0x12;
  static constexpr uint8_t kRegOlat = 0x14;

  Mcp23017(II2cBus &bus, uint8_t address) : bus_(bus), address_(address) {}

  // Configura os pinos de inputMask como entradas, com pull-up nos de pullUpMask; os
  // demais ficam como saída em nível baixo. Confere a escrita lendo de volta.
  // HardwareAbsent se o chip não responde; InvalidArgument se inputMask inclui GPA7 ou
  // GPB7; ProtocolError se o que foi lido de volta não confere.
  Status begin(uint16_t inputMask, uint16_t pullUpMask);

  // Nível dos 16 pinos.
  Result<uint16_t> readPins();

  Status writeRegister(uint8_t reg, uint8_t value);
  Result<uint8_t> readRegister(uint8_t reg);

 private:
  Status writePair(uint8_t reg, uint16_t value);
  Result<uint16_t> readPair(uint8_t reg);

  II2cBus &bus_;
  uint8_t address_;
};

}  // namespace dbmo::input
