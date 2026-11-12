#include "dbmo/storage/path.h"

#include <cstdio>
#include <cstring>

namespace dbmo::storage {

namespace {

bool allowed(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '.' ||
         c == '_' || c == '-';
}

bool validComponent(const char *text, size_t length) {
  if (length == 0 || length > kMaxComponent) return false;
  if (length == 1 && text[0] == '.') return false;
  if (length == 2 && text[0] == '.' && text[1] == '.') return false;
  for (size_t i = 0; i < length; ++i)
    if (!allowed(text[i])) return false;
  return true;
}

}  // namespace

bool isValidName(const char *name) {
  if (name == nullptr) return false;
  const size_t length = strnlen(name, kMaxComponent + 1);
  return validComponent(name, length);
}

bool isValidPath(const char *path) {
  if (path == nullptr || path[0] != '/') return false;
  const size_t length = strnlen(path, kMaxPath);
  if (length >= kMaxPath) return false;
  if (length == 1) return true;  // raiz

  size_t start = 1;
  while (start <= length) {
    size_t end = start;
    while (end < length && path[end] != '/') ++end;
    if (!validComponent(path + start, end - start)) return false;
    start = end + 1;
  }
  return path[length - 1] != '/';
}

Status joinPath(char *out, size_t capacity, const char *directory, const char *name) {
  if (out == nullptr || !isValidPath(directory) || !isValidName(name))
    return Status::InvalidArgument;
  const bool root = directory[1] == '\0';
  const int written = snprintf(out, capacity, "%s/%s", root ? "" : directory, name);
  if (written < 0 || static_cast<size_t>(written) >= capacity) return Status::NoSpace;
  return isValidPath(out) ? Status::Ok : Status::InvalidArgument;
}

Status cartridgeDataDir(uint16_t vendorId, uint16_t productId, char *out, size_t capacity) {
  if (out == nullptr) return Status::InvalidArgument;
  const int written = snprintf(out, capacity, "%s/%04x-%04x", kCartridgeDir,
                               static_cast<unsigned>(vendorId), static_cast<unsigned>(productId));
  if (written < 0 || static_cast<size_t>(written) >= capacity) return Status::NoSpace;
  return Status::Ok;
}

}  // namespace dbmo::storage
