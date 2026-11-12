// Versão do firmware e comparação de versões (usada também pelos cartuchos).
#pragma once

#include <cstddef>
#include <cstdint>

#include "dbmo/core/status.h"

namespace dbmo {

struct Version {
  uint8_t major = 0;
  uint8_t minor = 0;
  uint8_t patch = 0;

  constexpr int compare(const Version &other) const {
    if (major != other.major) return major < other.major ? -1 : 1;
    if (minor != other.minor) return minor < other.minor ? -1 : 1;
    if (patch != other.patch) return patch < other.patch ? -1 : 1;
    return 0;
  }
  constexpr bool operator==(const Version &other) const { return compare(other) == 0; }
  constexpr bool operator!=(const Version &other) const { return compare(other) != 0; }
  constexpr bool operator<(const Version &other) const { return compare(other) < 0; }
  constexpr bool operator>=(const Version &other) const { return compare(other) >= 0; }
};

inline constexpr Version kFirmwareVersion{0, 1, 0};
inline constexpr char kHardwareName[] = "d-BMO v1";

// Lê "1.2.3" (também aceita "1.2" e "1"). Cada parte de 0 a 255.
Result<Version> parseVersion(const char *text);

// Escreve "major.minor.patch" em out; devolve o tamanho, ou 0 se não coube.
size_t formatVersion(const Version &version, char *out, size_t capacity);

}  // namespace dbmo
