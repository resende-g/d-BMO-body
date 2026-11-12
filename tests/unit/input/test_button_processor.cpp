#include "dbmo/input/button_processor.h"
#include "dbmo_test.h"

using dbmo::Status;
using namespace dbmo::input;

namespace {

struct Log {
  ButtonEvent events[96];
  int count = 0;
  uint8_t combos[32];
  int comboCount = 0;

  int countOf(Button button, ButtonAction action) const {
    int matches = 0;
    for (int i = 0; i < count; ++i)
      if (events[i].button == button && events[i].action == action) ++matches;
    return matches;
  }
};

void onButton(const ButtonEvent &event, void *context) {
  auto *log = static_cast<Log *>(context);
  if (log->count < 96) log->events[log->count++] = event;
}

void onCombo(uint8_t id, void *context) {
  auto *log = static_cast<Log *>(context);
  if (log->comboCount < 32) log->combos[log->comboCount++] = id;
}

struct Fixture {
  ButtonProcessor processor;
  Log log;
  uint32_t now = 0;

  Fixture() { processor.setSinks(onButton, onCombo, &log); }
  // Mantém a leitura bruta por durationMs, chamando update a cada milissegundo.
  void hold(ButtonMask raw, uint32_t durationMs) {
    for (uint32_t i = 0; i < durationMs; ++i) processor.update(raw, ++now);
  }
};

constexpr ButtonMask kA = maskOf(Button::A);
constexpr ButtonMask kB = maskOf(Button::B);
constexpr ButtonMask kUp = maskOf(Button::DpadUp);
constexpr ButtonMask kSelect = maskOf(Button::Select);

}  // namespace

void setUp() {}
void tearDown() {}

static void test_bounce_shorter_than_the_debounce_time_is_ignored() {
  Fixture f;
  // Repique: a leitura alterna a cada 5 ms durante 60 ms (o tempo estável exigido é 20).
  for (int i = 0; i < 12; ++i) f.hold((i % 2 == 0) ? kA : 0, 5);
  TEST_ASSERT_EQUAL_INT(0, f.log.count);

  f.hold(kA, 19);
  TEST_ASSERT_EQUAL_INT(0, f.log.count);  // ainda não completou 20 ms estável
  f.hold(kA, 2);
  TEST_ASSERT_EQUAL_INT(1, f.log.count);
  TEST_ASSERT_TRUE(f.log.events[0].action == ButtonAction::Pressed);
  TEST_ASSERT_TRUE(f.processor.isPressed(Button::A));
}

static void test_short_press_is_a_click() {
  Fixture f;
  f.hold(kA, 100);
  f.hold(0, 50);
  TEST_ASSERT_EQUAL_INT(3, f.log.count);
  TEST_ASSERT_TRUE(f.log.events[0].action == ButtonAction::Pressed);
  TEST_ASSERT_TRUE(f.log.events[1].action == ButtonAction::Released);
  TEST_ASSERT_TRUE(f.log.events[2].action == ButtonAction::Click);
  TEST_ASSERT_FALSE(f.processor.isPressed(Button::A));
}

static void test_long_press_fires_once_and_cancels_the_click() {
  Fixture f;
  f.hold(kA, 20 + 599);  // 20 ms de debounce + 599 ms segurado
  TEST_ASSERT_EQUAL_INT(0, f.log.countOf(Button::A, ButtonAction::LongPress));
  f.hold(kA, 2);
  TEST_ASSERT_EQUAL_INT(1, f.log.countOf(Button::A, ButtonAction::LongPress));
  f.hold(kA, 3000);
  TEST_ASSERT_EQUAL_INT(1, f.log.countOf(Button::A, ButtonAction::LongPress));
  f.hold(0, 50);
  TEST_ASSERT_EQUAL_INT(1, f.log.countOf(Button::A, ButtonAction::Released));
  TEST_ASSERT_EQUAL_INT(0, f.log.countOf(Button::A, ButtonAction::Click));
}

