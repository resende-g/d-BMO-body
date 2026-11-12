#include "dbmo/core/system.h"
#include "dbmo/mock/mock_hal.h"
#include "dbmo_test.h"

using dbmo::Event;
using dbmo::EventBus;
using dbmo::ISubsystem;
using dbmo::Status;
using dbmo::SubsystemOptions;
using dbmo::SubsystemState;
using dbmo::System;
using dbmo::SystemState;
using dbmo::mock::MockClock;

namespace {

// Ordem global das chamadas, para conferir a sequência de init/shutdown.
int gSequence = 0;

class FakeSubsystem final : public ISubsystem {
 public:
  explicit FakeSubsystem(const char *name) : name_(name) {}
  const char *name() const override { return name_; }
  Status init() override {
    ++initCalls;
    initOrder = ++gSequence;
    return initStatus;
  }
  void tick(uint32_t nowMs) override {
    ++tickCalls;
    lastTickMs = nowMs;
  }
  Status shutdown() override {
    ++shutdownCalls;
    shutdownOrder = ++gSequence;
    return shutdownStatus;
  }

  Status initStatus = Status::Ok;
  Status shutdownStatus = Status::Ok;
  int initCalls = 0;
  int tickCalls = 0;
  int shutdownCalls = 0;
  int initOrder = 0;
  int shutdownOrder = 0;
  uint32_t lastTickMs = 0;

 private:
  const char *name_;
};

struct EventLog {
  static constexpr int kMax = 32;
  Event events[kMax];
  int count = 0;

  int countOf(dbmo::EventId id) const {
    int matches = 0;
    for (int i = 0; i < count; ++i)
      if (events[i].id == id) ++matches;
    return matches;
  }
  const Event *lastOf(dbmo::EventId id) const {
    for (int i = count - 1; i >= 0; --i)
      if (events[i].id == id) return &events[i];
    return nullptr;
  }
};

void collect(const Event &event, void *context) {
  auto *log = static_cast<EventLog *>(context);
  if (log->count < EventLog::kMax) log->events[log->count++] = event;
}

}  // namespace

void setUp() { gSequence = 0; }
void tearDown() {}

static void test_boot_initialises_in_registration_order_and_runs() {
  MockClock clock;
  EventBus bus(&clock);
  System system(clock, bus);
  FakeSubsystem a("a"), b("b"), c("c");
  TEST_ASSERT_OK(system.add(a));
  TEST_ASSERT_OK(system.add(b));
  TEST_ASSERT_OK(system.add(c));

  TEST_ASSERT_TRUE(system.state() == SystemState::Off);
  TEST_ASSERT_OK(system.boot());
  TEST_ASSERT_TRUE(system.state() == SystemState::Running);
  TEST_ASSERT_TRUE(a.initOrder < b.initOrder && b.initOrder < c.initOrder);
  TEST_ASSERT_TRUE(system.info(1).state == SubsystemState::Ready);
  TEST_ASSERT_EQUAL_UINT8(1, system.info(1).initAttempts);
  TEST_ASSERT_STATUS(Status::AlreadyInitialized, system.boot());
}

static void test_absent_optional_hardware_degrades_but_does_not_stop_boot() {
  MockClock clock;
  EventBus bus(&clock);
  System system(clock, bus);
  FakeSubsystem display("display"), nfc("nfc"), audio("audio");
  nfc.initStatus = Status::HardwareAbsent;
  TEST_ASSERT_OK(system.add(display));
  TEST_ASSERT_OK(system.add(nfc));
  TEST_ASSERT_OK(system.add(audio));

  TEST_ASSERT_OK(system.boot());
  TEST_ASSERT_TRUE(system.state() == SystemState::Degraded);
  TEST_ASSERT_TRUE(system.find("nfc")->state == SubsystemState::Absent);
  TEST_ASSERT_STATUS(Status::HardwareAbsent, system.find("nfc")->lastStatus);
  // Os que vêm depois do ausente continuam sendo iniciados.
  TEST_ASSERT_EQUAL_INT(1, audio.initCalls);
  // Hardware ausente não é falha: nada no histórico de falhas.
  TEST_ASSERT_EQUAL_size_t(0, system.faultCount());
}

static void test_failed_optional_subsystem_is_recorded_as_fault() {
  MockClock clock;
  clock.setMs(777);
  EventBus bus(&clock);
  System system(clock, bus);
  FakeSubsystem radio("radio");
  radio.initStatus = Status::IoError;
  TEST_ASSERT_OK(system.add(radio));

  TEST_ASSERT_OK(system.boot());
  TEST_ASSERT_TRUE(system.state() == SystemState::Degraded);
  TEST_ASSERT_TRUE(system.info(0).state == SubsystemState::Failed);
  TEST_ASSERT_EQUAL_size_t(1, system.faultCount());
  TEST_ASSERT_STATUS(Status::IoError, system.fault(0).status);
  TEST_ASSERT_EQUAL_UINT32(777, system.fault(0).timestampMs);
}

