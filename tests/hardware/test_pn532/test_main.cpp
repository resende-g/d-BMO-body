// Teste de bancada do PN532 por I2C, pelo driver do firmware próprio.
// Ligações: SDA e SCL de pins.h com pull-ups; chaves do módulo em I2C; VCC em 3,3 V.
// Rodar: pio test -e dbmo-bench -f test_pn532
//
// Esperado: o chip responde em 0x24, devolve IC = 0x32 e aceita a configuração de leitor.
// O teste termina com o campo de RF desligado e não lê etiqueta nenhuma.
#include <Arduino.h>
#include <Wire.h>
#include <unity.h>

#include "dbmo/config/pins.h"
#include "dbmo/nfc/pn532.h"
#include "dbmo/platform/esp32/hal.h"

namespace pins = dbmo::config::target;
namespace esp32 = dbmo::platform::esp32;

static esp32::ArduinoClock gClock;
static esp32::ArduinoI2cBus gBus(Wire);
static dbmo::nfc::Pn532 gChip(gBus, gClock, dbmo::config::i2c::kPn532);

void setUp() {}
void tearDown() {}

// Acompanha o comando em andamento por até 300 ms.
static bool finishCommand() {
  for (int i = 0; i < 300; ++i) {
    const dbmo::nfc::CommandState state = gChip.poll();
    if (state == dbmo::nfc::CommandState::Done) return true;
    if (state == dbmo::nfc::CommandState::Failed) return false;
    delay(1);
  }
  gChip.abort();
  return false;
}

static void test_answers_at_its_address() {
  TEST_ASSERT_TRUE_MESSAGE(dbmo::isOk(gChip.probe()), "nada respondeu em 0x24");
}

static void test_firmware_version() {
  TEST_ASSERT_TRUE(dbmo::isOk(gChip.startGetFirmwareVersion()));
  TEST_ASSERT_TRUE_MESSAGE(finishCommand(), "sem resposta ao GetFirmwareVersion");
  dbmo::nfc::FirmwareVersion version{};
  TEST_ASSERT_TRUE(dbmo::isOk(
      dbmo::nfc::Pn532::parseFirmwareVersion(gChip.response(), gChip.responseLength(), &version)));
  char line[48];
  snprintf(line, sizeof(line), "IC 0x%02X firmware %u.%u", version.ic, version.version,
           version.revision);
  TEST_MESSAGE(line);
  TEST_ASSERT_EQUAL_HEX8(0x32, version.ic);
}

static void test_reader_configuration_and_field_off() {
  TEST_ASSERT_TRUE(dbmo::isOk(gChip.startSamConfiguration()));
  TEST_ASSERT_TRUE(finishCommand());
  TEST_ASSERT_TRUE(dbmo::isOk(gChip.startSetPassiveActivationRetries(1)));
  TEST_ASSERT_TRUE(finishCommand());
  TEST_ASSERT_TRUE(dbmo::isOk(gChip.startRfField(false)));
  TEST_ASSERT_TRUE(finishCommand());
}

void setup() {
  delay(2000);  // tempo para a serial USB aparecer no computador
  Wire.begin(pins::kI2cSda, pins::kI2cScl, dbmo::config::i2c::kBusHz);
  UNITY_BEGIN();
  RUN_TEST(test_answers_at_its_address);
  RUN_TEST(test_firmware_version);
  RUN_TEST(test_reader_configuration_and_field_off);
  UNITY_END();
}

void loop() {}
