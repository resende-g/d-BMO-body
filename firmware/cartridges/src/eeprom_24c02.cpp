#include "dbmo/cartridges/eeprom_24c02.h"

#include <cstring>

namespace dbmo::cartridges {

Status Eeprom24c02::probe() { return bus_.probe(address_); }

Status Eeprom24c02::read(size_t offset, uint8_t *out, size_t length) {
  if (out == nullptr) return Status::InvalidArgument;
  if (offset > kSize || length > kSize - offset) return Status::OutOfRange;
  while (length > 0) {
    const size_t chunk = length < kReadChunk ? length : kReadChunk;
    const uint8_t pointer = static_cast<uint8_t>(offset);
    DBMO_TRY(bus_.writeRead(address_, &pointer, 1, out, chunk));
    offset += chunk;
    out += chunk;
    length -= chunk;
  }
  return Status::Ok;
}

Status Eeprom24c02::waitWriteCycle() {
  const uint32_t start = clock_.nowMs();
  while (true) {
    // Escrita de zero bytes: só o endereço. O chip só responde quando termina de gravar.
    const Status status = bus_.write(address_, nullptr, 0);
    if (isOk(status)) return Status::Ok;
    if (status != Status::HardwareAbsent) return status;
    if (elapsedMs(clock_.nowMs(), start) >= kWriteCycleTimeoutMs) return Status::Timeout;
    clock_.delayMs(1);
  }
}

Status Eeprom24c02::write(size_t offset, const uint8_t *data, size_t length) {
  if (data == nullptr) return Status::InvalidArgument;
  if (pageSize_ == 0 || pageSize_ > 16) return Status::InvalidArgument;
  if (offset > kSize || length > kSize - offset) return Status::OutOfRange;

  size_t position = offset;
  size_t remaining = length;
  const uint8_t *cursor = data;
  while (remaining > 0) {
    // Uma escrita não pode cruzar o limite de página: o chip voltaria ao início dela.
    const size_t room = pageSize_ - (position % pageSize_);
    const size_t chunk = remaining < room ? remaining : room;
    uint8_t frame[1 + 16];
    frame[0] = static_cast<uint8_t>(position);
    memcpy(frame + 1, cursor, chunk);
    DBMO_TRY(bus_.write(address_, frame, chunk + 1));
    DBMO_TRY(waitWriteCycle());
    position += chunk;
    cursor += chunk;
    remaining -= chunk;
  }

  // Conferência: com a proteção de escrita ligada o chip aceita o comando e não grava.
  uint8_t check[kReadChunk];
  position = offset;
  remaining = length;
  cursor = data;
  while (remaining > 0) {
    const size_t chunk = remaining < kReadChunk ? remaining : kReadChunk;
    DBMO_TRY(read(position, check, chunk));
    if (memcmp(check, cursor, chunk) != 0) return Status::IoError;
    position += chunk;
    cursor += chunk;
    remaining -= chunk;
  }
  return Status::Ok;
}

}  // namespace dbmo::cartridges
