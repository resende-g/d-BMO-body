#include <initializer_list>

#include "dbmo/rosto/face_engine.h"
#include "dbmo_test.h"

using dbmo::Status;
using namespace dbmo::rosto;

void setUp() {}
void tearDown() {}

static void test_starts_idle_and_blinks_on_schedule() {
  FaceEngine engine;
  engine.reset(0);
  TEST_ASSERT_TRUE(engine.current() == Expression::Idle);
  TEST_ASSERT_EQUAL_UINT8(0, engine.frameIndex());
  const uint32_t before = engine.revision();

  engine.tick(2999);  // o primeiro quadro do IDLE dura 3000 ms
  TEST_ASSERT_EQUAL_UINT8(0, engine.frameIndex());
  TEST_ASSERT_EQUAL_UINT32(before, engine.revision());

  engine.tick(3000);
  TEST_ASSERT_EQUAL_UINT8(1, engine.frameIndex());  // piscada
  engine.tick(3149);
  TEST_ASSERT_EQUAL_UINT8(1, engine.frameIndex());
  engine.tick(3150);  // a piscada dura 150 ms
  TEST_ASSERT_EQUAL_UINT8(0, engine.frameIndex());
  TEST_ASSERT_EQUAL_UINT32(before + 2, engine.revision());
}

static void test_persistent_request_changes_the_base() {
  FaceEngine engine;
  engine.reset(0);
  TEST_ASSERT_OK(engine.request(Expression::Listening, 100));
  TEST_ASSERT_TRUE(engine.current() == Expression::Listening);
  TEST_ASSERT_TRUE(engine.base() == Expression::Listening);
  TEST_ASSERT_FALSE(engine.overlayActive());
  engine.tick(60000);
  TEST_ASSERT_TRUE(engine.current() == Expression::Listening);  // fica até ser trocada
}

static void test_transient_overlays_the_base_and_returns() {
  FaceEngine engine;
  engine.reset(0);
  TEST_ASSERT_OK(engine.request(Expression::Thinking, 0));
  TEST_ASSERT_OK(engine.request(Expression::Success, 1000));  // padrão: 1500 ms
  TEST_ASSERT_TRUE(engine.current() == Expression::Success);
  TEST_ASSERT_TRUE(engine.base() == Expression::Thinking);
  TEST_ASSERT_TRUE(engine.overlayActive());

  engine.tick(2499);
  TEST_ASSERT_TRUE(engine.current() == Expression::Success);
  engine.tick(2500);
  TEST_ASSERT_TRUE(engine.current() == Expression::Thinking);
  TEST_ASSERT_FALSE(engine.overlayActive());
  TEST_ASSERT_EQUAL_UINT8(0, engine.frameIndex());  // a base recomeça do primeiro quadro
}

static void test_explicit_hold_overrides_the_default() {
  FaceEngine engine;
  engine.reset(0);
  TEST_ASSERT_OK(engine.request(Expression::Happy, 0, 300));
  engine.tick(299);
  TEST_ASSERT_TRUE(engine.current() == Expression::Happy);
  engine.tick(300);
  TEST_ASSERT_TRUE(engine.current() == Expression::Idle);

  // Uma expressão persistente por padrão pode ser pedida como transitória.
  TEST_ASSERT_OK(engine.request(Expression::Scanning, 1000, 500));
  TEST_ASSERT_TRUE(engine.overlayActive());
  TEST_ASSERT_TRUE(engine.base() == Expression::Idle);
  engine.tick(1500);
  TEST_ASSERT_TRUE(engine.current() == Expression::Idle);
}

static void test_higher_priority_is_not_hidden_by_lower() {
  FaceEngine engine;
  engine.reset(0);
  TEST_ASSERT_OK(engine.request(Expression::Error, 0));  // prioridade 4, 4000 ms
  TEST_ASSERT_STATUS(Status::NotPermitted, engine.request(Expression::Happy, 100));
  TEST_ASSERT_STATUS(Status::NotPermitted, engine.request(Expression::Success, 100));
  TEST_ASSERT_TRUE(engine.current() == Expression::Error);

  // Mudar a base por baixo é aceito e aparece quando o erro sair da tela.
  TEST_ASSERT_OK(engine.request(Expression::Offline, 200));
  TEST_ASSERT_TRUE(engine.current() == Expression::Error);
  engine.tick(4000);
  TEST_ASSERT_TRUE(engine.current() == Expression::Offline);
}

static void test_equal_or_higher_priority_replaces_the_overlay() {
  FaceEngine engine;
  engine.reset(0);
  TEST_ASSERT_OK(engine.request(Expression::Success, 0));    // prioridade 2
  TEST_ASSERT_OK(engine.request(Expression::Warning, 100));  // prioridade 3
  TEST_ASSERT_TRUE(engine.current() == Expression::Warning);
  TEST_ASSERT_OK(engine.request(Expression::Alert, 200));  // mesma prioridade: troca
  TEST_ASSERT_TRUE(engine.current() == Expression::Alert);
  // O tempo conta a partir do último pedido.
  engine.tick(3199);
  TEST_ASSERT_TRUE(engine.current() == Expression::Alert);
  engine.tick(3200);
  TEST_ASSERT_TRUE(engine.current() == Expression::Idle);
}

static void test_hold_forever_stays_until_cleared() {
  FaceEngine engine;
  engine.reset(0);
  TEST_ASSERT_OK(engine.request(Expression::Error, 0, FaceEngine::kHoldForever));
  engine.tick(10000000);
  TEST_ASSERT_TRUE(engine.current() == Expression::Error);
  engine.clearOverlay(10000001);
  TEST_ASSERT_TRUE(engine.current() == Expression::Idle);
  engine.clearOverlay(10000002);  // sem transitória: nada acontece
  TEST_ASSERT_TRUE(engine.current() == Expression::Idle);
}

