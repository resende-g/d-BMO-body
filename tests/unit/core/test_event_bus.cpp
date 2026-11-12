#include "dbmo/core/event_bus.h"
#include "dbmo/mock/mock_hal.h"
#include "dbmo_test.h"

using dbmo::Event;
using dbmo::EventBus;
using dbmo::EventDomain;
using dbmo::EventId;
using dbmo::makeEventId;
using dbmo::Status;

namespace {

constexpr EventId kButton = makeEventId(EventDomain::Input, 1);
constexpr EventId kStick = makeEventId(EventDomain::Input, 2);
constexpr EventId kTag = makeEventId(EventDomain::Nfc, 1);

struct Recorder {
  int calls = 0;
  Event last;
};

void record(const Event &event, void *context) {
  auto *recorder = static_cast<Recorder *>(context);
  ++recorder->calls;
  recorder->last = event;
}

}  // namespace

void setUp() {}
void tearDown() {}

static void test_event_id_packs_domain_and_code() {
  TEST_ASSERT_EQUAL_HEX16(0x0201, kButton);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(EventDomain::Input),
                        static_cast<int>(dbmo::domainOf(kButton)));
  TEST_ASSERT_EQUAL_UINT8(1, dbmo::codeOf(kButton));
}

static void test_publish_is_deferred_until_dispatch() {
  EventBus bus;
  Recorder recorder;
  TEST_ASSERT_OK(bus.subscribe(kButton, record, &recorder));

  TEST_ASSERT_OK(bus.publish(kButton, 7, 9));
  TEST_ASSERT_EQUAL_INT(0, recorder.calls);
  TEST_ASSERT_EQUAL_size_t(1, bus.pending());

  TEST_ASSERT_EQUAL_size_t(1, bus.dispatch());
  TEST_ASSERT_EQUAL_INT(1, recorder.calls);
  TEST_ASSERT_EQUAL_UINT32(7, recorder.last.a);
  TEST_ASSERT_EQUAL_UINT32(9, recorder.last.b);
  TEST_ASSERT_EQUAL_size_t(0, bus.pending());
}

static void test_filters_exact_domain_and_all() {
  EventBus bus;
  Recorder exact, domain, all;
  TEST_ASSERT_OK(bus.subscribe(kButton, record, &exact));
  TEST_ASSERT_OK(bus.subscribeDomain(EventDomain::Input, record, &domain));
  TEST_ASSERT_OK(bus.subscribeAll(record, &all));

  TEST_ASSERT_OK(bus.publish(kButton));
  TEST_ASSERT_OK(bus.publish(kStick));
  TEST_ASSERT_OK(bus.publish(kTag));
  TEST_ASSERT_EQUAL_size_t(3, bus.dispatch());

  TEST_ASSERT_EQUAL_INT(1, exact.calls);
  TEST_ASSERT_EQUAL_INT(2, domain.calls);
  TEST_ASSERT_EQUAL_INT(3, all.calls);
}

static void test_events_are_delivered_in_publication_order() {
  EventBus bus;
  struct Order {
    uint32_t seen[4];
    int count = 0;
  } order;
  TEST_ASSERT_OK(bus.subscribeAll(
      [](const Event &event, void *context) {
        auto *o = static_cast<Order *>(context);
        o->seen[o->count++] = event.a;
      },
      &order));
  for (uint32_t i = 1; i <= 4; ++i) TEST_ASSERT_OK(bus.publish(kButton, i));
  bus.dispatch();
  TEST_ASSERT_EQUAL_INT(4, order.count);
  for (uint32_t i = 0; i < 4; ++i) TEST_ASSERT_EQUAL_UINT32(i + 1, order.seen[i]);
}

static void test_full_queue_drops_and_counts() {
  EventBus bus;
  for (size_t i = 0; i < EventBus::kQueueCapacity; ++i) TEST_ASSERT_OK(bus.publish(kButton));
  TEST_ASSERT_STATUS(Status::NoSpace, bus.publish(kButton));
  TEST_ASSERT_STATUS(Status::NoSpace, bus.publish(kButton));
  TEST_ASSERT_EQUAL_UINT32(2, bus.droppedCount());
  TEST_ASSERT_EQUAL_size_t(EventBus::kQueueCapacity, bus.pending());
}

