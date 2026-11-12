#include <cstring>

#include "dbmo/display/framebuffer_display.h"
#include "dbmo_test.h"

using dbmo::Status;
using namespace dbmo::display;

namespace {
constexpr uint16_t kW = 8;
constexpr uint16_t kH = 6;
Color gPixels[kW * kH];

size_t countColor(const FramebufferDisplay &display, Color color) {
  size_t count = 0;
  for (uint16_t y = 0; y < kH; ++y)
    for (uint16_t x = 0; x < kW; ++x)
      if (display.pixel(x, y) == color) ++count;
  return count;
}

struct Capture {
  uint8_t bytes[256];
  size_t length = 0;
  size_t failAfter = SIZE_MAX;
};

bool capture(const uint8_t *data, size_t length, void *context) {
  auto *out = static_cast<Capture *>(context);
  if (out->length + length > out->failAfter) return false;
  memcpy(out->bytes + out->length, data, length);
  out->length += length;
  return true;
}
}  // namespace

void setUp() { memset(gPixels, 0, sizeof(gPixels)); }
void tearDown() {}

static void test_fill_inside_writes_exactly_the_rectangle() {
  FramebufferDisplay display(gPixels, kW, kH);
  TEST_ASSERT_OK(display.fillRect(2, 1, 3, 2, 0xFFFF));
  TEST_ASSERT_EQUAL_size_t(6, countColor(display, 0xFFFF));
  TEST_ASSERT_EQUAL_HEX16(0xFFFF, display.pixel(2, 1));
  TEST_ASSERT_EQUAL_HEX16(0xFFFF, display.pixel(4, 2));
  TEST_ASSERT_EQUAL_HEX16(0x0000, display.pixel(5, 2));
  TEST_ASSERT_EQUAL_HEX16(0x0000, display.pixel(2, 3));
}

static void test_fill_is_clipped_at_every_edge() {
  FramebufferDisplay display(gPixels, kW, kH);
  TEST_ASSERT_OK(display.fillRect(-2, -2, 4, 4, 0x1111));  // canto superior esquerdo
  TEST_ASSERT_EQUAL_size_t(4, countColor(display, 0x1111));
  TEST_ASSERT_OK(display.fillRect(6, 4, 10, 10, 0x2222));  // canto inferior direito
  TEST_ASSERT_EQUAL_size_t(4, countColor(display, 0x2222));
  TEST_ASSERT_EQUAL_HEX16(0x2222, display.pixel(7, 5));
}

static void test_fill_fully_outside_or_empty_is_not_an_error() {
  FramebufferDisplay display(gPixels, kW, kH);
  TEST_ASSERT_OK(display.fillRect(20, 20, 5, 5, 0x3333));
  TEST_ASSERT_OK(display.fillRect(-10, 0, 5, 5, 0x3333));
  TEST_ASSERT_OK(display.fillRect(0, 0, 0, 5, 0x3333));
  TEST_ASSERT_OK(display.fillRect(0, 0, 5, -1, 0x3333));
  TEST_ASSERT_EQUAL_size_t(0, countColor(display, 0x3333));
  TEST_ASSERT_EQUAL_UINT(0, display.pixelsWritten);
}

static void test_coordinates_near_int16_limits_do_not_overflow() {
  FramebufferDisplay display(gPixels, kW, kH);
  TEST_ASSERT_OK(display.fillRect(32000, 32000, 32000, 32000, 0x4444));
  TEST_ASSERT_OK(display.fillRect(-32000, -32000, 32767, 32767, 0x4444));
  TEST_ASSERT_EQUAL_size_t(static_cast<size_t>(kW) * kH, countColor(display, 0x4444));
}

static void test_backlight_sleep_and_fault_injection() {
  FramebufferDisplay display(gPixels, kW, kH);
  TEST_ASSERT_OK(display.setBacklight(70));
  TEST_ASSERT_EQUAL_UINT8(70, display.backlightPercent);
  TEST_ASSERT_STATUS(Status::OutOfRange, display.setBacklight(101));
  display.backlightSupported = false;
  TEST_ASSERT_STATUS(Status::Unsupported, display.setBacklight(10));

  TEST_ASSERT_OK(display.setSleep(true));
  TEST_ASSERT_TRUE(display.asleep);

  display.failNext(Status::IoError);
  TEST_ASSERT_STATUS(Status::IoError, display.fillRect(0, 0, 1, 1, 0xFFFF));
  TEST_ASSERT_EQUAL_size_t(0, countColor(display, 0xFFFF));
  TEST_ASSERT_OK(display.fillRect(0, 0, 1, 1, 0xFFFF));  // a falha vale uma vez só
}

static void test_rgb565_expansion_replicates_high_bits() {
  uint8_t rgb[3];
  rgb565ToRgb888(0xFFFF, rgb);
  TEST_ASSERT_EQUAL_HEX8(0xFF, rgb[0]);
  TEST_ASSERT_EQUAL_HEX8(0xFF, rgb[1]);
  TEST_ASSERT_EQUAL_HEX8(0xFF, rgb[2]);
  rgb565ToRgb888(0x0000, rgb);
  TEST_ASSERT_EQUAL_HEX8(0x00, rgb[0]);
  // 0xD906 (vermelho do d-BMO): R=11011 -> 0xDE, G=001000 -> 0x20, B=00110 -> 0x31.
  rgb565ToRgb888(palette::kSignalRed, rgb);
  TEST_ASSERT_EQUAL_HEX8(0xDE, rgb[0]);
  TEST_ASSERT_EQUAL_HEX8(0x20, rgb[1]);
  TEST_ASSERT_EQUAL_HEX8(0x31, rgb[2]);
}

static void test_ppm_bytes_for_a_two_pixel_image() {
  const Color image[2] = {0xF800, 0x001F};  // vermelho puro, azul puro
  Capture out;
  TEST_ASSERT_OK(writePpm(image, 2, 1, capture, &out));
  const uint8_t expected[] = {'P', '6',  '\n', '2',  ' ',  '1',  '\n', '2', '5',
                              '5', '\n', 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF};
  TEST_ASSERT_EQUAL_size_t(sizeof(expected), out.length);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, out.bytes, sizeof(expected));
}

static void test_ppm_reports_sink_failure_and_bad_arguments() {
  const Color image[4] = {};
  Capture out;
  out.failAfter = 12;  // aceita o cabeçalho (11 bytes) e recusa os pixels
  TEST_ASSERT_STATUS(Status::IoError, writePpm(image, 2, 2, capture, &out));
  TEST_ASSERT_STATUS(Status::InvalidArgument, writePpm(nullptr, 2, 2, capture, &out));
  TEST_ASSERT_STATUS(Status::InvalidArgument, writePpm(image, 0, 2, capture, &out));
  TEST_ASSERT_STATUS(Status::InvalidArgument, writePpm(image, 2, 2, nullptr, nullptr));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_fill_inside_writes_exactly_the_rectangle);
  RUN_TEST(test_fill_is_clipped_at_every_edge);
  RUN_TEST(test_fill_fully_outside_or_empty_is_not_an_error);
  RUN_TEST(test_coordinates_near_int16_limits_do_not_overflow);
  RUN_TEST(test_backlight_sleep_and_fault_injection);
  RUN_TEST(test_rgb565_expansion_replicates_high_bits);
  RUN_TEST(test_ppm_bytes_for_a_two_pixel_image);
  RUN_TEST(test_ppm_reports_sink_failure_and_bad_arguments);
  return UNITY_END();
}
