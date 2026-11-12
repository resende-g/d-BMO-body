#include <cstring>

#include "dbmo/core/log.h"
#include "dbmo/sim/memory_file_system.h"
#include "dbmo/storage/path.h"
#include "dbmo/storage/storage_subsystem.h"
#include "dbmo_test.h"

using dbmo::ConfigStore;
using dbmo::Event;
using dbmo::EventBus;
using dbmo::LogLevel;
using dbmo::LogRecord;
using dbmo::Status;
using dbmo::sim::MemoryFileSystem;
using namespace dbmo::storage;

namespace {

struct Fixture {
  MemoryFileSystem fs;
  EventBus bus;
  ConfigStore config;
  StorageSubsystem storage{fs, bus, config};
  Event events[64];
  int count = 0;
  uint32_t now = 0;

  Fixture() {
    TEST_ASSERT_OK(StorageSubsystem::registerConfig(config));
    TEST_ASSERT_OK(bus.subscribeAll(&Fixture::collect, this));
  }
  static void collect(const Event &event, void *context) {
    auto *self = static_cast<Fixture *>(context);
    if (self->count < 64) self->events[self->count++] = event;
  }
  void run(uint32_t durationMs, uint32_t stepMs = 100) {
    for (uint32_t elapsed = 0; elapsed < durationMs; elapsed += stepMs) {
      now += stepMs;
      storage.tick(now);
      bus.dispatch();
    }
  }
  int countOf(dbmo::EventId id) const {
    int matches = 0;
    for (int i = 0; i < count; ++i)
      if (events[i].id == id) ++matches;
    return matches;
  }
  void log(const char *message) {
    storage.logSink().write(LogRecord{now, LogLevel::Info, "teste", message});
  }
};

Fixture *f = nullptr;

}  // namespace

void setUp() { f = new Fixture(); }
void tearDown() { delete f; }

void test_init_mounts_and_creates_the_layout() {
  TEST_ASSERT_OK(f->storage.init());
  f->bus.dispatch();
  TEST_ASSERT_TRUE(f->storage.available());
  TEST_ASSERT_TRUE(f->fs.exists(kConfigDir));
  TEST_ASSERT_TRUE(f->fs.exists(kLogDir));
  TEST_ASSERT_TRUE(f->fs.exists(kCartridgeDir));
  TEST_ASSERT_TRUE(f->fs.exists(kIrDir));
  TEST_ASSERT_EQUAL_INT(1, f->countOf(storage_event::kAvailable));
}

void test_init_without_card_is_hardware_absent() {
  f->fs.cardPresent = false;
  TEST_ASSERT_STATUS(Status::HardwareAbsent, f->storage.init());
  TEST_ASSERT_FALSE(f->storage.available());
  f->bus.dispatch();
  TEST_ASSERT_EQUAL_INT(0, f->countOf(storage_event::kAvailable));
}

void test_init_fails_cleanly_when_layout_cannot_be_created() {
  f->fs.failMakeDir = 1;
  TEST_ASSERT_STATUS(Status::IoError, f->storage.init());
  TEST_ASSERT_FALSE(f->storage.available());
  TEST_ASSERT_FALSE(f->fs.mounted());
  // Uma nova tentativa (retry do System) funciona.
  TEST_ASSERT_OK(f->storage.init());
  TEST_ASSERT_TRUE(f->storage.available());
}

void test_log_is_flushed_in_the_background() {
  TEST_ASSERT_OK(f->storage.init());
  f->log("evento");
  f->run(1000);
  TEST_ASSERT_FALSE(f->fs.exists(kLogFile));  // ainda não deu o período
  f->run(1500);
  TEST_ASSERT_NOT_NULL(strstr(f->fs.text(kLogFile), "evento"));
}

void test_card_inserted_later_is_picked_up() {
  f->fs.cardPresent = false;
  TEST_ASSERT_STATUS(Status::HardwareAbsent, f->storage.init());
  f->run(12000);
  TEST_ASSERT_FALSE(f->storage.available());
  f->fs.insertCard();
  f->run(6000);
  TEST_ASSERT_TRUE(f->storage.available());
  TEST_ASSERT_EQUAL_INT(1, f->countOf(storage_event::kAvailable));
  TEST_ASSERT_TRUE(f->fs.exists(kIrDir));
}

void test_card_removed_in_use_is_detected_and_recovers() {
  TEST_ASSERT_OK(f->storage.init());
  f->run(3000);
  f->fs.ejectCard();
  f->run(6000);
  TEST_ASSERT_FALSE(f->storage.available());
  TEST_ASSERT_EQUAL_INT(1, f->countOf(storage_event::kUnavailable));

  // O log continua sendo aceito na memória e é gravado quando o cartão volta.
  f->log("durante a ausencia");
  f->run(10000);
  TEST_ASSERT_EQUAL_INT(1, f->countOf(storage_event::kUnavailable));  // sem repetir

  f->fs.insertCard();
  f->run(8000);
  TEST_ASSERT_TRUE(f->storage.available());
  TEST_ASSERT_EQUAL_INT(2, f->countOf(storage_event::kAvailable));
  TEST_ASSERT_NOT_NULL(strstr(f->fs.text(kLogFile), "durante a ausencia"));
}

