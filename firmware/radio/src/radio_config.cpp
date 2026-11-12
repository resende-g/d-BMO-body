#include "dbmo/radio/radio_config.h"

namespace dbmo::radio {

bool inChipBand(uint32_t hz) {
  return (hz >= 300000000u && hz <= 348000000u) || (hz >= 387000000u && hz <= 464000000u) ||
         (hz >= 779000000u && hz <= 928000000u);
}

uint32_t frequencyWord(uint32_t xtalHz, uint32_t frequencyHz) {
  if (xtalHz == 0) return 0;
  const uint64_t scaled = (static_cast<uint64_t>(frequencyHz) << 16) + xtalHz / 2;
  return static_cast<uint32_t>(scaled / xtalHz) & 0x00FFFFFFu;
}

uint32_t frequencyFromWord(uint32_t xtalHz, uint32_t word) {
  return static_cast<uint32_t>((static_cast<uint64_t>(word) * xtalHz + (1u << 15)) >> 16);
}

Result<FieldPair> computeDataRate(uint32_t xtalHz, uint32_t baud) {
  if (xtalHz == 0) return Status::InvalidArgument;
  if (baud < 600 || baud > 500000) return Status::OutOfRange;
  const uint64_t numerator = static_cast<uint64_t>(baud) << 28;
  for (uint8_t exponent = 0; exponent < 16; ++exponent) {
    const uint64_t denominator = static_cast<uint64_t>(xtalHz) << exponent;
    const uint64_t total = (numerator + denominator / 2) / denominator;  // 256 + M
    if (total < 256) return FieldPair{exponent, 0};
    if (total <= 511) return FieldPair{exponent, static_cast<uint8_t>(total - 256)};
  }
  return Status::OutOfRange;
}

uint32_t dataRateFromFields(uint32_t xtalHz, FieldPair fields) {
  const uint64_t value = (static_cast<uint64_t>(256 + fields.mantissa) << fields.exponent) * xtalHz;
  return static_cast<uint32_t>((value + (1u << 27)) >> 28);
}

uint32_t bandwidthFromFields(uint32_t xtalHz, FieldPair fields) {
  return xtalHz / (8u * (4u + fields.mantissa) * (1u << fields.exponent));
}

Result<FieldPair> computeBandwidth(uint32_t xtalHz, uint32_t bandwidthHz) {
  if (xtalHz == 0 || bandwidthHz == 0) return Status::InvalidArgument;
  // Da banda mais estreita (E=3, M=3) para a mais larga (E=0, M=0).
  for (int exponent = 3; exponent >= 0; --exponent) {
    for (int mantissa = 3; mantissa >= 0; --mantissa) {
      const FieldPair fields{static_cast<uint8_t>(exponent), static_cast<uint8_t>(mantissa)};
      if (bandwidthFromFields(xtalHz, fields) >= bandwidthHz) return fields;
    }
  }
  return Status::OutOfRange;
}

uint32_t deviationFromFields(uint32_t xtalHz, FieldPair fields) {
  const uint64_t value = (static_cast<uint64_t>(8 + fields.mantissa) << fields.exponent) * xtalHz;
  return static_cast<uint32_t>((value + (1u << 16)) >> 17);
}

FieldPair computeDeviation(uint32_t xtalHz, uint32_t deviationHz) {
  FieldPair best{0, 0};
  uint32_t bestError = UINT32_MAX;
  for (uint8_t exponent = 0; exponent < 8; ++exponent) {
    for (uint8_t mantissa = 0; mantissa < 8; ++mantissa) {
      const FieldPair fields{exponent, mantissa};
      const uint32_t actual = deviationFromFields(xtalHz, fields);
      const uint32_t error = actual > deviationHz ? actual - deviationHz : deviationHz - actual;
      if (error < bestError) {
        bestError = error;
        best = fields;
      }
    }
  }
  return best;
}

Status validate(const RadioConfig &config, uint32_t xtalHz) {
  if (xtalHz < 26000000u || xtalHz > 27000000u) return Status::InvalidArgument;
  if (!inChipBand(config.frequencyHz)) return Status::OutOfRange;
  if (config.modulation != Modulation::Fsk2 && config.modulation != Modulation::Gfsk &&
      config.modulation != Modulation::Ook)
    return Status::InvalidArgument;
  if (config.maxPayload == 0 || config.maxPayload > kMaxPayload) return Status::InvalidArgument;
  const Result<FieldPair> rate = computeDataRate(xtalHz, config.dataRateBaud);
  if (!rate.ok()) return rate.status();
  const Result<FieldPair> bandwidth = computeBandwidth(xtalHz, config.bandwidthHz);
  if (!bandwidth.ok()) return bandwidth.status();
  if (config.modulation != Modulation::Ook) {
    // Limites do campo DEVIATN para o cristal em uso.
    const uint32_t minimum = deviationFromFields(xtalHz, FieldPair{0, 0});
    const uint32_t maximum = deviationFromFields(xtalHz, FieldPair{7, 7});
    if (config.deviationHz < minimum || config.deviationHz > maximum) return Status::OutOfRange;
  }
  return Status::Ok;
}

uint32_t airtimeMs(const RadioConfig &config, size_t payloadLength) {
  if (config.dataRateBaud == 0) return UINT32_MAX;
  const uint64_t bytes = 4 + 2 + 1 + payloadLength + (config.crc ? 2 : 0);
  const uint64_t bits = bytes * 8;
  return static_cast<uint32_t>((bits * 1000 + config.dataRateBaud - 1) / config.dataRateBaud);
}

uint32_t occupiedBandwidthHz(const RadioConfig &config) {
  if (config.modulation == Modulation::Ook) return 2 * config.dataRateBaud;
  return 2 * config.deviationHz + config.dataRateBaud;
}

}  // namespace dbmo::radio
