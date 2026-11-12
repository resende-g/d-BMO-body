#include "dbmo/mock/mock_hal.h"
#include "dbmo/power/battery_monitor.h"
#include "dbmo_test.h"

using dbmo::Status;
using dbmo::mock::MockAdcChannel;
using namespace dbmo::power;

namespace {

// Tensão da bateria → tensão no pino com o divisor ×2 padrão.
void setBattery(MockAdcChannel &adc, uint16_t batteryMv) { adc.millivolts = batteryMv / 2; }

void feed(BatteryMonitor &monitor, int samples) {
  for (int i = 0; i < samples; ++i) (void)monitor.sample();
}

}  // namespace

void setUp() {}
void tearDown() {}

void test_disabled_by_default_never_reports_a_value() {
  MockAdcChannel adc;
  setBattery(adc, 3900);
  BatteryMonitor monitor;
  monitor.setAdc(&adc);

  TEST_ASSERT_EQUAL(BatteryFault::Disabled, monitor.sample());
  TEST_ASSERT_EQUAL(BatteryLevel::Unknown, monitor.level());
  TEST_ASSERT_FALSE(monitor.hasReading());
  TEST_ASSERT_STATUS(Status::NotInitialized, monitor.millivolts().status());
  TEST_ASSERT_EQUAL_UINT(0, adc.readCount);  // desabilitado nem toca no ADC
}

void test_enabled_without_adc_is_unknown() {
  BatteryMonitor monitor;
  monitor.setEnabled(true);
  TEST_ASSERT_EQUAL(BatteryFault::NoSensor, monitor.sample());
  TEST_ASSERT_EQUAL(BatteryLevel::Unknown, monitor.level());
  TEST_ASSERT_FALSE(monitor.millivolts().ok());
}

void test_applies_divider_ratio() {
  MockAdcChannel adc;
  BatteryMonitor monitor;
  monitor.setAdc(&adc);
  monitor.setEnabled(true);

  adc.millivolts = 1950;
  TEST_ASSERT_EQUAL(BatteryFault::None, monitor.sample());
  TEST_ASSERT_EQUAL_UINT16(3900, monitor.millivolts().value());

  BatteryConfig config;
  config.ratioPermille = 1500;  // outro divisor
  BatteryMonitor other;
  other.setAdc(&adc);
  other.configure(config);
  other.setEnabled(true);
  adc.millivolts = 2600;
  (void)other.sample();
  TEST_ASSERT_EQUAL_UINT16(3900, other.millivolts().value());
}

void test_averages_the_window() {
  MockAdcChannel adc;
  BatteryMonitor monitor;
  monitor.setAdc(&adc);
  monitor.setEnabled(true);

  setBattery(adc, 4000);
  feed(monitor, 4);
  setBattery(adc, 3800);
  feed(monitor, 4);
  TEST_ASSERT_EQUAL_UINT16(3900, monitor.millivolts().value());
  // A janela tem 8 amostras: mais 8 de 3800 apagam as de 4000.
  feed(monitor, 8);
  TEST_ASSERT_EQUAL_UINT16(3800, monitor.millivolts().value());
}

void test_levels_and_hysteresis() {
  MockAdcChannel adc;
  BatteryMonitor monitor;
  monitor.setAdc(&adc);
  monitor.setEnabled(true);
  const size_t window = BatteryMonitor::kWindow;

  setBattery(adc, 3900);
  feed(monitor, window);
  TEST_ASSERT_EQUAL(BatteryLevel::Normal, monitor.level());

  setBattery(adc, 3480);  // abaixo de low (3500)
  feed(monitor, window);
  TEST_ASSERT_EQUAL(BatteryLevel::Low, monitor.level());

  setBattery(adc, 3560);  // acima de low, dentro da histerese (3600): continua Low
  feed(monitor, window);
  TEST_ASSERT_EQUAL(BatteryLevel::Low, monitor.level());

  setBattery(adc, 3620);
  feed(monitor, window);
  TEST_ASSERT_EQUAL(BatteryLevel::Normal, monitor.level());

  setBattery(adc, 3280);  // abaixo de critical (3300)
  feed(monitor, window);
  TEST_ASSERT_EQUAL(BatteryLevel::Critical, monitor.level());

  setBattery(adc, 3360);  // dentro da histerese do crítico (3400)
  feed(monitor, window);
  TEST_ASSERT_EQUAL(BatteryLevel::Critical, monitor.level());

  setBattery(adc, 3420);
  feed(monitor, window);
  TEST_ASSERT_EQUAL(BatteryLevel::Low, monitor.level());
}

