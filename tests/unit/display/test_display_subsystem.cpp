#include "dbmo/display/display_subsystem.h"
#include "dbmo/display/framebuffer_display.h"
#include "dbmo/mock/mock_hal.h"
#include "dbmo_test.h"

using dbmo::ConfigStore;
using dbmo::Event;
using dbmo::EventBus;
using dbmo::Status;
using namespace dbmo::display;

namespace {
constexpr uint16_t kW = 16;
constexpr uint16_t kH = 12;
Color gPixels[kW * kH];

struct Fixture {
  FramebufferDisplay panel{gPixels, kW, kH};
  EventBus bus;
  ConfigStore config;
  DisplaySubsystem display{panel, bus, config};
  int powerEvents = 0;
  Event lastEvent;

  Fixture() {
    TEST_ASSERT_OK(DisplaySubsystem::registerConfig(config));
    TEST_ASSERT_OK(bus.subscribe(display_event::kPowerChanged, &Fixture::onEvent, this));
  }
  static void onEvent(const Event &event, void *context) {
    auto *self = static_cast<Fixture *>(context);
    ++self->powerEvents;
    self->lastEvent = event;
  }
};
}  // namespace

void setUp() {
  for (Color &pixel : gPixels) pixel = 0xFFFF;
}
void tearDown() { dbmo::logger().clearSinks(); }  // um teste que falha não chega a limpar

static void test_init_clears_to_black_and_applies_brightness() {
  Fixture f;
  TEST_ASSERT_OK(f.display.init());
  TEST_ASSERT_EQUAL_HEX16(palette::kBlack, f.panel.pixel(0, 0));
  TEST_ASSERT_EQUAL_HEX16(palette::kBlack, f.panel.pixel(kW - 1, kH - 1));
  TEST_ASSERT_EQUAL_UINT8(80, f.panel.backlightPercent);  // padrão
  TEST_ASSERT_FALSE(f.panel.asleep);
  TEST_ASSERT_TRUE(f.display.power() == ScreenPower::On);
}

static void test_controller_id_is_checked_without_blocking_the_screen() {
  // Bancada, 2026-11-21: sem o SDO da tela ligado o ID lido foi 0xFFFF e depois 0x0000, e
  // a tela foi dada como pronta. A tela desenha sem o MISO, então o init continua Ok.
  struct Case {
    dbmo::Result<uint16_t> id;
    Status expected;
  };
  const Case cases[] = {
      {static_cast<uint16_t>(0x9341), Status::Ok},
      {Status::Unsupported, Status::Ok},  // tela que não informa o ID: nada a conferir
      {static_cast<uint16_t>(0xFFFF), Status::IoError},
      {static_cast<uint16_t>(0x0000), Status::IoError},
      {static_cast<uint16_t>(0x7789), Status::Incompatible},
  };
  for (const Case &c : cases) {
    dbmo::mock::RecordingLogSink sink;
    TEST_ASSERT_OK(dbmo::logger().addSink(&sink));
    Fixture f;
    f.panel.controllerIdResult = c.id;
    TEST_ASSERT_OK(f.display.init());
    TEST_ASSERT_EQUAL_HEX16(palette::kBlack, f.panel.pixel(0, 0));  // desenhou
    TEST_ASSERT_STATUS(c.expected, f.display.controllerCheck());
    TEST_ASSERT_EQUAL_UINT(isOk(c.expected) ? 0 : 1, sink.countAtLevel(dbmo::LogLevel::Warn));
    dbmo::logger().clearSinks();
  }
}

static void test_inactivity_dims_then_turns_off_and_activity_wakes() {
  Fixture f;
  TEST_ASSERT_OK(f.display.init());
  f.display.tick(1000);  // primeiro tick: começa a contar daqui
  f.display.tick(1000 + 29999);
  TEST_ASSERT_TRUE(f.display.power() == ScreenPower::On);

  f.display.tick(1000 + 30000);  // display.dim_after_s = 30
  TEST_ASSERT_TRUE(f.display.power() == ScreenPower::Dimmed);
  TEST_ASSERT_EQUAL_UINT8(20, f.panel.backlightPercent);  // 25 % de 80

  f.display.tick(1000 + 120000);  // display.off_after_s = 120
  TEST_ASSERT_TRUE(f.display.power() == ScreenPower::Off);
  TEST_ASSERT_TRUE(f.panel.asleep);
  TEST_ASSERT_EQUAL_UINT8(0, f.panel.backlightPercent);

  f.display.notifyActivity(1000 + 130000);
  TEST_ASSERT_TRUE(f.display.power() == ScreenPower::On);
  TEST_ASSERT_FALSE(f.panel.asleep);
  TEST_ASSERT_EQUAL_UINT8(80, f.panel.backlightPercent);

  f.bus.dispatch();
  TEST_ASSERT_EQUAL_INT(3, f.powerEvents);
  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(ScreenPower::On), f.lastEvent.a);
  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(ScreenPower::Off), f.lastEvent.b);
}

