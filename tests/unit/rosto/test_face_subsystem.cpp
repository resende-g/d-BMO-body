#include "dbmo/display/framebuffer_display.h"
#include "dbmo/mock/mock_hal.h"
#include "dbmo/rosto/face_subsystem.h"
#include "dbmo_test.h"

using dbmo::Event;
using dbmo::EventBus;
using dbmo::EventDomain;
using dbmo::Status;
using dbmo::display::Color;
using dbmo::display::FramebufferDisplay;
using dbmo::mock::MockClock;
using namespace dbmo::rosto;
namespace palette = dbmo::display::palette;

namespace {
Color gPixels[320 * 240];

constexpr dbmo::EventId kTagRead = dbmo::makeEventId(EventDomain::Nfc, 1);
constexpr dbmo::EventId kRadioError = dbmo::makeEventId(EventDomain::Radio, 3);

struct Fixture {
  FramebufferDisplay panel{gPixels, 320, 240};
  MockClock clock;
  EventBus bus{&clock};
  FaceSubsystem face{panel, bus, clock};
  int changes = 0;
  Event lastChange;

  Fixture() { TEST_ASSERT_OK(bus.subscribe(face_event::kChanged, &Fixture::onChange, this)); }
  static void onChange(const Event &event, void *context) {
    auto *self = static_cast<Fixture *>(context);
    ++self->changes;
    self->lastChange = event;
  }
  // Uma volta do laço principal.
  void step(uint32_t advanceMs = 0) {
    clock.advanceMs(advanceMs);
    bus.dispatch();
    face.tick(clock.nowMs());
  }
};
}  // namespace

void setUp() {
  for (Color &pixel : gPixels) pixel = 0xFFFF;
}
void tearDown() {}

static void test_init_draws_idle() {
  Fixture f;
  TEST_ASSERT_OK(f.face.init());
  TEST_ASSERT_TRUE(f.face.engine().current() == Expression::Idle);
  TEST_ASSERT_EQUAL_HEX16(palette::kSignalRed, f.panel.pixel(80, 60));  // olho esquerdo
  TEST_ASSERT_EQUAL_HEX16(palette::kBlack, f.panel.pixel(0, 0));
}

static void test_request_event_changes_the_face_asynchronously() {
  Fixture f;
  TEST_ASSERT_OK(f.face.init());
  TEST_ASSERT_OK(f.bus.publish(face_event::kRequest, static_cast<uint32_t>(Expression::Error)));
  // Publicar não muda nada: a troca acontece quando o laço principal entrega o evento.
  TEST_ASSERT_TRUE(f.face.engine().current() == Expression::Idle);

  f.step();
  TEST_ASSERT_TRUE(f.face.engine().current() == Expression::Error);
  // Olhos em X do ERROR: bloco (3,3) aceso -> pixel (60, 60).
  TEST_ASSERT_EQUAL_HEX16(palette::kSignalRed, f.panel.pixel(60, 60));

  f.step();  // entrega o aviso de mudança
  TEST_ASSERT_EQUAL_INT(1, f.changes);
  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(Expression::Error), f.lastChange.a);
  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(Expression::Idle), f.lastChange.b);
}

static void test_request_event_carries_the_hold_time() {
  Fixture f;
  TEST_ASSERT_OK(f.face.init());
  TEST_ASSERT_OK(
      f.bus.publish(face_event::kRequest, static_cast<uint32_t>(Expression::Happy), 300));
  f.step();
  TEST_ASSERT_TRUE(f.face.engine().current() == Expression::Happy);
  f.step(299);
  TEST_ASSERT_TRUE(f.face.engine().current() == Expression::Happy);
  f.step(1);
  TEST_ASSERT_TRUE(f.face.engine().current() == Expression::Idle);
}

static void test_invalid_request_is_ignored() {
  Fixture f;
  TEST_ASSERT_OK(f.face.init());
  TEST_ASSERT_OK(f.bus.publish(face_event::kRequest, 99));
  f.step();
  TEST_ASSERT_TRUE(f.face.engine().current() == Expression::Idle);
}

static void test_bindings_map_other_modules_events_to_expressions() {
  Fixture f;
  TEST_ASSERT_OK(f.face.bind(kTagRead, Expression::Success, 500));
  TEST_ASSERT_OK(f.face.bind(kRadioError, Expression::Error));
  TEST_ASSERT_OK(f.face.init());

  TEST_ASSERT_OK(f.bus.publish(kTagRead, 0xDEADBEEF));
  f.step();
  TEST_ASSERT_TRUE(f.face.engine().current() == Expression::Success);
  f.step(500);
  TEST_ASSERT_TRUE(f.face.engine().current() == Expression::Idle);

  TEST_ASSERT_OK(f.bus.publish(kRadioError));
  f.step();
  TEST_ASSERT_TRUE(f.face.engine().current() == Expression::Error);

  // Um evento sem ligação não muda o rosto.
  f.face.engine().clearOverlay(f.clock.nowMs());
  TEST_ASSERT_OK(f.bus.publish(dbmo::makeEventId(EventDomain::Audio, 9)));
  f.step();
  TEST_ASSERT_TRUE(f.face.engine().current() == Expression::Idle);
}

