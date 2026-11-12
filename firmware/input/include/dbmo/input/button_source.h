// De onde vêm os botões. O resto do módulo só conhece IButtonSource, então o MCP23017
// pode ser trocado por GPIO direto, por outro expansor ou por um simulador.
#pragma once

#include <cstddef>
#include <cstdint>

#include "dbmo/core/hal/gpio.h"
#include "dbmo/core/status.h"
#include "dbmo/input/mcp23017.h"
#include "dbmo/input/types.h"

namespace dbmo::input {

class IButtonSource {
 public:
  virtual ~IButtonSource() = default;
  // Prepara o hardware. HardwareAbsent se ele não responde. Pode ser chamado de novo.
  virtual Status begin() = 0;
  // Botões apertados neste instante, sem tratamento de repique.
  virtual Result<ButtonMask> read() = 0;
};

// Botões no MCP23017: botão entre o pino e o GND, pull-up interno, ativo em nível baixo.
class Mcp23017ButtonSource final : public IButtonSource {
 public:
  static constexpr uint8_t kUnmapped = 0xFF;

  struct Map {
    // Para cada um dos 16 pinos do expansor, o Button ligado nele ou kUnmapped.
    uint8_t buttonOfPin[16];
  };

  // Mapa proposto em pins.h (namespace mcp_buttons): 14 botões, GPA7 e GPB7 livres.
  static Map defaultMap();

  Mcp23017ButtonSource(Mcp23017 &chip, const Map &map) : chip_(chip), map_(map) {}

  // InvalidArgument se o mapa põe botão em GPA7 ou GPB7 (somente saída no MCP23017).
  Status begin() override;
  Result<ButtonMask> read() override;

 private:
  uint16_t inputMask() const;

  Mcp23017 &chip_;
  Map map_;
};

// Botões ligados direto em GPIOs do microcontrolador (a bancada de hoje).
class GpioButtonSource final : public IButtonSource {
 public:
  struct Entry {
    int pin;
    Button button;
  };

  // entries precisa viver mais que este objeto. activeLow: botão ao GND com pull-up.
  GpioButtonSource(IGpio &gpio, const Entry *entries, size_t count, bool activeLow = true)
      : gpio_(gpio), entries_(entries), count_(count), activeLow_(activeLow) {}

  Status begin() override;
  Result<ButtonMask> read() override;

 private:
  IGpio &gpio_;
  const Entry *entries_;
  size_t count_;
  bool activeLow_;
};

}  // namespace dbmo::input
