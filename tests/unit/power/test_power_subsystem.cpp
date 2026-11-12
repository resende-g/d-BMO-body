#include "dbmo/config/pins.h"
#include "dbmo/mock/mock_hal.h"
#include "dbmo/power/power_subsystem.h"
#include "dbmo_test.h"

using dbmo::ConfigStore;
using dbmo::Event;
using dbmo::EventBus;
using dbmo::Status;
using dbmo::mock::MockAdcChannel;
using dbmo::mock::MockGpio;
using namespace dbmo::power;

namespace {

constexpr int kButtonPin = dbmo::config::target::kPowerButton;
constexpr int kHoldPin = dbmo::config::target::kPowerHold;

class FakeSleep final : public ISleepControl {
 public:
  Status deepSleep(const WakeSource &wake) override {
    ++calls;
    lastWake = wake;
    return status;
  }
  int calls = 0;
  WakeSource lastWake;
  Status status = Status::Ok;
};

class FakeCharger final : public IChargerStatus {
 public:
  dbmo::Result<ChargeState> read() override {
    if (!dbmo::isOk(status)) return status;
    return state;
  }
  ChargeState state = ChargeState::NotCharging;
  Status status = Status::Ok;
};

struct Fixture {
  EventBus bus;
  ConfigStore config;
  MockGpio gpio;
  MockAdcChannel adc;
  GpioPowerLatch latch{gpio, kHoldPin};
  FakeSleep sleep;
  FakeCharger charger;
  PowerSubsystem power{bus, config};
  Event events[64];
  int count = 0;
  uint32_t now = 0;

  Fixture() {
    TEST_ASSERT_OK(PowerSubsystem::registerConfig(config));
    TEST_ASSERT_OK(bus.subscribeAll(&Fixture::collect, this));
    gpio.setInput(kButtonPin, true);  // solto (ativo em nível baixo)
  }
  static void collect(const Event &event, void *context) {
    auto *self = static_cast<Fixture *>(context);
    if (self->count < 64) self->events[self->count++] = event;
  }
  void run(uint32_t durationMs) {
    for (uint32_t i = 0; i < durationMs; ++i) {
      power.tick(++now);
      bus.dispatch();
    }
  }
  void press() { gpio.setInput(kButtonPin, false); }
  void release() { gpio.setInput(kButtonPin, true); }
  int countOf(dbmo::EventId id) const {
    int matches = 0;
    for (int i = 0; i < count; ++i)
      if (events[i].id == id) ++matches;
    return matches;
  }
  const Event *last(dbmo::EventId id) const {
    for (int i = count - 1; i >= 0; --i)
      if (events[i].id == id) return &events[i];
    return nullptr;
  }
  void enableBattery(uint16_t batteryMv) {
    TEST_ASSERT_OK(config.setBool(config_key::kVbatEnabled, true));
    adc.millivolts = batteryMv / 2;
    power.setBatteryAdc(adc);
  }
};

}  // namespace

void setUp() {}
void tearDown() {}

void test_init_holds_the_power_latch_first() {
  Fixture f;
  f.power.setPowerSwitch(f.latch);
  f.power.setPowerButton(f.gpio, kButtonPin);
  TEST_ASSERT_OK(f.power.init());
  TEST_ASSERT_TRUE(f.gpio.read(kHoldPin).value());
  TEST_ASSERT_EQUAL(PowerPhase::Running, f.power.phase());
}

void test_init_fails_when_the_latch_cannot_be_held() {
  Fixture f;
  f.power.setPowerSwitch(f.latch);
  f.gpio.failNext(Status::IoError);
  TEST_ASSERT_STATUS(Status::IoError, f.power.init());
}

void test_works_with_no_hardware_at_all() {
  Fixture f;
  TEST_ASSERT_OK(f.power.init());
  f.run(5000);
  TEST_ASSERT_EQUAL(BatteryLevel::Unknown, f.power.battery().level());
  TEST_ASSERT_EQUAL(ChargeState::Unknown, f.power.chargeState());
  TEST_ASSERT_EQUAL_INT(0, f.countOf(power_event::kBatteryLevel));
  TEST_ASSERT_EQUAL_INT(0, f.countOf(power_event::kShutdownRequested));
  // Sem trava e sem controle de sono não há como desligar.
  f.power.requestShutdown(ShutdownReason::Command);
  TEST_ASSERT_STATUS(Status::Unsupported, f.power.completeShutdown());
}

