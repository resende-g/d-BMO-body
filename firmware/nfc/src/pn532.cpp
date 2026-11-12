#include "dbmo/nfc/pn532.h"

#include <cstring>

namespace dbmo::nfc {

bool sameTag(const TagInfo &a, const TagInfo &b) {
  return a.uidLength == b.uidLength && memcmp(a.uid, b.uid, a.uidLength) == 0;
}

void Pn532::fail(Status status) {
  failure_ = status;
  state_ = CommandState::Failed;
}

Status Pn532::start(const uint8_t *payload, size_t length, uint32_t responseTimeoutMs) {
  if (payload == nullptr || length == 0) return Status::InvalidArgument;
  if (!command::isAllowed(payload[0])) return Status::NotPermitted;
  if (state_ == CommandState::WaitingAck || state_ == CommandState::WaitingResponse)
    return Status::Busy;
  uint8_t frame[kMaxFrame];
  const Result<size_t> size = encodeFrame(kTfiHostToPn532, payload, length, frame, sizeof(frame));
  if (!size.ok()) return size.status();
  const Status written = bus_.write(address_, frame, size.value());
  if (!isOk(written)) {
    fail(written);
    return written;
  }
  commandCode_ = payload[0];
  startedMs_ = clock_.nowMs();
  responseTimeoutMs_ = responseTimeoutMs;
  responseLength_ = 0;
  failure_ = Status::Ok;
  state_ = CommandState::WaitingAck;
  return Status::Ok;
}

Status Pn532::readFrame(Frame *out) {
  // Em I2C toda leitura começa com um byte de estado; o bit 0 em 1 indica que há um
  // quadro pronto (PN-10). Lê o estado e o quadro numa única transação.
  DBMO_TRY(bus_.read(address_, buffer_, sizeof(buffer_)));
  if ((buffer_[0] & 0x01) == 0) return Status::Busy;
  return decodeFrame(buffer_ + 1, sizeof(buffer_) - 1, out);
}

CommandState Pn532::poll() {
  if (state_ != CommandState::WaitingAck && state_ != CommandState::WaitingResponse) return state_;

  const uint32_t elapsed = elapsedMs(clock_.nowMs(), startedMs_);
  Frame frame{};
  const Status status = readFrame(&frame);

  if (status == Status::Busy) {
    const uint32_t limit =
        state_ == CommandState::WaitingAck ? kAckTimeoutMs : kAckTimeoutMs + responseTimeoutMs_;
    if (elapsed >= limit) {
      // Sem resposta no prazo: manda o PN532 desistir do comando.
      (void)bus_.write(address_, kAckFrame, sizeof(kAckFrame));
      fail(Status::Timeout);
    }
    return state_;
  }
  if (!isOk(status)) {
    fail(status);
    return state_;
  }

  if (state_ == CommandState::WaitingAck) {
    if (frame.kind == FrameKind::Ack) {
      state_ = CommandState::WaitingResponse;
    } else {
      fail(Status::ProtocolError);  // NACK, erro ou dado no lugar do ACK
    }
    return state_;
  }

  // WaitingResponse.
  if (frame.kind != FrameKind::Data || frame.tfi != kTfiPn532ToHost || frame.length == 0 ||
      frame.payload[0] != static_cast<uint8_t>(commandCode_ + 1)) {
    fail(Status::ProtocolError);
    return state_;
  }
  responseLength_ = frame.length - 1;
  if (responseLength_ > sizeof(response_)) {
    fail(Status::NoSpace);
    return state_;
  }
  memcpy(response_, frame.payload + 1, responseLength_);
  state_ = CommandState::Done;
  return state_;
}

void Pn532::abort() {
  if (state_ == CommandState::WaitingAck || state_ == CommandState::WaitingResponse)
    (void)bus_.write(address_, kAckFrame, sizeof(kAckFrame));
  state_ = CommandState::Idle;
}

Status Pn532::startGetFirmwareVersion() {
  const uint8_t payload[] = {command::kGetFirmwareVersion};
  return start(payload, sizeof(payload), 100);
}

Status Pn532::startSamConfiguration() {
  // Modo normal (SAM não usado), sem tempo limite, sem usar a linha IRQ.
  const uint8_t payload[] = {command::kSamConfiguration, 0x01, 0x00, 0x00};
  return start(payload, sizeof(payload), 100);
}

Status Pn532::startSetPassiveActivationRetries(uint8_t retries) {
  // Item 5 (MaxRetries): ATR_REQ e PSL_REQ nos valores padrão, ativação passiva = retries.
  const uint8_t payload[] = {command::kRfConfiguration, 0x05, 0xFF, 0x01, retries};
  return start(payload, sizeof(payload), 100);
}

Status Pn532::startRfField(bool on) {
  // Item 1 (campo de RF): bit 0 liga ou desliga o campo.
  const uint8_t payload[] = {command::kRfConfiguration, 0x01,
                             static_cast<uint8_t>(on ? 0x01 : 0x00)};
  return start(payload, sizeof(payload), 100);
}

Status Pn532::startFindTagIso14443A() {
  // No máximo um alvo, 106 kbps tipo A.
  const uint8_t payload[] = {command::kInListPassiveTarget, 0x01, 0x00};
  return start(payload, sizeof(payload), 200);
}

Status Pn532::parseFirmwareVersion(const uint8_t *data, size_t length, FirmwareVersion *out) {
  if (data == nullptr || out == nullptr) return Status::InvalidArgument;
  if (length != 4) return Status::ProtocolError;
  *out = FirmwareVersion{data[0], data[1], data[2], data[3]};
  return Status::Ok;
}

Status Pn532::parseTag(const uint8_t *data, size_t length, TagInfo *out) {
  if (data == nullptr || out == nullptr) return Status::InvalidArgument;
  // NbTg [Tg SENS_RES(2) SEL_RES NFCIDLength NFCID...]
  if (length < 1) return Status::ProtocolError;
  if (data[0] == 0) return Status::NotFound;
  if (data[0] != 1 || length < 6) return Status::ProtocolError;
  const uint8_t uidLength = data[5];
  if (uidLength != 4 && uidLength != 7 && uidLength != 10) return Status::ProtocolError;
  if (length < static_cast<size_t>(6) + uidLength) return Status::ProtocolError;
  TagInfo tag{};
  tag.atqa = static_cast<uint16_t>((data[2] << 8) | data[3]);
  tag.sak = data[4];
  tag.uidLength = uidLength;
  memcpy(tag.uid, data + 6, uidLength);
  *out = tag;
  return Status::Ok;
}

}  // namespace dbmo::nfc
