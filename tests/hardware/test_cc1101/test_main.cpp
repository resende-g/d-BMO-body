// Teste de bancada do CC1101 pelo driver do firmware próprio. NÃO TRANSMITE.
// Ligações: SCK, MOSI e MISO do SPI compartilhado; CS e GDO0 de pins.h; VCC em 3,3 V
// (o CC1101 não tolera 5 V em nenhum pino); antena conectada.
// Rodar: pio test -e dbmo-bench -f test_cc1101
//
// Esperado: PARTNUM 0x00 e VERSION 0x14 (ou 0x04 em peças antigas); a configuração é
// aceita porque a frequência gravada é relida igual; o RSSI lido em recepção é plausível.
// Se VERSION vier 0x00 ou 0xFF, o chip não está respondendo (ligação, alimentação ou CS).
#include <Arduino.h>
#include <SPI.h>
#include <unity.h>

#include "dbmo/config/pins.h"
#include "dbmo/platform/esp32/hal.h"
#include "dbmo/radio/cc1101.h"
#include "dbmo/radio/cc1101_regs.h"

namespace pins = dbmo::config::target;
namespace esp32 = dbmo::platform::esp32;

static esp32::ArduinoClock gClock;
static esp32::ArduinoSpiDevice gSpi(SPI, pins::kCc1101Cs, pins::kSpiMiso,
                                    dbmo::config::spi::kCc1101Hz);
static dbmo::radio::Cc1101 gChip(gSpi, gClock);

void setUp() {}
void tearDown() {}

static void test_reset_and_identify() {
  TEST_ASSERT_TRUE_MESSAGE(dbmo::isOk(gChip.reset()), "o chip nao sinalizou pronto apos o reset");
  const dbmo::Result<dbmo::radio::ChipId> id = gChip.identify();
  TEST_ASSERT_TRUE_MESSAGE(id.ok(), "sem resposta de um CC1101 (VERSION 0x00 ou 0xFF)");
  char line[48];
  snprintf(line, sizeof(line), "PARTNUM 0x%02X VERSION 0x%02X", id.value().partnum,
           id.value().version);
  TEST_MESSAGE(line);
  TEST_ASSERT_EQUAL_HEX8(dbmo::radio::cc1101::kExpectedPartnum, id.value().partnum);
}

static void test_configuration_is_read_back() {
  // 433,92 MHz, 2-FSK, 1200 baud: os valores padrão do firmware. configure() relê a
  // frequência gravada e falha se ela não confere.
  TEST_ASSERT_TRUE(dbmo::isOk(gChip.configure(dbmo::radio::RadioConfig(), 0x12)));
  const dbmo::Result<uint8_t> freq2 = gChip.readRegister(dbmo::radio::cc1101::kFreq2);
  TEST_ASSERT_TRUE(freq2.ok());
  TEST_ASSERT_EQUAL_HEX8(0x10, freq2.value());  // com cristal de 26 MHz
}

static void test_receive_state_and_rssi() {
  TEST_ASSERT_TRUE(dbmo::isOk(gChip.startReceive()));
  delay(10);
  const dbmo::Result<uint8_t> state = gChip.marcState();
  TEST_ASSERT_TRUE(state.ok());
  TEST_ASSERT_EQUAL_HEX8(dbmo::radio::cc1101::kStateRx, state.value());
  const dbmo::Result<int16_t> rssi = gChip.rssiDbm();
  TEST_ASSERT_TRUE(rssi.ok());
  char line[32];
  snprintf(line, sizeof(line), "RSSI %d dBm", static_cast<int>(rssi.value()));
  TEST_MESSAGE(line);
  TEST_ASSERT_TRUE(rssi.value() <= 0 && rssi.value() >= -140);
}

static void test_sleep() { TEST_ASSERT_TRUE(dbmo::isOk(gChip.sleep())); }

void setup() {
  delay(2000);  // tempo para a serial USB aparecer no computador
  for (int8_t cs : {pins::kDisplayCs, pins::kCc1101Cs, pins::kSdCs}) {
    pinMode(static_cast<uint8_t>(cs), OUTPUT);
    digitalWrite(static_cast<uint8_t>(cs), HIGH);
  }
  SPI.begin(pins::kSpiSck, pins::kSpiMiso, pins::kSpiMosi);
  gSpi.begin();
  UNITY_BEGIN();
  RUN_TEST(test_reset_and_identify);
  RUN_TEST(test_configuration_is_read_back);
  RUN_TEST(test_receive_state_and_rssi);
  RUN_TEST(test_sleep);
  UNITY_END();
}

void loop() {}
