#include <cstring>

#include "dbmo/sim/memory_file_system.h"
#include "dbmo/storage/file_log_sink.h"
#include "dbmo/storage/file_utils.h"
#include "dbmo/storage/path.h"
#include "dbmo_test.h"

using dbmo::LogLevel;
using dbmo::LogRecord;
using dbmo::Status;
using dbmo::sim::MemoryFileSystem;
using namespace dbmo::storage;

namespace {

MemoryFileSystem *fs = nullptr;

void log(FileLogSink &sink, const char *message, LogLevel level = LogLevel::Info,
         uint32_t timestampMs = 1234) {
  sink.write(LogRecord{timestampMs, level, "teste", message});
}

}  // namespace

void setUp() {
  fs = new MemoryFileSystem();
  TEST_ASSERT_OK(fs->mount());
  TEST_ASSERT_OK(ensureLayout(*fs));
}
void tearDown() { delete fs; }

void test_write_only_buffers_and_flush_persists() {
  FileLogSink sink(*fs);
  log(sink, "primeira");
  TEST_ASSERT_TRUE(sink.pending());
  TEST_ASSERT_EQUAL_UINT(0, fs->writeCount);  // write() não toca o cartão
  TEST_ASSERT_OK(sink.flush());
  TEST_ASSERT_FALSE(sink.pending());
  TEST_ASSERT_EQUAL_STRING("1234 I teste: primeira\n", fs->text(kLogFile));
}

void test_appends_across_flushes() {
  FileLogSink sink(*fs);
  log(sink, "um", LogLevel::Warn, 10);
  TEST_ASSERT_OK(sink.flush());
  log(sink, "dois", LogLevel::Error, 20);
  TEST_ASSERT_OK(sink.flush());
  TEST_ASSERT_EQUAL_STRING("10 W teste: um\n20 E teste: dois\n", fs->text(kLogFile));
}

void test_min_level_filters() {
  FileLogSink sink(*fs);
  log(sink, "detalhe", LogLevel::Debug);
  TEST_ASSERT_FALSE(sink.pending());  // padrão: Info
  sink.setMinLevel(LogLevel::Error);
  log(sink, "aviso", LogLevel::Warn);
  TEST_ASSERT_FALSE(sink.pending());
  log(sink, "erro", LogLevel::Error);
  TEST_ASSERT_TRUE(sink.pending());
}

void test_disabled_sink_stores_nothing() {
  FileLogSink sink(*fs);
  log(sink, "antes");
  sink.setEnabled(false);
  TEST_ASSERT_FALSE(sink.pending());  // desabilitar descarta o que estava na memória
  log(sink, "depois");
  TEST_ASSERT_OK(sink.flush());
  TEST_ASSERT_FALSE(fs->exists(kLogFile));
}

void test_rotation_keeps_one_previous_file() {
  FileLogSink sink(*fs);
  sink.setMaxBytes(100);
  for (int i = 0; i < 3; ++i) log(sink, "linha de log com tamanho razoavel");  // ~46 bytes cada
  TEST_ASSERT_OK(sink.flush());
  TEST_ASSERT_FALSE(fs->exists(kLogFileOld));  // primeiro arquivo: sem rotação

  log(sink, "mais uma");
  TEST_ASSERT_OK(sink.flush());
  TEST_ASSERT_TRUE(fs->exists(kLogFileOld));
  TEST_ASSERT_EQUAL_STRING("1234 I teste: mais uma\n", fs->text(kLogFile));

  // Segunda rotação: "mais uma" desce para o log.1.txt...
  for (int i = 0; i < 3; ++i) log(sink, "linha de log com tamanho razoavel");
  TEST_ASSERT_OK(sink.flush());
  TEST_ASSERT_NOT_NULL(strstr(fs->text(kLogFileOld), "mais uma"));
  // ...e na terceira o log.1.txt antigo é substituído, não acumulado.
  log(sink, "ultima");
  TEST_ASSERT_OK(sink.flush());
  TEST_ASSERT_EQUAL_STRING("1234 I teste: ultima\n", fs->text(kLogFile));
  TEST_ASSERT_NULL(strstr(fs->text(kLogFileOld), "mais uma"));
  TEST_ASSERT_NOT_NULL(strstr(fs->text(kLogFileOld), "tamanho razoavel"));
}

void test_flush_failure_keeps_lines_for_the_next_attempt() {
  FileLogSink sink(*fs);
  log(sink, "importante");
  fs->failWrite = 1;
  TEST_ASSERT_STATUS(Status::IoError, sink.flush());
  TEST_ASSERT_TRUE(sink.pending());
  TEST_ASSERT_OK(sink.flush());
  TEST_ASSERT_NOT_NULL(strstr(fs->text(kLogFile), "importante"));
}

void test_overflow_drops_lines_and_the_file_records_the_gap() {
  FileLogSink sink(*fs);
  for (int i = 0; i < 200; ++i) log(sink, "linha que enche a memoria do destino de log");
  TEST_ASSERT_TRUE(sink.droppedLines() > 0);
  const uint32_t dropped = sink.droppedLines();
  TEST_ASSERT_OK(sink.flush());
  char expected[64];
  snprintf(expected, sizeof(expected), "# %lu linhas de log descartadas",
           static_cast<unsigned long>(dropped));
  TEST_ASSERT_NOT_NULL(strstr(fs->text(kLogFile), expected));
  // A lacuna é registrada uma vez só.
  log(sink, "depois");
  TEST_ASSERT_OK(sink.flush());
  const char *first = strstr(fs->text(kLogFile), "descartadas");
  TEST_ASSERT_NULL(strstr(first + 1, "descartadas"));
}

void test_oversized_message_is_truncated_with_newline() {
  FileLogSink sink(*fs);
  char message[400];
  memset(message, 'x', sizeof(message) - 1);
  message[sizeof(message) - 1] = '\0';
  log(sink, message);
  TEST_ASSERT_OK(sink.flush());
  const char *text = fs->text(kLogFile);
  const size_t length = strlen(text);
  TEST_ASSERT_TRUE(length < 200);
  TEST_ASSERT_EQUAL_CHAR('\n', text[length - 1]);
}

void test_flush_without_card() {
  MemoryFileSystem empty;  // não montado
  FileLogSink sink(empty);
  TEST_ASSERT_OK(sink.flush());  // nada pendente
  log(sink, "sem cartao");
  TEST_ASSERT_STATUS(Status::NotInitialized, sink.flush());
  TEST_ASSERT_TRUE(sink.pending());
}

void test_null_tag_and_message_do_not_crash() {
  FileLogSink sink(*fs);
  sink.write(LogRecord{1, LogLevel::Info, nullptr, nullptr});
  TEST_ASSERT_OK(sink.flush());
  TEST_ASSERT_EQUAL_STRING("1 I -: \n", fs->text(kLogFile));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_write_only_buffers_and_flush_persists);
  RUN_TEST(test_appends_across_flushes);
  RUN_TEST(test_min_level_filters);
  RUN_TEST(test_disabled_sink_stores_nothing);
  RUN_TEST(test_rotation_keeps_one_previous_file);
  RUN_TEST(test_flush_failure_keeps_lines_for_the_next_attempt);
  RUN_TEST(test_overflow_drops_lines_and_the_file_records_the_gap);
  RUN_TEST(test_oversized_message_is_truncated_with_newline);
  RUN_TEST(test_flush_without_card);
  RUN_TEST(test_null_tag_and_message_do_not_crash);
  return UNITY_END();
}
