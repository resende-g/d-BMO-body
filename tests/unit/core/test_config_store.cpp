#include <cstring>

#include "dbmo/core/config_store.h"
#include "dbmo/mock/mock_hal.h"
#include "dbmo_test.h"

using dbmo::ConfigLoadReport;
using dbmo::ConfigSpec;
using dbmo::ConfigStore;
using dbmo::Status;
using dbmo::mock::MemoryConfigBackend;

namespace {

void defineSample(ConfigStore &store) {
  TEST_ASSERT_OK(store.define(ConfigSpec::integer("audio.volume", 60, 0, 100)));
  TEST_ASSERT_OK(store.define(ConfigSpec::boolean("radio.tx_enabled", false)));
  TEST_ASSERT_OK(store.define(ConfigSpec::string("device.name", "d-BMO")));
  TEST_ASSERT_OK(store.define(ConfigSpec::integer("input.deadzone", -5, -100, 100)));
}

struct Changes {
  int count = 0;
  char lastKey[32] = {};
};

void onChange(const char *key, void *context) {
  auto *changes = static_cast<Changes *>(context);
  ++changes->count;
  snprintf(changes->lastKey, sizeof(changes->lastKey), "%s", key);
}

}  // namespace

void setUp() {}
void tearDown() {}

static void test_defaults_are_returned_until_changed() {
  ConfigStore store;
  defineSample(store);
  TEST_ASSERT_EQUAL_INT32(60, store.getInt("audio.volume").value());
  TEST_ASSERT_FALSE(store.getBool("radio.tx_enabled").value());
  char name[16];
  TEST_ASSERT_OK(store.getString("device.name", name, sizeof(name)));
  TEST_ASSERT_EQUAL_STRING("d-BMO", name);
  TEST_ASSERT_FALSE(store.dirty());
  TEST_ASSERT_EQUAL_size_t(4, store.size());
  TEST_ASSERT_EQUAL_STRING("audio.volume", store.keyAt(0));
}

static void test_define_validates_key_default_and_duplicates() {
  ConfigStore store;
  TEST_ASSERT_STATUS(Status::InvalidArgument, store.define(ConfigSpec::integer("", 1, 0, 2)));
  TEST_ASSERT_STATUS(Status::InvalidArgument, store.define(ConfigSpec::integer("Com Espaco", 1, 0, 2)));
  TEST_ASSERT_STATUS(Status::InvalidArgument, store.define(ConfigSpec::integer("a.b=c", 1, 0, 2)));
  TEST_ASSERT_STATUS(Status::InvalidArgument,
                     store.define(ConfigSpec::integer("chave.com.mais.de.trinta.e.um.chars", 1, 0, 2)));
  TEST_ASSERT_STATUS(Status::InvalidArgument, store.define(ConfigSpec::integer("a.fora", 5, 0, 2)));
  TEST_ASSERT_STATUS(Status::InvalidArgument, store.define(ConfigSpec::integer("a.invertido", 1, 2, 0)));
  TEST_ASSERT_STATUS(Status::InvalidArgument, store.define(ConfigSpec::string("a.nulo", nullptr)));
  TEST_ASSERT_OK(store.define(ConfigSpec::integer("a.ok", 1, 0, 2)));
  TEST_ASSERT_STATUS(Status::AlreadyInitialized, store.define(ConfigSpec::boolean("a.ok", true)));
}

static void test_table_capacity() {
  ConfigStore store;
  static char keys[ConfigStore::kMaxEntries + 1][8];
  for (size_t i = 0; i < ConfigStore::kMaxEntries; ++i) {
    snprintf(keys[i], sizeof(keys[i]), "k.%02u", static_cast<unsigned>(i));
    TEST_ASSERT_OK(store.define(ConfigSpec::boolean(keys[i], false)));
  }
  TEST_ASSERT_STATUS(Status::NoSpace, store.define(ConfigSpec::boolean("k.extra", false)));
}

static void test_set_enforces_type_and_range() {
  ConfigStore store;
  defineSample(store);
  TEST_ASSERT_OK(store.setInt("audio.volume", 100));
  TEST_ASSERT_STATUS(Status::OutOfRange, store.setInt("audio.volume", 101));
  TEST_ASSERT_STATUS(Status::OutOfRange, store.setInt("audio.volume", -1));
  TEST_ASSERT_EQUAL_INT32(100, store.getInt("audio.volume").value());

  TEST_ASSERT_STATUS(Status::InvalidArgument, store.setBool("audio.volume", true));
  TEST_ASSERT_STATUS(Status::InvalidArgument, store.setInt("radio.tx_enabled", 1));
  TEST_ASSERT_STATUS(Status::InvalidArgument, store.getInt("device.name").status());
  TEST_ASSERT_STATUS(Status::NotFound, store.setInt("nao.existe", 1));
  TEST_ASSERT_STATUS(Status::NotFound, store.getBool("nao.existe").status());

  TEST_ASSERT_STATUS(Status::InvalidArgument,
                     store.setString("device.name", "um nome com mais de trinta e um caracteres"));
  TEST_ASSERT_STATUS(Status::InvalidArgument, store.setString("device.name", "linha\nnova"));
  TEST_ASSERT_STATUS(Status::InvalidArgument, store.setString("device.name", " espaco"));
  TEST_ASSERT_OK(store.setString("device.name", "bancada 1"));

  char tiny[4];
  TEST_ASSERT_STATUS(Status::NoSpace, store.getString("device.name", tiny, sizeof(tiny)));
}