static void test_dispatch_respects_the_limit() {
  EventBus bus;
  Recorder recorder;
  TEST_ASSERT_OK(bus.subscribeAll(record, &recorder));
  for (int i = 0; i < 5; ++i) TEST_ASSERT_OK(bus.publish(kButton));
  TEST_ASSERT_EQUAL_size_t(2, bus.dispatch(2));
  TEST_ASSERT_EQUAL_size_t(3, bus.pending());
  TEST_ASSERT_EQUAL_size_t(3, bus.dispatch());
}

static void test_subscription_errors_and_unsubscribe() {
  EventBus bus;
  Recorder recorder;
  TEST_ASSERT_STATUS(Status::InvalidArgument, bus.subscribe(kButton, nullptr, nullptr));
  TEST_ASSERT_OK(bus.subscribe(kButton, record, &recorder));
  TEST_ASSERT_STATUS(Status::AlreadyInitialized, bus.subscribe(kButton, record, &recorder));
  TEST_ASSERT_OK(bus.subscribe(kStick, record, &recorder));

  TEST_ASSERT_OK(bus.unsubscribe(record, &recorder));
  TEST_ASSERT_EQUAL_size_t(0, bus.subscriberCount());
  TEST_ASSERT_STATUS(Status::NotFound, bus.unsubscribe(record, &recorder));

  TEST_ASSERT_OK(bus.publish(kButton));
  bus.dispatch();
  TEST_ASSERT_EQUAL_INT(0, recorder.calls);
}

static void test_subscriber_table_limit() {
  EventBus bus;
  Recorder recorders[EventBus::kMaxSubscribers + 1];
  for (size_t i = 0; i < EventBus::kMaxSubscribers; ++i)
    TEST_ASSERT_OK(bus.subscribeAll(record, &recorders[i]));
  TEST_ASSERT_STATUS(Status::NoSpace,
                     bus.subscribeAll(record, &recorders[EventBus::kMaxSubscribers]));
}

static void test_handler_may_publish_and_chain_is_bounded() {
  EventBus bus;
  struct Chain {
    EventBus *bus;
    int calls = 0;
  } chain{&bus};
  // Cada entrega publica outro evento: sem limite, dispatch() nunca terminaria.
  TEST_ASSERT_OK(bus.subscribeAll(
      [](const Event &, void *context) {
        auto *c = static_cast<Chain *>(context);
        ++c->calls;
        (void)c->bus->publish(kButton);
      },
      &chain));
  TEST_ASSERT_OK(bus.publish(kButton));
  TEST_ASSERT_EQUAL_size_t(5, bus.dispatch(5));
  TEST_ASSERT_EQUAL_INT(5, chain.calls);
  TEST_ASSERT_EQUAL_size_t(1, bus.pending());
}

namespace {
struct Once {
  EventBus *bus;
  int calls = 0;
};

// Remove a própria inscrição no meio da entrega.
void removeItself(const Event &, void *context) {
  auto *once = static_cast<Once *>(context);
  ++once->calls;
  (void)once->bus->unsubscribe(removeItself, once);
}
}  // namespace

static void test_handler_may_unsubscribe_itself_during_dispatch() {
  EventBus bus;
  Once once{&bus};
  Recorder other;
  TEST_ASSERT_OK(bus.subscribeAll(removeItself, &once));
  TEST_ASSERT_OK(bus.subscribeAll(record, &other));

  TEST_ASSERT_OK(bus.publish(kButton));
  TEST_ASSERT_OK(bus.publish(kButton));
  bus.dispatch();

  // O handler saiu depois da primeira entrega; o outro inscrito não perdeu nenhum evento.
  TEST_ASSERT_EQUAL_INT(1, once.calls);
  TEST_ASSERT_EQUAL_INT(2, other.calls);
  TEST_ASSERT_EQUAL_size_t(1, bus.subscriberCount());
}

