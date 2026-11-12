#include "dbmo/display/framebuffer_display.h"
#include "dbmo/rosto/face_renderer.h"
#include "dbmo_test.h"

using dbmo::Status;
using dbmo::display::Color;
using dbmo::display::FramebufferDisplay;
using namespace dbmo::rosto;
namespace palette = dbmo::display::palette;

namespace {
Color gPixels[320 * 240];

// Quantos blocos da grade diferem entre dois quadros.
int differingBlocks(const FrameView &a, const FrameView &b) {
  int count = 0;
  for (int y = 0; y < kGridHeight; ++y)
    for (int x = 0; x < kGridWidth; ++x)
      if (a.rows[y][x] != b.rows[y][x]) ++count;
  return count;
}

int litBlocks(const FrameView &view) {
  int count = 0;
  for (int y = 0; y < kGridHeight; ++y)
    for (int x = 0; x < kGridWidth; ++x)
      if (view.rows[y][x] != '.') ++count;
  return count;
}
}  // namespace

void setUp() {
  for (Color &pixel : gPixels) pixel = 0xFFFF;
}
void tearDown() {}

static void test_geometry_on_the_320x240_panel() {
  FramebufferDisplay display(gPixels, 320, 240);
  FaceRenderer renderer(display);
  // 320/16 = 20 e 240/12 = 20: blocos de 20 px, sem margem.
  TEST_ASSERT_EQUAL_UINT16(20, renderer.blockSize());
  TEST_ASSERT_EQUAL_INT16(0, renderer.originX());
  TEST_ASSERT_EQUAL_INT16(0, renderer.originY());
}

static void test_geometry_is_centred_on_other_sizes() {
  FramebufferDisplay display(gPixels, 240, 320);  // tela em pé
  FaceRenderer renderer(display);
  // 240/16 = 15, 320/12 = 26: vale o menor. Rosto de 240 x 180, centrado na vertical.
  TEST_ASSERT_EQUAL_UINT16(15, renderer.blockSize());
  TEST_ASSERT_EQUAL_INT16(0, renderer.originX());
  TEST_ASSERT_EQUAL_INT16(70, renderer.originY());
}

static void test_first_draw_paints_background_and_lit_blocks() {
  FramebufferDisplay display(gPixels, 320, 240);
  FaceRenderer renderer(display);
  const FrameView idle = frame(Expression::Idle, 0);
  TEST_ASSERT_OK(renderer.draw(idle));

  // Uma chamada para limpar a tela e uma por bloco aceso.
  TEST_ASSERT_EQUAL_UINT(1 + litBlocks(idle), display.fillRectCalls);
  // Olho esquerdo do IDLE: colunas 4 e 5, fileiras 3 a 5 -> pixels x 80..119, y 60..119.
  TEST_ASSERT_EQUAL_HEX16(palette::kSignalRed, display.pixel(80, 60));
  TEST_ASSERT_EQUAL_HEX16(palette::kSignalRed, display.pixel(119, 119));
  TEST_ASSERT_EQUAL_HEX16(palette::kBlack, display.pixel(79, 60));
  TEST_ASSERT_EQUAL_HEX16(palette::kBlack, display.pixel(120, 60));
  TEST_ASSERT_EQUAL_HEX16(palette::kBlack, display.pixel(0, 0));
  TEST_ASSERT_EQUAL_HEX16(palette::kBlack, display.pixel(319, 239));
}

static void test_redrawing_the_same_frame_touches_nothing() {
  FramebufferDisplay display(gPixels, 320, 240);
  FaceRenderer renderer(display);
  TEST_ASSERT_OK(renderer.draw(frame(Expression::Idle, 0)));
  const unsigned calls = display.fillRectCalls;
  TEST_ASSERT_OK(renderer.draw(frame(Expression::Idle, 0)));
  TEST_ASSERT_EQUAL_UINT(calls, display.fillRectCalls);
}

