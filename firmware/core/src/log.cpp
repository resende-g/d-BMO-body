#include "dbmo/core/log.h"

#include <cstdio>
#include <cstring>

namespace dbmo {

const char *toString(LogLevel level) {
  switch (level) {
    case LogLevel::Error: return "E";
    case LogLevel::Warn: return "W";
    case LogLevel::Info: return "I";
    case LogLevel::Debug: return "D";
  }
  return "?";
}

Status Logger::addSink(ILogSink *sink) {
  if (sink == nullptr) return Status::InvalidArgument;
  for (ILogSink *existing : sinks_)
    if (existing == sink) return Status::AlreadyInitialized;
  for (ILogSink *&slot : sinks_) {
    if (slot == nullptr) {
      slot = sink;
      return Status::Ok;
    }
  }
  return Status::NoSpace;
}

void Logger::removeSink(ILogSink *sink) {
  for (ILogSink *&slot : sinks_)
    if (slot == sink) slot = nullptr;
}

void Logger::clearSinks() {
  for (ILogSink *&slot : sinks_) slot = nullptr;
}

void Logger::log(LogLevel level, const char *tag, const char *format, ...) {
  va_list args;
  va_start(args, format);
  vlog(level, tag, format, args);
  va_end(args);
}

void Logger::vlog(LogLevel level, const char *tag, const char *format, va_list args) {
  if (level > level_ || format == nullptr || writing_) return;

  char message[kMessageMax];
  vsnprintf(message, sizeof(message), format, args);

  const LogRecord record{clock_ != nullptr ? clock_->nowMs() : 0, level, tag != nullptr ? tag : "",
                         message};
  writing_ = true;
  for (ILogSink *sink : sinks_)
    if (sink != nullptr) sink->write(record);
  writing_ = false;
}

Logger &logger() {
  static Logger instance;
  return instance;
}

void LogHistoryBase::write(const LogRecord &record) {
  size_t slot;
  if (size_ < capacity_) {
    slot = (head_ + size_) % capacity_;
    ++size_;
  } else {
    slot = head_;
    head_ = (head_ + 1) % capacity_;
  }
  LogEntry &entry = entries_[slot];
  entry.timestampMs = record.timestampMs;
  entry.level = record.level;
  snprintf(entry.tag, sizeof(entry.tag), "%s", record.tag);
  snprintf(entry.text, sizeof(entry.text), "%s", record.message);
}

void LogHistoryBase::clear() {
  head_ = 0;
  size_ = 0;
}

}  // namespace dbmo
