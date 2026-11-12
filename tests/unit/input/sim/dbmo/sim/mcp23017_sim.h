// Simulador do MCP23017 no nível dos registradores (IOCON.BANK = 0), escrito a partir
// do datasheet DS20001952: direção (IODIR), inversão (IPOL), pull-up (GPPU), leitura dos
// pinos (GPIO) e latch de saída (OLAT). Um pino de entrada sem nada ligado lê 1 se o
// pull-up estiver ligado e 0 se estiver flutuando.
//
// Revisão D do datasheet: GPA7 e GPB7 são somente saída. Se o driver os configurar como
// entrada, o simulador marca outputOnlyViolated e esses pinos leem sempre 0.
#pragma once

#include <cstddef>
#include <cstdint>

#include "dbmo/mock/mock_hal.h"

namespace dbmo::sim {

class Mcp23017Sim final : public mock::II2cDeviceSim {
 public:
  static constexpr uint8_t kRegisterCount = 0x16;
  static constexpr uint8_t kIodirA = 0x00;
  static constexpr uint8_t kIpolA = 0x02;
  static constexpr uint8_t kGppuA = 0x0C;
  static constexpr uint8_t kGpioA = 0x12;
  static constexpr uint8_t kOlatA = 0x14;

  Mcp23017Sim() {
    // Valores de reset: todos os pinos como entrada.
    registers_[kIodirA] = 0xFF;
    registers_[kIodirA + 1] = 0xFF;
  }

  Status onWrite(const uint8_t *data, size_t n) override {
    if (n == 0) return Status::Ok;
    pointer_ = static_cast<uint8_t>(data[0] % kRegisterCount);
    for (size_t i = 1; i < n; ++i) {
      write(pointer_, data[i]);
      pointer_ = static_cast<uint8_t>((pointer_ + 1) % kRegisterCount);
    }
    return Status::Ok;
  }

  Status onRead(uint8_t *data, size_t n) override {
    for (size_t i = 0; i < n; ++i) {
      data[i] = read(pointer_);
      pointer_ = static_cast<uint8_t>((pointer_ + 1) % kRegisterCount);
    }
    return Status::Ok;
  }

  // Botão ao GND apertado: o pino é puxado para nível baixo.
  void press(uint8_t pin) { drive(pin, false); }
  // Botão solto: nada aciona o pino.
  void release(uint8_t pin) { driven_ = static_cast<uint16_t>(driven_ & ~(1u << pin)); }
  void drive(uint8_t pin, bool high) {
    driven_ = static_cast<uint16_t>(driven_ | (1u << pin));
    levels_ = static_cast<uint16_t>(high ? (levels_ | (1u << pin)) : (levels_ & ~(1u << pin)));
  }

  uint16_t reg16(uint8_t regA) const {
    return static_cast<uint16_t>(registers_[regA] | (registers_[regA + 1] << 8));
  }
  // Simula um chip que não guarda o registrador de direção (para testar a conferência).
  bool stuckDirection = false;
  bool outputOnlyViolated = false;

 private:
  void write(uint8_t reg, uint8_t value) {
    if ((reg == kIodirA || reg == kIodirA + 1)) {
      if (stuckDirection) return;
      if ((value & 0x80) != 0) outputOnlyViolated = true;
    }
    if (reg == kGpioA || reg == kGpioA + 1) reg = static_cast<uint8_t>(reg + 2);  // escreve OLAT
    registers_[reg] = value;
  }

  uint8_t read(uint8_t reg) const {
    if (reg != kGpioA && reg != kGpioA + 1) return registers_[reg];
    const uint8_t port = static_cast<uint8_t>(reg - kGpioA);
    const uint8_t direction = registers_[kIodirA + port];
    const uint8_t pullUp = registers_[kGppuA + port];
    const uint8_t driven = static_cast<uint8_t>(driven_ >> (8 * port));
    const uint8_t levels = static_cast<uint8_t>(levels_ >> (8 * port));
    uint8_t pins = static_cast<uint8_t>((levels & driven) | (pullUp & ~driven));
    pins = static_cast<uint8_t>(pins ^ registers_[kIpolA + port]);
    // Entradas leem o pino; saídas leem o latch. O bit 7 nunca funciona como entrada.
    const uint8_t usableInputs = static_cast<uint8_t>(direction & 0x7F);
    return static_cast<uint8_t>((pins & usableInputs) |
                                (registers_[kOlatA + port] & static_cast<uint8_t>(~direction)));
  }

  uint8_t registers_[kRegisterCount] = {};
  uint8_t pointer_ = 0;
  uint16_t driven_ = 0;
  uint16_t levels_ = 0;
};

}  // namespace dbmo::sim
