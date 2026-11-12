// Somas de verificação usadas em cabeçalhos persistidos e protocolos.
#pragma once

#include <cstddef>
#include <cstdint>

namespace dbmo {

// CRC-8, polinômio 0x07, valor inicial 0x00 (CRC-8/SMBUS). "123456789" -> 0xF4.
uint8_t crc8(const uint8_t *data, size_t n, uint8_t initial = 0x00);

// CRC-16/CCITT-FALSE: polinômio 0x1021, inicial 0xFFFF. "123456789" -> 0x29B1.
uint16_t crc16Ccitt(const uint8_t *data, size_t n, uint16_t initial = 0xFFFF);

// CRC-32 (IEEE 802.3, refletido). "123456789" -> 0xCBF43926.
uint32_t crc32(const uint8_t *data, size_t n);

}  // namespace dbmo
