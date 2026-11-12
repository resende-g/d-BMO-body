#include <cstring>

#include "dbmo/cartridges/cartridge_subsystem.h"
#include "dbmo/config/pins.h"
#include "dbmo/mock/mock_hal.h"
#include "dbmo/sim/eeprom_24c02_sim.h"
#include "dbmo_test.h"
#include "example_sensor_cartridge.h"

using dbmo::ConfigStore;
using dbmo::Event;
using dbmo::EventBus;
using dbmo::ResourceKind;
using dbmo::ResourceRegistry;
using dbmo::Status;
using dbmo::Version;
using dbmo::mock::MockClock;
using dbmo::mock::MockI2cBus;
using dbmo::mock::RegisterDeviceSim;
using dbmo::sim::Eeprom24c02Sim;
using namespace dbmo::cartridges;
using examples::ExampleSensorCartridge;

namespace {

constexpr uint8_t kEepromAddress = dbmo::config::i2c::kCartridgeEeprom;

// Um driver que falha quando mandado, para testar o isolamento de falhas.
class FlakyDriver final : public ICartridgeDriver {
 public:
  static constexpr uint16_t kProduct = 0x0BAD;
  const char *name() const override { return "flaky"; }
  bool matches(const CartridgeHeader &header) const override {
    return header.productId == kProduct;
  }
  Status start(CartridgeContext &context) override {
    ++starts;
    if (context.resources != nullptr)
      (void)context.resources->claim(ResourceKind::Gpio, 38, name());
    return startStatus;
  }
  Status tick(uint32_t) override {
    ++ticks;
    return tickStatus;
  }
  void stop() override { ++stops; }

  Status startStatus = Status::Ok;
  Status tickStatus = Status::Ok;
  int starts = 0;
  int ticks = 0;
  int stops = 0;
};

struct Fixture {
  MockClock clock;
  MockI2cBus i2c;
  EventBus bus{&clock};
  ConfigStore config;
  ResourceRegistry resources;
  Eeprom24c02Sim chip{clock};
  Eeprom24c02 eeprom{i2c, clock, kEepromAddress};
  CartridgeRegistry registry;
  ExampleSensorCartridge sensorDriver;
  FlakyDriver flakyDriver;
  RegisterDeviceSim sensor;
  CartridgeSubsystem slot{eeprom, registry, bus, config, resources};
  Event events[32];
  int count = 0;

  Fixture() {
    TEST_ASSERT_OK(CartridgeSubsystem::registerConfig(config));
    TEST_ASSERT_OK(registry.add(sensorDriver));
    TEST_ASSERT_OK(registry.add(flakyDriver));
    slot.setBuses(&i2c, nullptr, nullptr);
    TEST_ASSERT_OK(bus.subscribeDomain(dbmo::EventDomain::Cartridge, &Fixture::collect, this));
    TEST_ASSERT_OK(slot.init());
  }
  static void collect(const Event &event, void *context) {
    auto *self = static_cast<Fixture *>(context);
    if (self->count < 32) self->events[self->count++] = event;
  }
  void insert(const CartridgeHeader &header) {
    uint8_t raw[kHeaderSize];
    TEST_ASSERT_OK(encodeHeader(header, raw));
    chip.load(raw, sizeof(raw));
    i2c.attach(kEepromAddress, &chip);
  }
  void insertExampleSensor() {
    sensor.memory[ExampleSensorCartridge::kValueRegister] = 42;
    i2c.attach(ExampleSensorCartridge::kSensorAddress, &sensor);
    insert(ExampleSensorCartridge::makeHeader());
  }
  void pull() {
    i2c.detach(kEepromAddress);
    i2c.detach(ExampleSensorCartridge::kSensorAddress);
  }
  // Roda o laço principal por durationMs em passos de 10 ms.
  void run(uint32_t durationMs) {
    for (uint32_t elapsed = 0; elapsed < durationMs; elapsed += 10) {
      clock.advanceMs(10);
      slot.tick(clock.nowMs());
      bus.dispatch();
    }
  }
  const Event *last(dbmo::EventId id) const {
    for (int i = count - 1; i >= 0; --i)
      if (events[i].id == id) return &events[i];
    return nullptr;
  }
  int countOf(dbmo::EventId id) const {
    int matches = 0;
    for (int i = 0; i < count; ++i)
      if (events[i].id == id) ++matches;
    return matches;
  }
};

CartridgeHeader flakyHeader() {
  CartridgeHeader header = ExampleSensorCartridge::makeHeader();
  header.productId = FlakyDriver::kProduct;
  header.i2cAddressCount = 0;
  header.capabilities = capability::kGpio0;
  return header;
}

}  // namespace

