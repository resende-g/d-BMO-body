#include <cstring>

#include "dbmo/core/log.h"
#include "dbmo/mock/mock_hal.h"
#include "dbmo_test.h"

using dbmo::Logger;
using dbmo::LogHistory;
using dbmo::LogLevel;
using dbmo::Status;
using dbmo::mock::MockClock;
using dbmo::mock::RecordingLogSink;

void setUp() {}
void tearDown() {}

static void test_level_filter_and_formatting() {
  Logger log;
  RecordingLogSink sink;
  MockClock clock;
  clock.setMs(1234);
  log.setClock(&clock);
  TEST_ASSERT_OK(log.addSink(&sink));

  log.log(LogLevel::Info, "radio", "freq=%d Hz", 433920000);
  log.log(LogLevel::Debug, "radio", "descartado");  // nível padrão é Info
  TEST_ASSERT_EQUAL_size_t(1, sink.count);
  TEST_ASSERT_TRUE(sink.contains("freq=433920000 Hz"));

  log.setLevel(LogLevel::Debug);
  log.log(LogLevel::Debug, "radio", "agora passa");
  TEST_ASSERT_EQUAL_size_t(2, sink.count);

  log.setLevel(LogLevel::Error);
  log.log(LogLevel::Warn, "radio", "filtrado");
  TEST_ASSERT_EQUAL_size_t(2, sink.count);
}

static void test_sink_management() {
  Logger log;
  RecordingLogSink sinks[Logger::kMaxSinks + 1];
  TEST_ASSERT_STATUS(Status::InvalidArgument, log.addSink(nullptr));
  for (size_t i = 0; i < Logger::kMaxSinks; ++i) TEST_ASSERT_OK(log.addSink(&sinks[i]));
  TEST_ASSERT_STATUS(Status::AlreadyInitialized, log.addSink(&sinks[0]));
  TEST_ASSERT_STATUS(Status::NoSpace, log.addSink(&sinks[Logger::kMaxSinks]));

  log.removeSink(&sinks[1]);
  log.log(LogLevel::Info, "t", "x");
  TEST_ASSERT_EQUAL_size_t(1, sinks[0].count);
  TEST_ASSERT_EQUAL_size_t(0, sinks[1].count);

  log.clearSinks();
  log.log(LogLevel::Info, "t", "y");
  TEST_ASSERT_EQUAL_size_t(1, sinks[0].count);
}

static void test_long_message_is_truncated_not_overflowed() {
  Logger log;
  RecordingLogSink sink;
  TEST_ASSERT_OK(log.addSink(&sink));
  char big[400];
  memset(big, 'a', sizeof(big) - 1);
  big[sizeof(big) - 1] = '\0';
  log.log(LogLevel::Info, "t", "%s", big);
  TEST_ASSERT_EQUAL_size_t(1, sink.count);
}

namespace {
// Um sink que tenta logar de dentro do write(): o Logger não pode reentrar.
class ReentrantSink final : public dbmo::ILogSink {
 public:
  explicit ReentrantSink(Logger &target) : target_(target) {}
  void write(const dbmo::LogRecord &) override {
    ++calls;
    target_.log(LogLevel::Error, "sink", "reentrada");
  }
  int calls = 0;

 private:
  Logger &target_;
};
}  // namespace

static void test_sink_logging_does_not_recurse() {
  Logger log;
  ReentrantSink sink(log);
  TEST_ASSERT_OK(log.addSink(&sink));
  log.log(LogLevel::Info, "t", "uma vez");
  TEST_ASSERT_EQUAL_INT(1, sink.calls);
}

static void test_history_keeps_the_latest_entries() {
  Logger log;
  LogHistory<3> history;
  MockClock clock;
  log.setClock(&clock);
  TEST_ASSERT_OK(log.addSink(&history));

  for (int i = 0; i < 5; ++i) {
    clock.setMs(static_cast<uint32_t>(100 * i));
    log.log(LogLevel::Warn, "modulo-com-nome-longo", "msg %d", i);
  }
  TEST_ASSERT_EQUAL_size_t(3, history.size());
  TEST_ASSERT_EQUAL_STRING("msg 2", history.at(0).text);
  TEST_ASSERT_EQUAL_STRING("msg 4", history.at(2).text);
  TEST_ASSERT_EQUAL_UINT32(400, history.at(2).timestampMs);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(LogLevel::Warn), static_cast<int>(history.at(0).level));
  // A tag é truncada no tamanho do campo, com terminador.
  TEST_ASSERT_EQUAL_size_t(dbmo::LogEntry::kTagMax - 1, strlen(history.at(0).tag));

  history.clear();
  TEST_ASSERT_EQUAL_size_t(0, history.size());
}

static void test_global_logger_is_a_single_instance() {
  TEST_ASSERT_EQUAL_PTR(&dbmo::logger(), &dbmo::logger());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_level_filter_and_formatting);
  RUN_TEST(test_sink_management);
  RUN_TEST(test_long_message_is_truncated_not_overflowed);
  RUN_TEST(test_sink_logging_does_not_recurse);
  RUN_TEST(test_history_keeps_the_latest_entries);
  RUN_TEST(test_global_logger_is_a_single_instance);
  return UNITY_END();
}
