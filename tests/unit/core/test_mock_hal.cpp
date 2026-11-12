// Os mocks sustentam todos os outros testes: aqui se confere que eles próprios se
// comportam como os contratos de HAL descrevem.
#include "dbmo/mock/mock_hal.h"
#include "dbmo_test.h"

using dbmo::PinMode;
using dbmo::Status;
using namespace dbmo::mock;

void setUp() {}
void tearDown() {}

static void test_clock_advances_only_on_demand() {
  MockClock clock;
  TEST_ASSERT_EQUAL_UINT32(0, clock.nowMs());
  clock.advanceMs(1500);
  clock.delayMs(500);
  clock.delayUs(250);
  TEST_ASSERT_EQUAL_UINT32(2000, clock.nowMs());
  TEST_ASSERT_EQUAL_UINT64(2000250, clock.nowUs());
  TEST_ASSERT_EQUAL_UINT(2, clock.delayCalls);
}

static void test_elapsed_is_correct_across_rollover() {
  // 50 ms antes de estourar até 50 ms depois: a diferença continua 100.
  const uint32_t before = 0xFFFFFFFFu - 49;
  const uint32_t after = 50;
  TEST_ASSERT_EQUAL_UINT32(100, dbmo::elapsedMs(after, before));
}

static void test_gpio_modes_and_levels() {
  MockGpio gpio;
  TEST_ASSERT_STATUS(Status::NotInitialized, gpio.read(2).status());
  TEST_ASSERT_STATUS(Status::InvalidArgument, gpio.configure(64, PinMode::Input));

  TEST_ASSERT_OK(gpio.configure(2, PinMode::InputPullUp));
  TEST_ASSERT_TRUE(gpio.read(2).value());  // pull-up sem ninguém puxando
  gpio.setInput(2, false);                 // botão apertado
  TEST_ASSERT_FALSE(gpio.read(2).value());
  gpio.floatInput(2);
  TEST_ASSERT_TRUE(gpio.read(2).value());

  TEST_ASSERT_OK(gpio.configure(3, PinMode::InputPullDown));
  TEST_ASSERT_FALSE(gpio.read(3).value());

  // Escrever em pino de entrada é erro de uso.
  TEST_ASSERT_STATUS(Status::NotInitialized, gpio.write(2, true));
  TEST_ASSERT_OK(gpio.configure(10, PinMode::Output));
  TEST_ASSERT_OK(gpio.write(10, true));
  TEST_ASSERT_TRUE(gpio.output(10));
  TEST_ASSERT_TRUE(gpio.read(10).value());

  gpio.failNext(Status::IoError);
  TEST_ASSERT_STATUS(Status::IoError, gpio.write(10, false));
  TEST_ASSERT_TRUE(gpio.output(10));
  TEST_ASSERT_OK(gpio.write(10, false));
}

static void test_i2c_bus_nacks_missing_devices_and_injects_faults() {
  MockI2cBus bus;
  RegisterDeviceSim device;
  uint8_t byte = 0;
  TEST_ASSERT_STATUS(Status::HardwareAbsent, bus.probe(0x20));
  TEST_ASSERT_STATUS(Status::HardwareAbsent, bus.read(0x20, &byte, 1));

  bus.attach(0x20, &device);
  TEST_ASSERT_OK(bus.probe(0x20));

  bus.failNext(Status::IoError, 2);
  TEST_ASSERT_STATUS(Status::IoError, bus.probe(0x20));
  TEST_ASSERT_STATUS(Status::IoError, bus.read(0x20, &byte, 1));
  TEST_ASSERT_OK(bus.probe(0x20));

  bus.detach(0x20);
  TEST_ASSERT_STATUS(Status::HardwareAbsent, bus.probe(0x20));
}