void test_saturated_adc_is_not_a_reading() {
  MockAdcChannel adc;
  BatteryMonitor monitor;
  monitor.setAdc(&adc);
  monitor.setEnabled(true);

  adc.millivolts = 3100;  // acima da faixa calibrada do ADC (2900 mV)
  TEST_ASSERT_EQUAL(BatteryFault::Saturated, monitor.sample());
  TEST_ASSERT_FALSE(monitor.hasReading());
  TEST_ASSERT_EQUAL(BatteryLevel::Unknown, monitor.level());
}

void test_implausible_voltage_is_unknown_not_critical() {
  // Pino flutuando ou aparelho alimentado só por USB: 0,3 V "de bateria" não é uma
  // bateria crítica, é ausência de medida.
  MockAdcChannel adc;
  BatteryMonitor monitor;
  monitor.setAdc(&adc);
  monitor.setEnabled(true);

  adc.millivolts = 150;
  TEST_ASSERT_EQUAL(BatteryFault::Implausible, monitor.sample());
  feed(monitor, 10);
  TEST_ASSERT_EQUAL(BatteryLevel::Unknown, monitor.level());

  adc.millivolts = 2300;  // 4,6 V: acima do que uma célula 1S pode ter
  TEST_ASSERT_EQUAL(BatteryFault::Implausible, monitor.sample());
}

void test_single_bad_sample_keeps_reading_but_a_streak_discards_it() {
  MockAdcChannel adc;
  BatteryMonitor monitor;
  monitor.setAdc(&adc);
  monitor.setEnabled(true);
  setBattery(adc, 3900);
  feed(monitor, 4);

  adc.status = Status::IoError;
  TEST_ASSERT_EQUAL(BatteryFault::ReadError, monitor.sample());
  TEST_ASSERT_TRUE(monitor.hasReading());
  TEST_ASSERT_EQUAL(BatteryLevel::Normal, monitor.level());

  feed(monitor, BatteryMonitor::kFaultSamples);
  TEST_ASSERT_FALSE(monitor.hasReading());
  TEST_ASSERT_EQUAL(BatteryLevel::Unknown, monitor.level());

  adc.status = Status::Ok;
  TEST_ASSERT_EQUAL(BatteryFault::None, monitor.sample());
  TEST_ASSERT_EQUAL(BatteryLevel::Normal, monitor.level());
}

void test_disabling_discards_the_reading() {
  MockAdcChannel adc;
  BatteryMonitor monitor;
  monitor.setAdc(&adc);
  monitor.setEnabled(true);
  setBattery(adc, 3900);
  feed(monitor, 4);
  monitor.setEnabled(false);
  TEST_ASSERT_FALSE(monitor.hasReading());
  TEST_ASSERT_EQUAL(BatteryLevel::Unknown, monitor.level());
  TEST_ASSERT_EQUAL(BatteryFault::Disabled, monitor.fault());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_disabled_by_default_never_reports_a_value);
  RUN_TEST(test_enabled_without_adc_is_unknown);
  RUN_TEST(test_applies_divider_ratio);
  RUN_TEST(test_averages_the_window);
  RUN_TEST(test_levels_and_hysteresis);
  RUN_TEST(test_saturated_adc_is_not_a_reading);
  RUN_TEST(test_implausible_voltage_is_unknown_not_critical);
  RUN_TEST(test_single_bad_sample_keeps_reading_but_a_streak_discards_it);
  RUN_TEST(test_disabling_discards_the_reading);
  return UNITY_END();
}