void setUp() {}
void tearDown() {}

static void test_empty_slot_is_the_normal_state() {
  Fixture f;
  f.run(2000);
  TEST_ASSERT_TRUE(f.slot.state() == SlotState::Empty);
  TEST_ASSERT_NULL(f.slot.header());
  TEST_ASSERT_EQUAL_INT(0, f.count);
}

static void test_valid_cartridge_is_identified_and_its_driver_runs() {
  Fixture f;
  f.insertExampleSensor();
  f.run(600);
  TEST_ASSERT_TRUE(f.slot.state() == SlotState::Active);
  TEST_ASSERT_NOT_NULL(f.slot.header());
  TEST_ASSERT_EQUAL_STRING("EXEMPLO SENSOR", f.slot.header()->name);
  TEST_ASSERT_TRUE(f.sensorDriver.running());
  TEST_ASSERT_EQUAL_UINT8(42, f.sensorDriver.lastValue());
  // O driver reservou o endereço do sensor no registro de recursos.
  TEST_ASSERT_EQUAL_STRING(
      "example-sensor",
      f.resources.ownerOf(ResourceKind::I2cAddress, ExampleSensorCartridge::kSensorAddress));

  const Event *changed = f.last(cartridge_event::kStateChanged);
  TEST_ASSERT_NOT_NULL(changed);
  TEST_ASSERT_EQUAL_HEX32(0xFFFF0001u, changed->a);  // fornecedor e produto
  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(SlotState::Active), changed->b);
  TEST_ASSERT_NOT_NULL(f.last(ExampleSensorCartridge::kSampleEvent));
}

static void test_removal_stops_the_driver_and_frees_its_resources() {
  Fixture f;
  f.insertExampleSensor();
  f.run(600);
  f.pull();
  f.run(400);  // uma consulta sem resposta ainda não basta
  TEST_ASSERT_TRUE(f.slot.state() != SlotState::Empty);
  f.run(1000);
  TEST_ASSERT_TRUE(f.slot.state() == SlotState::Empty);
  TEST_ASSERT_FALSE(f.sensorDriver.running());
  TEST_ASSERT_NULL(
      f.resources.ownerOf(ResourceKind::I2cAddress, ExampleSensorCartridge::kSensorAddress));
  TEST_ASSERT_NULL(f.slot.header());
  const Event *changed = f.last(cartridge_event::kStateChanged);
  TEST_ASSERT_EQUAL_HEX32(0xFFFF0001u, changed->a);  // ainda identifica quem saiu
  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(SlotState::Empty), changed->b);

  // E pode ser inserido de novo.
  f.insertExampleSensor();
  f.run(600);
  TEST_ASSERT_TRUE(f.slot.state() == SlotState::Active);
}

static void test_blank_eeprom_is_unknown_not_an_error() {
  Fixture f;
  f.i2c.attach(kEepromAddress, &f.chip);  // EEPROM de fábrica, tudo 0xFF
  f.run(600);
  TEST_ASSERT_TRUE(f.slot.state() == SlotState::Unknown);
  TEST_ASSERT_TRUE(f.slot.lastHeaderCheck() == HeaderCheck::Blank);
  TEST_ASSERT_NULL(f.slot.header());
  TEST_ASSERT_EQUAL_INT(0, f.sensorDriver.running() ? 1 : 0);
}

static void test_corrupted_header_is_unknown() {
  Fixture f;
  f.insertExampleSensor();
  f.chip.memory[20] ^= 0x10;  // um bit trocado no campo de corrente
  f.run(600);
  TEST_ASSERT_TRUE(f.slot.state() == SlotState::Unknown);
  TEST_ASSERT_TRUE(f.slot.lastHeaderCheck() == HeaderCheck::BadCrc);
  TEST_ASSERT_FALSE(f.sensorDriver.running());
}

