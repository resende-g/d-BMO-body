#include "dbmo/core/state_machine.h"
#include "dbmo_test.h"

namespace {

enum class Door { Closed, Open, Locked };
enum class Action { Open, Close, Lock, Unlock };
using Machine = dbmo::StateMachine<Door, Action>;

const Machine::Transition kTable[] = {
    {Door::Closed, Action::Open, Door::Open},
    {Door::Open, Action::Close, Door::Closed},
    {Door::Closed, Action::Lock, Door::Locked},
    {Door::Locked, Action::Unlock, Door::Closed},
};

struct Trace {
  int count = 0;
  Door from = Door::Closed;
  Door to = Door::Closed;
};

void observe(Door from, Door to, void *context) {
  auto *trace = static_cast<Trace *>(context);
  ++trace->count;
  trace->from = from;
  trace->to = to;
}

}  // namespace

void setUp() {}
void tearDown() {}

static void test_only_listed_transitions_are_accepted() {
  Machine machine(kTable, Door::Closed);
  TEST_ASSERT_TRUE(machine.canFire(Action::Open));
  TEST_ASSERT_FALSE(machine.canFire(Action::Unlock));

  TEST_ASSERT_TRUE(machine.fire(Action::Lock));
  TEST_ASSERT_TRUE(machine.state() == Door::Locked);
  // Trancada não abre: o estado não muda.
  TEST_ASSERT_FALSE(machine.fire(Action::Open));
  TEST_ASSERT_TRUE(machine.state() == Door::Locked);

  TEST_ASSERT_TRUE(machine.fire(Action::Unlock));
  TEST_ASSERT_TRUE(machine.fire(Action::Open));
  TEST_ASSERT_TRUE(machine.state() == Door::Open);
}

static void test_observer_sees_applied_transitions_only() {
  Machine machine(kTable, Door::Closed);
  Trace trace;
  machine.setObserver(observe, &trace);

  TEST_ASSERT_FALSE(machine.fire(Action::Close));
  TEST_ASSERT_EQUAL_INT(0, trace.count);

  TEST_ASSERT_TRUE(machine.fire(Action::Open));
  TEST_ASSERT_EQUAL_INT(1, trace.count);
  TEST_ASSERT_TRUE(trace.from == Door::Closed);
  TEST_ASSERT_TRUE(trace.to == Door::Open);
}

static void test_reset_forces_a_state_without_notifying() {
  Machine machine(kTable, Door::Closed);
  Trace trace;
  machine.setObserver(observe, &trace);
  machine.reset(Door::Locked);
  TEST_ASSERT_TRUE(machine.state() == Door::Locked);
  TEST_ASSERT_EQUAL_INT(0, trace.count);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_only_listed_transitions_are_accepted);
  RUN_TEST(test_observer_sees_applied_transitions_only);
  RUN_TEST(test_reset_forces_a_state_without_notifying);
  return UNITY_END();
}
