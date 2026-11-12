// Quadros do PN532: preâmbulo 00, início 00 FF, tamanho e seu complemento, identificador
// (D4 do host, D5 do PN532), dados, soma de verificação e pós-âmbulo 00
// (docs/hardware/sources/PERIPHERALS.md, PN-11).
#pragma once

#include <cstddef>
#include <cstdint>

#include "dbmo/core/status.h"

namespace dbmo::nfc {

inline constexpr uint8_t kTfiHostToPn532 = 0xD4;
inline constexpr uint8_t kTfiPn532ToHost = 0xD5;
// Maior comando/resposta que este módulo usa (dados depois do identificador).
inline constexpr size_t kMaxPayload = 40;
// Quadro completo: 00 00 FF LEN LCS TFI dados DCS 00.
inline constexpr size_t kFrameOverhead = 8;
inline constexpr size_t kMaxFrame = kMaxPayload + kFrameOverhead;

inline constexpr uint8_t kAckFrame[6] = {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};
inline constexpr uint8_t kNackFrame[6] = {0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00};

// Monta o quadro de um comando (payload[0] é o código do comando).
// InvalidArgument com payload vazio ou grande demais; NoSpace se não cabe em out.
Result<size_t> encodeFrame(uint8_t tfi, const uint8_t *payload, size_t length, uint8_t *out,
                           size_t capacity);

enum class FrameKind : uint8_t {
  Ack,
  Nack,
  Error,  // quadro de erro de aplicação do PN532 (00 00 FF 01 FF 7F 81 00)
  Data,
};

struct Frame {
  FrameKind kind;
  uint8_t tfi;             // só em Data
  const uint8_t *payload;  // dados depois do identificador (aponta para dentro da entrada)
  size_t length;
};

// Interpreta um quadro. ProtocolError se não há quadro inteiro; CrcError se o complemento
// do tamanho ou a soma de verificação não conferem.
Status decodeFrame(const uint8_t *data, size_t length, Frame *out);

}  // namespace dbmo::nfc
