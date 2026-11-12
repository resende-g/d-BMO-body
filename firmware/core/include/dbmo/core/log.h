// Registro de eventos do firmware: níveis, destinos (sinks) e histórico em RAM.
//
// O Logger roda na tarefa principal. Não chamar de dentro de interrupção.
#pragma once

#include <cstdarg>
#include <cstddef>
#include <cstdint>

#include "dbmo/core/hal/clock.h"
#include "dbmo/core/status.h"

namespace dbmo {

enum class LogLevel : uint8_t { Error = 0, Warn, Info, Debug };

const char *toString(LogLevel level);

struct LogRecord {
  uint32_t timestampMs;
  LogLevel level;
  const char *tag;      // válido só durante write()
  const char *message;  // válido só durante write()
};

class ILogSink {
 public:
  virtual ~ILogSink() = default;
  virtual void write(const LogRecord &record) = 0;
};

class Logger {
 public:
  static constexpr size_t kMaxSinks = 4;
  static constexpr size_t kMessageMax = 128;  // mensagens maiores são truncadas

  void setClock(IClock *clock) { clock_ = clock; }
  void setLevel(LogLevel level) { level_ = level; }
  LogLevel level() const { return level_; }

  Status addSink(ILogSink *sink);
  void removeSink(ILogSink *sink);
  void clearSinks();

  void log(LogLevel level, const char *tag, const char *format, ...)
      __attribute__((format(printf, 4, 5)));
  void vlog(LogLevel level, const char *tag, const char *format, va_list args);

 private:
  IClock *clock_ = nullptr;
  LogLevel level_ = LogLevel::Info;
  ILogSink *sinks_[kMaxSinks] = {};
  bool writing_ = false;  // um sink que loga não reentra
};

// Logger global do firmware.
Logger &logger();

// Histórico em RAM dos últimos N registros, para o console e para gravar no cartão.
struct LogEntry {
  static constexpr size_t kTagMax = 12;
  static constexpr size_t kTextMax = 96;
  uint32_t timestampMs = 0;
  LogLevel level = LogLevel::Info;
  char tag[kTagMax] = {};
  char text[kTextMax] = {};
};

class LogHistoryBase : public ILogSink {
 public:
  void write(const LogRecord &record) override;
  size_t size() const { return size_; }
  size_t capacity() const { return capacity_; }
  // Registro index a partir do mais antigo (0). Requer index < size().
  const LogEntry &at(size_t index) const { return entries_[(head_ + index) % capacity_]; }
  void clear();

 protected:
  LogHistoryBase(LogEntry *entries, size_t capacity) : entries_(entries), capacity_(capacity) {}

 private:
  LogEntry *entries_;
  size_t capacity_;
  size_t head_ = 0;
  size_t size_ = 0;
};

template <size_t N>
class LogHistory final : public LogHistoryBase {
 public:
  LogHistory() : LogHistoryBase(storage_, N) {}

 private:
  LogEntry storage_[N];
};

}  // namespace dbmo

#define DBMO_LOGE(tag, ...) ::dbmo::logger().log(::dbmo::LogLevel::Error, tag, __VA_ARGS__)
#define DBMO_LOGW(tag, ...) ::dbmo::logger().log(::dbmo::LogLevel::Warn, tag, __VA_ARGS__)
#define DBMO_LOGI(tag, ...) ::dbmo::logger().log(::dbmo::LogLevel::Info, tag, __VA_ARGS__)
#define DBMO_LOGD(tag, ...) ::dbmo::logger().log(::dbmo::LogLevel::Debug, tag, __VA_ARGS__)