static void test_binding_rules() {
  Fixture f;
  // Ligar eventos do próprio rosto criaria um laço.
  TEST_ASSERT_STATUS(Status::InvalidArgument, f.face.bind(face_event::kChanged, Expression::Happy));
  TEST_ASSERT_STATUS(Status::InvalidArgument, f.face.bind(kTagRead, static_cast<Expression>(40)));

  // Ligar de novo o mesmo evento substitui a ligação, sem ocupar outra posição.
  TEST_ASSERT_OK(f.face.bind(kTagRead, Expression::Success));
  TEST_ASSERT_OK(f.face.bind(kTagRead, Expression::Scanning));
  for (uint8_t i = 1; i < FaceSubsystem::kMaxBindings; ++i)
    TEST_ASSERT_OK(f.face.bind(dbmo::makeEventId(EventDomain::App, i), Expression::Curious));
  TEST_ASSERT_STATUS(Status::NoSpace,
                     f.face.bind(dbmo::makeEventId(EventDomain::App, 200), Expression::Curious));

  TEST_ASSERT_OK(f.face.init());
  TEST_ASSERT_OK(f.bus.publish(kTagRead));
  f.step();
  TEST_ASSERT_TRUE(f.face.engine().current() == Expression::Scanning);
}

static void test_animation_is_redrawn_as_time_passes() {
  Fixture f;
  TEST_ASSERT_OK(f.face.init());
  const unsigned calls = f.panel.fillRectCalls;
  f.step(2999);
  TEST_ASSERT_EQUAL_UINT(calls, f.panel.fillRectCalls);  // nada mudou: nada é redesenhado
  f.step(1);                                             // piscada do IDLE
  TEST_ASSERT_EQUAL_UINT(calls + 8, f.panel.fillRectCalls);
  TEST_ASSERT_EQUAL_HEX16(palette::kBlack, f.panel.pixel(80, 60));
}

static void test_display_failure_at_init_is_reported() {
  Fixture f;
  f.panel.failNext(Status::IoError);
  TEST_ASSERT_STATUS(Status::IoError, f.face.init());
  TEST_ASSERT_OK(f.face.init());  // segunda tentativa funciona e não duplica a inscrição
  TEST_ASSERT_EQUAL_size_t(2, f.bus.subscriberCount());
}

static void test_display_failure_during_tick_is_retried_after_a_pause() {
  Fixture f;
  TEST_ASSERT_OK(f.face.init());
  TEST_ASSERT_OK(f.bus.publish(face_event::kRequest, static_cast<uint32_t>(Expression::Error)));
  f.panel.failNext(Status::Timeout);
  f.step();  // o desenho falha
  const unsigned calls = f.panel.fillRectCalls;
  f.step(100);  // ainda dentro da pausa: não tenta
  TEST_ASSERT_EQUAL_UINT(calls, f.panel.fillRectCalls);
  f.step(FaceSubsystem::kRedrawRetryMs);
  TEST_ASSERT_EQUAL_HEX16(palette::kSignalRed, f.panel.pixel(60, 60));  // agora desenhou
}

static void test_invalidate_repaints_everything() {
  Fixture f;
  TEST_ASSERT_OK(f.face.init());
  for (Color &pixel : gPixels) pixel = 0xFFFF;
  f.face.invalidate();
  f.step();
  TEST_ASSERT_EQUAL_HEX16(palette::kBlack, f.panel.pixel(0, 0));
  TEST_ASSERT_EQUAL_HEX16(palette::kSignalRed, f.panel.pixel(80, 60));
}

static void test_shutdown_stops_listening() {
  Fixture f;
  TEST_ASSERT_OK(f.face.init());
  TEST_ASSERT_OK(f.face.shutdown());
  TEST_ASSERT_EQUAL_size_t(1, f.bus.subscriberCount());  // só o observador do teste
  TEST_ASSERT_OK(f.bus.publish(face_event::kRequest, static_cast<uint32_t>(Expression::Error)));
  f.bus.dispatch();
  TEST_ASSERT_TRUE(f.face.engine().current() == Expression::Idle);
  TEST_ASSERT_OK(f.face.shutdown());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_init_draws_idle);
  RUN_TEST(test_request_event_changes_the_face_asynchronously);
  RUN_TEST(test_request_event_carries_the_hold_time);
  RUN_TEST(test_invalid_request_is_ignored);
  RUN_TEST(test_bindings_map_other_modules_events_to_expressions);
  RUN_TEST(test_binding_rules);
  RUN_TEST(test_animation_is_redrawn_as_time_passes);
  RUN_TEST(test_display_failure_at_init_is_reported);
  RUN_TEST(test_display_failure_during_tick_is_retried_after_a_pause);
  RUN_TEST(test_invalidate_repaints_everything);
  RUN_TEST(test_shutdown_stops_listening);
  return UNITY_END();
}