static void test_next_frame_redraws_only_the_changed_blocks() {
  FramebufferDisplay display(gPixels, 320, 240);
  FaceRenderer renderer(display);
  const FrameView open = frame(Expression::Idle, 0);
  const FrameView blink = frame(Expression::Idle, 1);
  TEST_ASSERT_OK(renderer.draw(open));
  const unsigned calls = display.fillRectCalls;

  TEST_ASSERT_OK(renderer.draw(blink));
  // A piscada apaga as duas fileiras de cima de cada olho: 8 blocos.
  TEST_ASSERT_EQUAL_INT(8, differingBlocks(open, blink));
  TEST_ASSERT_EQUAL_UINT(calls + 8, display.fillRectCalls);
  TEST_ASSERT_EQUAL_HEX16(palette::kBlack, display.pixel(80, 60));       // apagou
  TEST_ASSERT_EQUAL_HEX16(palette::kSignalRed, display.pixel(80, 100));  // ficou
}

static void test_dark_red_blocks_use_the_second_colour() {
  FramebufferDisplay display(gPixels, 320, 240);
  FaceRenderer renderer(display);
  TEST_ASSERT_OK(renderer.draw(frame(Expression::Offline, 0)));
  // OFFLINE: olhos fechados em vermelho escuro na fileira 5, colunas 4 e 5.
  TEST_ASSERT_EQUAL_HEX16(palette::kDarkRed, display.pixel(80, 100));
  TEST_ASSERT_EQUAL_HEX16(palette::kDarkRed, FaceRenderer::colorOf('+'));
  TEST_ASSERT_EQUAL_HEX16(palette::kSignalRed, FaceRenderer::colorOf('#'));
  TEST_ASSERT_EQUAL_HEX16(palette::kBlack, FaceRenderer::colorOf('.'));
}

static void test_invalidate_forces_a_full_redraw() {
  FramebufferDisplay display(gPixels, 320, 240);
  FaceRenderer renderer(display);
  const FrameView idle = frame(Expression::Idle, 0);
  TEST_ASSERT_OK(renderer.draw(idle));
  for (Color &pixel : gPixels) pixel = 0xFFFF;  // outra coisa desenhou por cima
  renderer.invalidate();
  const unsigned calls = display.fillRectCalls;
  TEST_ASSERT_OK(renderer.draw(idle));
  TEST_ASSERT_EQUAL_UINT(calls + 1 + litBlocks(idle), display.fillRectCalls);
  TEST_ASSERT_EQUAL_HEX16(palette::kBlack, display.pixel(0, 0));
}

static void test_display_error_is_reported_and_next_draw_recovers() {
  FramebufferDisplay display(gPixels, 320, 240);
  FaceRenderer renderer(display);
  const FrameView idle = frame(Expression::Idle, 0);
  TEST_ASSERT_OK(renderer.draw(idle));

  display.failNext(Status::IoError);
  TEST_ASSERT_STATUS(Status::IoError, renderer.draw(frame(Expression::Error, 0)));
  // A tela ficou em estado desconhecido: o desenho seguinte é completo e fica certo.
  const FrameView error = frame(Expression::Error, 0);
  const unsigned calls = display.fillRectCalls;
  TEST_ASSERT_OK(renderer.draw(error));
  TEST_ASSERT_EQUAL_UINT(calls + 1 + litBlocks(error), display.fillRectCalls);
}

static void test_display_smaller_than_the_grid_is_unsupported() {
  FramebufferDisplay display(gPixels, 15, 11);
  FaceRenderer renderer(display);
  TEST_ASSERT_STATUS(Status::Unsupported, renderer.draw(frame(Expression::Idle, 0)));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_geometry_on_the_320x240_panel);
  RUN_TEST(test_geometry_is_centred_on_other_sizes);
  RUN_TEST(test_first_draw_paints_background_and_lit_blocks);
  RUN_TEST(test_redrawing_the_same_frame_touches_nothing);
  RUN_TEST(test_next_frame_redraws_only_the_changed_blocks);
  RUN_TEST(test_dark_red_blocks_use_the_second_colour);
  RUN_TEST(test_invalidate_forces_a_full_redraw);
  RUN_TEST(test_display_error_is_reported_and_next_draw_recovers);
  RUN_TEST(test_display_smaller_than_the_grid_is_unsupported);
  return UNITY_END();
}
