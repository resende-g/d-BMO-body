#include "dbmo/infrared/ir_library.h"

#include <cstdio>
#include <cstring>

#include "dbmo/infrared/ir_codec.h"

namespace dbmo::infrared {

namespace {

constexpr uint32_t kMinCarrierHz = 30000;
constexpr uint32_t kMaxCarrierHz = 60000;

// Cursor sobre o texto, sem depender de terminador.
struct Cursor {
  const char *text;
  size_t length;
  size_t position;

  bool atEnd() const { return position >= length; }
  bool take(char expected) {
    if (atEnd() || text[position] != expected) return false;
    ++position;
    return true;
  }
  bool takeWord(const char *word) {
    const size_t size = strlen(word);
    if (length - position < size || memcmp(text + position, word, size) != 0) return false;
    position += size;
    return true;
  }
  // Número sem sinal na base dada; falha se não há dígito ou se passa de limit.
  bool takeNumber(unsigned base, uint32_t limit, uint32_t *out) {
    uint64_t value = 0;
    size_t digits = 0;
    while (!atEnd()) {
      const char c = text[position];
      unsigned digit;
      if (c >= '0' && c <= '9') {
        digit = static_cast<unsigned>(c - '0');
      } else if (base == 16 && c >= 'a' && c <= 'f') {
        digit = static_cast<unsigned>(c - 'a') + 10;
      } else if (base == 16 && c >= 'A' && c <= 'F') {
        digit = static_cast<unsigned>(c - 'A') + 10;
      } else {
        break;
      }
      value = value * base + digit;
      if (value > limit) return false;
      ++position;
      ++digits;
    }
    if (digits == 0) return false;
    *out = static_cast<uint32_t>(value);
    return true;
  }
};

// Acrescenta prefixo + número decimal + sufixo.
bool append(char *out, size_t capacity, size_t *used, const char *prefix, unsigned long value,
            const char *suffix) {
  const int written = snprintf(out + *used, capacity - *used, "%s%lu%s", prefix, value, suffix);
  if (written < 0 || static_cast<size_t>(written) >= capacity - *used) return false;
  *used += static_cast<size_t>(written);
  return true;
}

}  // namespace

bool isValidCodeName(const char *name) {
  if (name == nullptr) return false;
  size_t length = 0;
  for (; name[length] != '\0'; ++length) {
    if (length >= kMaxCodeNameLength) return false;
    const char c = name[length];
    if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-')) return false;
  }
  return length > 0;
}

Result<size_t> formatCode(const StoredCode &code, char *out, size_t capacity) {
  if (out == nullptr || capacity == 0) return Status::InvalidArgument;
  size_t used = 0;
  if (code.isRaw) {
    if (code.raw.count == 0 || code.raw.count > RawTiming::kMaxEntries)
      return Status::InvalidArgument;
    if (!append(out, capacity, &used, "raw:", code.raw.carrierHz, ":")) return Status::NoSpace;
    for (uint16_t i = 0; i < code.raw.count; ++i)
      if (!append(out, capacity, &used, i == 0 ? "" : ",", code.raw.durationsUs[i], ""))
        return Status::NoSpace;
    return used;
  }

  int written = -1;
  if (code.code.protocol == IrProtocol::Nec) {
    // "necx" marca endereço estendido de 16 bits, inclusive quando o byte alto é zero.
    written = snprintf(out, capacity, "%s:%04X:%02X",
                       code.code.extended || code.code.address > 0xFF ? "necx" : "nec",
                       static_cast<unsigned>(code.code.address),
                       static_cast<unsigned>(code.code.command));
  } else if (code.code.protocol == IrProtocol::Sony) {
    written = snprintf(out, capacity, "sony:%u:%04X:%02X", static_cast<unsigned>(code.code.bits),
                       static_cast<unsigned>(code.code.address),
                       static_cast<unsigned>(code.code.command));
  } else {
    return Status::InvalidArgument;
  }
  if (written < 0 || static_cast<size_t>(written) >= capacity) return Status::NoSpace;
  return static_cast<size_t>(written);
}

Status parseCode(const char *text, size_t length, StoredCode *out) {
  if (text == nullptr || out == nullptr) return Status::InvalidArgument;
  // Ignora espaço e quebra de linha no fim (arquivo editado à mão).
  while (length > 0 && (text[length - 1] == '\n' || text[length - 1] == '\r' ||
                        text[length - 1] == ' ' || text[length - 1] == '\t'))
    --length;
  Cursor cursor{text, length, 0};
  StoredCode result;
  uint32_t value = 0;

  const bool necExtended = cursor.takeWord("necx:");
  if (necExtended || cursor.takeWord("nec:")) {
    result.code.protocol = IrProtocol::Nec;
    result.code.bits = 32;
    if (!cursor.takeNumber(16, 0xFFFF, &value)) return Status::Corrupt;
    result.code.address = static_cast<uint16_t>(value);
    result.code.extended = necExtended || value > 0xFF;
    if (!cursor.take(':') || !cursor.takeNumber(16, 0xFF, &value)) return Status::Corrupt;
    result.code.command = static_cast<uint16_t>(value);
  } else if (cursor.takeWord("sony:")) {
    result.code.protocol = IrProtocol::Sony;
    if (!cursor.takeNumber(10, 20, &value)) return Status::Corrupt;
    result.code.bits = static_cast<uint8_t>(value);
    if (!cursor.take(':') || !cursor.takeNumber(16, 0x1FFF, &value)) return Status::Corrupt;
    result.code.address = static_cast<uint16_t>(value);
    if (!cursor.take(':') || !cursor.takeNumber(16, 0x7F, &value)) return Status::Corrupt;
    result.code.command = static_cast<uint16_t>(value);
  } else if (cursor.takeWord("raw:")) {
    result.isRaw = true;
    if (!cursor.takeNumber(10, kMaxCarrierHz, &value) || value < kMinCarrierHz)
      return Status::Corrupt;
    result.raw.carrierHz = value;
    if (!cursor.take(':')) return Status::Corrupt;
    for (;;) {
      if (result.raw.count >= RawTiming::kMaxEntries) return Status::Corrupt;
      if (!cursor.takeNumber(10, 0xFFFF, &value) || value == 0) return Status::Corrupt;
      result.raw.durationsUs[result.raw.count++] = static_cast<uint16_t>(value);
      if (cursor.atEnd()) break;
      if (!cursor.take(',')) return Status::Corrupt;
    }
  } else {
    return Status::Corrupt;
  }
  if (!cursor.atEnd()) return Status::Corrupt;

  // Um código decodificado precisa ser representável no protocolo.
  if (!result.isRaw) {
    RawTiming check;
    if (!isOk(encode(result.code, &check, 1))) return Status::Corrupt;
  }
  *out = result;
  return Status::Ok;
}

Status IrLibrary::save(const char *name, const StoredCode &code) {
  if (!isValidCodeName(name)) return Status::InvalidArgument;
  char text[kMaxCodeText];
  const Result<size_t> length = formatCode(code, text, sizeof(text));
  if (!length.ok()) return length.status();
  return storage_.save(name, text, length.value());
}

Status IrLibrary::load(const char *name, StoredCode *out) {
  if (!isValidCodeName(name) || out == nullptr) return Status::InvalidArgument;
  char text[kMaxCodeText];
  const Result<size_t> length = storage_.load(name, text, sizeof(text));
  if (!length.ok()) return length.status();
  return parseCode(text, length.value(), out);
}

Status IrLibrary::remove(const char *name) {
  if (!isValidCodeName(name)) return Status::InvalidArgument;
  return storage_.remove(name);
}

}  // namespace dbmo::infrared
