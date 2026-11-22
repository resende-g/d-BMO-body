#include <cstring>

#include "dbmo/display/framebuffer_display.h"
#include "dbmo/display/text.h"
#include "dbmo_test.h"

using dbmo::Status;
using namespace dbmo::display;

namespace {
constexpr uint16_t kW = 320;
constexpr uint16_t kH = 240;
Color gPixels[kW * kH];

constexpr Color kInk = 0xFFFF;

// Retângulos que drawText precisa para um caractere: um por sequência de pixels acesos.
unsigned runsOf(char c) {
  unsigned runs = 0;
  const uint8_t *rows = glyph(c);
  for (uint8_t row = 0; row < kGlyphHeight; ++row)
    for (uint8_t column = 0; column < kGlyphWidth; ++column) {
      const bool lit = (rows[row] & (0x10 >> column)) != 0;
      const bool previous = column > 0 && (rows[row] & (0x10 >> (column - 1))) != 0;
      if (lit && !previous) ++runs;
    }
  return runs;
}

bool rowIsBlank(const FramebufferDisplay &panel, size_t row) {
  for (uint16_t y = static_cast<uint16_t>(row * TextScreen::kCellHeight);
       y < (row + 1) * TextScreen::kCellHeight; ++y)
    for (uint16_t x = 0; x < kW; ++x)
      if (panel.pixel(x, y) != palette::kBlack) return false;
  return true;
}
}  // namespace

void setUp() { memset(gPixels, 0, sizeof(gPixels)); }
void tearDown() {}

static void test_every_printable_character_has_its_own_glyph() {
  const uint8_t *question = glyph('?');
  for (int c = 0x20; c <= 0x7E; ++c) {
    const uint8_t *rows = glyph(static_cast<char>(c));
    TEST_ASSERT_NOT_NULL(rows);
    for (uint8_t row = 0; row < kGlyphHeight; ++row) TEST_ASSERT_EQUAL_HEX8(0, rows[row] & 0xE0);
    if (c != '?') TEST_ASSERT_TRUE(memcmp(rows, question, kGlyphHeight) != 0);
  }
  // Espaço é vazio; fora da faixa vira '?'.
  for (uint8_t row = 0; row < kGlyphHeight; ++row) TEST_ASSERT_EQUAL_HEX8(0, glyph(' ')[row]);
  TEST_ASSERT_EQUAL_PTR(question, glyph('\x7F'));
  TEST_ASSERT_EQUAL_PTR(question, glyph('\n'));
  TEST_ASSERT_EQUAL_PTR(question, glyph(static_cast<char>(0xC3)));
}

static void test_draw_text_reproduces_the_glyph_at_scale() {
  FramebufferDisplay panel{gPixels, kW, kH};
  TEST_ASSERT_OK(drawText(panel, 10, 20, "Ab", 2, kInk));
  const char text[] = "Ab";
  for (int i = 0; i < 2; ++i) {
    const uint8_t *rows = glyph(text[i]);
    const int x0 = 10 + i * (kGlyphWidth + 1) * 2;
    for (int row = 0; row < kGlyphHeight; ++row)
      for (int column = 0; column < kGlyphWidth; ++column) {
        const bool lit = (rows[row] & (0x10 >> column)) != 0;
        for (int dy = 0; dy < 2; ++dy)
          for (int dx = 0; dx < 2; ++dx)
            TEST_ASSERT_EQUAL_HEX16(lit ? kInk : 0,
                                    panel.pixel(static_cast<uint16_t>(x0 + column * 2 + dx),
                                                static_cast<uint16_t>(20 + row * 2 + dy)));
      }
    // A coluna de espaço entre caracteres fica livre.
    for (int y = 20; y < 20 + kGlyphHeight * 2; ++y)
      TEST_ASSERT_EQUAL_HEX16(0, panel.pixel(static_cast<uint16_t>(x0 + kGlyphWidth * 2), y));
  }
  // Uma chamada de fillRect por sequência de pixels acesos, não uma por pixel.
  TEST_ASSERT_EQUAL_UINT(runsOf('A') + runsOf('b'), panel.fillRectCalls);
}

static void test_draw_text_limits_and_errors() {
  FramebufferDisplay panel{gPixels, kW, kH};
  TEST_ASSERT_OK(drawText(panel, 0, 0, "AAAA", 1, kInk, 2));
  TEST_ASSERT_EQUAL_UINT(runsOf('A') * 2, panel.fillRectCalls);
  TEST_ASSERT_STATUS(Status::InvalidArgument, drawText(panel, 0, 0, nullptr, 1, kInk));
  TEST_ASSERT_STATUS(Status::InvalidArgument, drawText(panel, 0, 0, "A", 0, kInk));
  // Fora da tela não é erro: o FramebufferDisplay recorta.
  TEST_ASSERT_OK(drawText(panel, 315, 235, "WW", 2, kInk));
  panel.failNext(Status::IoError);
  TEST_ASSERT_STATUS(Status::IoError, drawText(panel, 0, 0, "A", 1, kInk));
}