static void test_repeating_the_same_request_does_not_restart_the_animation() {
  FaceEngine engine;
  engine.reset(0);
  TEST_ASSERT_OK(engine.request(Expression::Scanning, 0));  // 4 quadros de 300 ms
  engine.tick(300);
  TEST_ASSERT_EQUAL_UINT8(1, engine.frameIndex());
  const uint32_t revision = engine.revision();
  TEST_ASSERT_OK(engine.request(Expression::Scanning, 310));
  TEST_ASSERT_EQUAL_UINT8(1, engine.frameIndex());
  TEST_ASSERT_EQUAL_UINT32(revision, engine.revision());
}

static void test_play_once_stops_on_the_last_frame() {
  FaceEngine engine;
  engine.reset(0);
  ExpressionPolicy once = engine.policy(Expression::Thinking);
  once.loop = false;
  TEST_ASSERT_OK(engine.setPolicy(Expression::Thinking, once));
  TEST_ASSERT_OK(engine.request(Expression::Thinking, 0));  // 3 quadros de 400 ms
  engine.tick(400);
  engine.tick(800);
  TEST_ASSERT_EQUAL_UINT8(2, engine.frameIndex());
  engine.tick(5000);
  TEST_ASSERT_EQUAL_UINT8(2, engine.frameIndex());

  TEST_ASSERT_STATUS(Status::InvalidArgument, engine.setPolicy(static_cast<Expression>(99), once));
}

static void test_policy_can_be_reconfigured_at_run_time() {
  FaceEngine engine;
  engine.reset(0);
  TEST_ASSERT_OK(engine.setPolicy(Expression::Success, {2, 200, true}));
  TEST_ASSERT_OK(engine.request(Expression::Success, 0));
  engine.tick(200);
  TEST_ASSERT_TRUE(engine.current() == Expression::Idle);
}

static void test_late_tick_does_not_loop_forever_nor_replay_the_backlog() {
  FaceEngine engine;
  engine.reset(0);
  TEST_ASSERT_OK(engine.request(Expression::Speaking, 0));  // 3 quadros de 180 ms
  const uint32_t before = engine.revision();
  engine.tick(1000000);  // o laço principal ficou parado muito tempo
  // No máximo uma volta completa da animação é percorrida em um tick.
  TEST_ASSERT_TRUE(engine.revision() - before <= frameCount(Expression::Speaking));
  const uint8_t index = engine.frameIndex();
  engine.tick(1000179);
  TEST_ASSERT_EQUAL_UINT8(index, engine.frameIndex());
  engine.tick(1000180);
  TEST_ASSERT_NOT_EQUAL(index, engine.frameIndex());
}

static void test_timing_survives_clock_rollover() {
  FaceEngine engine;
  const uint32_t start = 0xFFFFFFFFu - 99;  // 100 ms antes de o relógio estourar
  engine.reset(start);
  TEST_ASSERT_OK(engine.request(Expression::Success, start, 500));
  engine.tick(start + 499);  // já depois do estouro
  TEST_ASSERT_TRUE(engine.current() == Expression::Success);
  engine.tick(start + 500);
  TEST_ASSERT_TRUE(engine.current() == Expression::Idle);

  // E a troca de quadro também: a piscada do IDLE, 3000 ms depois.
  engine.tick(start + 500 + 2999);
  TEST_ASSERT_EQUAL_UINT8(0, engine.frameIndex());
  engine.tick(start + 500 + 3000);
  TEST_ASSERT_EQUAL_UINT8(1, engine.frameIndex());
}

static void test_invalid_expression_is_rejected() {
  FaceEngine engine;
  engine.reset(0);
  TEST_ASSERT_STATUS(Status::InvalidArgument, engine.request(static_cast<Expression>(17), 0));
  TEST_ASSERT_TRUE(engine.current() == Expression::Idle);
}

static void test_emotions_are_transient_by_default() {
  // HAPPY, SAD e ANGRY só aparecem a pedido e saem sozinhos: não há humor permanente.
  FaceEngine engine;
  engine.reset(0);
  for (Expression emotion : {Expression::Happy, Expression::Sad, Expression::Angry}) {
    TEST_ASSERT_TRUE(engine.policy(emotion).defaultHoldMs > 0);
    TEST_ASSERT_OK(engine.request(emotion, 0));
    TEST_ASSERT_TRUE(engine.base() == Expression::Idle);
  }
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_starts_idle_and_blinks_on_schedule);
  RUN_TEST(test_persistent_request_changes_the_base);
  RUN_TEST(test_transient_overlays_the_base_and_returns);
  RUN_TEST(test_explicit_hold_overrides_the_default);
  RUN_TEST(test_higher_priority_is_not_hidden_by_lower);
  RUN_TEST(test_equal_or_higher_priority_replaces_the_overlay);
  RUN_TEST(test_hold_forever_stays_until_cleared);
  RUN_TEST(test_repeating_the_same_request_does_not_restart_the_animation);
  RUN_TEST(test_play_once_stops_on_the_last_frame);
  RUN_TEST(test_policy_can_be_reconfigured_at_run_time);
  RUN_TEST(test_late_tick_does_not_loop_forever_nor_replay_the_backlog);
  RUN_TEST(test_timing_survives_clock_rollover);
  RUN_TEST(test_invalid_expression_is_rejected);
  RUN_TEST(test_emotions_are_transient_by_default);
  return UNITY_END();
}
