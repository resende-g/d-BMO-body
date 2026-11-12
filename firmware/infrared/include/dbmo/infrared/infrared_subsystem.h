// Subsistema de infravermelho: recebe e identifica códigos, aprende um código a pedido
// e reenvia códigos quando o envio está habilitado.
//
// O envio vem DESABILITADO (infrared.tx_enabled = false) e tem um orçamento de tempo de
// emissão por minuto: o módulo serve para repetir um código de um aparelho do próprio
// usuário, não para inundar receptores. Não há modo de emissão contínua.
#pragma once

#include <cstdint>

#include "dbmo/core/config_store.h"
#include "dbmo/core/event_bus.h"
#include "dbmo/core/hal/clock.h"
#include "dbmo/core/subsystem.h"
#include "dbmo/infrared/ir_types.h"

namespace dbmo::infrared {

enum class IrTxDenial : uint8_t {
  None,
  Disabled,        // infrared.tx_enabled = false
  NoTransmitter,   // nenhum emissor ligado ao subsistema
  Busy,            // envio anterior em andamento
  InvalidSignal,   // vazio, portadora fora de 30–60 kHz ou duração zero
  TooLong,         // um único envio acima de kMaxSendUs
  BudgetExceeded,  // estouraria o tempo de emissão por minuto
};

const char *toString(IrTxDenial denial);

namespace infrared_event {
// Código identificado. a = protocolo | (bits << 8), b = (endereço << 16) | comando.
inline constexpr EventId kCodeReceived = makeEventId(EventDomain::Infrared, 1);
// O mesmo código de novo em seguida (tecla mantida apertada). Mesmos argumentos.
inline constexpr EventId kRepeat = makeEventId(EventDomain::Infrared, 2);
// Sinal de protocolo desconhecido. a = número de durações.
inline constexpr EventId kRawReceived = makeEventId(EventDomain::Infrared, 3);
// O aprendizado capturou um código (learned()). a = 1 se bruto, 0 se identificado.
inline constexpr EventId kLearned = makeEventId(EventDomain::Infrared, 4);
inline constexpr EventId kLearnTimeout = makeEventId(EventDomain::Infrared, 5);
inline constexpr EventId kTxDone = makeEventId(EventDomain::Infrared, 6);
// a = IrTxDenial.
inline constexpr EventId kTxDenied = makeEventId(EventDomain::Infrared, 7);
// a = Status.
inline constexpr EventId kFault = makeEventId(EventDomain::Infrared, 8);
}  // namespace infrared_event

namespace config_key {
inline constexpr char kRxEnabled[] = "infrared.rx_enabled";
inline constexpr char kTxEnabled[] = "infrared.tx_enabled";
inline constexpr char kTxMsPerMin[] = "infrared.tx_ms_per_min";
}  // namespace config_key

class InfraredSubsystem final : public ISubsystem {
 public:
  static constexpr uint32_t kMinCarrierHz = 30000;
  static constexpr uint32_t kMaxCarrierHz = 60000;
  // Maior duração de um único envio.
  static constexpr uint32_t kMaxSendUs = 500000;
  // Janela em que o mesmo código recebido de novo conta como repetição.
  static constexpr uint32_t kRepeatWindowMs = 200;
  static constexpr uint32_t kBudgetWindowMs = 60000;
  static constexpr uint8_t kFaultThreshold = 5;

  InfraredSubsystem(EventBus &bus, ConfigStore &config, IClock &clock);

  static Status registerConfig(ConfigStore &config);

  // Ligações opcionais, antes de init().
  void setReceiver(IIrReceiver &receiver) { receiver_ = &receiver; }
  void setTransmitter(IIrTransmitter &transmitter) { transmitter_ = &transmitter; }

  const char *name() const override { return "infrared"; }
  // HardwareAbsent se não há receptor nem emissor ligados.
  Status init() override;
  void tick(uint32_t nowMs) override;
  Status shutdown() override;

  // Último sinal recebido. false se ainda não houve.
  bool lastReceived(StoredCode *out) const;

  // O próximo sinal recebido é guardado em learned() e gera kLearned.
  Status startLearning(uint32_t timeoutMs = 10000);
  void cancelLearning();
  bool learning() const { return learning_; }
  bool learned(StoredCode *out) const;

  // Envio. NotPermitted quando negado (motivo em lastDenial() e no evento kTxDenied).
  Status send(const IrCode &code);
  Status sendRaw(const RawTiming &timing);
  Status send(const StoredCode &code);
  IrTxDenial lastDenial() const { return lastDenial_; }
  bool transmitting() const { return transmitting_; }
  // Tempo de emissão gasto no minuto corrente, em ms.
  uint32_t airtimeUsedMs() const { return usedUs_ / 1000; }

  void applyConfig();

 private:
  void handleReceived(const RawTiming &timing, uint32_t nowMs);
  Status deny(IrTxDenial denial);
  void rollBudget(uint32_t nowMs);

  EventBus &bus_;
  ConfigStore &config_;
  IClock &clock_;
  IIrReceiver *receiver_ = nullptr;
  IIrTransmitter *transmitter_ = nullptr;
  bool initialized_ = false;
  bool rxEnabled_ = true;
  bool txEnabled_ = false;
  uint32_t budgetMs_ = 6000;

  bool hasLast_ = false;
  StoredCode last_;
  uint32_t lastCodeMs_ = 0;
  bool learning_ = false;
  bool hasLearned_ = false;
  StoredCode learnedCode_;
  uint32_t learnStartMs_ = 0;
  uint32_t learnTimeoutMs_ = 0;

  bool transmitting_ = false;
  IrTxDenial lastDenial_ = IrTxDenial::None;
  uint32_t windowStartMs_ = 0;
  uint32_t usedUs_ = 0;
  uint8_t rxErrors_ = 0;
  bool rxFaulted_ = false;
  RawTiming scratch_;
};

}  // namespace dbmo::infrared