static void test_text_screen_fits_the_landscape_panel() {
  FramebufferDisplay panel{gPixels, kW, kH};
  TextScreen screen(panel);
  TEST_ASSERT_EQUAL_UINT(26, screen.columns());
  TEST_ASSERT_EQUAL_UINT(15, screen.rows());
  screen.setLine(0, "abcdefghijklmnopqrstuvwxyz0123456789");  // corta em 26
  TEST_ASSERT_EQUAL_STRING("abcdefghijklmnopqrstuvwxyz", screen.line(0));
  screen.printLine(1, TextScreen::Style::Hint, "%s %d", "x", 7);
  TEST_ASSERT_EQUAL_STRING("x 7", screen.line(1));
  TEST_ASSERT_TRUE(screen.style(1) == TextScreen::Style::Hint);
  screen.setLine(99, "fora");  // ignorada
  TEST_ASSERT_EQUAL_STRING("", screen.line(99));
}

static void test_text_screen_redraws_only_what_changed() {
  FramebufferDisplay panel{gPixels, kW, kH};
  TextScreen screen(panel);
  screen.setLine(0, "Menu", TextScreen::Style::Title);
  screen.setLine(2, "> Estado", TextScreen::Style::Selected);
  screen.setLine(3, "  Sobre");
  TEST_ASSERT_OK(screen.render());
  TEST_ASSERT_EQUAL_HEX16(palette::kSignalRed, panel.pixel(0, 0));  // faixa do título
  TEST_ASSERT_EQUAL_HEX16(palette::kSilver, panel.pixel(0, 2 * TextScreen::kCellHeight));
  TEST_ASSERT_TRUE(rowIsBlank(panel, 1));
  TEST_ASSERT_FALSE(rowIsBlank(panel, 3));

  // Nada mudou: nenhum desenho.
  const unsigned calls = panel.fillRectCalls;
  TEST_ASSERT_OK(screen.render());
  TEST_ASSERT_EQUAL_UINT(calls, panel.fillRectCalls);

  // Só a linha 3 mudou: a linha 2 não é tocada.
  panel.pixelsWritten = 0;
  screen.setLine(3, "  Botoes");
  TEST_ASSERT_OK(screen.render());
  TEST_ASSERT_TRUE(panel.pixelsWritten <= static_cast<unsigned>(kW) * TextScreen::kCellHeight +
                                              static_cast<unsigned>(kW) * TextScreen::kCellHeight);
  TEST_ASSERT_EQUAL_HEX16(palette::kSilver, panel.pixel(0, 2 * TextScreen::kCellHeight));

  // Linha apagada volta a ser fundo preto.
  screen.setLine(3, "");
  TEST_ASSERT_OK(screen.render());
  TEST_ASSERT_TRUE(rowIsBlank(panel, 3));

  // Outra coisa desenhou na tela: invalidate() repinta tudo.
  TEST_ASSERT_OK(panel.fillRect(0, 0, kW, kH, 0x1234));
  screen.invalidate();
  TEST_ASSERT_OK(screen.render());
  TEST_ASSERT_EQUAL_HEX16(palette::kSignalRed, panel.pixel(0, 0));
  TEST_ASSERT_TRUE(rowIsBlank(panel, 14));
}

static void test_text_screen_retries_after_a_panel_error() {
  FramebufferDisplay panel{gPixels, kW, kH};
  TextScreen screen(panel);
  TEST_ASSERT_OK(screen.render());
  screen.setLine(4, "Radio");
  panel.failNext(Status::IoError);
  TEST_ASSERT_STATUS(Status::IoError, screen.render());
  TEST_ASSERT_TRUE(rowIsBlank(panel, 4));
  TEST_ASSERT_OK(screen.render());
  TEST_ASSERT_FALSE(rowIsBlank(panel, 4));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_every_printable_character_has_its_own_glyph);
  RUN_TEST(test_draw_text_reproduces_the_glyph_at_scale);
  RUN_TEST(test_draw_text_limits_and_errors);
  RUN_TEST(test_text_screen_fits_the_landscape_panel);
  RUN_TEST(test_text_screen_redraws_only_what_changed);
  RUN_TEST(test_text_screen_retries_after_a_panel_error);
  return UNITY_END();
}
