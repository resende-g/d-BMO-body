#include "dbmo/config/pins.h"
#include "dbmo/input/button_source.h"
#include "dbmo/input/mcp23017.h"
#include "dbmo/mock/mock_hal.h"
#include "dbmo/sim/mcp23017_sim.h"
#include "dbmo_test.h"

using dbmo::Status;
using dbmo::mock::MockGpio;
using dbmo::mock::MockI2cBus;
using dbmo::sim::Mcp23017Sim;
using namespace dbmo::input;

namespace {
constexpr uint8_t kAddress = dbmo::config::i2c::kMcp23017Buttons;
}

void setUp() {}
void tearDown() {}

static void test_begin_configures_direction_pullups_and_polarity() {
  MockI2cBus bus;
  Mcp23017Sim sim;
  bus.attach(kAddress, &sim);
  Mcp23017 chip(bus, kAddress);

  TEST_ASSERT_OK(chip.begin(0x7F7F, 0x7F7F));
  TEST_ASSERT_EQUAL_HEX16(0x7F7F, sim.reg16(Mcp23017Sim::kIodirA));  // GPA7/GPB7 como saída
  TEST_ASSERT_EQUAL_HEX16(0x7F7F, sim.reg16(Mcp23017Sim::kGppuA));
  TEST_ASSERT_EQUAL_HEX16(0x0000, sim.reg16(Mcp23017Sim::kIpolA));
  TEST_ASSERT_EQUAL_HEX16(0x0000, sim.reg16(Mcp23017Sim::kOlatA));
  TEST_ASSERT_FALSE(sim.outputOnlyViolated);
}

static void test_begin_refuses_gpa7_and_gpb7_as_inputs() {
  MockI2cBus bus;
  Mcp23017Sim sim;
  bus.attach(kAddress, &sim);
  Mcp23017 chip(bus, kAddress);
  TEST_ASSERT_STATUS(Status::InvalidArgument, chip.begin(0xFFFF, 0xFFFF));
  TEST_ASSERT_STATUS(Status::InvalidArgument, chip.begin(0x0080, 0));
  TEST_ASSERT_STATUS(Status::InvalidArgument, chip.begin(0x8000, 0));
  TEST_ASSERT_EQUAL_UINT(0, bus.writeCount);  // nada chegou ao chip
}

static void test_absent_chip_and_bus_errors() {
  MockI2cBus bus;
  Mcp23017 chip(bus, kAddress);
  TEST_ASSERT_STATUS(Status::HardwareAbsent, chip.begin(0x7F7F, 0x7F7F));
  TEST_ASSERT_STATUS(Status::HardwareAbsent, chip.readPins().status());

  Mcp23017Sim sim;
  bus.attach(kAddress, &sim);
  bus.failNext(Status::IoError);
  TEST_ASSERT_STATUS(Status::IoError, chip.begin(0x7F7F, 0x7F7F));
  TEST_ASSERT_OK(chip.begin(0x7F7F, 0x7F7F));  // pode ser chamado de novo
  bus.failNext(Status::Timeout);
  TEST_ASSERT_STATUS(Status::Timeout, chip.readPins().status());
}

static void test_begin_detects_a_chip_that_does_not_keep_the_configuration() {
  MockI2cBus bus;
  Mcp23017Sim sim;
  sim.stuckDirection = true;  // IODIR fica no valor de reset (0xFFFF)
  bus.attach(kAddress, &sim);
  Mcp23017 chip(bus, kAddress);
  TEST_ASSERT_STATUS(Status::ProtocolError, chip.begin(0x7F7F, 0x7F7F));
}

static void test_read_pins_reflects_pullups_and_pressed_buttons() {
  MockI2cBus bus;
  Mcp23017Sim sim;
  bus.attach(kAddress, &sim);
  Mcp23017 chip(bus, kAddress);
  TEST_ASSERT_OK(chip.begin(0x7F7F, 0x7F7F));

  TEST_ASSERT_EQUAL_HEX16(0x7F7F, chip.readPins().value());  // tudo solto: pull-ups em 1
  sim.press(0);
  sim.press(11);
  TEST_ASSERT_EQUAL_HEX16(0x7F7F & ~((1u << 0) | (1u << 11)), chip.readPins().value());
  sim.release(0);
  TEST_ASSERT_EQUAL_HEX16(0x7F7F & ~(1u << 11), chip.readPins().value());
}

