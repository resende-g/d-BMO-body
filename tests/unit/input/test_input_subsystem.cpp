#include "dbmo/input/input_subsystem.h"
#include "dbmo/mock/mock_hal.h"
#include "dbmo/sim/mcp23017_sim.h"
#include "dbmo_test.h"

using dbmo::ConfigStore;
using dbmo::Event;
using dbmo::EventBus;
using dbmo::Status;
using dbmo::mock::MockAdcChannel;
using dbmo::mock::MockI2cBus;
using namespace dbmo::input;

namespace {

class FakeSource final : public IButtonSource {
 public:
  Status begin() override {
    ++beginCalls;
    return beginStatus;
  }
  dbmo::Result<ButtonMask> read() override {
    if (!dbmo::isOk(readStatus)) return readStatus;
    return mask;
  }
  Status beginStatus = Status::Ok;
  Status readStatus = Status::Ok;
  ButtonMask mask = 0;
  int beginCalls = 0;
};

struct Fixture {
  EventBus bus;
  ConfigStore config;
  InputSubsystem input{bus, config};
  Event events[64];
  int count = 0;
  uint32_t now = 0;

  Fixture() {
    TEST_ASSERT_OK(InputSubsystem::registerConfig(config));
    TEST_ASSERT_OK(bus.subscribeAll(&Fixture::collect, this));
  }
  static void collect(const Event &event, void *context) {
    auto *self = static_cast<Fixture *>(context);
    if (self->count < 64) self->events[self->count++] = event;
  }
  // Roda o laço principal por durationMs, um tick por milissegundo.
  void run(uint32_t durationMs) {
    for (uint32_t i = 0; i < durationMs; ++i) {
      input.tick(++now);
      bus.dispatch();
    }
  }
  int countOf(dbmo::EventId id, uint32_t a, uint32_t b) const {
    int matches = 0;
    for (int i = 0; i < count; ++i)
      if (events[i].id == id && events[i].a == a && events[i].b == b) ++matches;
    return matches;
  }
  int countOf(dbmo::EventId id) const {
    int matches = 0;
    for (int i = 0; i < count; ++i)
      if (events[i].id == id) ++matches;
    return matches;
  }
};

constexpr uint32_t kA = static_cast<uint32_t>(Button::A);
constexpr uint32_t kPressed = static_cast<uint32_t>(ButtonAction::Pressed);
constexpr uint32_t kReleased = static_cast<uint32_t>(ButtonAction::Released);
constexpr uint32_t kClick = static_cast<uint32_t>(ButtonAction::Click);

}  // namespace

void setUp() {}
void tearDown() {}

static void test_button_events_are_published_on_the_bus() {
  Fixture f;
  FakeSource source;
  TEST_ASSERT_OK(f.input.addButtonSource(source));
  TEST_ASSERT_OK(f.input.init());

  source.mask = maskOf(Button::A);
  f.run(100);
  TEST_ASSERT_EQUAL_INT(1, f.countOf(input_event::kButton, kA, kPressed));
  TEST_ASSERT_TRUE(f.input.isPressed(Button::A));
  source.mask = 0;
  f.run(100);
  TEST_ASSERT_EQUAL_INT(1, f.countOf(input_event::kButton, kA, kReleased));
  TEST_ASSERT_EQUAL_INT(1, f.countOf(input_event::kButton, kA, kClick));
}

static void test_combo_events_are_published() {
  Fixture f;
  FakeSource source;
  TEST_ASSERT_OK(f.input.addButtonSource(source));
  TEST_ASSERT_OK(
      f.input.buttons().addCombo({3, maskOf(Button::Select) | maskOf(Button::DpadUp), 0, false}));
  TEST_ASSERT_OK(f.input.init());
  source.mask = maskOf(Button::Select) | maskOf(Button::DpadUp);
  f.run(100);
  TEST_ASSERT_EQUAL_INT(1, f.countOf(input_event::kCombo, 3, 0));
}

