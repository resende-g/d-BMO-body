#include <cstring>

#include "dbmo/core/config_store.h"
#include "dbmo/sim/memory_file_system.h"
#include "dbmo/storage/file_config_backend.h"
#include "dbmo/storage/file_utils.h"
#include "dbmo_test.h"

using dbmo::ConfigSpec;
using dbmo::ConfigStore;
using dbmo::Status;
using dbmo::sim::MemoryFileSystem;
using namespace dbmo::storage;

namespace {

MemoryFileSystem *fs = nullptr;
constexpr char kFile[] = "/dbmo/config/config.txt";
constexpr char kBackup[] = "/dbmo/config/config.txt.bak";
constexpr char kTemp[] = "/dbmo/config/config.txt.tmp";

Status save(const char *text, bool keepBackup = true) {
  return writeFileAtomic(*fs, kFile, text, strlen(text), keepBackup);
}

}  // namespace

void setUp() {
  fs = new MemoryFileSystem();
  TEST_ASSERT_OK(fs->mount());
  TEST_ASSERT_OK(ensureLayout(*fs));
}
void tearDown() { delete fs; }

void test_ensure_layout_creates_the_directories_once() {
  TEST_ASSERT_TRUE(fs->exists(kRootDir));
  TEST_ASSERT_TRUE(fs->exists(kConfigDir));
  TEST_ASSERT_TRUE(fs->exists(kLogDir));
  TEST_ASSERT_TRUE(fs->exists(kCartridgeDir));
  TEST_ASSERT_TRUE(fs->exists(kIrDir));
  TEST_ASSERT_OK(ensureLayout(*fs));  // repetir não é erro
}

void test_ensure_layout_reports_failure() {
  MemoryFileSystem other;
  TEST_ASSERT_OK(other.mount());
  other.failMakeDir = 1;
  TEST_ASSERT_STATUS(Status::IoError, ensureLayout(other));
}

void test_read_file() {
  fs->put("/dbmo/ir/a.txt", "conteudo");
  char buffer[32];
  const dbmo::Result<size_t> length = readFile(*fs, "/dbmo/ir/a.txt", buffer, sizeof(buffer));
  TEST_ASSERT_TRUE(length.ok());
  TEST_ASSERT_EQUAL_UINT(8, length.value());
  TEST_ASSERT_EQUAL_MEMORY("conteudo", buffer, 8);

  TEST_ASSERT_STATUS(Status::NotFound, readFile(*fs, "/dbmo/ir/nao.txt", buffer, 32).status());
  TEST_ASSERT_STATUS(Status::InvalidArgument, readFile(*fs, "../x", buffer, 32).status());
  // Arquivo maior que o buffer é erro, não truncamento silencioso.
  TEST_ASSERT_STATUS(Status::NoSpace, readFile(*fs, "/dbmo/ir/a.txt", buffer, 4).status());
  // Tamanho exato do buffer cabe.
  TEST_ASSERT_EQUAL_UINT(8, readFile(*fs, "/dbmo/ir/a.txt", buffer, 8).value());
  fs->failRead = 1;
  TEST_ASSERT_STATUS(Status::IoError, readFile(*fs, "/dbmo/ir/a.txt", buffer, 32).status());
}

void test_atomic_write_creates_file_and_leaves_no_temp() {
  TEST_ASSERT_OK(save("a=1\n"));
  TEST_ASSERT_EQUAL_STRING("a=1\n", fs->text(kFile));
  TEST_ASSERT_FALSE(fs->exists(kTemp));
  TEST_ASSERT_FALSE(fs->exists(kBackup));  // não havia versão anterior
  TEST_ASSERT_TRUE(fs->syncCount > 0);     // sincronizou antes de trocar
}

void test_atomic_write_keeps_previous_version_as_backup() {
  TEST_ASSERT_OK(save("a=1\n"));
  TEST_ASSERT_OK(save("a=2\n"));
  TEST_ASSERT_EQUAL_STRING("a=2\n", fs->text(kFile));
  TEST_ASSERT_EQUAL_STRING("a=1\n", fs->text(kBackup));
  TEST_ASSERT_OK(save("a=3\n"));
  TEST_ASSERT_EQUAL_STRING("a=3\n", fs->text(kFile));
  TEST_ASSERT_EQUAL_STRING("a=2\n", fs->text(kBackup));
}

