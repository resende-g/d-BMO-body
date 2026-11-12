#include "dbmo/core/version.h"
#include "dbmo_test.h"

using dbmo::Status;
using dbmo::Version;

void setUp() {}
void tearDown() {}

static void test_parse_accepts_one_to_three_parts() {
  auto full = dbmo::parseVersion("1.2.3");
  TEST_ASSERT_TRUE(full.ok());
  TEST_ASSERT_TRUE(full.value() == (Version{1, 2, 3}));

  auto two = dbmo::parseVersion("4.5");
  TEST_ASSERT_TRUE(two.ok());
  TEST_ASSERT_TRUE(two.value() == (Version{4, 5, 0}));

  auto one = dbmo::parseVersion("255");
  TEST_ASSERT_TRUE(one.ok());
  TEST_ASSERT_TRUE(one.value() == (Version{255, 0, 0}));
}

static void test_parse_rejects_malformed_text() {
  TEST_ASSERT_STATUS(Status::InvalidArgument, dbmo::parseVersion(nullptr).status());
  TEST_ASSERT_STATUS(Status::InvalidArgument, dbmo::parseVersion("").status());
  TEST_ASSERT_STATUS(Status::InvalidArgument, dbmo::parseVersion("1.").status());
  TEST_ASSERT_STATUS(Status::InvalidArgument, dbmo::parseVersion(".1").status());
  TEST_ASSERT_STATUS(Status::InvalidArgument, dbmo::parseVersion("1.2.3.4").status());
  TEST_ASSERT_STATUS(Status::InvalidArgument, dbmo::parseVersion("1.x").status());
  TEST_ASSERT_STATUS(Status::InvalidArgument, dbmo::parseVersion("v1.0").status());
  TEST_ASSERT_STATUS(Status::OutOfRange, dbmo::parseVersion("256.0.0").status());
}

static void test_ordering() {
  TEST_ASSERT_TRUE((Version{1, 2, 3}) < (Version{1, 2, 4}));
  TEST_ASSERT_TRUE((Version{1, 9, 9}) < (Version{2, 0, 0}));
  TEST_ASSERT_TRUE((Version{1, 2, 3}) >= (Version{1, 2, 3}));
  TEST_ASSERT_TRUE((Version{1, 2, 3}) != (Version{1, 3, 3}));
  TEST_ASSERT_EQUAL_INT(1, (Version{0, 2, 0}).compare(Version{0, 1, 9}));
}

static void test_format_round_trip_and_small_buffer() {
  char text[12];
  TEST_ASSERT_EQUAL_size_t(6, dbmo::formatVersion(Version{10, 0, 7}, text, sizeof(text)));
  TEST_ASSERT_EQUAL_STRING("10.0.7", text);
  TEST_ASSERT_TRUE(dbmo::parseVersion(text).value() == (Version{10, 0, 7}));

  char tiny[4];
  TEST_ASSERT_EQUAL_size_t(0, dbmo::formatVersion(Version{10, 0, 7}, tiny, sizeof(tiny)));
  TEST_ASSERT_EQUAL_STRING("", tiny);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_parse_accepts_one_to_three_parts);
  RUN_TEST(test_parse_rejects_malformed_text);
  RUN_TEST(test_ordering);
  RUN_TEST(test_format_round_trip_and_small_buffer);
  return UNITY_END();
}
