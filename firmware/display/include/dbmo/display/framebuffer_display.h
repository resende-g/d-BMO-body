// Tela em memória: um IDisplay sobre um buffer RGB565 fornecido por quem chama. Usada
// nos testes, na pré-visualização dos rostos e nos testes de integração.
#pragma once

#include <cstddef>
#include <cstdint>

#include "dbmo/display/display.h"

namespace dbmo::display {

class FramebufferDisplay final : public IDisplay {
 public:
  // pixels precisa ter width * height posições e viver mais que este objeto.
  FramebufferDisplay(Color *pixels, uint16_t width, uint16_t height);

  uint16_t width() const override { return width_; }
  uint16_t height() const override { return height_; }
  Status fillRect(int16_t x, int16_t y, int16_t w, int16_t h, Color color) override;
  Status setBacklight(uint8_t percent) override;
  Status setSleep(bool asleep) override;

  Color pixel(uint16_t x, uint16_t y) const { return pixels_[static_cast<size_t>(y) * width_ + x]; }
  const Color *pixels() const { return pixels_; }

  // A próxima chamada de fillRect/setBacklight/setSleep devolve este erro (uma vez).
  void failNext(Status status) { nextStatus_ = status; }

  // Falso simula o backlight ligado direto no 3V3: setBacklight() devolve Unsupported.
  bool backlightSupported = true;
  uint8_t backlightPercent = 0;
  bool asleep = false;
  unsigned fillRectCalls = 0;
  unsigned pixelsWritten = 0;

 private:
  Status takeFault();

  Color *pixels_;
  uint16_t width_;
  uint16_t height_;
  Status nextStatus_ = Status::Ok;
};

// Um pixel RGB565 em três bytes R, G, B (os bits altos são repetidos nos baixos).
void rgb565ToRgb888(Color color, uint8_t rgb[3]);

// Destino dos bytes do arquivo PPM. Devolver false interrompe a escrita.
using ByteSink = bool (*)(const uint8_t *data, size_t length, void *context);

// Escreve a imagem em PPM binário (P6). IoError se o destino recusar bytes.
Status writePpm(const Color *pixels, uint16_t width, uint16_t height, ByteSink sink, void *context);

}  // namespace dbmo::display