void test_atomic_write_without_backup() {
  TEST_ASSERT_OK(save("a=1\n", false));
  TEST_ASSERT_OK(save("a=2\n", false));
  TEST_ASSERT_EQUAL_STRING("a=2\n", fs->text(kFile));
  TEST_ASSERT_FALSE(fs->exists(kBackup));
}

void test_failed_write_leaves_the_old_file_untouched() {
  TEST_ASSERT_OK(save("a=1\n"));
  fs->failWrite = 1;
  TEST_ASSERT_STATUS(Status::IoError, save("a=2\n"));
  TEST_ASSERT_EQUAL_STRING("a=1\n", fs->text(kFile));
  TEST_ASSERT_FALSE(fs->exists(kTemp));

  fs->failSync = 1;
  TEST_ASSERT_STATUS(Status::IoError, save("a=2\n"));
  TEST_ASSERT_EQUAL_STRING("a=1\n", fs->text(kFile));
  TEST_ASSERT_FALSE(fs->exists(kTemp));
}

void test_full_card_is_reported_and_old_file_survives() {
  TEST_ASSERT_OK(save("a=1\n"));
  fs->capacityBytes = fs->usedBytes() + 2;  // só cabem 2 bytes
  TEST_ASSERT_STATUS(Status::NoSpace, save("a=22222222\n"));
  TEST_ASSERT_EQUAL_STRING("a=1\n", fs->text(kFile));
  TEST_ASSERT_FALSE(fs->exists(kTemp));
}

void test_failed_final_rename_restores_the_previous_version() {
  TEST_ASSERT_OK(save("a=1\n"));
  fs->renameFaultDelay = 1;  // arquivo -> .bak funciona
  fs->failRename = 1;        // .tmp -> arquivo falha
  TEST_ASSERT_STATUS(Status::IoError, save("a=2\n"));
  TEST_ASSERT_EQUAL_STRING("a=1\n", fs->text(kFile));  // a versão anterior voltou ao lugar
  TEST_ASSERT_FALSE(fs->exists(kTemp));
}

void test_without_backup_a_failed_swap_still_keeps_the_old_file() {
  TEST_ASSERT_OK(save("a=1\n", false));
  fs->renameFaultDelay = 1;  // antigo -> .bak funciona
  fs->failRename = 1;        // .tmp -> arquivo falha
  TEST_ASSERT_STATUS(Status::IoError, save("a=2\n", false));
  TEST_ASSERT_EQUAL_STRING("a=1\n", fs->text(kFile));  // nada se perdeu
  TEST_ASSERT_FALSE(fs->exists(kTemp));
}

void test_read_falls_back_to_backup_after_power_loss_mid_swap() {
  // Estado deixado por uma queda de energia entre os dois renames: só o .bak existe.
  fs->put(kBackup, "a=1\n");
  char buffer[16] = {};
  TEST_ASSERT_STATUS(Status::NotFound, readFile(*fs, kFile, buffer, sizeof(buffer)).status());
  const dbmo::Result<size_t> length = readFileOrBackup(*fs, kFile, buffer, sizeof(buffer));
  TEST_ASSERT_TRUE(length.ok());
  TEST_ASSERT_EQUAL_MEMORY("a=1\n", buffer, 4);
}

void test_rejects_invalid_paths() {
  TEST_ASSERT_STATUS(Status::InvalidArgument, writeFileAtomic(*fs, "/dbmo/../x", "a", 1, true));
  TEST_ASSERT_STATUS(Status::InvalidArgument, writeFileAtomic(*fs, kFile, nullptr, 3, true));
}

// ---- FileConfigBackend ----

