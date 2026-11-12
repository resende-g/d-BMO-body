// Teste de bancada da tela ILI9341 pelo porte do firmware próprio.
// Ligações: pins.h (perfil target; iguais às da bancada para a tela).
// Rodar: pio test -e dbmo-bench -f test_display
//
// Esperado: o identificador lido é 0x9341 (o mesmo que o firmware/teste_tela mostra) e a
// tela pisca vermelho e volta ao preto. Conferir a cor a olho: este teste não vê a tela.
#include <Adafruit_ILI9341.h>
#include <Arduino.h>
#include <SPI.h>
#include <unity.h>

#include "dbmo/config/pins.h"
#include "dbmo/display/display.h"
#include "dbmo/platform/esp32/hal.h"
#include "dbmo/platform/esp32/ili9341_display.h"

namespace pins = dbmo::config::target;
namespace esp32 = dbmo::platform::esp32;

static esp32::ArduinoClock gClock;
static Adafruit_ILI9341 gTft(&SPI, pins::kDisplayDc, pins::kDisplayCs, pins::kDisplayReset);
static esp32::Ili9341Display gDisplay(gTft, gClock, nullptr);

void setUp() {}
void tearDown() {}

static void test_controller_id_is_ili9341() {
  // 0x0000 ou 0xFFFF: MISO solto (a tela pode funcionar mesmo assim) ou outro controlador.
  TEST_ASSERT_EQUAL_HEX16(0x9341, gDisplay.readId());
}

static void test_landscape_resolution() {
  TEST_ASSERT_EQUAL_UINT16(320, gDisplay.width());
  TEST_ASSERT_EQUAL_UINT16(240, gDisplay.height());
}

static void test_fill_and_sleep_commands_are_accepted() {
  const int16_t w = static_cast<int16_t>(gDisplay.width());
  const int16_t h = static_cast<int16_t>(gDisplay.height());
  TEST_ASSERT_TRUE(dbmo::isOk(gDisplay.fillRect(0, 0, w, h, dbmo::display::palette::kSignalRed)));
  delay(500);
  TEST_ASSERT_TRUE(dbmo::isOk(gDisplay.setSleep(true)));
  delay(300);
  TEST_ASSERT_TRUE(dbmo::isOk(gDisplay.setSleep(false)));
  TEST_ASSERT_TRUE(dbmo::isOk(gDisplay.fillRect(0, 0, w, h, dbmo::display::palette::kBlack)));
}

static void test_backlight_is_reported_as_not_controlled() {
  // Na bancada o LED da tela está direto no 3V3.
  TEST_ASSERT_TRUE(gDisplay.setBacklight(50) == dbmo::Status::Unsupported);
}

void setup() {
  delay(2000);  // tempo para a serial USB aparecer no computador
  for (int8_t cs : {pins::kDisplayCs, pins::kCc1101Cs, pins::kSdCs}) {
    pinMode(static_cast<uint8_t>(cs), OUTPUT);
    digitalWrite(static_cast<uint8_t>(cs), HIGH);
  }
  SPI.begin(pins::kSpiSck, pins::kSpiMiso, pins::kSpiMosi);
  gDisplay.begin(dbmo::config::spi::kDisplayHz, 1);

  UNITY_BEGIN();
  RUN_TEST(test_controller_id_is_ili9341);
  RUN_TEST(test_landscape_resolution);
  RUN_TEST(test_fill_and_sleep_commands_are_accepted);
  RUN_TEST(test_backlight_is_reported_as_not_controlled);
  UNITY_END();
}

void loop() {}
