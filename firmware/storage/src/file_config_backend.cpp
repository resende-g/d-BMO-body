#include "dbmo/storage/file_config_backend.h"

#include <cstdio>

#include "dbmo/storage/file_utils.h"

namespace dbmo::storage {

Status FileConfigBackend::load(char *buffer, size_t capacity, size_t *length) {
  if (buffer == nullptr || length == nullptr) return Status::InvalidArgument;
  fromBackup_ = false;
  if (!fs_.mounted()) return Status::NotInitialized;

  const Result<size_t> primary = readFile(fs_, path_, buffer, capacity);
  if (primary.ok() && primary.value() > 0) {
    *length = primary.value();
    return Status::Ok;
  }

  char backup[kMaxPath];
  const int written = snprintf(backup, sizeof(backup), "%s.bak", path_);
  if (written < 0 || static_cast<size_t>(written) >= sizeof(backup)) return Status::NoSpace;
  const Result<size_t> previous = readFile(fs_, backup, buffer, capacity);
  if (previous.ok() && previous.value() > 0) {
    *length = previous.value();
    fromBackup_ = true;
    return Status::Ok;
  }

  // Nenhum dos dois serviu: o erro que importa é o do arquivo principal, salvo quando
  // ele simplesmente não existe ou está vazio.
  if (!primary.ok() && primary.status() != Status::NotFound) return primary.status();
  return Status::NotFound;
}

Status FileConfigBackend::save(const char *text, size_t length) {
  if (!fs_.mounted()) return Status::NotInitialized;
  return writeFileAtomic(fs_, path_, text, length, true);
}

}  // namespace dbmo::storage
