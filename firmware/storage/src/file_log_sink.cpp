#include "dbmo/storage/file_log_sink.h"

#include <cstdio>
#include <cstring>

#include "dbmo/storage/path.h"

namespace dbmo::storage {

void FileLogSink::setEnabled(bool enabled) {
  enabled_ = enabled;
  if (!enabled) used_ = 0;
}

void FileLogSink::write(const LogRecord &record) {
  if (!enabled_ || static_cast<uint8_t>(record.level) > static_cast<uint8_t>(minLevel_)) return;

  char line[Logger::kMessageMax + 48];
  int length = snprintf(line, sizeof(line), "%lu %c %s: %s\n",
                        static_cast<unsigned long>(record.timestampMs), toString(record.level)[0],
                        record.tag != nullptr ? record.tag : "-",
                        record.message != nullptr ? record.message : "");
  if (length < 0) return;
  if (static_cast<size_t>(length) >= sizeof(line)) {
    length = static_cast<int>(sizeof(line)) - 1;
    line[length - 1] = '\n';
  }
  if (used_ + static_cast<size_t>(length) > kBufferSize) {
    ++dropped_;
    return;
  }
  memcpy(buffer_ + used_, line, static_cast<size_t>(length));
  used_ += static_cast<size_t>(length);
}

Status FileLogSink::rotateIfNeeded() {
  if (!fs_.exists(kLogFile)) return Status::Ok;
  const Result<uint32_t> size = fs_.fileSize(kLogFile);
  if (!size.ok()) return size.status();
  if (static_cast<uint64_t>(size.value()) + used_ <= maxBytes_) return Status::Ok;
  if (fs_.exists(kLogFileOld)) DBMO_TRY(fs_.remove(kLogFileOld));
  return fs_.rename(kLogFile, kLogFileOld);
}

Status FileLogSink::flush() {
  if (used_ == 0 && dropped_ == reportedDropped_) return Status::Ok;
  if (!fs_.mounted()) return Status::NotInitialized;
  DBMO_TRY(rotateIfNeeded());

  const Result<FileHandle> file = fs_.open(kLogFile, OpenMode::Append);
  if (!file.ok()) return file.status();

  Status status = Status::Ok;
  if (dropped_ != reportedDropped_) {
    // O arquivo registra a própria lacuna, para ninguém ler o log como se fosse completo.
    char note[64];
    const int length = snprintf(note, sizeof(note), "# %lu linhas de log descartadas\n",
                                static_cast<unsigned long>(dropped_ - reportedDropped_));
    const Result<size_t> written = fs_.write(file.value(), note, static_cast<size_t>(length));
    if (!written.ok()) {
      status = written.status();
    } else if (written.value() != static_cast<size_t>(length)) {
      status = Status::NoSpace;
    } else {
      reportedDropped_ = dropped_;
    }
  }
  if (isOk(status) && used_ > 0) {
    const Result<size_t> written = fs_.write(file.value(), buffer_, used_);
    if (!written.ok()) {
      status = written.status();
    } else if (written.value() != used_) {
      status = Status::NoSpace;
    }
  }
  if (isOk(status)) status = fs_.sync(file.value());
  const Status closed = fs_.close(file.value());
  if (isOk(status)) status = closed;
  if (isOk(status)) used_ = 0;
  return status;
}

}  // namespace dbmo::storage
