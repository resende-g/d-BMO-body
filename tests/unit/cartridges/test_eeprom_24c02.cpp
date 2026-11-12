#include <cstring>

#include "dbmo/cartridges/eeprom_24c02.h"
#include "dbmo/mock/mock_hal.h"
#include "dbmo/sim/eeprom_24c02_sim.h"
#include "dbmo_test.h"

using dbmo::Status;
using dbmo::cartridges::Eeprom24c02;
using dbmo::mock::MockClock;
using dbmo::mock::MockI2cBus;
using dbmo::sim::Eeprom24c02Sim;

namespace {
constexpr uint8_t kAddress = 0x50;

struct Fixture {
  MockClock clock;
  MockI2cBus bus;
  Eeprom24c02Sim chip{clock};
  Eeprom24c02 eeprom{bus, clock, kAddress};
  Fixture() { bus.attach(kAddress, &chip); }
};
}  // namespace

void setUp() {}
void tearDown() {}

static void test_factory_fresh_chip_reads_ff() {
  Fixture f;
  uint8_t data[64];
  TEST_ASSERT_OK(f.eeprom.probe());
  TEST_ASSERT_OK(f.eeprom.read(0, data, sizeof(data)));
  for (uint8_t byte : data) TEST_ASSERT_EQUAL_HEX8(0xFF, byte);
}

static void test_write_spanning_pages_lands_at_the_right_addresses() {
  Fixture f;
  // 20 bytes a partir do endereço 6: cruza dois limites de página de 8 bytes. Um driver
  // que mandasse tudo em uma transação sobrescreveria o início da primeira página.
  uint8_t data[20];
  for (uint8_t i = 0; i < sizeof(data); ++i) data[i] = static_cast<uint8_t>(0xA0 + i);
  TEST_ASSERT_OK(f.eeprom.write(6, data, sizeof(data)));

  TEST_ASSERT_EQUAL_HEX8(0xFF, f.chip.memory[5]);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(data, f.chip.memory + 6, sizeof(data));
  TEST_ASSERT_EQUAL_HEX8(0xFF, f.chip.memory[26]);
  TEST_ASSERT_EQUAL_HEX8(0xFF, f.chip.memory[0]);  // o início da página não foi atingido
  // Páginas tocadas: 0–7 (2 bytes), 8–15, 16–23 e 24–31 (2 bytes): quatro ciclos.
  TEST_ASSERT_EQUAL_UINT(4, f.chip.writeCycles);

  uint8_t back[20];
  TEST_ASSERT_OK(f.eeprom.read(6, back, sizeof(back)));
  TEST_ASSERT_EQUAL_UINT8_ARRAY(data, back, sizeof(data));
}

static void test_driver_waits_for_the_write_cycle() {
  Fixture f;
  const uint8_t data[4] = {1, 2, 3, 4};
  const uint32_t before = f.clock.nowMs();
  TEST_ASSERT_OK(f.eeprom.write(0, data, sizeof(data)));
  // O chip ficou 5 ms sem responder; o driver esperou em vez de falhar.
  TEST_ASSERT_TRUE(f.clock.nowMs() - before >= 5);
}

static void test_write_cycle_that_never_ends_times_out() {
  Fixture f;
  f.chip.writeCycleMs = 1000;
  const uint8_t data[2] = {1, 2};
  TEST_ASSERT_STATUS(Status::Timeout, f.eeprom.write(0, data, sizeof(data)));
}

static void test_write_protect_is_detected_by_read_back() {
  Fixture f;
  f.chip.writeProtect = true;
  const uint8_t data[3] = {9, 8, 7};
  TEST_ASSERT_STATUS(Status::IoError, f.eeprom.write(0x10, data, sizeof(data)));
  TEST_ASSERT_EQUAL_HEX8(0xFF, f.chip.memory[0x10]);
}

static void test_bounds_are_enforced() {
  Fixture f;
  uint8_t data[8] = {};
  TEST_ASSERT_OK(f.eeprom.read(248, data, 8));  // os últimos 8 bytes
  TEST_ASSERT_STATUS(Status::OutOfRange, f.eeprom.read(249, data, 8));
  TEST_ASSERT_STATUS(Status::OutOfRange, f.eeprom.read(257, data, 0));
  TEST_ASSERT_STATUS(Status::OutOfRange, f.eeprom.write(250, data, 8));
  TEST_ASSERT_STATUS(Status::InvalidArgument, f.eeprom.read(0, nullptr, 1));
  TEST_ASSERT_STATUS(Status::InvalidArgument, f.eeprom.write(0, nullptr, 1));
  TEST_ASSERT_OK(f.eeprom.write(248, data, 8));
}

static void test_absent_chip_and_bus_errors() {
  MockClock clock;
  MockI2cBus bus;
  Eeprom24c02 eeprom(bus, clock, kAddress);
  uint8_t data[4] = {};
  TEST_ASSERT_STATUS(Status::HardwareAbsent, eeprom.probe());
  TEST_ASSERT_STATUS(Status::HardwareAbsent, eeprom.read(0, data, 4));
  TEST_ASSERT_STATUS(Status::HardwareAbsent, eeprom.write(0, data, 4));

  Eeprom24c02Sim chip(clock);
  bus.attach(kAddress, &chip);
  bus.failNext(Status::IoError);
  TEST_ASSERT_STATUS(Status::IoError, eeprom.read(0, data, 4));
  TEST_ASSERT_OK(eeprom.read(0, data, 4));
}

static void test_default_page_size_is_safe_on_a_16_byte_page_chip() {
  MockClock clock;
  MockI2cBus bus;
  Eeprom24c02Sim chip(clock, 16);  // ST M24C02
  bus.attach(kAddress, &chip);
  Eeprom24c02 eeprom(bus, clock, kAddress);  // driver com página de 8
  uint8_t data[40];
  for (uint8_t i = 0; i < sizeof(data); ++i) data[i] = i;
  TEST_ASSERT_OK(eeprom.write(3, data, sizeof(data)));
  TEST_ASSERT_EQUAL_UINT8_ARRAY(data, chip.memory + 3, sizeof(data));
}

static void test_long_read_is_split_into_chunks() {
  Fixture f;
  for (int i = 0; i < 256; ++i) f.chip.memory[i] = static_cast<uint8_t>(i);
  uint8_t data[256];
  TEST_ASSERT_OK(f.eeprom.read(0, data, sizeof(data)));
  for (int i = 0; i < 256; ++i) TEST_ASSERT_EQUAL_UINT8(i, data[i]);
  TEST_ASSERT_EQUAL_UINT(256 / Eeprom24c02::kReadChunk, f.bus.writeReadCount);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_factory_fresh_chip_reads_ff);
  RUN_TEST(test_write_spanning_pages_lands_at_the_right_addresses);
  RUN_TEST(test_driver_waits_for_the_write_cycle);
  RUN_TEST(test_write_cycle_that_never_ends_times_out);
  RUN_TEST(test_write_protect_is_detected_by_read_back);
  RUN_TEST(test_bounds_are_enforced);
  RUN_TEST(test_absent_chip_and_bus_errors);
  RUN_TEST(test_default_page_size_is_safe_on_a_16_byte_page_chip);
  RUN_TEST(test_long_read_is_split_into_chunks);
  return UNITY_END();
}