namespace {
struct LateJoin {
  EventBus *bus;
  Recorder *newcomer;
  int calls = 0;
};

// Inscreve outro handler enquanto um evento está sendo entregue.
void subscribeNewcomer(const Event &, void *context) {
  auto *join = static_cast<LateJoin *>(context);
  ++join->calls;
  (void)join->bus->subscribeAll(record, join->newcomer);
}

struct Reentry {
  EventBus *bus;
  size_t innerDelivered = 99;
};

void dispatchFromHandler(const Event &, void *context) {
  auto *reentry = static_cast<Reentry *>(context);
  reentry->innerDelivered = reentry->bus->dispatch();
}
}  // namespace

static void test_subscriber_added_during_dispatch_gets_only_later_events() {
  EventBus bus;
  Recorder newcomer;
  LateJoin join{&bus, &newcomer};
  TEST_ASSERT_OK(bus.subscribeAll(subscribeNewcomer, &join));

  TEST_ASSERT_OK(bus.publish(kButton));
  bus.dispatch();
  // O evento em entrega não chega a quem acabou de entrar.
  TEST_ASSERT_EQUAL_INT(0, newcomer.calls);

  TEST_ASSERT_OK(bus.publish(kButton));
  bus.dispatch();
  TEST_ASSERT_EQUAL_INT(1, newcomer.calls);
  TEST_ASSERT_EQUAL_size_t(2, bus.subscriberCount());  // a segunda inscrição era duplicata
}

static void test_dispatch_is_not_reentrant() {
  EventBus bus;
  Reentry reentry{&bus};
  TEST_ASSERT_OK(bus.subscribeAll(dispatchFromHandler, &reentry));
  TEST_ASSERT_OK(bus.publish(kButton));
  TEST_ASSERT_OK(bus.publish(kButton));
  TEST_ASSERT_EQUAL_size_t(2, bus.dispatch());
  TEST_ASSERT_EQUAL_size_t(0, reentry.innerDelivered);
}

static void test_unsubscribe_rejects_null_handler() {
  EventBus bus;
  TEST_ASSERT_STATUS(Status::InvalidArgument, bus.unsubscribe(nullptr, nullptr));
}

static void test_clock_stamps_and_lock_guards_the_queue() {
  dbmo::mock::MockClock clock;
  dbmo::mock::CountingCriticalSection lock;
  EventBus bus(&clock, &lock);
  Recorder recorder;
  TEST_ASSERT_OK(bus.subscribeAll(record, &recorder));

  clock.setMs(5000);
  TEST_ASSERT_OK(bus.publish(kButton));
  bus.dispatch();
  TEST_ASSERT_EQUAL_UINT32(5000, recorder.last.timestampMs);
  TEST_ASSERT_TRUE(lock.enterCount >= 2);  // publish + pop
  TEST_ASSERT_EQUAL_INT(0, lock.depth);

  // Um carimbo já preenchido por quem publicou é preservado.
  Event stamped(kButton);
  stamped.timestampMs = 42;
  TEST_ASSERT_OK(bus.publish(stamped));
  bus.dispatch();
  TEST_ASSERT_EQUAL_UINT32(42, recorder.last.timestampMs);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_event_id_packs_domain_and_code);
  RUN_TEST(test_publish_is_deferred_until_dispatch);
  RUN_TEST(test_filters_exact_domain_and_all);
  RUN_TEST(test_events_are_delivered_in_publication_order);
  RUN_TEST(test_full_queue_drops_and_counts);
  RUN_TEST(test_dispatch_respects_the_limit);
  RUN_TEST(test_subscription_errors_and_unsubscribe);
  RUN_TEST(test_subscriber_table_limit);
  RUN_TEST(test_handler_may_publish_and_chain_is_bounded);
  RUN_TEST(test_handler_may_unsubscribe_itself_during_dispatch);
  RUN_TEST(test_subscriber_added_during_dispatch_gets_only_later_events);
  RUN_TEST(test_dispatch_is_not_reentrant);
  RUN_TEST(test_unsubscribe_rejects_null_handler);
  RUN_TEST(test_clock_stamps_and_lock_guards_the_queue);
  return UNITY_END();
}
