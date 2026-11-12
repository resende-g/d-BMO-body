#include "dbmo/rosto/expressions.h"

// A tabela de desenhos. Incluída só aqui: ela define nomes globais (Frame, Face, FACES).
#include "rosto_faces.h"

namespace dbmo::rosto {

static_assert(GRID_W == kGridWidth && GRID_H == kGridHeight, "grade diferente da tabela");
static_assert(FACE_COUNT == kExpressionCount, "tabela e enum Expression fora de sincronia");

namespace {

char lower(char c) { return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c; }

bool equalsIgnoreCase(const char *a, const char *b) {
  while (*a != '\0' && *b != '\0') {
    if (lower(*a) != lower(*b)) return false;
    ++a;
    ++b;
  }
  return *a == *b;
}

const Face &faceOf(Expression expression) { return FACES[static_cast<uint8_t>(expression)]; }

}  // namespace

const char *name(Expression expression) {
  if (static_cast<uint8_t>(expression) >= kExpressionCount) return "?";
  return faceOf(expression).name;
}

Result<Expression> expressionFromName(const char *text) {
  if (text == nullptr) return Status::InvalidArgument;
  for (uint8_t i = 0; i < kExpressionCount; ++i)
    if (equalsIgnoreCase(text, FACES[i].name)) return static_cast<Expression>(i);
  return Status::NotFound;
}

Result<Expression> expressionFromIndex(uint32_t index) {
  if (index >= kExpressionCount) return Status::OutOfRange;
  return static_cast<Expression>(index);
}

uint8_t frameCount(Expression expression) {
  if (static_cast<uint8_t>(expression) >= kExpressionCount) return 0;
  return faceOf(expression).count;
}

FrameView frame(Expression expression, uint8_t index) {
  const Frame &source = faceOf(expression).frames[index];
  return FrameView{source.rows, source.ms};
}

size_t renderText(const FrameView &view, char *out, size_t capacity) {
  const size_t needed = static_cast<size_t>(kGridHeight) * (kGridWidth + 1) + 1;
  if (out == nullptr || capacity < needed) return 0;
  size_t used = 0;
  for (int y = 0; y < kGridHeight; ++y) {
    for (int x = 0; x < kGridWidth; ++x) out[used++] = view.rows[y][x];
    out[used++] = '\n';
  }
  out[used] = '\0';
  return used;
}

}  // namespace dbmo::rosto