static void test_two_sources_are_merged() {
  Fixture f;
  FakeSource bench, expander;
  TEST_ASSERT_OK(f.input.addButtonSource(bench));
  TEST_ASSERT_OK(f.input.addButtonSource(expander));
  FakeSource extra;
  TEST_ASSERT_STATUS(Status::NoSpace, f.input.addButtonSource(extra));
  TEST_ASSERT_OK(f.input.init());

  bench.mask = maskOf(Button::A);
  expander.mask = maskOf(Button::Start);
  f.run(100);
  TEST_ASSERT_TRUE(f.input.isPressed(Button::A));
  TEST_ASSERT_TRUE(f.input.isPressed(Button::Start));
}

static void test_init_reports_absent_hardware_only_when_nothing_answers() {
  Fixture f;
  FakeSource present, absent;
  absent.beginStatus = Status::HardwareAbsent;
  TEST_ASSERT_OK(f.input.addButtonSource(absent));
  TEST_ASSERT_STATUS(Status::HardwareAbsent, f.input.init());

  Fixture g;
  FakeSource missing;
  missing.beginStatus = Status::HardwareAbsent;
  TEST_ASSERT_OK(g.input.addButtonSource(missing));
  TEST_ASSERT_OK(g.input.addButtonSource(present));
  TEST_ASSERT_OK(g.input.init());  // os botões da bancada bastam
  TEST_ASSERT_EQUAL_size_t(1, g.input.healthySources());
}

static void test_failing_source_releases_buttons_and_is_reported_once() {
  Fixture f;
  FakeSource source;
  TEST_ASSERT_OK(f.input.addButtonSource(source));
  TEST_ASSERT_OK(f.input.init());
  source.mask = maskOf(Button::A);
  f.run(100);
  TEST_ASSERT_TRUE(f.input.isPressed(Button::A));

  source.readStatus = Status::IoError;  // o expansor parou de responder
  f.run(15);                            // menos que o limite de erros: mantém o estado
  TEST_ASSERT_TRUE(f.input.isPressed(Button::A));
  f.run(100);
  TEST_ASSERT_FALSE(f.input.isPressed(Button::A));  // nenhuma tecla fica presa
  TEST_ASSERT_EQUAL_INT(1, f.countOf(input_event::kButton, kA, kReleased));
  TEST_ASSERT_EQUAL_INT(0, f.countOf(input_event::kButton, kA, kClick));
  TEST_ASSERT_EQUAL_INT(
      1, f.countOf(input_event::kSourceFault, 0, static_cast<uint32_t>(Status::IoError)));
  f.run(3000);
  TEST_ASSERT_EQUAL_INT(1, f.countOf(input_event::kSourceFault));  // avisa uma vez só
}

static void test_failed_source_is_retried_and_recovers() {
  Fixture f;
  FakeSource source;
  TEST_ASSERT_OK(f.input.addButtonSource(source));
  TEST_ASSERT_OK(f.input.init());
  source.readStatus = Status::HardwareAbsent;
  source.beginStatus = Status::HardwareAbsent;
  f.run(200);
  TEST_ASSERT_EQUAL_size_t(0, f.input.healthySources());
  const int begins = source.beginCalls;
  f.run(3000);  // tenta reiniciar a cada segundo, não a cada volta
  TEST_ASSERT_TRUE(source.beginCalls - begins >= 2 && source.beginCalls - begins <= 4);

  source.readStatus = Status::Ok;  // o módulo foi religado
  source.beginStatus = Status::Ok;
  source.mask = maskOf(Button::A);
  f.run(1200);
  TEST_ASSERT_EQUAL_size_t(1, f.input.healthySources());
  TEST_ASSERT_EQUAL_INT(1, f.countOf(input_event::kSourceRecovered, 0, 0));
  TEST_ASSERT_TRUE(f.input.isPressed(Button::A));
}

