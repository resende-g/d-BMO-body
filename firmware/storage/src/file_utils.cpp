#include "dbmo/storage/file_utils.h"

#include <cstdio>

#include "dbmo/storage/path.h"

namespace dbmo::storage {

namespace {

Status sibling(const char *path, const char *suffix, char *out, size_t capacity) {
  const int written = snprintf(out, capacity, "%s%s", path, suffix);
  if (written < 0 || static_cast<size_t>(written) >= capacity) return Status::NoSpace;
  return isValidPath(out) ? Status::Ok : Status::InvalidArgument;
}

Status writeAll(IFileSystem &fs, const char *path, const void *data, size_t n) {
  const Result<FileHandle> file = fs.open(path, OpenMode::Write);
  if (!file.ok()) return file.status();
  Status status = Status::Ok;
  const Result<size_t> written = fs.write(file.value(), data, n);
  if (!written.ok()) {
    status = written.status();
  } else if (written.value() != n) {
    status = Status::NoSpace;
  } else {
    status = fs.sync(file.value());
  }
  const Status closed = fs.close(file.value());
  return isOk(status) ? closed : status;
}

}  // namespace

Result<size_t> readFile(IFileSystem &fs, const char *path, void *buffer, size_t capacity) {
  if (!isValidPath(path) || buffer == nullptr) return Status::InvalidArgument;
  const Result<FileHandle> file = fs.open(path, OpenMode::Read);
  if (!file.ok()) return file.status();

  auto *bytes = static_cast<uint8_t *>(buffer);
  size_t total = 0;
  Status status = Status::Ok;
  for (;;) {
    if (total == capacity) {
      // Buffer cheio: o arquivo só coube se não há mais nada para ler.
      uint8_t extra = 0;
      const Result<size_t> more = fs.read(file.value(), &extra, 1);
      if (!more.ok()) {
        status = more.status();
      } else if (more.value() != 0) {
        status = Status::NoSpace;
      }
      break;
    }
    const Result<size_t> chunk = fs.read(file.value(), bytes + total, capacity - total);
    if (!chunk.ok()) {
      status = chunk.status();
      break;
    }
    if (chunk.value() == 0) break;
    total += chunk.value();
  }
  (void)fs.close(file.value());
  if (!isOk(status)) return status;
  return total;
}

Status writeFileAtomic(IFileSystem &fs, const char *path, const void *data, size_t n,
                       bool keepBackup) {
  if (!isValidPath(path) || (data == nullptr && n != 0)) return Status::InvalidArgument;
  char temporary[kMaxPath];
  char backup[kMaxPath];
  DBMO_TRY(sibling(path, ".tmp", temporary, sizeof(temporary)));
  DBMO_TRY(sibling(path, ".bak", backup, sizeof(backup)));

  const Status written = writeAll(fs, temporary, data, n);
  if (!isOk(written)) {
    (void)fs.remove(temporary);
    return written;
  }

  bool movedToBackup = false;
  if (fs.exists(path)) {
    if (keepBackup) {
      if (fs.exists(backup)) DBMO_TRY(fs.remove(backup));
      DBMO_TRY(fs.rename(path, backup));
      movedToBackup = true;
    } else {
      DBMO_TRY(fs.remove(path));
    }
  }

  const Status renamed = fs.rename(temporary, path);
  if (!isOk(renamed)) {
    // Devolve a versão anterior ao lugar; se nem isso der certo, ela continua em .bak.
    if (movedToBackup) (void)fs.rename(backup, path);
    (void)fs.remove(temporary);
  }
  return renamed;
}

Status ensureLayout(IFileSystem &fs) {
  static const char *const kDirs[] = {kRootDir, kConfigDir, kLogDir, kCartridgeDir, kIrDir};
  for (const char *dir : kDirs)
    if (!fs.exists(dir)) DBMO_TRY(fs.makeDir(dir));
  return Status::Ok;
}

}  // namespace dbmo::storage
