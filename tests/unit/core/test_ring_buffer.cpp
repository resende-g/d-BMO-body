#include "dbmo/core/ring_buffer.h"
#include "dbmo_test.h"

using dbmo::RingBuffer;

void setUp() {}
void tearDown() {}

static void test_fifo_order_and_limits() {
  RingBuffer<int, 3> ring;
  TEST_ASSERT_TRUE(ring.empty());
  TEST_ASSERT_TRUE(ring.push(1));
  TEST_ASSERT_TRUE(ring.push(2));
  TEST_ASSERT_TRUE(ring.push(3));
  TEST_ASSERT_TRUE(ring.full());
  TEST_ASSERT_FALSE(ring.push(4));
  TEST_ASSERT_EQUAL_size_t(3, ring.size());

  int value = 0;
  TEST_ASSERT_TRUE(ring.pop(value));
  TEST_ASSERT_EQUAL_INT(1, value);
  TEST_ASSERT_TRUE(ring.push(4));
  TEST_ASSERT_EQUAL_INT(2, ring.at(0));
  TEST_ASSERT_EQUAL_INT(4, ring.at(2));

  TEST_ASSERT_TRUE(ring.pop(value));
  TEST_ASSERT_TRUE(ring.pop(value));
  TEST_ASSERT_TRUE(ring.pop(value));
  TEST_ASSERT_EQUAL_INT(4, value);
  TEST_ASSERT_FALSE(ring.pop(value));
}

static void test_overwrite_drops_the_oldest() {
  RingBuffer<int, 2> ring;
  TEST_ASSERT_FALSE(ring.pushOverwrite(1));
  TEST_ASSERT_FALSE(ring.pushOverwrite(2));
  TEST_ASSERT_TRUE(ring.pushOverwrite(3));
  TEST_ASSERT_EQUAL_size_t(2, ring.size());
  TEST_ASSERT_EQUAL_INT(2, ring.at(0));
  TEST_ASSERT_EQUAL_INT(3, ring.at(1));
}

static void test_bulk_write_and_read_wrap_around() {
  RingBuffer<int, 4> ring;
  const int first[] = {1, 2, 3};
  TEST_ASSERT_EQUAL_size_t(3, ring.write(first, 3));
  int out[4] = {};
  TEST_ASSERT_EQUAL_size_t(2, ring.read(out, 2));
  TEST_ASSERT_EQUAL_INT(1, out[0]);
  TEST_ASSERT_EQUAL_INT(2, out[1]);

  const int second[] = {4, 5, 6, 7};
  TEST_ASSERT_EQUAL_size_t(3, ring.write(second, 4));  // só cabem 3
  TEST_ASSERT_EQUAL_size_t(0, ring.free());
  TEST_ASSERT_EQUAL_size_t(4, ring.read(out, 4));
  TEST_ASSERT_EQUAL_INT(3, out[0]);
  TEST_ASSERT_EQUAL_INT(6, out[3]);
  TEST_ASSERT_EQUAL_size_t(0, ring.read(out, 4));
}

static void test_clear() {
  RingBuffer<int, 2> ring;
  ring.push(9);
  ring.clear();
  TEST_ASSERT_TRUE(ring.empty());
  TEST_ASSERT_TRUE(ring.push(1));
  TEST_ASSERT_EQUAL_INT(1, ring.at(0));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_fifo_order_and_limits);
  RUN_TEST(test_overwrite_drops_the_oldest);
  RUN_TEST(test_bulk_write_and_read_wrap_around);
  RUN_TEST(test_clear);
  return UNITY_END();
}