static void test_dirty_flag_and_observer_fire_only_on_real_changes() {
  ConfigStore store;
  defineSample(store);
  Changes changes;
  store.setChangeObserver(onChange, &changes);

  TEST_ASSERT_OK(store.setInt("audio.volume", 60));  // mesmo valor do padrão
  TEST_ASSERT_FALSE(store.dirty());
  TEST_ASSERT_EQUAL_INT(0, changes.count);

  TEST_ASSERT_OK(store.setInt("audio.volume", 61));
  TEST_ASSERT_TRUE(store.dirty());
  TEST_ASSERT_EQUAL_INT(1, changes.count);
  TEST_ASSERT_EQUAL_STRING("audio.volume", changes.lastKey);

  store.clearDirty();
  TEST_ASSERT_STATUS(Status::OutOfRange, store.setInt("audio.volume", 999));
  TEST_ASSERT_FALSE(store.dirty());
  TEST_ASSERT_EQUAL_INT(1, changes.count);
}

static void test_set_from_text_parses_by_type() {
  ConfigStore store;
  defineSample(store);
  TEST_ASSERT_OK(store.setFromText("audio.volume", "42"));
  TEST_ASSERT_EQUAL_INT32(42, store.getInt("audio.volume").value());
  TEST_ASSERT_OK(store.setFromText("input.deadzone", "-100"));
  TEST_ASSERT_EQUAL_INT32(-100, store.getInt("input.deadzone").value());
  TEST_ASSERT_OK(store.setFromText("radio.tx_enabled", "true"));
  TEST_ASSERT_TRUE(store.getBool("radio.tx_enabled").value());
  TEST_ASSERT_OK(store.setFromText("radio.tx_enabled", "off"));
  TEST_ASSERT_FALSE(store.getBool("radio.tx_enabled").value());

  TEST_ASSERT_STATUS(Status::InvalidArgument, store.setFromText("audio.volume", "4x"));
  TEST_ASSERT_STATUS(Status::InvalidArgument, store.setFromText("audio.volume", ""));
  TEST_ASSERT_STATUS(Status::InvalidArgument, store.setFromText("audio.volume", "-"));
  TEST_ASSERT_STATUS(Status::InvalidArgument, store.setFromText("audio.volume", "99999999999999"));
  TEST_ASSERT_STATUS(Status::OutOfRange, store.setFromText("audio.volume", "101"));
  TEST_ASSERT_STATUS(Status::InvalidArgument, store.setFromText("radio.tx_enabled", "talvez"));
  TEST_ASSERT_STATUS(Status::NotFound, store.setFromText("nao.existe", "1"));
}

static void test_serialize_round_trip() {
  ConfigStore store;
  defineSample(store);
  TEST_ASSERT_OK(store.setInt("audio.volume", 35));
  TEST_ASSERT_OK(store.setBool("radio.tx_enabled", true));
  TEST_ASSERT_OK(store.setString("device.name", "bancada 2"));

  char text[256];
  const auto length = store.serialize(text, sizeof(text));
  TEST_ASSERT_TRUE(length.ok());
  TEST_ASSERT_EQUAL_size_t(strlen(text), length.value());
  TEST_ASSERT_NOT_NULL(strstr(text, "audio.volume=35\n"));
  TEST_ASSERT_NOT_NULL(strstr(text, "radio.tx_enabled=1\n"));
  TEST_ASSERT_NOT_NULL(strstr(text, "device.name=bancada 2\n"));

  ConfigStore other;
  defineSample(other);
  ConfigLoadReport report;
  TEST_ASSERT_OK(other.deserialize(text, length.value(), &report));
  TEST_ASSERT_EQUAL_UINT16(4, report.applied);
  TEST_ASSERT_EQUAL_INT32(35, other.getInt("audio.volume").value());
  TEST_ASSERT_TRUE(other.getBool("radio.tx_enabled").value());
  char name[16];
  TEST_ASSERT_OK(other.getString("device.name", name, sizeof(name)));
  TEST_ASSERT_EQUAL_STRING("bancada 2", name);

  char small[20];
  TEST_ASSERT_STATUS(Status::NoSpace, store.serialize(small, sizeof(small)).status());
}