static void test_required_subsystem_failure_stops_boot_in_fault() {
  MockClock clock;
  EventBus bus(&clock);
  System system(clock, bus);
  FakeSubsystem storage("storage"), later("later");
  storage.initStatus = Status::Corrupt;
  SubsystemOptions required;
  required.required = true;
  TEST_ASSERT_OK(system.add(storage, required));
  TEST_ASSERT_OK(system.add(later));

  TEST_ASSERT_STATUS(Status::Corrupt, system.boot());
  TEST_ASSERT_TRUE(system.state() == SystemState::Fault);
  TEST_ASSERT_EQUAL_INT(0, later.initCalls);
  TEST_ASSERT_TRUE(system.info(1).state == SubsystemState::Uninitialized);
}

static void test_dependency_gates_initialisation() {
  MockClock clock;
  EventBus bus(&clock);
  System system(clock, bus);
  FakeSubsystem i2c("i2c"), nfc("nfc"), other("other");
  i2c.initStatus = Status::HardwareAbsent;
  SubsystemOptions needsI2c;
  needsI2c.dependsOn = &i2c;

  // A dependência precisa estar registrada antes.
  TEST_ASSERT_STATUS(Status::InvalidArgument, system.add(nfc, needsI2c));
  TEST_ASSERT_OK(system.add(i2c));
  TEST_ASSERT_OK(system.add(nfc, needsI2c));
  TEST_ASSERT_OK(system.add(other));

  TEST_ASSERT_OK(system.boot());
  TEST_ASSERT_EQUAL_INT(0, nfc.initCalls);
  TEST_ASSERT_TRUE(system.find("nfc")->state == SubsystemState::Absent);
  TEST_ASSERT_STATUS(Status::NotInitialized, system.find("nfc")->lastStatus);
  TEST_ASSERT_EQUAL_INT(1, other.initCalls);
}

static void test_tick_dispatches_events_and_ticks_only_operational_subsystems() {
  MockClock clock;
  EventBus bus(&clock);
  System system(clock, bus);
  FakeSubsystem good("good"), absent("absent");
  absent.initStatus = Status::HardwareAbsent;
  TEST_ASSERT_OK(system.add(good));
  TEST_ASSERT_OK(system.add(absent));

  system.tick();  // antes do boot: nada acontece
  TEST_ASSERT_EQUAL_INT(0, good.tickCalls);

  TEST_ASSERT_OK(system.boot());
  EventLog log;
  TEST_ASSERT_OK(bus.subscribeAll(collect, &log));
  clock.setMs(1500);
  system.tick();
  TEST_ASSERT_EQUAL_INT(1, good.tickCalls);
  TEST_ASSERT_EQUAL_UINT32(1500, good.lastTickMs);
  TEST_ASSERT_EQUAL_INT(0, absent.tickCalls);
  // O boot publicou as mudanças de estado; o tick as entregou.
  TEST_ASSERT_TRUE(log.countOf(dbmo::system_event::kStateChanged) >= 2);
  TEST_ASSERT_EQUAL_size_t(0, bus.pending());
}

static void test_state_change_events_carry_old_and_new_state() {
  MockClock clock;
  EventBus bus(&clock);
  System system(clock, bus);
  FakeSubsystem a("a");
  TEST_ASSERT_OK(system.add(a));
  EventLog log;
  TEST_ASSERT_OK(bus.subscribeAll(collect, &log));

  TEST_ASSERT_OK(system.boot());
  bus.dispatch();
  const Event *last = log.lastOf(dbmo::system_event::kStateChanged);
  TEST_ASSERT_NOT_NULL(last);
  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(SystemState::Booting), last->a);
  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(SystemState::Running), last->b);

  const Event *subsystem = log.lastOf(dbmo::system_event::kSubsystemChanged);
  TEST_ASSERT_NOT_NULL(subsystem);
  TEST_ASSERT_EQUAL_UINT32(0, subsystem->a);
  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(SubsystemState::Ready), subsystem->b);
}

