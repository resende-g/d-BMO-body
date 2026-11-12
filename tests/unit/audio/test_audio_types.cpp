#include "dbmo/audio/audio_types.h"
#include "dbmo_test.h"

using namespace dbmo::audio;

void setUp() {}
void tearDown() {}

void test_supported_sample_rates() {
  const uint32_t supported[] = {8000, 16000, 32000, 44100, 48000};
  for (uint32_t rate : supported) TEST_ASSERT_TRUE(isSupportedSampleRate(rate));
  // Taxas que o MAX98357A não aceita (AMP-08) e taxas acima do INMP441 (MIC-05).
  const uint32_t unsupported[] = {0, 11025, 12000, 22050, 24000, 88200, 96000, 15999};
  for (uint32_t rate : unsupported) TEST_ASSERT_FALSE(isSupportedSampleRate(rate));
}

void test_volume_curve() {
  Volume volume;
  volume.setPercent(100);
  TEST_ASSERT_EQUAL_UINT16(32768, volume.gainQ15());
  volume.setPercent(50);
  TEST_ASSERT_EQUAL_UINT16(8192, volume.gainQ15());  // (0,5)² = 0,25
  volume.setPercent(10);
  TEST_ASSERT_EQUAL_UINT16(327, volume.gainQ15());
  volume.setPercent(0);
  TEST_ASSERT_EQUAL_UINT16(0, volume.gainQ15());
  TEST_ASSERT_TRUE(volume.silent());
  volume.setPercent(250);  // acima do máximo: limitado a 100
  TEST_ASSERT_EQUAL_UINT8(100, volume.percent());
}

void test_volume_is_monotonic() {
  Volume volume;
  uint16_t previous = 0;
  for (uint8_t percent = 1; percent <= 100; ++percent) {
    volume.setPercent(percent);
    TEST_ASSERT_TRUE(volume.gainQ15() >= previous);
    previous = volume.gainQ15();
  }
}

void test_volume_apply() {
  Volume volume;
  volume.setPercent(50);
  int16_t samples[] = {32767, -32768, 1000, -1000, 0, 3};
  volume.apply(samples, 6);
  TEST_ASSERT_EQUAL_INT16(8191, samples[0]);
  TEST_ASSERT_EQUAL_INT16(-8192, samples[1]);
  TEST_ASSERT_EQUAL_INT16(250, samples[2]);
  TEST_ASSERT_EQUAL_INT16(-250, samples[3]);
  TEST_ASSERT_EQUAL_INT16(0, samples[4]);
  TEST_ASSERT_EQUAL_INT16(0, samples[5]);

  // Volume máximo não altera e não satura os extremos.
  volume.setPercent(100);
  int16_t extremes[] = {32767, -32768};
  volume.apply(extremes, 2);
  TEST_ASSERT_EQUAL_INT16(32767, extremes[0]);
  TEST_ASSERT_EQUAL_INT16(-32768, extremes[1]);
}

void test_mute() {
  Volume volume;
  volume.setPercent(80);
  volume.setMuted(true);
  TEST_ASSERT_TRUE(volume.silent());
  int16_t samples[] = {12345, -12345};
  volume.apply(samples, 2);
  TEST_ASSERT_EQUAL_INT16(0, samples[0]);
  TEST_ASSERT_EQUAL_INT16(0, samples[1]);
  volume.setMuted(false);
  TEST_ASSERT_FALSE(volume.silent());
  TEST_ASSERT_EQUAL_UINT8(80, volume.percent());  // o mudo não perde o volume
}

void test_peak_level() {
  const int16_t samples[] = {10, -200, 150, 0};
  TEST_ASSERT_EQUAL_UINT16(200, peakLevel(samples, 4));
  const int16_t extreme[] = {-32768, 32767};
  TEST_ASSERT_EQUAL_UINT16(32768, peakLevel(extreme, 2));
  TEST_ASSERT_EQUAL_UINT16(0, peakLevel(samples, 0));
}

void test_buffer_source() {
  const int16_t data[] = {1, 2, 3, 4, 5};
  BufferSource source;
  int16_t out[4] = {};
  TEST_ASSERT_EQUAL_UINT(0, source.read(out, 4));  // sem dados
  source.set(data, 5);
  TEST_ASSERT_EQUAL_UINT(4, source.read(out, 4));
  TEST_ASSERT_EQUAL_INT16(1, out[0]);
  TEST_ASSERT_EQUAL_INT16(4, out[3]);
  TEST_ASSERT_EQUAL_UINT(1, source.read(out, 4));
  TEST_ASSERT_EQUAL_INT16(5, out[0]);
  TEST_ASSERT_EQUAL_UINT(0, source.read(out, 4));
  source.set(data, 5);  // recomeça
  TEST_ASSERT_EQUAL_UINT(4, source.read(out, 4));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_supported_sample_rates);
  RUN_TEST(test_volume_curve);
  RUN_TEST(test_volume_is_monotonic);
  RUN_TEST(test_volume_apply);
  RUN_TEST(test_mute);
  RUN_TEST(test_peak_level);
  RUN_TEST(test_buffer_source);
  return UNITY_END();
}
