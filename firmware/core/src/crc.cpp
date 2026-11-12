#include "dbmo/core/crc.h"

namespace dbmo {

// Cálculo bit a bit: sem tabelas, para não gastar flash. Os blocos são pequenos
// (cabeçalhos e quadros de protocolo).

uint8_t crc8(const uint8_t *data, size_t n, uint8_t initial) {
  uint8_t crc = initial;
  for (size_t i = 0; i < n; ++i) {
    crc ^= data[i];
    for (int bit = 0; bit < 8; ++bit)
      crc = (crc & 0x80) ? static_cast<uint8_t>((crc << 1) ^ 0x07) : static_cast<uint8_t>(crc << 1);
  }
  return crc;
}

uint16_t crc16Ccitt(const uint8_t *data, size_t n, uint16_t initial) {
  uint16_t crc = initial;
  for (size_t i = 0; i < n; ++i) {
    crc ^= static_cast<uint16_t>(data[i] << 8);
    for (int bit = 0; bit < 8; ++bit)
      crc = (crc & 0x8000) ? static_cast<uint16_t>((crc << 1) ^ 0x1021)
                           : static_cast<uint16_t>(crc << 1);
  }
  return crc;
}

uint32_t crc32(const uint8_t *data, size_t n) {
  uint32_t crc = 0xFFFFFFFFu;
  for (size_t i = 0; i < n; ++i) {
    crc ^= data[i];
    for (int bit = 0; bit < 8; ++bit) crc = (crc & 1u) ? (crc >> 1) ^ 0xEDB88320u : crc >> 1;
  }
  return ~crc;
}

}  // namespace dbmo