static void test_shutdown_runs_in_reverse_order_and_reports_first_error() {
  MockClock clock;
  EventBus bus(&clock);
  System system(clock, bus);
  FakeSubsystem a("a"), b("b"), c("c");
  b.shutdownStatus = Status::IoError;
  c.shutdownStatus = Status::Timeout;
  TEST_ASSERT_OK(system.add(a));
  TEST_ASSERT_OK(system.add(b));
  TEST_ASSERT_OK(system.add(c));

  TEST_ASSERT_STATUS(Status::NotInitialized, system.shutdown());  // ainda em Off
  TEST_ASSERT_OK(system.boot());
  // c é o primeiro a desligar, então o erro dele é o primeiro.
  TEST_ASSERT_STATUS(Status::Timeout, system.shutdown());
  TEST_ASSERT_TRUE(c.shutdownOrder < b.shutdownOrder && b.shutdownOrder < a.shutdownOrder);
  TEST_ASSERT_EQUAL_INT(1, a.shutdownCalls);  // todos são desligados, apesar dos erros
  TEST_ASSERT_TRUE(system.state() == SystemState::Off);
  TEST_ASSERT_TRUE(system.info(0).state == SubsystemState::Stopped);

  // Depois de desligado, pode iniciar de novo.
  TEST_ASSERT_OK(system.boot());
  TEST_ASSERT_EQUAL_INT(2, a.initCalls);
}

static void test_shutdown_skips_subsystems_that_never_started() {
  MockClock clock;
  EventBus bus(&clock);
  System system(clock, bus);
  FakeSubsystem absent("absent");
  absent.initStatus = Status::HardwareAbsent;
  TEST_ASSERT_OK(system.add(absent));
  TEST_ASSERT_OK(system.boot());
  TEST_ASSERT_OK(system.shutdown());
  TEST_ASSERT_EQUAL_INT(0, absent.shutdownCalls);
}

static void test_retry_brings_an_absent_subsystem_back() {
  MockClock clock;
  EventBus bus(&clock);
  System system(clock, bus);
  FakeSubsystem nfc("nfc"), stranger("stranger");
  nfc.initStatus = Status::HardwareAbsent;
  TEST_ASSERT_OK(system.add(nfc));

  TEST_ASSERT_STATUS(Status::NotPermitted, system.retry(nfc));  // sistema em Off
  TEST_ASSERT_OK(system.boot());
  TEST_ASSERT_TRUE(system.state() == SystemState::Degraded);
  TEST_ASSERT_STATUS(Status::NotFound, system.retry(stranger));

  // O módulo foi plugado na bancada.
  nfc.initStatus = Status::Ok;
  TEST_ASSERT_OK(system.retry(nfc));
  TEST_ASSERT_TRUE(system.info(0).state == SubsystemState::Ready);
  TEST_ASSERT_EQUAL_UINT8(2, system.info(0).initAttempts);
  TEST_ASSERT_TRUE(system.state() == SystemState::Running);
  TEST_ASSERT_STATUS(Status::AlreadyInitialized, system.retry(nfc));
}

static void test_runtime_fault_degrades_and_recovery_restores() {
  MockClock clock;
  EventBus bus(&clock);
  System system(clock, bus);
  FakeSubsystem storage("storage");
  TEST_ASSERT_OK(system.add(storage));
  TEST_ASSERT_OK(system.boot());
  EventLog log;
  TEST_ASSERT_OK(bus.subscribeAll(collect, &log));

  system.reportFault(storage, Status::IoError);
  TEST_ASSERT_TRUE(system.info(0).state == SubsystemState::Degraded);
  TEST_ASSERT_TRUE(system.state() == SystemState::Degraded);
  TEST_ASSERT_EQUAL_size_t(1, system.faultCount());
  bus.dispatch();
  const Event *fault = log.lastOf(dbmo::system_event::kFault);
  TEST_ASSERT_NOT_NULL(fault);
  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(Status::IoError), fault->b);

  // Degradado continua recebendo tick.
  const int before = storage.tickCalls;
  system.tick();
  TEST_ASSERT_EQUAL_INT(before + 1, storage.tickCalls);

  system.reportRecovered(storage);
  TEST_ASSERT_TRUE(system.info(0).state == SubsystemState::Ready);
  TEST_ASSERT_TRUE(system.state() == SystemState::Running);
}

static void test_losing_a_required_subsystem_is_fatal() {
  MockClock clock;
  EventBus bus(&clock);
  System system(clock, bus);
  FakeSubsystem display("display");
  SubsystemOptions required;
  required.required = true;
  TEST_ASSERT_OK(system.add(display, required));
  TEST_ASSERT_OK(system.boot());

  // Degradar um obrigatório não é fatal; perdê-lo é.
  system.reportFault(display, Status::Timeout, SubsystemState::Degraded);
  TEST_ASSERT_TRUE(system.state() == SystemState::Degraded);
  system.reportFault(display, Status::IoError, SubsystemState::Failed);
  TEST_ASSERT_TRUE(system.state() == SystemState::Fault);
  // Em Fault ainda é possível desligar com ordem.
  TEST_ASSERT_OK(system.shutdown());
  TEST_ASSERT_TRUE(system.state() == SystemState::Off);
}

