#include <cstring>

#include "dbmo/core/crc.h"
#include "dbmo/rosto/expressions.h"
#include "dbmo_test.h"

using dbmo::Status;
using namespace dbmo::rosto;

void setUp() {}
void tearDown() {}

static void test_every_expression_has_valid_frames() {
  for (uint8_t e = 0; e < kExpressionCount; ++e) {
    const Expression expression = static_cast<Expression>(e);
    TEST_ASSERT_TRUE_MESSAGE(frameCount(expression) >= 1, name(expression));
    for (uint8_t f = 0; f < frameCount(expression); ++f) {
      const FrameView view = frame(expression, f);
      TEST_ASSERT_TRUE_MESSAGE(view.durationMs > 0, name(expression));
      for (int y = 0; y < kGridHeight; ++y) {
        TEST_ASSERT_NOT_NULL_MESSAGE(view.rows[y], name(expression));
        TEST_ASSERT_EQUAL_size_t_MESSAGE(kGridWidth, strlen(view.rows[y]), name(expression));
        TEST_ASSERT_EQUAL_size_t_MESSAGE(kGridWidth, strspn(view.rows[y], "#+."), name(expression));
      }
    }
  }
}

static void test_names_are_unique_and_the_required_ones_exist() {
  for (uint8_t a = 0; a < kExpressionCount; ++a)
    for (uint8_t b = static_cast<uint8_t>(a + 1); b < kExpressionCount; ++b)
      TEST_ASSERT_NOT_EQUAL(
          0, strcmp(name(static_cast<Expression>(a)), name(static_cast<Expression>(b))));

  // Os treze estados pedidos para o v1 e os quatro da especificação que foram mantidos.
  const char *required[] = {"IDLE",       "HAPPY",    "SAD",      "ANGRY",     "THINKING",
                            "LISTENING",  "SPEAKING", "SCANNING", "SUCCESS",   "WARNING",
                            "ERROR",      "SLEEP",    "OFFLINE",  "ANALYZING", "ALERT",
                            "PROTECTING", "CURIOUS"};
  TEST_ASSERT_EQUAL_size_t(kExpressionCount, sizeof(required) / sizeof(required[0]));
  for (const char *text : required) TEST_ASSERT_TRUE_MESSAGE(expressionFromName(text).ok(), text);
}

static void test_enum_order_matches_the_table() {
  TEST_ASSERT_EQUAL_STRING("IDLE", name(Expression::Idle));
  TEST_ASSERT_EQUAL_STRING("OFFLINE", name(Expression::Offline));
  TEST_ASSERT_EQUAL_STRING("HAPPY", name(Expression::Happy));
  TEST_ASSERT_EQUAL_STRING("LISTENING", name(Expression::Listening));
  TEST_ASSERT_EQUAL_STRING("WARNING", name(Expression::Warning));
  TEST_ASSERT_EQUAL_STRING("SLEEP", name(Expression::Sleep));
}

// Os dez rostos da especificação não podem mudar sem querer. Cada linha é o CRC-32 de
// todos os quadros da expressão (as 12 fileiras e a duração em little-endian),
// calculado sobre a versão original de rosto_faces.h (commit 49f0d06).
static void test_the_ten_specification_faces_are_unchanged() {
  struct Golden {
    const char *name;
    uint8_t frames;
    uint32_t crc;
  };
  const Golden golden[] = {
      {"IDLE", 2, 0x39F6FB9Fu},      {"THINKING", 3, 0x94C7CB1Du},   {"SCANNING", 4, 0x9FA46C50u},
      {"ANALYZING", 2, 0x15651EE9u}, {"SUCCESS", 1, 0x9772B1E1u},    {"ALERT", 2, 0x83F2104Fu},
      {"ERROR", 1, 0x297C75D9u},     {"PROTECTING", 2, 0x62282462u}, {"CURIOUS", 1, 0x47066F9Du},
      {"OFFLINE", 2, 0xCFC42F08u},
  };
  for (uint8_t e = 0; e < 10; ++e) {
    const Expression expression = static_cast<Expression>(e);
    TEST_ASSERT_EQUAL_STRING(golden[e].name, name(expression));
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(golden[e].frames, frameCount(expression), golden[e].name);

    uint8_t data[4 * (kGridHeight * kGridWidth + 2)];
    size_t length = 0;
    for (uint8_t f = 0; f < frameCount(expression); ++f) {
      const FrameView view = frame(expression, f);
      for (int y = 0; y < kGridHeight; ++y) {
        memcpy(data + length, view.rows[y], kGridWidth);
        length += kGridWidth;
      }
      data[length++] = static_cast<uint8_t>(view.durationMs & 0xFF);
      data[length++] = static_cast<uint8_t>(view.durationMs >> 8);
    }
    TEST_ASSERT_EQUAL_HEX32_MESSAGE(golden[e].crc, dbmo::crc32(data, length), golden[e].name);
  }
}

