// Texto na tela com a fonte de bitmap 5x7 própria (src/font5x7.cpp), desenhado com
// fillRect. Sem biblioteca de fontes: tudo passa pelo IDisplay, então o desenho é
// testável com o FramebufferDisplay.
#pragma once

#include <cstddef>
#include <cstdint>

#include "dbmo/display/display.h"

namespace dbmo::display {

inline constexpr uint8_t kGlyphWidth = 5;
inline constexpr uint8_t kGlyphHeight = 7;

// As 7 linhas do caractere, de cima para baixo; o bit 4 é a coluna da esquerda. Fora da
// faixa 0x20 a 0x7E devolve o desenho de '?'.
const uint8_t *glyph(char c);

// Desenha só os pixels acesos de text a partir de (x, y), cada um como um quadrado de
// scale x scale. Cada caractere ocupa (kGlyphWidth + 1) * scale de largura. Desenha no
// máximo maxChars caracteres e não pinta o fundo.
Status drawText(IDisplay &display, int16_t x, int16_t y, const char *text, uint8_t scale,
                Color color, size_t maxChars = SIZE_MAX);

// Tela de texto em linhas, para o menu. Guarda o que já está desenhado e, em render(),
// redesenha só as linhas que mudaram: o laço principal não repinta a tela à toa.
class TextScreen {
 public:
  static constexpr uint8_t kScale = 2;
  static constexpr uint8_t kCellWidth = (kGlyphWidth + 1) * kScale;
  static constexpr uint8_t kCellHeight = (kGlyphHeight + 1) * kScale;
  static constexpr size_t kMaxColumns = 26;  // 320 px de largura
  static constexpr size_t kMaxRows = 15;     // 240 px de altura

  enum class Style : uint8_t {
    Normal,    // texto claro em fundo preto
    Title,     // faixa vermelha
    Selected,  // item sob o cursor: fundo prata, texto preto
    Hint,      // dica de uso, em prata
  };

  explicit TextScreen(IDisplay &display);

  size_t columns() const { return columns_; }
  size_t rows() const { return rows_; }

  // Esvazia todas as linhas (o que vale para o próximo render()).
  void clear();
  // Texto da linha row, cortado em columns(). Linha fora da tela é ignorada.
  void setLine(size_t row, const char *text, Style style = Style::Normal);
  void printLine(size_t row, Style style, const char *format, ...)
      __attribute__((format(printf, 4, 5)));
  const char *line(size_t row) const { return row < rows_ ? wanted_[row].text : ""; }
  Style style(size_t row) const { return row < rows_ ? wanted_[row].style : Style::Normal; }

  // Esquece o que está na tela: o próximo render() pinta tudo de novo.
  void invalidate() { valid_ = false; }
  // Desenha as linhas que mudaram. Em caso de erro da tela, a linha fica para a próxima.
  Status render();

 private:
  struct Line {
    char text[kMaxColumns + 1] = {};
    Style style = Style::Normal;
  };
  static bool same(const Line &a, const Line &b);
  Status drawLine(size_t row);

  IDisplay &display_;
  size_t columns_;
  size_t rows_;
  int16_t marginX_;
  bool valid_ = false;
  Line wanted_[kMaxRows];
  Line shown_[kMaxRows];
};

}  // namespace dbmo::display
