#include "dbmo/core/version.h"

#include <cstdio>

namespace dbmo {

Result<Version> parseVersion(const char *text) {
  if (text == nullptr || *text == '\0') return Status::InvalidArgument;

  uint8_t parts[3] = {0, 0, 0};
  int part = 0;
  const char *cursor = text;
  while (true) {
    if (*cursor < '0' || *cursor > '9') return Status::InvalidArgument;
    unsigned value = 0;
    while (*cursor >= '0' && *cursor <= '9') {
      value = value * 10 + static_cast<unsigned>(*cursor - '0');
      if (value > 255) return Status::OutOfRange;
      ++cursor;
    }
    parts[part++] = static_cast<uint8_t>(value);
    if (*cursor == '\0') break;
    if (*cursor != '.' || part == 3) return Status::InvalidArgument;
    ++cursor;
  }
  return Version{parts[0], parts[1], parts[2]};
}

size_t formatVersion(const Version &version, char *out, size_t capacity) {
  if (out == nullptr || capacity == 0) return 0;
  const int written =
      snprintf(out, capacity, "%u.%u.%u", static_cast<unsigned>(version.major),
               static_cast<unsigned>(version.minor), static_cast<unsigned>(version.patch));
  if (written < 0 || static_cast<size_t>(written) >= capacity) {
    out[0] = '\0';
    return 0;
  }
  return static_cast<size_t>(written);
}

}  // namespace dbmo
