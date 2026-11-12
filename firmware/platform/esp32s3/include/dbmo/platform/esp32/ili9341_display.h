// Adapta a biblioteca Adafruit_ILI9341 (a mesma dos sketches de bancada, já validada
// com a tela) ao contrato dbmo::display::IDisplay.
#pragma once

#include <Adafruit_ILI9341.h>

#include "dbmo/core/hal/adc.h"
#include "dbmo/core/hal/clock.h"
#include "dbmo/display/display.h"

namespace dbmo::platform::esp32 {

class Ili9341Display final : public display::IDisplay {
 public:
  // backlight == nullptr: o LED da tela não é controlado pelo firmware (na bancada de
  // hoje ele está direto no 3V3) e setBacklight() devolve Unsupported.
  Ili9341Display(Adafruit_ILI9341 &tft, IClock &clock, IPwmOutput *backlight)
      : tft_(tft), clock_(clock), backlight_(backlight) {}

  void begin(uint32_t spiHz, uint8_t rotation) {
    tft_.begin(spiHz);
    tft_.setRotation(rotation);
  }

  // Identificador lido do controlador (0x9341 esperado). 0x0000 ou 0xFFFF indicam MISO
  // solto ou chip diferente; a tela pode funcionar mesmo assim, só de escrita.
  uint16_t readId() {
    return static_cast<uint16_t>((tft_.readcommand8(0xD3, 2) << 8) | tft_.readcommand8(0xD3, 3));
  }

  uint16_t width() const override { return static_cast<uint16_t>(tft_.width()); }
  uint16_t height() const override { return static_cast<uint16_t>(tft_.height()); }

  Status fillRect(int16_t x, int16_t y, int16_t w, int16_t h, display::Color color) override {
    tft_.fillRect(x, y, w, h, color);
    return Status::Ok;
  }

  Status setBacklight(uint8_t percent) override {
    if (backlight_ == nullptr) return Status::Unsupported;
    if (percent > 100) return Status::OutOfRange;
    return backlight_->setDutyPermille(static_cast<uint16_t>(percent * 10));
  }

  Status setSleep(bool asleep) override {
    if (asleep) {
      tft_.sendCommand(ILI9341_DISPOFF);
      tft_.sendCommand(ILI9341_SLPIN);
    } else {
      tft_.sendCommand(ILI9341_SLPOUT);
      // Mesma espera que a Adafruit_ILI9341 usa depois do Sleep Out na inicialização
      // (150 ms). Só ocorre ao acordar a tela, não no laço normal.
      clock_.delayMs(kSleepOutDelayMs);
      tft_.sendCommand(ILI9341_DISPON);
    }
    return Status::Ok;
  }

 private:
  static constexpr uint32_t kSleepOutDelayMs = 150;

  Adafruit_ILI9341 &tft_;
  IClock &clock_;
  IPwmOutput *backlight_;
};

}  // namespace dbmo::platform::esp32