static void test_default_map_reads_each_of_the_14_buttons() {
  MockI2cBus bus;
  Mcp23017Sim sim;
  bus.attach(kAddress, &sim);
  Mcp23017 chip(bus, kAddress);
  Mcp23017ButtonSource source(chip, Mcp23017ButtonSource::defaultMap());
  TEST_ASSERT_OK(source.begin());
  TEST_ASSERT_FALSE(sim.outputOnlyViolated);
  TEST_ASSERT_EQUAL_UINT32(0, source.read().value());

  namespace pins = dbmo::config::mcp_buttons;
  struct Wiring {
    uint8_t pin;
    Button button;
  };
  const Wiring wiring[] = {
      {pins::kDpadUp, Button::DpadUp},
      {pins::kDpadDown, Button::DpadDown},
      {pins::kDpadLeft, Button::DpadLeft},
      {pins::kDpadRight, Button::DpadRight},
      {pins::kActionA, Button::A},
      {pins::kActionB, Button::B},
      {pins::kActionX, Button::X},
      {pins::kActionY, Button::Y},
      {pins::kSelect, Button::Select},
      {pins::kStart, Button::Start},
      {pins::kL1, Button::L1},
      {pins::kR1, Button::R1},
      {pins::kL2, Button::L2},
      {pins::kR2, Button::R2},
  };
  for (const Wiring &wire : wiring) {
    sim.press(wire.pin);
    TEST_ASSERT_EQUAL_HEX32_MESSAGE(maskOf(wire.button), source.read().value(),
                                    toString(wire.button));
    sim.release(wire.pin);
  }
  // Os cliques dos analógicos não estão neste expansor.
  sim.press(7);
  sim.press(15);
  TEST_ASSERT_EQUAL_UINT32(0, source.read().value());
}

static void test_map_with_a_button_on_an_output_only_pin_is_rejected() {
  MockI2cBus bus;
  Mcp23017Sim sim;
  bus.attach(kAddress, &sim);
  Mcp23017 chip(bus, kAddress);
  Mcp23017ButtonSource::Map map = Mcp23017ButtonSource::defaultMap();
  map.buttonOfPin[15] = static_cast<uint8_t>(Button::StickRight);  // GPB7
  Mcp23017ButtonSource source(chip, map);
  TEST_ASSERT_STATUS(Status::InvalidArgument, source.begin());
  TEST_ASSERT_FALSE(sim.outputOnlyViolated);
}

static void test_source_propagates_read_errors() {
  MockI2cBus bus;
  Mcp23017Sim sim;
  bus.attach(kAddress, &sim);
  Mcp23017 chip(bus, kAddress);
  Mcp23017ButtonSource source(chip, Mcp23017ButtonSource::defaultMap());
  TEST_ASSERT_OK(source.begin());
  bus.detach(kAddress);  // o módulo foi desligado
  TEST_ASSERT_STATUS(Status::HardwareAbsent, source.read().status());
  TEST_ASSERT_STATUS(Status::HardwareAbsent, source.begin());
}

static void test_gpio_source_reads_the_bench_buttons() {
  namespace bench = dbmo::config::bench;
  MockGpio gpio;
  const GpioButtonSource::Entry entries[] = {{bench::kButtonUp, Button::DpadUp},
                                             {bench::kButtonOk, Button::A},
                                             {bench::kButtonDown, Button::DpadDown}};
  GpioButtonSource source(gpio, entries, 3);
  TEST_ASSERT_STATUS(Status::NotInitialized, source.read().status());  // antes do begin
  TEST_ASSERT_OK(source.begin());
  TEST_ASSERT_TRUE(gpio.mode(bench::kButtonOk) == dbmo::PinMode::InputPullUp);
  TEST_ASSERT_EQUAL_UINT32(0, source.read().value());

  gpio.setInput(bench::kButtonOk, false);  // botão ao GND apertado
  gpio.setInput(bench::kButtonDown, false);
  TEST_ASSERT_EQUAL_HEX32(maskOf(Button::A) | maskOf(Button::DpadDown), source.read().value());

  gpio.failNext(Status::IoError);
  TEST_ASSERT_STATUS(Status::IoError, source.read().status());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_begin_configures_direction_pullups_and_polarity);
  RUN_TEST(test_begin_refuses_gpa7_and_gpb7_as_inputs);
  RUN_TEST(test_absent_chip_and_bus_errors);
  RUN_TEST(test_begin_detects_a_chip_that_does_not_keep_the_configuration);
  RUN_TEST(test_read_pins_reflects_pullups_and_pressed_buttons);
  RUN_TEST(test_default_map_reads_each_of_the_14_buttons);
  RUN_TEST(test_map_with_a_button_on_an_output_only_pin_is_rejected);
  RUN_TEST(test_source_propagates_read_errors);
  RUN_TEST(test_gpio_source_reads_the_bench_buttons);
  return UNITY_END();
}
