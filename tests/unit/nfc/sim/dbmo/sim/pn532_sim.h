// Simulador do PN532 no nível do I2C: recebe quadros de comando, devolve o byte de estado
// seguido do ACK e depois da resposta. Só conhece os comandos que o módulo usa.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "dbmo/mock/mock_hal.h"
#include "dbmo/nfc/pn532.h"
#include "dbmo/nfc/pn532_frame.h"

namespace dbmo::sim {

class Pn532Sim final : public mock::II2cDeviceSim {
 public:
  Status onWrite(const uint8_t *data, size_t n) override {
    if (n == sizeof(nfc::kAckFrame) && memcmp(data, nfc::kAckFrame, n) == 0) {
      // ACK vindo do host: abandona o comando em andamento.
      ++abortCount;
      ackPending_ = false;
      responseReady_ = false;
      return Status::Ok;
    }
    nfc::Frame frame{};
    if (!isOk(nfc::decodeFrame(data, n, &frame)) || frame.kind != nfc::FrameKind::Data ||
        frame.tfi != nfc::kTfiHostToPn532 || frame.length == 0) {
      ++badFrames;
      return Status::Ok;  // quadro inválido: o chip não responde
    }
    const uint8_t code = frame.payload[0];
    ++commandCount[code];
    lastCommand = code;
    ackPending_ = true;
    ackReads_ = readsBeforeAck;
    responseReady_ = false;
    buildResponse(code, frame.payload + 1, frame.length - 1);
    return Status::Ok;
  }

  Status onRead(uint8_t *data, size_t n) override {
    memset(data, 0, n);
    if (ackPending_) {
      if (ackReads_ > 0) {
        --ackReads_;
        return Status::Ok;
      }
      ackPending_ = false;
      data[0] = 0x01;
      const uint8_t *reply = nackInsteadOfAck ? nfc::kNackFrame : nfc::kAckFrame;
      memcpy(data + 1, reply, n - 1 < 6 ? n - 1 : 6);
      return Status::Ok;
    }
    if (responseReady_) {
      if (responseReads_ > 0) {
        --responseReads_;
        return Status::Ok;
      }
      responseReady_ = false;
      data[0] = 0x01;
      memcpy(data + 1, response_, n - 1 < responseSize_ ? n - 1 : responseSize_);
      if (corruptNextResponse) {
        corruptNextResponse = false;
        data[1 + responseSize_ - 2] ^= 0x55;  // estraga a soma de verificação
      }
      return Status::Ok;
    }
    return Status::Ok;  // nada pronto: byte de estado 0x00
  }

  void setTag(const uint8_t *tagUid, uint8_t length, uint16_t tagAtqa = 0x0004,
              uint8_t tagSak = 0x08) {
    memcpy(uid, tagUid, length);
    uidLength = length;
    atqa = tagAtqa;
    sak = tagSak;
    tagPresent = true;
  }

  // ---- Estado visível ao teste ----
  bool tagPresent = false;
  uint8_t uid[nfc::kMaxUidLength] = {};
  uint8_t uidLength = 4;
  uint16_t atqa = 0x0004;
  uint8_t sak = 0x08;
  uint8_t ic = 0x32;
  uint8_t firmwareVersion = 1;
  uint8_t firmwareRevision = 6;

  unsigned readsBeforeAck = 0;
  unsigned readsBeforeResponse = 0;
  bool neverRespond = false;
  bool nackInsteadOfAck = false;
  bool corruptNextResponse = false;
  bool wrongResponseCode = false;
  bool errorFrameResponse = false;

  bool rfFieldOn = false;
  bool samConfigured = false;
  uint8_t passiveRetries = 0xFF;  // 0xFF = tenta para sempre (padrão do chip)
  unsigned commandCount[256] = {};
  uint8_t lastCommand = 0;
  unsigned abortCount = 0;
  unsigned badFrames = 0;

 private:
  void respond(const uint8_t *payload, size_t length) {
    if (neverRespond) return;
    if (errorFrameResponse) {
      static const uint8_t kError[] = {0x00, 0x00, 0xFF, 0x01, 0xFF, 0x7F, 0x81, 0x00};
      memcpy(response_, kError, sizeof(kError));
      responseSize_ = sizeof(kError);
    } else {
      responseSize_ =
          nfc::encodeFrame(nfc::kTfiPn532ToHost, payload, length, response_, sizeof(response_))
              .value();
    }
    responseReady_ = true;
    responseReads_ = readsBeforeResponse;
  }

  void buildResponse(uint8_t code, const uint8_t *args, size_t n) {
    uint8_t payload[nfc::kMaxPayload] = {};
    payload[0] = static_cast<uint8_t>(code + (wrongResponseCode ? 3 : 1));
    switch (code) {
      case nfc::command::kGetFirmwareVersion:
        payload[1] = ic;
        payload[2] = firmwareVersion;
        payload[3] = firmwareRevision;
        payload[4] = 0x07;
        respond(payload, 5);
        break;
      case nfc::command::kSamConfiguration:
        samConfigured = true;
        respond(payload, 1);
        break;
      case nfc::command::kRfConfiguration:
        if (n >= 2 && args[0] == 0x01) rfFieldOn = (args[1] & 0x01) != 0;
        if (n >= 4 && args[0] == 0x05) passiveRetries = args[3];
        respond(payload, 1);
        break;
      case nfc::command::kInListPassiveTarget:
        rfFieldOn = true;  // a busca liga o campo
        if (tagPresent) {
          payload[1] = 1;  // NbTg
          payload[2] = 1;  // Tg
          payload[3] = static_cast<uint8_t>(atqa >> 8);
          payload[4] = static_cast<uint8_t>(atqa & 0xFF);
          payload[5] = sak;
          payload[6] = uidLength;
          memcpy(payload + 7, uid, uidLength);
          respond(payload, static_cast<size_t>(7) + uidLength);
        } else if (passiveRetries != 0xFF) {
          payload[1] = 0;  // nenhuma etiqueta
          respond(payload, 2);
        }
        // Com tentativas infinitas e sem etiqueta, o chip real não responde.
        break;
      default: respond(payload, 1); break;
    }
  }

  bool ackPending_ = false;
  unsigned ackReads_ = 0;
  bool responseReady_ = false;
  unsigned responseReads_ = 0;
  uint8_t response_[nfc::kMaxFrame] = {};
  size_t responseSize_ = 0;
};

}  // namespace dbmo::sim