static void test_incompatible_cartridges_are_rejected_with_the_reason() {
  struct Case {
    Compatibility expected;
    void (*mutate)(CartridgeHeader &);
  };
  const Case cases[] = {
      {Compatibility::FirmwareTooOld,
       [](CartridgeHeader &h) { h.minHostFirmware = Version{9, 0, 0}; }},
      {Compatibility::CapabilityUnavailable,
       [](CartridgeHeader &h) { h.capabilities |= capability::kNeeds5V; }},
      {Compatibility::I2cAddressConflict, [](CartridgeHeader &h) { h.i2cAddresses[0] = 0x24; }},
      {Compatibility::OverCurrentBudget, [](CartridgeHeader &h) { h.maxCurrentMa = 400; }},
  };
  for (const Case &test : cases) {
    Fixture f;
    CartridgeHeader header = ExampleSensorCartridge::makeHeader();
    test.mutate(header);
    f.insert(header);
    f.run(600);
    TEST_ASSERT_TRUE_MESSAGE(f.slot.state() == SlotState::Rejected, toString(test.expected));
    TEST_ASSERT_TRUE_MESSAGE(f.slot.lastCompatibility() == test.expected, toString(test.expected));
    TEST_ASSERT_FALSE(f.sensorDriver.running());  // nenhum driver chega a ser iniciado
    const Event *rejected = f.last(cartridge_event::kRejected);
    TEST_ASSERT_NOT_NULL(rejected);
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(test.expected), rejected->b);
  }
}

static void test_current_budget_comes_from_configuration() {
  Fixture f;
  TEST_ASSERT_OK(f.config.setInt(config_key::kMaxCurrentMa, 3));
  f.insertExampleSensor();  // declara 5 mA
  f.run(600);
  TEST_ASSERT_TRUE(f.slot.state() == SlotState::Rejected);
  TEST_ASSERT_TRUE(f.slot.lastCompatibility() == Compatibility::OverCurrentBudget);
}

static void test_valid_cartridge_without_a_driver_is_identified() {
  Fixture f;
  CartridgeHeader header = ExampleSensorCartridge::makeHeader();
  header.productId = 0x7777;  // ninguém registrado para este produto
  f.insert(header);
  f.run(600);
  TEST_ASSERT_TRUE(f.slot.state() == SlotState::Identified);
  TEST_ASSERT_NOT_NULL(f.slot.header());
  TEST_ASSERT_EQUAL_INT(0, f.countOf(cartridge_event::kFault));
}

static void test_driver_that_fails_to_start_is_contained() {
  Fixture f;
  f.flakyDriver.startStatus = Status::IoError;
  f.insert(flakyHeader());
  f.run(600);
  TEST_ASSERT_TRUE(f.slot.state() == SlotState::Faulted);
  TEST_ASSERT_EQUAL_INT(1, f.flakyDriver.starts);
  TEST_ASSERT_EQUAL_INT(1, f.flakyDriver.stops);                  // levado a estado seguro
  TEST_ASSERT_NULL(f.resources.ownerOf(ResourceKind::Gpio, 38));  // reserva devolvida
  const Event *fault = f.last(cartridge_event::kFault);
  TEST_ASSERT_NOT_NULL(fault);
  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(Status::IoError), fault->b);
  f.run(3000);
  TEST_ASSERT_EQUAL_INT(1, f.flakyDriver.starts);  // não fica tentando de novo sozinho
}

static void test_driver_that_fails_while_running_is_stopped() {
  Fixture f;
  f.insert(flakyHeader());
  f.run(600);
  TEST_ASSERT_TRUE(f.slot.state() == SlotState::Active);
  f.flakyDriver.tickStatus = Status::Timeout;
  f.run(20);
  TEST_ASSERT_TRUE(f.slot.state() == SlotState::Faulted);
  TEST_ASSERT_EQUAL_INT(1, f.flakyDriver.stops);
  const int ticks = f.flakyDriver.ticks;
  f.run(1000);
  TEST_ASSERT_EQUAL_INT(ticks, f.flakyDriver.ticks);  // parado de verdade
  // O conector continua sendo acompanhado: retirar o cartucho volta a Empty.
  f.pull();
  f.run(1500);
  TEST_ASSERT_TRUE(f.slot.state() == SlotState::Empty);
}

