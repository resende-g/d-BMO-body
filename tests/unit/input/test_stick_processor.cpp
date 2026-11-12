#include <initializer_list>

#include "dbmo/input/stick_processor.h"
#include "dbmo/mock/mock_hal.h"
#include "dbmo_test.h"

using dbmo::Status;
using dbmo::mock::MockAdcChannel;
using namespace dbmo::input;

namespace {
struct Fixture {
  MockAdcChannel x;
  MockAdcChannel y;
  StickProcessor stick{x, y};

  // Calibração redonda para as contas à mão: 0 / 2000 / 4000 nos dois eixos.
  Fixture() {
    x.raw = 2000;
    y.raw = 2000;
    TEST_ASSERT_OK(stick.setCalibration({0, 2000, 4000}, {0, 2000, 4000}));
  }
  StickPosition at(uint16_t rawX, uint16_t rawY) {
    x.raw = rawX;
    y.raw = rawY;
    (void)stick.update();
    return stick.position();
  }
};
}  // namespace

void setUp() {}
void tearDown() {}

static void test_integer_square_root() {
  TEST_ASSERT_EQUAL_UINT32(0, integerSqrt(0));
  TEST_ASSERT_EQUAL_UINT32(1, integerSqrt(1));
  TEST_ASSERT_EQUAL_UINT32(1, integerSqrt(3));
  TEST_ASSERT_EQUAL_UINT32(848, integerSqrt(720000));  // 848² = 719104, 849² = 720801
  TEST_ASSERT_EQUAL_UINT32(1414, integerSqrt(2000000));
  TEST_ASSERT_EQUAL_UINT32(65535, integerSqrt(0xFFFFFFFFu));
}

static void test_full_deflection_reaches_the_limits() {
  Fixture f;
  StickPosition p = f.at(4000, 2000);
  TEST_ASSERT_EQUAL_INT16(1000, p.x);
  TEST_ASSERT_EQUAL_INT16(0, p.y);
  p = f.at(0, 2000);
  TEST_ASSERT_EQUAL_INT16(-1000, p.x);
  p = f.at(2000, 4000);
  TEST_ASSERT_EQUAL_INT16(1000, p.y);
  // Leitura além da calibração é limitada, não estoura.
  p = f.at(4095, 2000);
  TEST_ASSERT_EQUAL_INT16(1000, p.x);
}

static void test_dead_zone_is_radial_and_rescaled() {
  Fixture f;  // zona morta padrão: 120 milésimos
  // 2200 -> (200 * 1000) / 2000 = 100 milésimos: dentro da zona morta.
  StickPosition p = f.at(2200, 2000);
  TEST_ASSERT_EQUAL_INT16(0, p.x);
  // Dois eixos em 100: módulo = sqrt(20000) = 141 > 120: já fora da zona morta.
  //   saída = 100 * (141 - 120) * 1000 / ((1000 - 120) * 141) = 2100000 / 124080 = 16
  f.x.raw = 2200;
  f.y.raw = 2200;
  (void)f.stick.update();
  // (16 fica abaixo do limiar de mudança de 20, mas sair do zero sempre é avisado.)
  TEST_ASSERT_EQUAL_INT16(16, f.stick.position().x);
  TEST_ASSERT_EQUAL_INT16(16, f.stick.position().y);

  // 3000 -> 500 milésimos: saída = 500 * (500 - 120) * 1000 / (880 * 500) = 431.
  p = f.at(3000, 2000);
  TEST_ASSERT_EQUAL_INT16(431, p.x);
  // Diagonal 600/600: módulo 848; saída = 600 * 728000 / (880 * 848) = 585.
  p = f.at(3200, 3200);
  TEST_ASSERT_EQUAL_INT16(585, p.x);
  TEST_ASSERT_EQUAL_INT16(585, p.y);
}

static void test_dead_zone_is_configurable() {
  Fixture f;
  TEST_ASSERT_OK(f.stick.setDeadzonePermille(0));
  TEST_ASSERT_EQUAL_INT16(100, f.at(2200, 2000).x);
  TEST_ASSERT_STATUS(Status::OutOfRange, f.stick.setDeadzonePermille(901));
}

static void test_axes_can_be_inverted() {
  Fixture f;
  f.stick.setInverted(true, false);
  const StickPosition p = f.at(3200, 3200);
  TEST_ASSERT_EQUAL_INT16(-585, p.x);
  TEST_ASSERT_EQUAL_INT16(585, p.y);
}

static void test_update_reports_only_significant_changes() {
  Fixture f;
  f.x.raw = 3000;
  TEST_ASSERT_TRUE(f.stick.update().value());
  f.x.raw = 3010;  // +5 milésimos antes da zona morta: abaixo do limiar
  TEST_ASSERT_FALSE(f.stick.update().value());
  TEST_ASSERT_EQUAL_INT16(431, f.stick.position().x);  // a posição avisada não muda
  f.x.raw = 3100;
  TEST_ASSERT_TRUE(f.stick.update().value());
  f.x.raw = 2000;  // voltou ao centro: sempre avisa
  TEST_ASSERT_TRUE(f.stick.update().value());
  TEST_ASSERT_EQUAL_INT16(0, f.stick.position().x);
}

