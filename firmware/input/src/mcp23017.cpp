#include "dbmo/input/mcp23017.h"

namespace dbmo::input {

Status Mcp23017::writeRegister(uint8_t reg, uint8_t value) {
  const uint8_t frame[2] = {reg, value};
  return bus_.write(address_, frame, sizeof(frame));
}

Result<uint8_t> Mcp23017::readRegister(uint8_t reg) {
  uint8_t value = 0;
  const Status status = bus_.writeRead(address_, &reg, 1, &value, 1);
  if (!isOk(status)) return status;
  return value;
}

Status Mcp23017::writePair(uint8_t reg, uint16_t value) {
  DBMO_TRY(writeRegister(reg, static_cast<uint8_t>(value & 0xFF)));
  return writeRegister(static_cast<uint8_t>(reg + 1), static_cast<uint8_t>(value >> 8));
}

Result<uint16_t> Mcp23017::readPair(uint8_t reg) {
  const Result<uint8_t> low = readRegister(reg);
  if (!low.ok()) return low.status();
  const Result<uint8_t> high = readRegister(static_cast<uint8_t>(reg + 1));
  if (!high.ok()) return high.status();
  return static_cast<uint16_t>(low.value() | (high.value() << 8));
}

Status Mcp23017::begin(uint16_t inputMask, uint16_t pullUpMask) {
  if ((inputMask & kOutputOnlyPins) != 0) return Status::InvalidArgument;
  DBMO_TRY(bus_.probe(address_));

  // IOCON no valor de reset: BANK = 0, endereçamento sequencial, INT separadas.
  DBMO_TRY(writeRegister(kRegIocon, 0x00));
  // Saídas em nível baixo antes de definir a direção, para não gerar pulso.
  DBMO_TRY(writePair(kRegOlat, 0x0000));
  DBMO_TRY(writePair(kRegIodir, inputMask));
  DBMO_TRY(writePair(kRegIpol, 0x0000));
  DBMO_TRY(writePair(kRegGppu, static_cast<uint16_t>(pullUpMask & inputMask)));

  const Result<uint16_t> direction = readPair(kRegIodir);
  if (!direction.ok()) return direction.status();
  if (direction.value() != inputMask) return Status::ProtocolError;
  return Status::Ok;
}

Result<uint16_t> Mcp23017::readPins() { return readPair(kRegGpio); }

}  // namespace dbmo::input