void test_repeated_write_failures_mark_the_card_unavailable() {
  TEST_ASSERT_OK(f->storage.init());
  f->fs.failWrite = 100;
  f->log("nunca grava");
  f->run(2 * StorageSubsystem::kFlushPeriodMs + 200);
  TEST_ASSERT_TRUE(f->storage.available());  // duas falhas: ainda tolera
  f->run(StorageSubsystem::kFlushPeriodMs);
  TEST_ASSERT_FALSE(f->storage.available());
  TEST_ASSERT_EQUAL_INT(1, f->countOf(storage_event::kUnavailable));
  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(Status::IoError), f->events[f->count - 1].a);
}

void test_card_that_mounts_but_never_writes_is_retried_with_backoff() {
  TEST_ASSERT_OK(f->storage.init());
  f->fs.failWrite = 100000;
  f->log("nunca grava");
  f->run(10 * 60 * 1000, 500);
  // Sem a espera crescente seriam dezenas de ciclos monta/falha em dez minutos.
  const int cycles = f->countOf(storage_event::kUnavailable);
  TEST_ASSERT_TRUE(cycles >= 2);
  TEST_ASSERT_TRUE(cycles <= 8);
}

void test_one_write_failure_is_tolerated() {
  TEST_ASSERT_OK(f->storage.init());
  f->fs.failWrite = 1;
  f->log("tenta de novo");
  f->run(3 * StorageSubsystem::kFlushPeriodMs);
  TEST_ASSERT_TRUE(f->storage.available());
  TEST_ASSERT_NOT_NULL(strstr(f->fs.text(kLogFile), "tenta de novo"));
}

void test_shutdown_flushes_and_unmounts() {
  TEST_ASSERT_OK(f->storage.init());
  f->log("ultima linha");
  TEST_ASSERT_OK(f->storage.shutdown());
  TEST_ASSERT_FALSE(f->fs.mounted());
  TEST_ASSERT_FALSE(f->storage.available());
  TEST_ASSERT_OK(f->fs.mount());
  TEST_ASSERT_NOT_NULL(strstr(f->fs.text(kLogFile), "ultima linha"));
}

void test_log_can_be_disabled_by_config() {
  TEST_ASSERT_OK(f->config.setBool(config_key::kLogEnabled, false));
  TEST_ASSERT_OK(f->storage.init());
  f->log("nao deve aparecer");
  f->run(5000);
  TEST_ASSERT_FALSE(f->fs.exists(kLogFile));
}

void test_log_size_comes_from_config() {
  TEST_ASSERT_OK(f->config.setInt(config_key::kLogMaxKb, 16));
  TEST_ASSERT_OK(f->storage.init());
  // 16 KiB de limite: o simulador guarda até 8 KiB por arquivo, então a rotação não
  // ocorre aqui; o teste confere só que a configuração foi aceita e o log funciona.
  f->log("ok");
  f->run(2500);
  TEST_ASSERT_NOT_NULL(strstr(f->fs.text(kLogFile), "ok"));
  TEST_ASSERT_FALSE(f->fs.exists(kLogFileOld));
}

void test_config_backend_uses_the_card() {
  TEST_ASSERT_OK(f->storage.init());
  TEST_ASSERT_OK(f->config.save(f->storage.configBackend()));
  TEST_ASSERT_NOT_NULL(strstr(f->fs.text(kConfigFile), "storage.log_enabled=1"));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_init_mounts_and_creates_the_layout);
  RUN_TEST(test_init_without_card_is_hardware_absent);
  RUN_TEST(test_init_fails_cleanly_when_layout_cannot_be_created);
  RUN_TEST(test_log_is_flushed_in_the_background);
  RUN_TEST(test_card_inserted_later_is_picked_up);
  RUN_TEST(test_card_removed_in_use_is_detected_and_recovers);
  RUN_TEST(test_repeated_write_failures_mark_the_card_unavailable);
  RUN_TEST(test_card_that_mounts_but_never_writes_is_retried_with_backoff);
  RUN_TEST(test_one_write_failure_is_tolerated);
  RUN_TEST(test_shutdown_flushes_and_unmounts);
  RUN_TEST(test_log_can_be_disabled_by_config);
  RUN_TEST(test_log_size_comes_from_config);
  RUN_TEST(test_config_backend_uses_the_card);
  return UNITY_END();
}
