// Contrato mínimo de uma tela. O rosto e os demais módulos desenham só por aqui, então
// a mesma lógica roda na ILI9341 (porte ESP32) e num framebuffer em memória (testes).
#pragma once

#include <cstdint>

#include "dbmo/core/status.h"

namespace dbmo::display {

using Color = uint16_t;  // RGB565

constexpr Color rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return static_cast<Color>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

// Paleta do d-BMO (resende-g/d-BMO, docs/brand/DESIGN_TOKENS.md).
namespace palette {
inline constexpr Color kBlack = rgb565(0x09, 0x0A, 0x0C);      // color.black
inline constexpr Color kGraphite = rgb565(0x24, 0x27, 0x2B);   // color.graphite
inline constexpr Color kSignalRed = rgb565(0xD9, 0x23, 0x32);  // color.red.signal
inline constexpr Color kDarkRed = rgb565(0x8F, 0x17, 0x22);    // color.red.dark
inline constexpr Color kSilver = rgb565(0xD5, 0xD7, 0xD9);     // color.silver.metallic
inline constexpr Color kOffWhite = rgb565(0xF2, 0xF0, 0xEB);   // color.offwhite
}  // namespace palette

// Os mesmos valores RGB565 que o sketch de bancada (firmware/rosto/rosto.ino) usa.
static_assert(palette::kBlack == 0x0841, "color.black");
static_assert(palette::kSignalRed == 0xD906, "color.red.signal");
static_assert(palette::kDarkRed == 0x88A4, "color.red.dark");

class IDisplay {
 public:
  virtual ~IDisplay() = default;

  virtual uint16_t width() const = 0;
  virtual uint16_t height() const = 0;

  // Preenche um retângulo. A parte que cai fora da tela é recortada; um retângulo
  // inteiro fora da tela não é erro.
  virtual Status fillRect(int16_t x, int16_t y, int16_t w, int16_t h, Color color) = 0;

  // Brilho do backlight, de 0 a 100 %. Unsupported quando o hardware não controla o
  // backlight (na bancada de hoje o LED da tela está direto no 3V3).
  virtual Status setBacklight(uint8_t percent) = 0;

  // Põe o painel em repouso (true) ou o acorda (false).
  virtual Status setSleep(bool asleep) = 0;
};

}  // namespace dbmo::display