static void test_centre_calibration_averages_readings() {
  Fixture f;
  f.x.raw = 2100;
  f.y.raw = 1900;
  TEST_ASSERT_OK(f.stick.calibrateCenter());
  TEST_ASSERT_EQUAL_UINT16(2100, f.stick.calibrationX().center);
  TEST_ASSERT_EQUAL_UINT16(1900, f.stick.calibrationY().center);
  // Com o novo centro, a leitura de repouso dá zero.
  TEST_ASSERT_EQUAL_INT16(0, f.at(2100, 1900).x);
}

static void test_centre_calibration_rejects_a_stuck_stick() {
  Fixture f;
  f.x.raw = 3950;  // colado no extremo: sobra menos de 5 % de curso
  TEST_ASSERT_STATUS(Status::Corrupt, f.stick.calibrateCenter());
  TEST_ASSERT_EQUAL_UINT16(2000, f.stick.calibrationX().center);  // mantém a anterior

  f.x.raw = 2000;
  f.x.status = Status::Timeout;
  TEST_ASSERT_STATUS(Status::Timeout, f.stick.calibrateCenter());
}

static void test_range_calibration_learns_the_extremes() {
  Fixture f;
  f.stick.beginRangeCalibration();
  TEST_ASSERT_TRUE(f.stick.calibratingRange());
  for (uint16_t raw : {uint16_t{300}, uint16_t{3700}, uint16_t{2000}}) (void)f.at(raw, raw);
  (void)f.at(350, 3650);
  TEST_ASSERT_OK(f.stick.endRangeCalibration());
  TEST_ASSERT_EQUAL_UINT16(300, f.stick.calibrationX().min);
  TEST_ASSERT_EQUAL_UINT16(3700, f.stick.calibrationX().max);
  TEST_ASSERT_EQUAL_UINT16(2000, f.stick.calibrationX().center);
  TEST_ASSERT_EQUAL_INT16(1000, f.at(3700, 2000).x);
}

static void test_range_calibration_without_movement_is_rejected() {
  Fixture f;
  TEST_ASSERT_STATUS(Status::NotInitialized, f.stick.endRangeCalibration());
  f.stick.beginRangeCalibration();
  (void)f.at(2010, 1990);  // o usuário não girou o analógico
  TEST_ASSERT_STATUS(Status::Corrupt, f.stick.endRangeCalibration());
  TEST_ASSERT_EQUAL_UINT16(4000, f.stick.calibrationX().max);  // calibração anterior mantida
}

static void test_calibration_values_are_validated() {
  Fixture f;
  TEST_ASSERT_STATUS(Status::InvalidArgument,
                     f.stick.setCalibration({2000, 1000, 4000}, {0, 2000, 4000}));
  TEST_ASSERT_STATUS(Status::InvalidArgument,
                     f.stick.setCalibration({0, 2000, 5000}, {0, 2000, 4000}));
  TEST_ASSERT_STATUS(Status::InvalidArgument,
                     f.stick.setCalibration({0, 2000, 4000}, {1900, 2000, 4000}));
  TEST_ASSERT_OK(f.stick.setCalibration({100, 2100, 3900}, {200, 1900, 3800}));
}

static void test_adc_errors_keep_the_last_position() {
  Fixture f;
  (void)f.at(3000, 2000);
  f.y.status = Status::IoError;
  TEST_ASSERT_STATUS(Status::IoError, f.stick.update().status());
  TEST_ASSERT_STATUS(Status::IoError, f.stick.update().status());
  TEST_ASSERT_EQUAL_UINT16(2, f.stick.consecutiveErrors());
  TEST_ASSERT_EQUAL_INT16(431, f.stick.position().x);
  f.y.status = Status::Ok;
  TEST_ASSERT_TRUE(f.stick.update().ok());
  TEST_ASSERT_EQUAL_UINT16(0, f.stick.consecutiveErrors());
}

static void test_defaults_without_calibration_use_the_full_adc_scale() {
  MockAdcChannel x, y;  // maxRaw = 4095
  StickProcessor stick(x, y);
  TEST_ASSERT_EQUAL_UINT16(2047, stick.calibrationX().center);
  TEST_ASSERT_EQUAL_UINT16(4095, stick.calibrationX().max);
  x.raw = 4095;
  y.raw = 2047;
  (void)stick.update();
  TEST_ASSERT_EQUAL_INT16(1000, stick.position().x);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_integer_square_root);
  RUN_TEST(test_full_deflection_reaches_the_limits);
  RUN_TEST(test_dead_zone_is_radial_and_rescaled);
  RUN_TEST(test_dead_zone_is_configurable);
  RUN_TEST(test_axes_can_be_inverted);
  RUN_TEST(test_update_reports_only_significant_changes);
  RUN_TEST(test_centre_calibration_averages_readings);
  RUN_TEST(test_centre_calibration_rejects_a_stuck_stick);
  RUN_TEST(test_range_calibration_learns_the_extremes);
  RUN_TEST(test_range_calibration_without_movement_is_rejected);
  RUN_TEST(test_calibration_values_are_validated);
  RUN_TEST(test_adc_errors_keep_the_last_position);
  RUN_TEST(test_defaults_without_calibration_use_the_full_adc_scale);
  return UNITY_END();
}