static void test_deserialize_tolerates_and_reports_bad_input() {
  ConfigStore store;
  defineSample(store);
  const char text[] =
      "# comentario\r\n"
      "\r\n"
      "  audio.volume = 20  \r\n"   // espaços e CRLF são aceitos
      "chave.de.outro.firmware=1\n" // desconhecida: ignorada, não é erro
      "radio.tx_enabled=talvez\n"   // valor inválido: fica o padrão
      "linha sem igual\n"           // malformada
      "input.deadzone=500\n"        // fora da faixa
      "device.name=ok";             // última linha sem \n
  ConfigLoadReport report;
  TEST_ASSERT_STATUS(Status::Corrupt, store.deserialize(text, strlen(text), &report));
  TEST_ASSERT_EQUAL_UINT16(2, report.applied);
  TEST_ASSERT_EQUAL_UINT16(1, report.unknownKeys);
  TEST_ASSERT_EQUAL_UINT16(2, report.invalidValues);
  TEST_ASSERT_EQUAL_UINT16(1, report.malformedLines);

  TEST_ASSERT_EQUAL_INT32(20, store.getInt("audio.volume").value());
  TEST_ASSERT_FALSE(store.getBool("radio.tx_enabled").value());
  TEST_ASSERT_EQUAL_INT32(-5, store.getInt("input.deadzone").value());
  char name[16];
  TEST_ASSERT_OK(store.getString("device.name", name, sizeof(name)));
  TEST_ASSERT_EQUAL_STRING("ok", name);
}

static void test_unknown_keys_alone_are_not_corruption() {
  ConfigStore store;
  defineSample(store);
  const char text[] = "futuro.recurso=7\naudio.volume=10\n";
  TEST_ASSERT_OK(store.deserialize(text, strlen(text)));
  TEST_ASSERT_EQUAL_INT32(10, store.getInt("audio.volume").value());
}

static void test_reset_to_defaults() {
  ConfigStore store;
  defineSample(store);
  TEST_ASSERT_OK(store.setInt("audio.volume", 5));
  TEST_ASSERT_OK(store.setString("device.name", "x"));
  store.clearDirty();
  store.resetToDefaults();
  TEST_ASSERT_EQUAL_INT32(60, store.getInt("audio.volume").value());
  char name[16];
  TEST_ASSERT_OK(store.getString("device.name", name, sizeof(name)));
  TEST_ASSERT_EQUAL_STRING("d-BMO", name);
  TEST_ASSERT_TRUE(store.dirty());
}

static void test_backend_save_and_load() {
  ConfigStore store;
  defineSample(store);
  MemoryConfigBackend backend;

  // Primeira vez: nada salvo ainda. Os padrões permanecem.
  TEST_ASSERT_STATUS(Status::NotFound, store.load(backend));
  TEST_ASSERT_EQUAL_INT32(60, store.getInt("audio.volume").value());

  TEST_ASSERT_OK(store.setInt("audio.volume", 77));
  TEST_ASSERT_OK(store.save(backend));
  TEST_ASSERT_FALSE(store.dirty());

  ConfigStore restored;
  defineSample(restored);
  TEST_ASSERT_OK(restored.load(backend));
  TEST_ASSERT_EQUAL_INT32(77, restored.getInt("audio.volume").value());
  TEST_ASSERT_FALSE(restored.dirty());  // carregar não suja
}

static void test_backend_failures_are_propagated() {
  ConfigStore store;
  defineSample(store);
  MemoryConfigBackend backend;
  backend.saveStatus = Status::IoError;
  TEST_ASSERT_OK(store.setInt("audio.volume", 1));
  TEST_ASSERT_STATUS(Status::IoError, store.save(backend));
  TEST_ASSERT_TRUE(store.dirty());  // não salvou: continua pendente

  backend.loadStatus = Status::IoError;
  TEST_ASSERT_STATUS(Status::IoError, store.load(backend));
}

static void test_corrupt_file_keeps_store_dirty_for_rewrite() {
  ConfigStore store;
  defineSample(store);
  MemoryConfigBackend backend;
  backend.setText("audio.volume=30\nlixo\n");
  TEST_ASSERT_STATUS(Status::Corrupt, store.load(backend));
  TEST_ASSERT_EQUAL_INT32(30, store.getInt("audio.volume").value());
  TEST_ASSERT_TRUE(store.dirty());
  TEST_ASSERT_OK(store.save(backend));
  // O arquivo regravado não tem mais a linha inválida.
  TEST_ASSERT_NULL(strstr(backend.text, "lixo"));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_defaults_are_returned_until_changed);
  RUN_TEST(test_define_validates_key_default_and_duplicates);
  RUN_TEST(test_table_capacity);
  RUN_TEST(test_set_enforces_type_and_range);
  RUN_TEST(test_dirty_flag_and_observer_fire_only_on_real_changes);
  RUN_TEST(test_set_from_text_parses_by_type);
  RUN_TEST(test_serialize_round_trip);
  RUN_TEST(test_deserialize_tolerates_and_reports_bad_input);
  RUN_TEST(test_unknown_keys_alone_are_not_corruption);
  RUN_TEST(test_reset_to_defaults);
  RUN_TEST(test_backend_save_and_load);
  RUN_TEST(test_backend_failures_are_propagated);
  RUN_TEST(test_corrupt_file_keeps_store_dirty_for_rewrite);
  return UNITY_END();
}