static void test_register_device_pointer_semantics() {
  MockI2cBus bus;
  RegisterDeviceSim device;
  bus.attach(0x50, &device);

  const uint8_t write[] = {0x10, 0xAA, 0xBB};  // registrador 0x10, dois bytes
  TEST_ASSERT_OK(bus.write(0x50, write, sizeof(write)));
  TEST_ASSERT_EQUAL_HEX8(0xAA, device.memory[0x10]);
  TEST_ASSERT_EQUAL_HEX8(0xBB, device.memory[0x11]);

  const uint8_t pointer = 0x10;
  uint8_t read[2] = {};
  TEST_ASSERT_OK(bus.writeRead(0x50, &pointer, 1, read, 2));
  TEST_ASSERT_EQUAL_HEX8(0xAA, read[0]);
  TEST_ASSERT_EQUAL_HEX8(0xBB, read[1]);
  TEST_ASSERT_EQUAL_UINT(1, bus.writeReadCount);
  TEST_ASSERT_EQUAL_HEX8(0x50, bus.lastAddress);
}

static void test_spi_device_enforces_transaction_and_logs_bytes() {
  MockSpiDevice spi;
  uint8_t rx[2] = {0xFF, 0xFF};
  const uint8_t tx[2] = {0x30, 0x31};
  TEST_ASSERT_STATUS(Status::NotInitialized, spi.transfer(tx, rx, 2));  // sem select()

  spi.setHandler(
      [](const uint8_t *sent, uint8_t *received, size_t n, void *) {
        for (size_t i = 0; i < n && received != nullptr; ++i)
          received[i] = static_cast<uint8_t>(sent[i] + 1);
      },
      nullptr);
  {
    dbmo::SpiTransaction transaction(spi);
    TEST_ASSERT_OK(transaction.status());
    TEST_ASSERT_TRUE(spi.selected());
    TEST_ASSERT_STATUS(Status::Busy, spi.select());  // já em transação
    TEST_ASSERT_OK(spi.transfer(tx, rx, 2));
  }
  TEST_ASSERT_FALSE(spi.selected());  // o destrutor fechou
  TEST_ASSERT_EQUAL_HEX8(0x31, rx[0]);
  TEST_ASSERT_EQUAL_HEX8(0x32, rx[1]);
  TEST_ASSERT_EQUAL_size_t(2, spi.sentLength);
  TEST_ASSERT_EQUAL_HEX8(0x30, spi.sent[0]);
  TEST_ASSERT_EQUAL_UINT(1, spi.deselectCount);

  spi.misoLevel = true;
  TEST_ASSERT_TRUE(spi.readMiso().value());
  spi.misoReadable = false;
  TEST_ASSERT_STATUS(Status::Unsupported, spi.readMiso().status());
}

static void test_adc_and_pwm() {
  MockAdcChannel adc;
  adc.raw = 2048;
  adc.millivolts = 1650;
  TEST_ASSERT_EQUAL_UINT16(2048, adc.readRaw().value());
  TEST_ASSERT_EQUAL_UINT16(1650, adc.readMillivolts().value());
  adc.status = Status::Timeout;
  TEST_ASSERT_STATUS(Status::Timeout, adc.readRaw().status());

  MockPwmOutput pwm;
  TEST_ASSERT_OK(pwm.setDutyPermille(500));
  TEST_ASSERT_EQUAL_UINT16(500, pwm.duty);
  TEST_ASSERT_STATUS(Status::OutOfRange, pwm.setDutyPermille(1001));
  TEST_ASSERT_EQUAL_UINT16(500, pwm.duty);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_clock_advances_only_on_demand);
  RUN_TEST(test_elapsed_is_correct_across_rollover);
  RUN_TEST(test_gpio_modes_and_levels);
  RUN_TEST(test_i2c_bus_nacks_missing_devices_and_injects_faults);
  RUN_TEST(test_register_device_pointer_semantics);
  RUN_TEST(test_spi_device_enforces_transaction_and_logs_bytes);
  RUN_TEST(test_adc_and_pwm);
  return UNITY_END();
}