static void test_dpad_repeats_after_a_delay_and_never_long_presses() {
  Fixture f;
  f.hold(kUp, 20 + 399);
  TEST_ASSERT_EQUAL_INT(0, f.log.countOf(Button::DpadUp, ButtonAction::Repeat));
  f.hold(kUp, 2);  // 400 ms: primeira repetição
  TEST_ASSERT_EQUAL_INT(1, f.log.countOf(Button::DpadUp, ButtonAction::Repeat));
  f.hold(kUp, 120 * 5);  // mais cinco, a cada 120 ms
  TEST_ASSERT_EQUAL_INT(6, f.log.countOf(Button::DpadUp, ButtonAction::Repeat));
  TEST_ASSERT_EQUAL_INT(0, f.log.countOf(Button::DpadUp, ButtonAction::LongPress));
  f.hold(0, 50);
  // Quem repetiu ainda gera o clique ao soltar? Não: repetição não é toque longo, então
  // o clique final existe e cabe à aplicação ignorá-lo se quiser.
  TEST_ASSERT_EQUAL_INT(1, f.log.countOf(Button::DpadUp, ButtonAction::Click));
}

static void test_repeat_mask_and_timing_are_configurable() {
  Fixture f;
  ButtonTiming timing;
  timing.debounceMs = 5;
  timing.longPressMs = 200;
  timing.repeatDelayMs = 100;
  timing.repeatIntervalMs = 50;
  f.processor.setTiming(timing);
  f.processor.setRepeatMask(kA);  // agora quem repete é o A, não o D-pad

  f.hold(kA | kUp, 5 + 100 + 50 + 2);
  TEST_ASSERT_EQUAL_INT(2, f.log.countOf(Button::A, ButtonAction::Repeat));
  TEST_ASSERT_EQUAL_INT(0, f.log.countOf(Button::DpadUp, ButtonAction::Repeat));
  f.hold(kA | kUp, 60);
  TEST_ASSERT_EQUAL_INT(1, f.log.countOf(Button::DpadUp, ButtonAction::LongPress));
}

static void test_simultaneous_buttons_are_independent() {
  Fixture f;
  f.hold(kA | kB, 50);
  TEST_ASSERT_EQUAL_INT(1, f.log.countOf(Button::A, ButtonAction::Pressed));
  TEST_ASSERT_EQUAL_INT(1, f.log.countOf(Button::B, ButtonAction::Pressed));
  f.hold(kB, 50);  // solta só o A
  TEST_ASSERT_EQUAL_INT(1, f.log.countOf(Button::A, ButtonAction::Click));
  TEST_ASSERT_TRUE(f.processor.isPressed(Button::B));
  f.hold(0, 50);
  TEST_ASSERT_EQUAL_INT(1, f.log.countOf(Button::B, ButtonAction::Click));
}

static void test_combo_fires_once_and_suppresses_its_buttons() {
  Fixture f;
  TEST_ASSERT_OK(f.processor.addCombo({7, kSelect | kUp, 0, false}));
  f.hold(kSelect, 100);        // segura select
  f.hold(kSelect | kUp, 100);  // e aperta cima: combinação de volume
  TEST_ASSERT_EQUAL_INT(1, f.log.comboCount);
  TEST_ASSERT_EQUAL_UINT8(7, f.log.combos[0]);

  f.hold(kSelect | kUp, 2000);  // segurando: não repete (repeat = false) nem vira toque longo
  TEST_ASSERT_EQUAL_INT(1, f.log.comboCount);
  TEST_ASSERT_EQUAL_INT(0, f.log.countOf(Button::Select, ButtonAction::LongPress));
  TEST_ASSERT_EQUAL_INT(0, f.log.countOf(Button::DpadUp, ButtonAction::Repeat));

  f.hold(kUp, 50);  // solta select primeiro
  f.hold(0, 50);
  TEST_ASSERT_EQUAL_INT(0, f.log.countOf(Button::Select, ButtonAction::Click));
  TEST_ASSERT_EQUAL_INT(0, f.log.countOf(Button::DpadUp, ButtonAction::Click));
  TEST_ASSERT_EQUAL_INT(1, f.log.countOf(Button::Select, ButtonAction::Released));

  // Um novo aperto isolado volta a valer normalmente.
  f.hold(kSelect, 50);
  f.hold(0, 50);
  TEST_ASSERT_EQUAL_INT(1, f.log.countOf(Button::Select, ButtonAction::Click));
}

