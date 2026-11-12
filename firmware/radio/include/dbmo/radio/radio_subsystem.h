// Subsistema de rádio: identifica o CC1101, mantém-no em repouso de baixo consumo,
// recebe pacotes quando pedido e transmite somente o que a política autoriza.
#pragma once

#include <cstddef>
#include <cstdint>

#include "dbmo/core/config_store.h"
#include "dbmo/core/event_bus.h"
#include "dbmo/core/hal/clock.h"
#include "dbmo/core/subsystem.h"
#include "dbmo/radio/cc1101.h"
#include "dbmo/radio/radio_config.h"
#include "dbmo/radio/tx_policy.h"

namespace dbmo::radio {

enum class RadioState : uint8_t { Off, Idle, Receiving, Transmitting, Fault };

const char *toString(RadioState state);

namespace radio_event {
// a = RadioState.
inline constexpr EventId kStateChanged = makeEventId(EventDomain::Radio, 1);
// Pacote na fila (takePacket). a = tamanho, b = RSSI em dBm (int16 em 16 bits).
inline constexpr EventId kPacketReceived = makeEventId(EventDomain::Radio, 2);
inline constexpr EventId kTxDone = makeEventId(EventDomain::Radio, 3);
// a = Status.
inline constexpr EventId kTxFailed = makeEventId(EventDomain::Radio, 4);
// A política recusou a transmissão. a = TxDenial.
inline constexpr EventId kTxDenied = makeEventId(EventDomain::Radio, 5);
// O chip parou de responder. a = Status.
inline constexpr EventId kFault = makeEventId(EventDomain::Radio, 6);
}  // namespace radio_event

namespace config_key {
inline constexpr char kTxEnabled[] = "radio.tx_enabled";
inline constexpr char kTxMinKhz[] = "radio.tx_min_khz";
inline constexpr char kTxMaxKhz[] = "radio.tx_max_khz";
inline constexpr char kTxMsPerMin[] = "radio.tx_ms_per_min";
inline constexpr char kTxPaTable[] = "radio.tx_patable";
inline constexpr char kXtalKhz[] = "radio.xtal_khz";
}  // namespace config_key

struct RadioCounters {
  uint32_t packets = 0;
  uint32_t crcErrors = 0;
  uint32_t overflows = 0;
  uint32_t malformed = 0;
  uint32_t droppedPackets = 0;  // fila cheia
  uint32_t transmitted = 0;
  uint32_t denied = 0;
};

class RadioSubsystem final : public ISubsystem {
 public:
  static constexpr uint32_t kPollPeriodMs = 5;
  static constexpr uint32_t kRetryPeriodMs = 5000;
  static constexpr uint8_t kFaultThreshold = 3;
  static constexpr size_t kQueueSize = 4;
  // Folga sobre o tempo no ar calculado antes de dar a transmissão como travada.
  static constexpr uint32_t kTxMarginMs = 100;

  RadioSubsystem(Cc1101 &chip, EventBus &bus, ConfigStore &config, IClock &clock);

  static Status registerConfig(ConfigStore &config);

  const char *name() const override { return "radio"; }
  // HardwareAbsent se o chip não respondeu.
  Status init() override;
  void tick(uint32_t nowMs) override;
  // Para tudo e põe o chip em sono.
  Status shutdown() override;

  RadioState state() const { return state_; }
  ChipId chipId() const { return chipId_; }
  const RadioCounters &counters() const { return counters_; }
  TxPolicy &policy() { return policy_; }

  // Troca os parâmetros do rádio. Em recepção, a recepção recomeça com os novos.
  Status configure(const RadioConfig &config);
  const RadioConfig &radioConfig() const { return radioConfig_; }

  Status startReceive();
  Status stop();
  // Tira o pacote mais antigo da fila. false se ela está vazia.
  bool takePacket(RadioPacket *out);

  // Envia um pacote, se a política permitir. NotPermitted quando negado (o motivo vai
  // no evento kTxDenied e em lastDenial()).
  Status transmit(const uint8_t *data, size_t length);
  TxDenial lastDenial() const { return lastDenial_; }

  // Relê a política de transmissão e o cristal do ConfigStore.
  void applyConfig();

 private:
  Status bringUp();
  void setState(RadioState state);
  void noteError(Status status, uint32_t nowMs);
  void pollReceive(uint32_t nowMs);
  void pollTransmit(uint32_t nowMs);

  Cc1101 &chip_;
  EventBus &bus_;
  ConfigStore &config_;
  IClock &clock_;
  TxPolicy policy_;
  RadioConfig radioConfig_;
  uint8_t paTable_ = 0x12;
  ChipId chipId_{0, 0};
  RadioState state_ = RadioState::Off;
  bool initialized_ = false;
  bool wantReceive_ = false;
  uint8_t errors_ = 0;
  uint32_t lastPollMs_ = 0;
  uint32_t lastRetryMs_ = 0;
  uint32_t txStartMs_ = 0;
  uint32_t txTimeoutMs_ = 0;
  TxDenial lastDenial_ = TxDenial::None;
  RadioCounters counters_;
  RadioPacket queue_[kQueueSize];
  size_t queueHead_ = 0;
  size_t queueCount_ = 0;
};

}  // namespace dbmo::radio