void test_battery_is_not_read_while_disabled() {
  Fixture f;
  f.adc.millivolts = 1950;
  f.power.setBatteryAdc(f.adc);
  TEST_ASSERT_OK(f.power.init());
  f.run(5000);
  TEST_ASSERT_EQUAL_UINT(0, f.adc.readCount);
  TEST_ASSERT_EQUAL(BatteryLevel::Unknown, f.power.battery().level());
}

void test_battery_level_events() {
  Fixture f;
  f.enableBattery(3900);
  TEST_ASSERT_OK(f.power.init());
  f.run(1500);
  const Event *event = f.last(power_event::kBatteryLevel);
  TEST_ASSERT_NOT_NULL(event);
  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(BatteryLevel::Normal), event->a);
  TEST_ASSERT_EQUAL_UINT32(3900, event->b);

  f.adc.millivolts = 3400 / 2;
  f.run(10000);
  event = f.last(power_event::kBatteryLevel);
  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(BatteryLevel::Low), event->a);
  TEST_ASSERT_EQUAL_INT(2, f.countOf(power_event::kBatteryLevel));  // só nas mudanças
}

void test_battery_fault_is_reported_once_and_level_goes_unknown() {
  Fixture f;
  f.enableBattery(3900);
  TEST_ASSERT_OK(f.power.init());
  f.run(3000);
  f.adc.status = Status::IoError;
  f.run(6000);
  TEST_ASSERT_EQUAL_INT(1, f.countOf(power_event::kBatteryFault));
  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(BatteryFault::ReadError),
                           f.last(power_event::kBatteryFault)->a);
  const Event *level = f.last(power_event::kBatteryLevel);
  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(BatteryLevel::Unknown), level->a);
  TEST_ASSERT_EQUAL_UINT32(0, level->b);  // sem tensão inventada
  TEST_ASSERT_EQUAL_INT(0, f.countOf(power_event::kShutdownRequested));
}

void test_critical_battery_requests_shutdown_after_confirmation() {
  Fixture f;
  f.enableBattery(3200);
  f.power.setPowerSwitch(f.latch);
  TEST_ASSERT_OK(f.power.init());
  f.run(3000);
  TEST_ASSERT_EQUAL(BatteryLevel::Critical, f.power.battery().level());
  TEST_ASSERT_EQUAL_INT(0, f.countOf(power_event::kShutdownRequested));  // ainda confirmando
  f.run(3000);
  TEST_ASSERT_EQUAL_INT(1, f.countOf(power_event::kShutdownRequested));
  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(ShutdownReason::BatteryCritical),
                           f.last(power_event::kShutdownRequested)->a);
  TEST_ASSERT_EQUAL(PowerPhase::ShutdownRequested, f.power.phase());
  f.run(10000);
  TEST_ASSERT_EQUAL_INT(1, f.countOf(power_event::kShutdownRequested));  // sem repetir
}

void test_critical_battery_does_not_shut_down_while_charging() {
  Fixture f;
  f.enableBattery(3200);
  f.charger.state = ChargeState::Charging;
  f.power.setCharger(f.charger);
  TEST_ASSERT_OK(f.power.init());
  f.run(20000);
  TEST_ASSERT_EQUAL_INT(0, f.countOf(power_event::kShutdownRequested));
  TEST_ASSERT_EQUAL(ChargeState::Charging, f.power.chargeState());
}

void test_auto_off_can_be_disabled() {
  Fixture f;
  f.enableBattery(3200);
  TEST_ASSERT_OK(f.config.setBool(config_key::kAutoOffCritical, false));
  TEST_ASSERT_OK(f.power.init());
  f.run(20000);
  TEST_ASSERT_EQUAL_INT(0, f.countOf(power_event::kShutdownRequested));
}

void test_charge_state_events_and_charger_error() {
  Fixture f;
  f.power.setCharger(f.charger);
  TEST_ASSERT_OK(f.power.init());
  f.run(1500);
  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(ChargeState::NotCharging),
                           f.last(power_event::kChargeState)->a);
  f.charger.state = ChargeState::Charging;
  f.run(1500);
  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(ChargeState::Charging),
                           f.last(power_event::kChargeState)->a);
  f.charger.status = Status::IoError;
  f.run(1500);
  TEST_ASSERT_EQUAL(ChargeState::Unknown, f.power.chargeState());
}

