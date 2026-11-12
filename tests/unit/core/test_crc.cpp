#include <cstring>

#include "dbmo/core/crc.h"
#include "dbmo_test.h"

void setUp() {}
void tearDown() {}

// Vetores de verificação padrão ("check") dos três algoritmos.
static const uint8_t *kCheck = reinterpret_cast<const uint8_t *>("123456789");

static void test_crc8_check_value() { TEST_ASSERT_EQUAL_HEX8(0xF4, dbmo::crc8(kCheck, 9)); }

static void test_crc16_ccitt_check_value() {
  TEST_ASSERT_EQUAL_HEX16(0x29B1, dbmo::crc16Ccitt(kCheck, 9));
}

static void test_crc32_check_value() {
  TEST_ASSERT_EQUAL_HEX32(0xCBF43926u, dbmo::crc32(kCheck, 9));
}

static void test_empty_input_returns_initial_state() {
  TEST_ASSERT_EQUAL_HEX8(0x00, dbmo::crc8(nullptr, 0));
  TEST_ASSERT_EQUAL_HEX16(0xFFFF, dbmo::crc16Ccitt(nullptr, 0));
  TEST_ASSERT_EQUAL_HEX32(0x00000000u, dbmo::crc32(nullptr, 0));
}

static void test_crc16_can_be_chained() {
  const uint16_t whole = dbmo::crc16Ccitt(kCheck, 9);
  const uint16_t part = dbmo::crc16Ccitt(kCheck, 4);
  TEST_ASSERT_EQUAL_HEX16(whole, dbmo::crc16Ccitt(kCheck + 4, 5, part));
}

static void test_single_bit_flip_changes_the_crc() {
  uint8_t data[9];
  memcpy(data, kCheck, 9);
  data[3] ^= 0x01;
  TEST_ASSERT_NOT_EQUAL(0x29B1, dbmo::crc16Ccitt(data, 9));
  TEST_ASSERT_NOT_EQUAL(0xF4, dbmo::crc8(data, 9));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_crc8_check_value);
  RUN_TEST(test_crc16_ccitt_check_value);
  RUN_TEST(test_crc32_check_value);
  RUN_TEST(test_empty_input_returns_initial_state);
  RUN_TEST(test_crc16_can_be_chained);
  RUN_TEST(test_single_bit_flip_changes_the_crc);
  return UNITY_END();
}