static void test_stick_events_carry_both_axes() {
  Fixture f;
  FakeSource source;
  MockAdcChannel x, y;
  x.raw = 2000;
  y.raw = 2000;
  StickProcessor stick(x, y);
  TEST_ASSERT_OK(stick.setCalibration({0, 2000, 4000}, {0, 2000, 4000}));
  TEST_ASSERT_OK(f.input.addButtonSource(source));
  f.input.setStick(Stick::Right, stick);
  TEST_ASSERT_OK(f.input.init());
  f.run(20);
  TEST_ASSERT_EQUAL_INT(0, f.countOf(input_event::kStick));  // parado no centro: sem eventos

  x.raw = 3200;
  y.raw = 800;  // -600 milésimos
  f.run(10);
  TEST_ASSERT_EQUAL_INT(1, f.countOf(input_event::kStick));
  const Event &event = f.events[f.count - 1];
  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(Stick::Right), event.a);
  const StickPosition position = unpackStick(event.b);
  TEST_ASSERT_EQUAL_INT16(585, position.x);
  TEST_ASSERT_EQUAL_INT16(-585, position.y);
  TEST_ASSERT_EQUAL_INT16(585, f.input.stick(Stick::Right).x);
  TEST_ASSERT_EQUAL_INT16(0, f.input.stick(Stick::Left).x);  // analógico não instalado
}

static void test_configuration_changes_the_timing() {
  Fixture f;
  FakeSource source;
  TEST_ASSERT_OK(f.input.addButtonSource(source));
  TEST_ASSERT_OK(f.config.setInt(config_key::kLongPressMs, 200));
  TEST_ASSERT_OK(f.input.init());
  source.mask = maskOf(Button::A);
  f.run(260);
  TEST_ASSERT_EQUAL_INT(
      1, f.countOf(input_event::kButton, kA, static_cast<uint32_t>(ButtonAction::LongPress)));
  TEST_ASSERT_STATUS(Status::OutOfRange, f.config.setInt(config_key::kDebounceMs, 0));
}

static void test_polls_at_a_fixed_period() {
  Fixture f;
  MockI2cBus bus;
  dbmo::sim::Mcp23017Sim sim;
  bus.attach(0x20, &sim);
  Mcp23017 chip(bus, 0x20);
  Mcp23017ButtonSource source(chip, Mcp23017ButtonSource::defaultMap());
  TEST_ASSERT_OK(f.input.addButtonSource(source));
  TEST_ASSERT_OK(f.input.init());
  const unsigned before = bus.writeReadCount;
  f.run(100);
  // Uma consulta (duas leituras de registrador) a cada 5 ms: 20 consultas em 100 ms.
  TEST_ASSERT_EQUAL_UINT(before + 40, bus.writeReadCount);
}

static void test_shutdown_releases_and_stops() {
  Fixture f;
  FakeSource source;
  TEST_ASSERT_OK(f.input.addButtonSource(source));
  TEST_ASSERT_OK(f.input.init());
  source.mask = maskOf(Button::A);
  f.run(100);
  TEST_ASSERT_OK(f.input.shutdown());
  f.bus.dispatch();
  TEST_ASSERT_EQUAL_INT(1, f.countOf(input_event::kButton, kA, kReleased));
  const int events = f.count;
  f.run(100);
  TEST_ASSERT_EQUAL_INT(events, f.count);
}

static void test_sources_cannot_be_added_after_init() {
  Fixture f;
  FakeSource first, late;
  TEST_ASSERT_OK(f.input.addButtonSource(first));
  TEST_ASSERT_OK(f.input.init());
  TEST_ASSERT_STATUS(Status::NotPermitted, f.input.addButtonSource(late));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_button_events_are_published_on_the_bus);
  RUN_TEST(test_combo_events_are_published);
  RUN_TEST(test_two_sources_are_merged);
  RUN_TEST(test_init_reports_absent_hardware_only_when_nothing_answers);
  RUN_TEST(test_failing_source_releases_buttons_and_is_reported_once);
  RUN_TEST(test_failed_source_is_retried_and_recovers);
  RUN_TEST(test_stick_events_carry_both_axes);
  RUN_TEST(test_configuration_changes_the_timing);
  RUN_TEST(test_polls_at_a_fixed_period);
  RUN_TEST(test_shutdown_releases_and_stops);
  RUN_TEST(test_sources_cannot_be_added_after_init);
  return UNITY_END();
}