static void test_cartridge_swapped_between_polls_is_re_identified() {
  Fixture f;
  f.insertExampleSensor();
  f.run(600);
  TEST_ASSERT_TRUE(f.slot.state() == SlotState::Active);
  // Troca rápida: a EEPROM nunca deixa de responder, mas a identidade muda.
  f.insert(flakyHeader());
  f.run(600);
  TEST_ASSERT_FALSE(f.sensorDriver.running());
  TEST_ASSERT_TRUE(f.slot.state() == SlotState::Active);
  TEST_ASSERT_EQUAL_HEX16(FlakyDriver::kProduct, f.slot.header()->productId);
  TEST_ASSERT_EQUAL_INT(1, f.flakyDriver.starts);
}

static void test_read_errors_during_insertion_are_retried() {
  Fixture f;
  f.insertExampleSensor();
  f.clock.advanceMs(10);
  f.i2c.failNext(Status::IoError, 2);  // mau contato na primeira consulta
  f.slot.tick(f.clock.nowMs());
  TEST_ASSERT_TRUE(f.slot.state() == SlotState::Empty);
  f.run(1200);
  TEST_ASSERT_TRUE(f.slot.state() == SlotState::Active);
}

static void test_bus_only_offered_when_declared() {
  Fixture f;
  // O cartucho instável declara só GPIO: não recebe o barramento I2C.
  struct Probe final : public ICartridgeDriver {
    const char *name() const override { return "probe"; }
    bool matches(const CartridgeHeader &h) const override { return h.productId == 0x0C0C; }
    Status start(CartridgeContext &context) override {
      sawI2c = context.i2c != nullptr;
      sawSpi = context.spi != nullptr;
      return Status::Ok;
    }
    Status tick(uint32_t) override { return Status::Ok; }
    void stop() override {}
    bool sawI2c = true;
    bool sawSpi = true;
  } probe;
  TEST_ASSERT_OK(f.registry.add(probe));
  CartridgeHeader header = flakyHeader();
  header.productId = 0x0C0C;
  f.insert(header);
  f.run(600);
  TEST_ASSERT_TRUE(f.slot.state() == SlotState::Active);
  TEST_ASSERT_FALSE(probe.sawI2c);
  TEST_ASSERT_FALSE(probe.sawSpi);
}

static void test_registry_rules_and_shutdown() {
  Fixture f;
  TEST_ASSERT_STATUS(Status::AlreadyInitialized, f.registry.add(f.sensorDriver));
  FlakyDriver extra[CartridgeRegistry::kMaxDrivers];
  for (size_t i = f.registry.size(); i < CartridgeRegistry::kMaxDrivers; ++i)
    TEST_ASSERT_OK(f.registry.add(extra[i]));
  TEST_ASSERT_STATUS(Status::NoSpace, f.registry.add(extra[0]));

  f.insertExampleSensor();
  f.run(600);
  TEST_ASSERT_OK(f.slot.shutdown());
  TEST_ASSERT_FALSE(f.sensorDriver.running());
  f.run(1000);                    // depois de desligado, nada acontece
  TEST_ASSERT_OK(f.slot.init());  // e pode iniciar de novo
  f.run(600);
  TEST_ASSERT_TRUE(f.slot.state() == SlotState::Active);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_empty_slot_is_the_normal_state);
  RUN_TEST(test_valid_cartridge_is_identified_and_its_driver_runs);
  RUN_TEST(test_removal_stops_the_driver_and_frees_its_resources);
  RUN_TEST(test_blank_eeprom_is_unknown_not_an_error);
  RUN_TEST(test_corrupted_header_is_unknown);
  RUN_TEST(test_incompatible_cartridges_are_rejected_with_the_reason);
  RUN_TEST(test_current_budget_comes_from_configuration);
  RUN_TEST(test_valid_cartridge_without_a_driver_is_identified);
  RUN_TEST(test_driver_that_fails_to_start_is_contained);
  RUN_TEST(test_driver_that_fails_while_running_is_stopped);
  RUN_TEST(test_cartridge_swapped_between_polls_is_re_identified);
  RUN_TEST(test_read_errors_during_insertion_are_retried);
  RUN_TEST(test_bus_only_offered_when_declared);
  RUN_TEST(test_registry_rules_and_shutdown);
  return UNITY_END();
}
