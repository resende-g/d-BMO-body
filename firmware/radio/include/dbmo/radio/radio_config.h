// Configuração paramétrica do rádio e a conversão para os campos do CC1101.
// As fórmulas são as do datasheet SWRS061I (frequência, taxa de dados, banda do filtro e
// desvio em função do cristal).
#pragma once

#include <cstddef>
#include <cstdint>

#include "dbmo/core/status.h"

namespace dbmo::radio {

enum class Modulation : uint8_t { Fsk2 = 0, Gfsk = 1, Ook = 3 };

inline constexpr uint8_t kMaxPayload = 61;  // 64 de FIFO − tamanho − 2 bytes de status

struct RadioConfig {
  uint32_t frequencyHz = 433920000;
  Modulation modulation = Modulation::Fsk2;
  uint32_t dataRateBaud = 1200;
  uint32_t deviationHz = 5157;   // só FSK/GFSK
  uint32_t bandwidthHz = 58000;  // filtro de recepção
  uint16_t syncWord = 0xD391;
  bool crc = true;
  bool whitening = false;
  uint8_t maxPayload = kMaxPayload;
};

struct RadioPacket {
  uint8_t data[kMaxPayload];
  uint8_t length;
  int16_t rssiDbm;
  uint8_t lqi;
};

// Faixas em que o CC1101 opera: 300–348, 387–464 e 779–928 MHz (CC-04).
bool inChipBand(uint32_t frequencyHz);

// FREQ[23:0] = f × 2^16 / fxosc.
uint32_t frequencyWord(uint32_t xtalHz, uint32_t frequencyHz);
uint32_t frequencyFromWord(uint32_t xtalHz, uint32_t word);

struct FieldPair {
  uint8_t exponent;
  uint8_t mantissa;
};

// Taxa = (256 + M) × 2^E / 2^28 × fxosc. OutOfRange fora de 600 a 500000 baud.
Result<FieldPair> computeDataRate(uint32_t xtalHz, uint32_t baud);
uint32_t dataRateFromFields(uint32_t xtalHz, FieldPair fields);

// Banda = fxosc / (8 × (4 + M) × 2^E). Escolhe a menor banda que ainda cobre a pedida.
Result<FieldPair> computeBandwidth(uint32_t xtalHz, uint32_t bandwidthHz);
uint32_t bandwidthFromFields(uint32_t xtalHz, FieldPair fields);

// Desvio = fxosc / 2^17 × (8 + M) × 2^E. Escolhe o valor mais próximo.
FieldPair computeDeviation(uint32_t xtalHz, uint32_t deviationHz);
uint32_t deviationFromFields(uint32_t xtalHz, FieldPair fields);

// Verifica faixas e coerência. InvalidArgument ou OutOfRange.
Status validate(const RadioConfig &config, uint32_t xtalHz);

// Tempo no ar de um pacote com n bytes de dados: preâmbulo (4) + sincronismo (2) +
// tamanho (1) + dados + CRC (2, se ligado), arredondado para cima.
uint32_t airtimeMs(const RadioConfig &config, size_t payloadLength);

// Estimativa da banda ocupada na transmissão: 2 × desvio + taxa (FSK/GFSK, regra de
// Carson) ou 2 × taxa (OOK). É aproximação, usada só para conferir a borda da faixa.
uint32_t occupiedBandwidthHz(const RadioConfig &config);

}  // namespace dbmo::radio
