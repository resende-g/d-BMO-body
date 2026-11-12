// Driver do PN532 por I2C, sem bloqueio: um comando é enviado e o andamento (ACK e
// resposta) é acompanhado por poll().
//
// Escopo deliberadamente restrito: identificar o chip, configurá-lo como leitor e obter
// o identificador (UID) de uma etiqueta ISO 14443-A. O driver não implementa
// autenticação, leitura ou gravação de memória de etiquetas, emulação de cartão nem
// troca de dados com o alvo.
#pragma once

#include <cstddef>
#include <cstdint>

#include "dbmo/core/hal/clock.h"
#include "dbmo/core/hal/i2c.h"
#include "dbmo/core/status.h"
#include "dbmo/nfc/pn532_frame.h"

namespace dbmo::nfc {

// Códigos de comando usados (manual do usuário da NXP, UM0701-02; ver README).
namespace command {
inline constexpr uint8_t kGetFirmwareVersion = 0x02;
inline constexpr uint8_t kSamConfiguration = 0x14;
inline constexpr uint8_t kRfConfiguration = 0x32;
inline constexpr uint8_t kInListPassiveTarget = 0x4A;

// Os únicos comandos que o driver aceita enviar. Autenticação, leitura ou gravação de
// memória, troca de dados com o alvo e emulação de cartão ficam de fora por construção.
constexpr bool isAllowed(uint8_t code) {
  return code == kGetFirmwareVersion || code == kSamConfiguration || code == kRfConfiguration ||
         code == kInListPassiveTarget;
}
}  // namespace command

struct FirmwareVersion {
  uint8_t ic;  // 0x32 no PN532
  uint8_t version;
  uint8_t revision;
  uint8_t support;
};

inline constexpr size_t kMaxUidLength = 10;

struct TagInfo {
  uint8_t uid[kMaxUidLength];
  uint8_t uidLength;  // 4, 7 ou 10
  uint16_t atqa;      // SENS_RES
  uint8_t sak;        // SEL_RES
};

bool sameTag(const TagInfo &a, const TagInfo &b);

enum class CommandState : uint8_t { Idle, WaitingAck, WaitingResponse, Done, Failed };

class Pn532 {
 public:
  static constexpr uint32_t kAckTimeoutMs = 30;

  Pn532(II2cBus &bus, IClock &clock, uint8_t address)
      : bus_(bus), clock_(clock), address_(address) {}

  // O chip responde no barramento?
  Status probe() { return bus_.probe(address_); }

  // Envia um comando (payload[0] = código). Busy se há outro em andamento;
  // NotPermitted se o comando não está na lista de command::isAllowed().
  Status start(const uint8_t *payload, size_t length, uint32_t responseTimeoutMs);
  // Avança o comando em andamento. Devolve o estado depois do passo.
  CommandState poll();
  CommandState state() const { return state_; }
  // Motivo da falha quando state() == Failed.
  Status failure() const { return failure_; }
  // Dados da resposta (depois do código de resposta), válidos em Done.
  const uint8_t *response() const { return response_; }
  size_t responseLength() const { return responseLength_; }
  // Desiste do comando em andamento (envia um ACK, que o PN532 trata como "abortar").
  void abort();

  // Atalhos dos comandos usados.
  Status startGetFirmwareVersion();
  Status startSamConfiguration();
  // Número de tentativas de ativação de alvo passivo: com um valor finito, a busca por
  // etiqueta termina sozinha quando não há etiqueta no campo.
  Status startSetPassiveActivationRetries(uint8_t retries);
  Status startRfField(bool on);
  Status startFindTagIso14443A();

  // Interpretam a resposta em response()/responseLength().
  static Status parseFirmwareVersion(const uint8_t *data, size_t length, FirmwareVersion *out);
  // NotFound quando a resposta diz que não há etiqueta.
  static Status parseTag(const uint8_t *data, size_t length, TagInfo *out);

 private:
  void fail(Status status);
  // Lê o byte de estado e o quadro. Busy se o PN532 ainda não tem resposta.
  Status readFrame(Frame *out);

  II2cBus &bus_;
  IClock &clock_;
  uint8_t address_;
  CommandState state_ = CommandState::Idle;
  Status failure_ = Status::Ok;
  uint8_t commandCode_ = 0;
  uint32_t startedMs_ = 0;
  uint32_t responseTimeoutMs_ = 0;
  uint8_t buffer_[1 + kMaxFrame] = {};
  uint8_t response_[kMaxPayload] = {};
  size_t responseLength_ = 0;
};

}  // namespace dbmo::nfc
