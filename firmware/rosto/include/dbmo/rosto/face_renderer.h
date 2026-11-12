// Desenha um quadro do rosto em um IDisplay, em blocos quadrados, redesenhando só os
// blocos que mudaram desde o último desenho. Não decide o que mostrar (isso é do
// FaceEngine).
#pragma once

#include <cstdint>

#include "dbmo/display/display.h"
#include "dbmo/rosto/expressions.h"

namespace dbmo::rosto {

class FaceRenderer {
 public:
  explicit FaceRenderer(display::IDisplay &display);

  // Desenha o quadro. Na primeira vez (ou depois de invalidate()) limpa a tela inteira
  // e desenha tudo; depois, só os blocos diferentes do quadro anterior. Se a tela
  // devolver erro, o próximo draw() volta a desenhar tudo.
  Status draw(const FrameView &view);

  // Esquece o que está na tela (outra coisa desenhou por cima, ou a tela acordou).
  void invalidate() { valid_ = false; }

  // Maior bloco inteiro que cabe na tela, com o rosto centralizado.
  uint16_t blockSize() const;
  int16_t originX() const;
  int16_t originY() const;

  static display::Color colorOf(char pixel);

 private:
  display::IDisplay &display_;
  char shown_[kGridHeight][kGridWidth];
  bool valid_ = false;
};

}  // namespace dbmo::rosto
