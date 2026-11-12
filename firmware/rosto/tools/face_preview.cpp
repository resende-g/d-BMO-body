// Pré-visualização dos rostos no computador, sem tela.
//   dbmo_face_preview                 imprime todos os quadros em texto
//   dbmo_face_preview --ppm arquivo   grava uma folha com o primeiro quadro de cada
//                                     expressão, desenhado pelo FaceRenderer de verdade
#include <cstdio>
#include <cstring>

#include "dbmo/display/framebuffer_display.h"
#include "dbmo/rosto/expressions.h"
#include "dbmo/rosto/face_renderer.h"

namespace {

using dbmo::display::Color;
using dbmo::display::FramebufferDisplay;
using namespace dbmo::rosto;

constexpr uint16_t kCellW = 160;  // meia tela de 320 x 240 por expressão
constexpr uint16_t kCellH = 120;
constexpr uint16_t kColumns = 6;
constexpr uint16_t kRows = (kExpressionCount + kColumns - 1) / kColumns;
constexpr uint16_t kSheetW = kCellW * kColumns;
constexpr uint16_t kSheetH = kCellH * kRows;

Color gCell[kCellW * kCellH];
Color gSheet[static_cast<size_t>(kSheetW) * kSheetH];

bool toFile(const uint8_t *data, size_t length, void *context) {
  return fwrite(data, 1, length, static_cast<FILE *>(context)) == length;
}

void printAll() {
  char text[kGridHeight * (kGridWidth + 1) + 1];
  for (uint8_t e = 0; e < kExpressionCount; ++e) {
    const Expression expression = static_cast<Expression>(e);
    for (uint8_t f = 0; f < frameCount(expression); ++f) {
      const FrameView view = frame(expression, f);
      renderText(view, text, sizeof(text));
      printf("%u %s, quadro %u de %u (%u ms)\n%s\n", static_cast<unsigned>(e), name(expression),
             static_cast<unsigned>(f + 1), static_cast<unsigned>(frameCount(expression)),
             static_cast<unsigned>(view.durationMs), text);
    }
  }
}

int writeSheet(const char *path) {
  for (Color &pixel : gSheet) pixel = dbmo::display::palette::kGraphite;
  for (uint8_t e = 0; e < kExpressionCount; ++e) {
    FramebufferDisplay cell(gCell, kCellW, kCellH);
    FaceRenderer renderer(cell);
    if (!dbmo::isOk(renderer.draw(frame(static_cast<Expression>(e), 0)))) return 1;
    const uint16_t originX = static_cast<uint16_t>((e % kColumns) * kCellW);
    const uint16_t originY = static_cast<uint16_t>((e / kColumns) * kCellH);
    // Margem de 2 px em grafite entre as células.
    for (uint16_t y = 2; y < kCellH - 2; ++y)
      for (uint16_t x = 2; x < kCellW - 2; ++x)
        gSheet[static_cast<size_t>(originY + y) * kSheetW + originX + x] = cell.pixel(x, y);
  }
  FILE *file = fopen(path, "wb");
  if (file == nullptr) {
    fprintf(stderr, "nao foi possivel criar %s\n", path);
    return 1;
  }
  const dbmo::Status status = dbmo::display::writePpm(gSheet, kSheetW, kSheetH, toFile, file);
  fclose(file);
  if (!dbmo::isOk(status)) return 1;
  printf("%s: %u x %u px, %u expressoes\n", path, static_cast<unsigned>(kSheetW),
         static_cast<unsigned>(kSheetH), static_cast<unsigned>(kExpressionCount));
  return 0;
}

}  // namespace

int main(int argc, char **argv) {
  if (argc == 3 && strcmp(argv[1], "--ppm") == 0) return writeSheet(argv[2]);
  if (argc != 1) {
    fprintf(stderr, "uso: %s [--ppm arquivo.ppm]\n", argv[0]);
    return 2;
  }
  printAll();
  return 0;
}
