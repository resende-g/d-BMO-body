#include <cmath>

#include "dbmo/audio/tone.h"
#include "dbmo_test.h"

using dbmo::Status;
using namespace dbmo::audio;

void setUp() {}
void tearDown() {}

void test_sine_approximation_shape() {
  TEST_ASSERT_EQUAL_INT16(0, approximateSine(0));
  TEST_ASSERT_EQUAL_INT16(32767, approximateSine(0x4000));  // um quarto de volta
  TEST_ASSERT_EQUAL_INT16(0, approximateSine(0x8000));
  TEST_ASSERT_EQUAL_INT16(-32767, approximateSine(0xC000));  // três quartos
  // Simetria: sen(x + meia volta) = −sen(x).
  for (uint32_t phase = 0; phase < 0x8000; phase += 257)
    TEST_ASSERT_EQUAL_INT16(-approximateSine(static_cast<uint16_t>(phase)),
                            approximateSine(static_cast<uint16_t>(phase + 0x8000)));
}

void test_sine_approximation_error_is_bounded() {
  // A parábola fica a menos de 6 % do fundo de escala do seno verdadeiro.
  const double kPi = 3.14159265358979323846;
  double worst = 0;
  for (uint32_t phase = 0; phase < 65536; phase += 64) {
    const double exact = 32767.0 * std::sin(2.0 * kPi * phase / 65536.0);
    const double error = std::fabs(exact - approximateSine(static_cast<uint16_t>(phase)));
    if (error > worst) worst = error;
  }
  TEST_ASSERT_TRUE(worst < 0.06 * 32767.0);
}

void test_tone_length_and_amplitude() {
  ToneSource tone;
  TEST_ASSERT_OK(tone.start(1000, 100, 16000, 16000));
  int16_t buffer[256];
  size_t total = 0;
  int16_t maximum = 0;
  int16_t minimum = 0;
  for (;;) {
    const size_t count = tone.read(buffer, 256);
    if (count == 0) break;
    for (size_t i = 0; i < count; ++i) {
      if (buffer[i] > maximum) maximum = buffer[i];
      if (buffer[i] < minimum) minimum = buffer[i];
    }
    total += count;
  }
  TEST_ASSERT_EQUAL_UINT(1600, total);  // 100 ms a 16 kHz
  TEST_ASSERT_TRUE(tone.finished());
  TEST_ASSERT_INT16_WITHIN(200, 16000, maximum);
  TEST_ASSERT_INT16_WITHIN(200, -16000, minimum);
}

void test_tone_frequency() {
  // 1 kHz a 16 kHz por 1 s: 1000 ciclos, ou seja, 1000 cruzamentos de zero subindo.
  ToneSource tone;
  TEST_ASSERT_OK(tone.start(1000, 1000, 16000, 16000));
  int16_t buffer[128];
  int crossings = 0;
  int16_t previous = 0;
  for (;;) {
    const size_t count = tone.read(buffer, 128);
    if (count == 0) break;
    for (size_t i = 0; i < count; ++i) {
      if (previous <= 0 && buffer[i] > 0) ++crossings;
      previous = buffer[i];
    }
  }
  TEST_ASSERT_INT_WITHIN(2, 1000, crossings);
}

void test_tone_fades_in_and_out() {
  ToneSource tone;
  TEST_ASSERT_OK(tone.start(1000, 100, 16000, 16000));
  int16_t samples[1600];
  TEST_ASSERT_EQUAL_UINT(1600, tone.read(samples, 1600));
  // Começa e termina em silêncio, sem degrau.
  TEST_ASSERT_EQUAL_INT16(0, samples[0]);
  TEST_ASSERT_INT16_WITHIN(300, 0, samples[1]);
  TEST_ASSERT_INT16_WITHIN(300, 0, samples[1599]);
  // Nos 5 ms de rampa (80 amostras) o pico cresce.
  int16_t early = 0;
  int16_t later = 0;
  for (int i = 0; i < 20; ++i)
    if (samples[i] > early) early = samples[i];
  for (int i = 60; i < 100; ++i)
    if (samples[i] > later) later = samples[i];
  TEST_ASSERT_TRUE(early < later);
}

void test_very_short_tone() {
  ToneSource tone;
  TEST_ASSERT_OK(tone.start(1000, 1, 8000, 16000));  // 8 amostras, menor que a rampa
  int16_t samples[16];
  TEST_ASSERT_EQUAL_UINT(8, tone.read(samples, 16));
  TEST_ASSERT_EQUAL_UINT(0, tone.read(samples, 16));
}

void test_invalid_parameters() {
  ToneSource tone;
  TEST_ASSERT_STATUS(Status::InvalidArgument, tone.start(0, 100, 16000));
  TEST_ASSERT_STATUS(Status::InvalidArgument, tone.start(1000, 0, 16000));
  TEST_ASSERT_STATUS(Status::InvalidArgument, tone.start(1000, 100, 0));
  TEST_ASSERT_STATUS(Status::InvalidArgument, tone.start(1000, 100, 16000, 0));
  TEST_ASSERT_STATUS(Status::InvalidArgument, tone.start(1000, 100, 16000, 40000));
  TEST_ASSERT_STATUS(Status::InvalidArgument, tone.start(9000, 100, 16000));  // acima de taxa/2
  TEST_ASSERT_OK(tone.start(8000, 100, 16000));
}

void test_unstarted_tone_is_empty() {
  ToneSource tone;
  int16_t samples[8];
  TEST_ASSERT_EQUAL_UINT(0, tone.read(samples, 8));
  TEST_ASSERT_TRUE(tone.finished());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_sine_approximation_shape);
  RUN_TEST(test_sine_approximation_error_is_bounded);
  RUN_TEST(test_tone_length_and_amplitude);
  RUN_TEST(test_tone_frequency);
  RUN_TEST(test_tone_fades_in_and_out);
  RUN_TEST(test_very_short_tone);
  RUN_TEST(test_invalid_parameters);
  RUN_TEST(test_unstarted_tone_is_empty);
  return UNITY_END();
}