void test_config_round_trip_through_the_card() {
  ConfigStore config;
  TEST_ASSERT_OK(config.define(ConfigSpec::integer("display.brightness", 80, 0, 100)));
  TEST_ASSERT_OK(config.define(ConfigSpec::string("device.name", "d-BMO")));
  TEST_ASSERT_OK(config.setInt("display.brightness", 35));
  TEST_ASSERT_OK(config.setString("device.name", "bancada"));

  FileConfigBackend backend(*fs);
  TEST_ASSERT_OK(config.save(backend));
  TEST_ASSERT_NOT_NULL(strstr(fs->text(kFile), "display.brightness=35"));

  ConfigStore other;
  TEST_ASSERT_OK(other.define(ConfigSpec::integer("display.brightness", 80, 0, 100)));
  TEST_ASSERT_OK(other.define(ConfigSpec::string("device.name", "d-BMO")));
  TEST_ASSERT_OK(other.load(backend));
  TEST_ASSERT_EQUAL_INT32(35, other.getInt("display.brightness").value());
  char name[32];
  TEST_ASSERT_OK(other.getString("device.name", name, sizeof(name)));
  TEST_ASSERT_EQUAL_STRING("bancada", name);
  TEST_ASSERT_FALSE(backend.loadedFromBackup());
}

void test_config_load_without_file_is_not_found() {
  FileConfigBackend backend(*fs);
  char buffer[64];
  size_t length = 0;
  TEST_ASSERT_STATUS(Status::NotFound, backend.load(buffer, sizeof(buffer), &length));
}

void test_config_falls_back_to_backup() {
  FileConfigBackend backend(*fs);
  char buffer[64];
  size_t length = 0;

  // Queda de energia entre os dois renames: só o .bak existe.
  fs->put(kBackup, "a=1\n");
  TEST_ASSERT_OK(backend.load(buffer, sizeof(buffer), &length));
  TEST_ASSERT_EQUAL_MEMORY("a=1\n", buffer, 4);
  TEST_ASSERT_TRUE(backend.loadedFromBackup());

  // Arquivo principal vazio (gravação interrompida): usa o .bak.
  fs->put(kFile, "");
  TEST_ASSERT_OK(backend.load(buffer, sizeof(buffer), &length));
  TEST_ASSERT_TRUE(backend.loadedFromBackup());

  // Erro de leitura no principal: usa o .bak.
  fs->put(kFile, "a=2\n");
  fs->failRead = 1;
  TEST_ASSERT_OK(backend.load(buffer, sizeof(buffer), &length));
  TEST_ASSERT_EQUAL_MEMORY("a=1\n", buffer, 4);
  TEST_ASSERT_TRUE(backend.loadedFromBackup());

  // Principal íntegro tem prioridade.
  TEST_ASSERT_OK(backend.load(buffer, sizeof(buffer), &length));
  TEST_ASSERT_EQUAL_MEMORY("a=2\n", buffer, 4);
  TEST_ASSERT_FALSE(backend.loadedFromBackup());
}

void test_config_backend_without_card() {
  MemoryFileSystem empty;  // não montado
  FileConfigBackend backend(empty);
  char buffer[16];
  size_t length = 0;
  TEST_ASSERT_STATUS(Status::NotInitialized, backend.load(buffer, sizeof(buffer), &length));
  TEST_ASSERT_STATUS(Status::NotInitialized, backend.save("a=1\n", 4));
}

void test_config_larger_than_buffer_is_an_error() {
  fs->put(kFile, "chave.muito.longa=1234567890\n");
  FileConfigBackend backend(*fs);
  char buffer[8];
  size_t length = 0;
  TEST_ASSERT_STATUS(Status::NoSpace, backend.load(buffer, sizeof(buffer), &length));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_ensure_layout_creates_the_directories_once);
  RUN_TEST(test_ensure_layout_reports_failure);
  RUN_TEST(test_read_file);
  RUN_TEST(test_atomic_write_creates_file_and_leaves_no_temp);
  RUN_TEST(test_atomic_write_keeps_previous_version_as_backup);
  RUN_TEST(test_atomic_write_without_backup);
  RUN_TEST(test_failed_write_leaves_the_old_file_untouched);
  RUN_TEST(test_full_card_is_reported_and_old_file_survives);
  RUN_TEST(test_failed_final_rename_restores_the_previous_version);
  RUN_TEST(test_without_backup_a_failed_swap_still_keeps_the_old_file);
  RUN_TEST(test_read_falls_back_to_backup_after_power_loss_mid_swap);
  RUN_TEST(test_rejects_invalid_paths);
  RUN_TEST(test_config_round_trip_through_the_card);
  RUN_TEST(test_config_load_without_file_is_not_found);
  RUN_TEST(test_config_falls_back_to_backup);
  RUN_TEST(test_config_backend_without_card);
  RUN_TEST(test_config_larger_than_buffer_is_an_error);
  return UNITY_END();
}
