// Teste de bancada do barramento I2C: linhas em repouso e varredura de endereços.
// Ligações: SDA e SCL de pins.h, com pull-ups de 4,7 kΩ ao 3V3.
// Rodar: pio test -e dbmo-bench -f test_i2c_scan
//
// Este teste não exige nenhum dispositivo: ele lista o que respondeu e falha só se o
// barramento estiver travado. Endereços previstos no projeto: 0x20 (MCP23017 dos botões),
// 0x21 (segundo MCP23017, opcional), 0x24 (PN532) e 0x50 (EEPROM do cartucho).
#include <Arduino.h>
#include <Wire.h>
#include <unity.h>

#include "dbmo/config/pins.h"
#include "dbmo/platform/esp32/hal.h"

namespace pins = dbmo::config::target;

static dbmo::platform::esp32::ArduinoI2cBus gBus(Wire);

void setUp() {}
void tearDown() {}

static void test_lines_idle_high() {
  // Com o barramento parado, SDA e SCL ficam em alto pelos pull-ups. Em baixo: falta
  // pull-up, há curto ou algum dispositivo está segurando a linha.
  pinMode(static_cast<uint8_t>(pins::kI2cSda), INPUT);
  pinMode(static_cast<uint8_t>(pins::kI2cScl), INPUT);
  delay(5);
  TEST_ASSERT_EQUAL_MESSAGE(HIGH, digitalRead(static_cast<uint8_t>(pins::kI2cSda)), "SDA em baixo");
  TEST_ASSERT_EQUAL_MESSAGE(HIGH, digitalRead(static_cast<uint8_t>(pins::kI2cScl)), "SCL em baixo");
}

static void test_scan_completes_without_bus_errors() {
  TEST_ASSERT_TRUE(Wire.begin(pins::kI2cSda, pins::kI2cScl, dbmo::config::i2c::kBusHz));

  int found = 0;
  int busErrors = 0;
  char line[48];
  for (uint8_t address = 0x08; address <= 0x77; ++address) {
    const dbmo::Status status = gBus.probe(address);
    if (dbmo::isOk(status)) {
      ++found;
      snprintf(line, sizeof(line), "dispositivo em 0x%02X", address);
      TEST_MESSAGE(line);
    } else if (status != dbmo::Status::HardwareAbsent) {
      ++busErrors;
    }
  }
  snprintf(line, sizeof(line), "%d dispositivo(s) encontrado(s)", found);
  TEST_MESSAGE(line);
  TEST_ASSERT_EQUAL_MESSAGE(0, busErrors, "erro de barramento durante a varredura");
}

void setup() {
  delay(2000);  // tempo para a serial USB aparecer no computador
  UNITY_BEGIN();
  RUN_TEST(test_lines_idle_high);
  RUN_TEST(test_scan_completes_without_bus_errors);
  UNITY_END();
}

void loop() {}
