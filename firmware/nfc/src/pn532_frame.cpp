#include "dbmo/nfc/pn532_frame.h"

#include <cstring>

namespace dbmo::nfc {

Result<size_t> encodeFrame(uint8_t tfi, const uint8_t *payload, size_t length, uint8_t *out,
                           size_t capacity) {
  if (payload == nullptr || out == nullptr || length == 0 || length > kMaxPayload)
    return Status::InvalidArgument;
  const size_t total = length + kFrameOverhead;
  if (capacity < total) return Status::NoSpace;

  const uint8_t len = static_cast<uint8_t>(length + 1);  // identificador + dados
  out[0] = 0x00;
  out[1] = 0x00;
  out[2] = 0xFF;
  out[3] = len;
  out[4] = static_cast<uint8_t>(~len + 1);
  out[5] = tfi;
  uint8_t sum = tfi;
  for (size_t i = 0; i < length; ++i) {
    out[6 + i] = payload[i];
    sum = static_cast<uint8_t>(sum + payload[i]);
  }
  out[6 + length] = static_cast<uint8_t>(~sum + 1);
  out[7 + length] = 0x00;
  return total;
}

Status decodeFrame(const uint8_t *data, size_t length, Frame *out) {
  if (data == nullptr || out == nullptr) return Status::InvalidArgument;

  // Procura o código de início 00 FF (o preâmbulo pode ter zeros a mais).
  size_t start = 0;
  bool found = false;
  for (; start + 1 < length; ++start) {
    if (data[start] == 0x00 && data[start + 1] == 0xFF) {
      found = true;
      break;
    }
    if (data[start] != 0x00) return Status::ProtocolError;  // lixo antes do início
  }
  if (!found) return Status::ProtocolError;

  const uint8_t *frame = data + start + 2;  // LEN em diante
  const size_t remaining = length - start - 2;
  if (remaining < 2) return Status::ProtocolError;
  const uint8_t len = frame[0];
  const uint8_t lcs = frame[1];

  if (len == 0x00 && lcs == 0xFF) {
    *out = Frame{FrameKind::Ack, 0, nullptr, 0};
    return Status::Ok;
  }
  if (len == 0xFF && lcs == 0x00) {
    *out = Frame{FrameKind::Nack, 0, nullptr, 0};
    return Status::Ok;
  }
  if (static_cast<uint8_t>(len + lcs) != 0) return Status::CrcError;
  if (len == 0) return Status::ProtocolError;
  // LEN bytes de (identificador + dados) + soma de verificação.
  if (remaining < static_cast<size_t>(len) + 3) return Status::ProtocolError;

  const uint8_t *body = frame + 2;
  uint8_t sum = 0;
  for (size_t i = 0; i < len; ++i) sum = static_cast<uint8_t>(sum + body[i]);
  if (static_cast<uint8_t>(sum + body[len]) != 0) return Status::CrcError;

  if (len == 1 && body[0] == 0x7F) {
    *out = Frame{FrameKind::Error, 0, nullptr, 0};
    return Status::Ok;
  }
  *out = Frame{FrameKind::Data, body[0], body + 1, static_cast<size_t>(len) - 1};
  return Status::Ok;
}

}  // namespace dbmo::nfc
