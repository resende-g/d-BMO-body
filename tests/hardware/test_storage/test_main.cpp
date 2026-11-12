// Teste de bancada do cartão SD no SPI compartilhado, pelo armazenamento do firmware próprio.
// Ligações: slot SD do módulo da tela; CS de pins.h; cartão em FAT32.
// Rodar: pio test -e dbmo-bench -f test_storage
//
// Esperado: o cartão monta, as pastas do d-BMO são criadas, um arquivo gravado de forma
// atômica é lido de volta igual e depois apagado. O teste só mexe em /dbmo.
#include <Arduino.h>
#include <SPI.h>
#include <string.h>
#include <unity.h>

#include "dbmo/config/pins.h"
#include "dbmo/platform/esp32/hal.h"
#include "dbmo/platform/esp32/peripherals.h"
#include "dbmo/storage/file_utils.h"
#include "dbmo/storage/path.h"

namespace pins = dbmo::config::target;
namespace esp32 = dbmo::platform::esp32;

static esp32::SdFileSystem gCard(SPI, static_cast<uint8_t>(pins::kSdCs), 4000000);
static const char kFile[] = "/dbmo/logs/bench_test.txt";
static const char kText[] = "d-BMO bancada\n";

void setUp() {}
void tearDown() {}

static void test_card_mounts() {
  TEST_ASSERT_TRUE_MESSAGE(dbmo::isOk(gCard.mount()), "cartao ausente ou nao respondeu");
}

static void test_layout_is_created() {
  TEST_ASSERT_TRUE(dbmo::isOk(dbmo::storage::ensureLayout(gCard)));
  TEST_ASSERT_TRUE(gCard.exists(dbmo::storage::kConfigDir));
  TEST_ASSERT_TRUE(gCard.exists(dbmo::storage::kLogDir));
  TEST_ASSERT_TRUE(gCard.exists(dbmo::storage::kIrDir));
}

static void test_atomic_write_and_read_back() {
  const size_t length = strlen(kText);
  TEST_ASSERT_TRUE(dbmo::isOk(dbmo::storage::writeFileAtomic(gCard, kFile, kText, length, false)));
  char buffer[32] = {};
  const dbmo::Result<size_t> read = dbmo::storage::readFile(gCard, kFile, buffer, sizeof(buffer));
  TEST_ASSERT_TRUE(read.ok());
  TEST_ASSERT_EQUAL_UINT(length, read.value());
  TEST_ASSERT_EQUAL_MEMORY(kText, buffer, length);
  TEST_ASSERT_FALSE(gCard.exists("/dbmo/logs/bench_test.txt.tmp"));
}

static void test_remove_and_unmount() {
  TEST_ASSERT_TRUE(dbmo::isOk(gCard.remove(kFile)));
  TEST_ASSERT_FALSE(gCard.exists(kFile));
  gCard.unmount();
  TEST_ASSERT_FALSE(gCard.mounted());
}

void setup() {
  delay(2000);  // tempo para a serial USB aparecer no computador
  // Todos os CS em alto e o cartão como primeiro dispositivo a usar o barramento.
  for (int8_t cs : {pins::kDisplayCs, pins::kCc1101Cs, pins::kSdCs}) {
    pinMode(static_cast<uint8_t>(cs), OUTPUT);
    digitalWrite(static_cast<uint8_t>(cs), HIGH);
  }
  SPI.begin(pins::kSpiSck, pins::kSpiMiso, pins::kSpiMosi);
  UNITY_BEGIN();
  RUN_TEST(test_card_mounts);
  RUN_TEST(test_layout_is_created);
  RUN_TEST(test_atomic_write_and_read_back);
  RUN_TEST(test_remove_and_unmount);
  UNITY_END();
}

void loop() {}