static void test_activity_keeps_the_screen_on() {
  Fixture f;
  TEST_ASSERT_OK(f.display.init());
  f.display.tick(0);
  for (uint32_t t = 10000; t <= 200000; t += 10000) {
    f.display.notifyActivity(t);
    f.display.tick(t + 5000);
  }
  TEST_ASSERT_TRUE(f.display.power() == ScreenPower::On);
}

static void test_timeouts_of_zero_disable_the_feature() {
  Fixture f;
  TEST_ASSERT_OK(f.config.setInt(config_key::kDimAfterS, 0));
  TEST_ASSERT_OK(f.config.setInt(config_key::kOffAfterS, 0));
  TEST_ASSERT_OK(f.display.init());
  f.display.tick(0);
  f.display.tick(3000000);
  TEST_ASSERT_TRUE(f.display.power() == ScreenPower::On);
}

static void test_idle_time_survives_clock_rollover() {
  Fixture f;
  TEST_ASSERT_OK(f.display.init());
  const uint32_t start = 0xFFFFFFFFu - 10000;  // 10 s antes de o relógio estourar
  f.display.tick(start);
  f.display.tick(start + 20000);  // já depois do estouro: só 20 s de inatividade
  TEST_ASSERT_TRUE(f.display.power() == ScreenPower::On);
  f.display.tick(start + 31000);
  TEST_ASSERT_TRUE(f.display.power() == ScreenPower::Dimmed);
}

// Com o LED aceso, o painel em repouso fica branco (bancada, 2026-11-22): sem controle do
// backlight, a tela não esmaece nem dorme.
static void test_uncontrolled_backlight_never_dims_or_sleeps() {
  Fixture f;
  f.panel.backlightSupported = false;  // LED da tela direto no 3V3
  TEST_ASSERT_OK(f.display.init());
  TEST_ASSERT_FALSE(f.display.backlightControlled());
  f.display.tick(0);
  f.display.tick(60000);
  TEST_ASSERT_TRUE(f.display.power() == ScreenPower::On);
  f.display.tick(120000);
  f.display.tick(3600000);
  TEST_ASSERT_TRUE(f.display.power() == ScreenPower::On);
  TEST_ASSERT_FALSE(f.panel.asleep);
}

static void test_brightness_is_validated_stored_and_applied() {
  Fixture f;
  TEST_ASSERT_OK(f.display.init());
  TEST_ASSERT_OK(f.display.setBrightness(40));
  TEST_ASSERT_EQUAL_UINT8(40, f.panel.backlightPercent);
  TEST_ASSERT_EQUAL_INT32(40, f.config.getInt(config_key::kBrightness).value());
  TEST_ASSERT_STATUS(Status::OutOfRange, f.display.setBrightness(101));
  TEST_ASSERT_EQUAL_UINT8(40, f.panel.backlightPercent);
}

static void test_panel_errors_are_reported_and_retried() {
  Fixture f;
  f.panel.failNext(Status::IoError);
  TEST_ASSERT_STATUS(Status::IoError, f.display.init());
  TEST_ASSERT_OK(f.display.init());  // init pode ser chamado de novo

  f.display.tick(0);
  f.panel.failNext(Status::Timeout);
  f.display.tick(30000);  // a troca para Dimmed falha
  TEST_ASSERT_TRUE(f.display.power() == ScreenPower::On);
  f.display.tick(30001);  // e é tentada de novo
  TEST_ASSERT_TRUE(f.display.power() == ScreenPower::Dimmed);
}

static void test_shutdown_blanks_and_sleeps_the_panel() {
  Fixture f;
  TEST_ASSERT_OK(f.display.init());
  for (Color &pixel : gPixels) pixel = 0xFFFF;
  TEST_ASSERT_OK(f.display.shutdown());
  TEST_ASSERT_EQUAL_HEX16(palette::kBlack, f.panel.pixel(3, 3));
  TEST_ASSERT_TRUE(f.panel.asleep);
  TEST_ASSERT_EQUAL_UINT8(0, f.panel.backlightPercent);
  f.display.tick(999999);  // depois de desligado, o tick não faz nada
  TEST_ASSERT_OK(f.display.shutdown());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_init_clears_to_black_and_applies_brightness);
  RUN_TEST(test_controller_id_is_checked_without_blocking_the_screen);
  RUN_TEST(test_inactivity_dims_then_turns_off_and_activity_wakes);
  RUN_TEST(test_activity_keeps_the_screen_on);
  RUN_TEST(test_timeouts_of_zero_disable_the_feature);
  RUN_TEST(test_idle_time_survives_clock_rollover);
  RUN_TEST(test_uncontrolled_backlight_never_dims_or_sleeps);
  RUN_TEST(test_brightness_is_validated_stored_and_applied);
  RUN_TEST(test_panel_errors_are_reported_and_retried);
  RUN_TEST(test_shutdown_blanks_and_sleeps_the_panel);
  return UNITY_END();
}