// Estado não pode ser comunicado só pela cor: cada par de expressões precisa diferir na
// forma (onde há bloco aceso), e não apenas em qual das duas cores foi usada.
static void test_expressions_differ_by_shape_not_only_by_colour() {
  auto sameShape = [](Expression a, Expression b) {
    const FrameView va = frame(a, 0);
    const FrameView vb = frame(b, 0);
    for (int y = 0; y < kGridHeight; ++y)
      for (int x = 0; x < kGridWidth; ++x)
        if ((va.rows[y][x] == '.') != (vb.rows[y][x] == '.')) return false;
    return true;
  };
  for (uint8_t a = 0; a < kExpressionCount; ++a)
    for (uint8_t b = static_cast<uint8_t>(a + 1); b < kExpressionCount; ++b)
      TEST_ASSERT_FALSE_MESSAGE(sameShape(static_cast<Expression>(a), static_cast<Expression>(b)),
                                name(static_cast<Expression>(b)));
}

static void test_lookup_by_name_and_index() {
  TEST_ASSERT_TRUE(expressionFromName("warning").value() == Expression::Warning);
  TEST_ASSERT_TRUE(expressionFromName("Sleep").value() == Expression::Sleep);
  TEST_ASSERT_STATUS(Status::NotFound, expressionFromName("SLEEPY").status());
  TEST_ASSERT_STATUS(Status::NotFound, expressionFromName("").status());
  TEST_ASSERT_STATUS(Status::InvalidArgument, expressionFromName(nullptr).status());

  TEST_ASSERT_TRUE(expressionFromIndex(0).value() == Expression::Idle);
  TEST_ASSERT_TRUE(expressionFromIndex(16).value() == Expression::Sleep);
  TEST_ASSERT_STATUS(Status::OutOfRange, expressionFromIndex(17).status());
  TEST_ASSERT_EQUAL_UINT8(0, frameCount(static_cast<Expression>(200)));
}

static void test_text_rendering() {
  char text[kGridHeight * (kGridWidth + 1) + 1];
  const size_t length = renderText(frame(Expression::Idle, 0), text, sizeof(text));
  TEST_ASSERT_EQUAL_size_t(sizeof(text) - 1, length);
  // Quarta fileira do IDLE: os dois olhos de bloco.
  TEST_ASSERT_EQUAL_INT(0, strncmp(text + 3 * (kGridWidth + 1), "....##....##....\n", 17));

  char small[32];
  TEST_ASSERT_EQUAL_size_t(0, renderText(frame(Expression::Idle, 0), small, sizeof(small)));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_every_expression_has_valid_frames);
  RUN_TEST(test_names_are_unique_and_the_required_ones_exist);
  RUN_TEST(test_enum_order_matches_the_table);
  RUN_TEST(test_the_ten_specification_faces_are_unchanged);
  RUN_TEST(test_expressions_differ_by_shape_not_only_by_colour);
  RUN_TEST(test_lookup_by_name_and_index);
  RUN_TEST(test_text_rendering);
  return UNITY_END();
}