static void test_low_power_round_trip_recomputes_health() {
  MockClock clock;
  EventBus bus(&clock);
  System system(clock, bus);
  FakeSubsystem a("a"), b("b");
  b.initStatus = Status::HardwareAbsent;
  TEST_ASSERT_OK(system.add(a));
  TEST_ASSERT_OK(system.add(b));

  TEST_ASSERT_STATUS(Status::NotPermitted, system.enterLowPower());  // em Off
  TEST_ASSERT_OK(system.boot());
  TEST_ASSERT_OK(system.enterLowPower());
  TEST_ASSERT_TRUE(system.state() == SystemState::LowPower);
  TEST_ASSERT_STATUS(Status::NotPermitted, system.enterLowPower());
  system.tick();
  TEST_ASSERT_EQUAL_INT(1, a.tickCalls);  // em baixo consumo os módulos seguem recebendo tick

  TEST_ASSERT_OK(system.exitLowPower());
  // b continua ausente: a volta termina em Degraded, não em Running.
  TEST_ASSERT_TRUE(system.state() == SystemState::Degraded);
}

static void test_registration_rules() {
  MockClock clock;
  EventBus bus(&clock);
  System system(clock, bus);
  FakeSubsystem first("first");
  TEST_ASSERT_OK(system.add(first));
  TEST_ASSERT_STATUS(Status::AlreadyInitialized, system.add(first));

  FakeSubsystem extra[System::kMaxSubsystems] = {
      FakeSubsystem("s"), FakeSubsystem("s"), FakeSubsystem("s"), FakeSubsystem("s"),
      FakeSubsystem("s"), FakeSubsystem("s"), FakeSubsystem("s"), FakeSubsystem("s"),
      FakeSubsystem("s"), FakeSubsystem("s"), FakeSubsystem("s"), FakeSubsystem("s"),
      FakeSubsystem("s"), FakeSubsystem("s"), FakeSubsystem("s"), FakeSubsystem("s")};
  for (size_t i = 0; i + 1 < System::kMaxSubsystems; ++i) TEST_ASSERT_OK(system.add(extra[i]));
  TEST_ASSERT_STATUS(Status::NoSpace, system.add(extra[System::kMaxSubsystems - 1]));

  TEST_ASSERT_OK(system.boot());
  FakeSubsystem late("late");
  TEST_ASSERT_STATUS(Status::NotPermitted, system.add(late));  // só com o sistema em Off
  TEST_ASSERT_NULL(system.find("nao-existe"));
  TEST_ASSERT_NULL(system.find(late));
}

static void test_fault_history_keeps_the_most_recent() {
  MockClock clock;
  EventBus bus(&clock);
  System system(clock, bus);
  FakeSubsystem a("a");
  TEST_ASSERT_OK(system.add(a));
  TEST_ASSERT_OK(system.boot());
  for (size_t i = 0; i < System::kFaultHistory + 3; ++i) {
    clock.setMs(static_cast<uint32_t>(i));
    system.reportFault(a, Status::Timeout);
    bus.dispatch();
  }
  TEST_ASSERT_EQUAL_size_t(System::kFaultHistory, system.faultCount());
  TEST_ASSERT_EQUAL_UINT32(3, system.fault(0).timestampMs);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_boot_initialises_in_registration_order_and_runs);
  RUN_TEST(test_absent_optional_hardware_degrades_but_does_not_stop_boot);
  RUN_TEST(test_failed_optional_subsystem_is_recorded_as_fault);
  RUN_TEST(test_required_subsystem_failure_stops_boot_in_fault);
  RUN_TEST(test_dependency_gates_initialisation);
  RUN_TEST(test_tick_dispatches_events_and_ticks_only_operational_subsystems);
  RUN_TEST(test_state_change_events_carry_old_and_new_state);
  RUN_TEST(test_shutdown_runs_in_reverse_order_and_reports_first_error);
  RUN_TEST(test_shutdown_skips_subsystems_that_never_started);
  RUN_TEST(test_retry_brings_an_absent_subsystem_back);
  RUN_TEST(test_runtime_fault_degrades_and_recovery_restores);
  RUN_TEST(test_losing_a_required_subsystem_is_fatal);
  RUN_TEST(test_low_power_round_trip_recomputes_health);
  RUN_TEST(test_registration_rules);
  RUN_TEST(test_fault_history_keeps_the_most_recent);
  return UNITY_END();
}