void test_gpio_charger_status() {
  MockGpio gpio;
  GpioChargerStatus charger(gpio, 20, 21);
  TEST_ASSERT_OK(charger.begin());
  // Dreno aberto com pull-up: nenhum sinal ativo.
  TEST_ASSERT_EQUAL(ChargeState::NotCharging, charger.read().value());
  gpio.setInput(20, false);
  TEST_ASSERT_EQUAL(ChargeState::Charging, charger.read().value());
  gpio.setInput(20, true);
  gpio.setInput(21, false);
  TEST_ASSERT_EQUAL(ChargeState::Full, charger.read().value());
  gpio.failNext(Status::IoError);
  TEST_ASSERT_FALSE(charger.read().ok());
}

void test_short_press_publishes_button_event() {
  Fixture f;
  f.power.setPowerButton(f.gpio, kButtonPin);
  TEST_ASSERT_OK(f.power.init());
  f.run(100);
  f.press();
  f.run(200);
  f.release();
  f.run(100);
  TEST_ASSERT_EQUAL_INT(1, f.countOf(power_event::kButtonPress));
  TEST_ASSERT_EQUAL_INT(0, f.countOf(power_event::kShutdownRequested));
}

void test_bounce_is_ignored() {
  Fixture f;
  f.power.setPowerButton(f.gpio, kButtonPin);
  TEST_ASSERT_OK(f.power.init());
  f.run(100);
  for (int i = 0; i < 10; ++i) {  // ruído mais curto que o debounce
    f.press();
    f.run(8);
    f.release();
    f.run(25);
  }
  TEST_ASSERT_EQUAL_INT(0, f.countOf(power_event::kButtonPress));
}

void test_long_press_requests_shutdown_once() {
  Fixture f;
  f.power.setPowerButton(f.gpio, kButtonPin);
  f.power.setPowerSwitch(f.latch);
  TEST_ASSERT_OK(f.power.init());
  f.run(100);
  f.press();
  f.run(1400);
  TEST_ASSERT_EQUAL_INT(0, f.countOf(power_event::kShutdownRequested));
  f.run(300);
  TEST_ASSERT_EQUAL_INT(1, f.countOf(power_event::kShutdownRequested));
  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(ShutdownReason::UserButton),
                           f.last(power_event::kShutdownRequested)->a);
  f.run(5000);
  f.release();
  f.run(100);
  TEST_ASSERT_EQUAL_INT(1, f.countOf(power_event::kShutdownRequested));
  TEST_ASSERT_EQUAL_INT(0, f.countOf(power_event::kButtonPress));  // o toque longo não é curto
}

void test_press_that_turned_the_device_on_is_ignored() {
  // O usuário ainda está com o dedo no botão quando o firmware sobe.
  Fixture f;
  f.press();
  f.power.setPowerButton(f.gpio, kButtonPin);
  f.power.setPowerSwitch(f.latch);
  TEST_ASSERT_OK(f.power.init());
  f.run(5000);
  TEST_ASSERT_EQUAL_INT(0, f.countOf(power_event::kShutdownRequested));
  f.release();
  f.run(100);
  TEST_ASSERT_EQUAL_INT(0, f.countOf(power_event::kButtonPress));
  // Daí em diante o botão funciona normalmente.
  f.press();
  f.run(1600);
  TEST_ASSERT_EQUAL_INT(1, f.countOf(power_event::kShutdownRequested));
}

void test_complete_shutdown_waits_for_button_release_then_drops_latch() {
  Fixture f;
  f.power.setPowerButton(f.gpio, kButtonPin);
  f.power.setPowerSwitch(f.latch);
  TEST_ASSERT_OK(f.power.init());
  TEST_ASSERT_STATUS(Status::NotPermitted, f.power.completeShutdown());  // ninguém pediu

  f.run(100);
  f.press();
  f.run(1600);
  TEST_ASSERT_EQUAL(PowerPhase::ShutdownRequested, f.power.phase());
  TEST_ASSERT_STATUS(Status::Busy, f.power.completeShutdown());
  TEST_ASSERT_TRUE(f.gpio.read(kHoldPin).value());  // trava ainda segura

  f.release();
  TEST_ASSERT_OK(f.power.completeShutdown());
  TEST_ASSERT_FALSE(f.gpio.read(kHoldPin).value());
  TEST_ASSERT_EQUAL(PowerPhase::Off, f.power.phase());
  TEST_ASSERT_EQUAL_INT(0, f.sleep.calls);
}