static void test_combo_with_hold_time_and_repeat() {
  Fixture f;
  TEST_ASSERT_OK(f.processor.addCombo({1, kA | kB, 500, true}));
  f.hold(kA | kB, 20 + 499);
  TEST_ASSERT_EQUAL_INT(0, f.log.comboCount);
  f.hold(kA | kB, 2);
  TEST_ASSERT_EQUAL_INT(1, f.log.comboCount);
  f.hold(kA | kB, 400);  // primeira repetição depois de repeatDelayMs
  TEST_ASSERT_EQUAL_INT(2, f.log.comboCount);
  f.hold(kA | kB, 120 * 3);  // depois a cada repeatIntervalMs
  TEST_ASSERT_EQUAL_INT(5, f.log.comboCount);

  // Soltou um dos botões e apertou de novo: a contagem do tempo recomeça.
  f.hold(kA, 50);
  f.hold(kA | kB, 300);
  TEST_ASSERT_EQUAL_INT(5, f.log.comboCount);
}

static void test_combo_registration_rules() {
  ButtonProcessor processor;
  TEST_ASSERT_STATUS(Status::InvalidArgument,
                     processor.addCombo({1, kA, 0, false}));  // um botão só
  TEST_ASSERT_STATUS(Status::InvalidArgument, processor.addCombo({1, 0, 0, false}));
  TEST_ASSERT_STATUS(Status::InvalidArgument, processor.addCombo({1, 0x30000u, 0, false}));
  TEST_ASSERT_OK(processor.addCombo({1, kA | kB, 0, false}));
  TEST_ASSERT_STATUS(Status::AlreadyInitialized, processor.addCombo({1, kA | kUp, 0, false}));
  for (uint8_t id = 2; id <= ButtonProcessor::kMaxCombos; ++id)
    TEST_ASSERT_OK(processor.addCombo({id, kA | kUp, 0, false}));
  TEST_ASSERT_STATUS(Status::NoSpace, processor.addCombo({99, kA | kUp, 0, false}));
}

static void test_release_all_emits_released_without_click() {
  Fixture f;
  f.hold(kA | kUp, 50);
  f.processor.releaseAll(f.now);
  TEST_ASSERT_EQUAL_INT(1, f.log.countOf(Button::A, ButtonAction::Released));
  TEST_ASSERT_EQUAL_INT(1, f.log.countOf(Button::DpadUp, ButtonAction::Released));
  TEST_ASSERT_EQUAL_INT(0, f.log.countOf(Button::A, ButtonAction::Click));
  TEST_ASSERT_EQUAL_UINT32(0, f.processor.pressed());
  // Se o botão continua fisicamente apertado, ele é detectado de novo (com debounce).
  f.hold(kA, 30);
  TEST_ASSERT_EQUAL_INT(2, f.log.countOf(Button::A, ButtonAction::Pressed));
}

static void test_timing_survives_clock_rollover() {
  Fixture f;
  f.now = 0xFFFFFFFFu - 300;  // o toque longo (600 ms) atravessa o estouro do relógio
  f.hold(kA, 20 + 599);
  TEST_ASSERT_EQUAL_INT(1, f.log.countOf(Button::A, ButtonAction::Pressed));
  TEST_ASSERT_EQUAL_INT(0, f.log.countOf(Button::A, ButtonAction::LongPress));
  f.hold(kA, 2);
  TEST_ASSERT_EQUAL_INT(1, f.log.countOf(Button::A, ButtonAction::LongPress));
}

static void test_bits_above_the_button_count_are_ignored() {
  Fixture f;
  f.hold(0xFFFF0000u, 100);
  TEST_ASSERT_EQUAL_INT(0, f.log.count);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_bounce_shorter_than_the_debounce_time_is_ignored);
  RUN_TEST(test_short_press_is_a_click);
  RUN_TEST(test_long_press_fires_once_and_cancels_the_click);
  RUN_TEST(test_dpad_repeats_after_a_delay_and_never_long_presses);
  RUN_TEST(test_repeat_mask_and_timing_are_configurable);
  RUN_TEST(test_simultaneous_buttons_are_independent);
  RUN_TEST(test_combo_fires_once_and_suppresses_its_buttons);
  RUN_TEST(test_combo_with_hold_time_and_repeat);
  RUN_TEST(test_combo_registration_rules);
  RUN_TEST(test_release_all_emits_released_without_click);
  RUN_TEST(test_timing_survives_clock_rollover);
  RUN_TEST(test_bits_above_the_button_count_are_ignored);
  return UNITY_END();
}