void test_without_latch_shutdown_uses_deep_sleep_with_button_wake() {
  Fixture f;
  f.power.setPowerButton(f.gpio, kButtonPin);
  f.power.setSleepControl(f.sleep);
  TEST_ASSERT_OK(f.power.init());
  f.power.requestShutdown(ShutdownReason::Command);
  TEST_ASSERT_OK(f.power.completeShutdown());
  TEST_ASSERT_EQUAL_INT(1, f.sleep.calls);
  TEST_ASSERT_EQUAL_INT(kButtonPin, f.sleep.lastWake.pin);
  TEST_ASSERT_TRUE(f.sleep.lastWake.activeLow);
  TEST_ASSERT_EQUAL(PowerPhase::Off, f.power.phase());
}

void test_cancel_shutdown_returns_to_running() {
  Fixture f;
  f.power.setPowerSwitch(f.latch);
  TEST_ASSERT_OK(f.power.init());
  f.power.requestShutdown(ShutdownReason::Command);
  f.power.cancelShutdown();
  TEST_ASSERT_EQUAL(PowerPhase::Running, f.power.phase());
  TEST_ASSERT_STATUS(Status::NotPermitted, f.power.completeShutdown());
  TEST_ASSERT_TRUE(f.gpio.read(kHoldPin).value());
}

void test_idle_timeout_event() {
  Fixture f;
  TEST_ASSERT_OK(f.config.setInt(config_key::kSleepAfterS, 5));
  TEST_ASSERT_OK(f.power.init());
  f.run(4000);
  f.power.notifyActivity(f.now);
  f.run(4000);
  TEST_ASSERT_EQUAL_INT(0, f.countOf(power_event::kIdleTimeout));
  f.run(1500);
  TEST_ASSERT_EQUAL_INT(1, f.countOf(power_event::kIdleTimeout));
  f.run(20000);
  TEST_ASSERT_EQUAL_INT(1, f.countOf(power_event::kIdleTimeout));  // uma vez por período ocioso
  f.power.notifyActivity(f.now);
  f.run(5500);
  TEST_ASSERT_EQUAL_INT(2, f.countOf(power_event::kIdleTimeout));
}

void test_idle_timeout_is_off_by_default() {
  Fixture f;
  TEST_ASSERT_OK(f.power.init());
  f.run(60000);
  TEST_ASSERT_EQUAL_INT(0, f.countOf(power_event::kIdleTimeout));
}

void test_inconsistent_thresholds_are_reconciled() {
  Fixture f;
  f.enableBattery(3650);
  TEST_ASSERT_OK(f.config.setInt(config_key::kLowMv, 3400));
  TEST_ASSERT_OK(f.config.setInt(config_key::kCriticalMv, 3900));
  TEST_ASSERT_OK(f.power.init());
  f.run(2000);
  // critical foi limitado a low (3400): 3650 mV é Normal, não Critical.
  TEST_ASSERT_EQUAL(BatteryLevel::Normal, f.power.battery().level());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_init_holds_the_power_latch_first);
  RUN_TEST(test_init_fails_when_the_latch_cannot_be_held);
  RUN_TEST(test_works_with_no_hardware_at_all);
  RUN_TEST(test_battery_is_not_read_while_disabled);
  RUN_TEST(test_battery_level_events);
  RUN_TEST(test_battery_fault_is_reported_once_and_level_goes_unknown);
  RUN_TEST(test_critical_battery_requests_shutdown_after_confirmation);
  RUN_TEST(test_critical_battery_does_not_shut_down_while_charging);
  RUN_TEST(test_auto_off_can_be_disabled);
  RUN_TEST(test_charge_state_events_and_charger_error);
  RUN_TEST(test_gpio_charger_status);
  RUN_TEST(test_short_press_publishes_button_event);
  RUN_TEST(test_bounce_is_ignored);
  RUN_TEST(test_long_press_requests_shutdown_once);
  RUN_TEST(test_press_that_turned_the_device_on_is_ignored);
  RUN_TEST(test_complete_shutdown_waits_for_button_release_then_drops_latch);
  RUN_TEST(test_without_latch_shutdown_uses_deep_sleep_with_button_wake);
  RUN_TEST(test_cancel_shutdown_returns_to_running);
  RUN_TEST(test_idle_timeout_event);
  RUN_TEST(test_idle_timeout_is_off_by_default);
  RUN_TEST(test_inconsistent_thresholds_are_reconciled);
  return UNITY_END();
}
