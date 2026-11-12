// Subsistema de NFC: identifica o PN532 e, quando a busca está ligada, informa a chegada
// e a saída de etiquetas ISO 14443-A pelo identificador (UID).
//
// A busca vem DESLIGADA (campo de RF desligado): o aparelho só energiza etiquetas quando
// o usuário pede. O UID fica só na memória, não é gravado em log nem em arquivo, e os
// eventos carregam um resumo (CRC-32) no lugar do UID.
#pragma once

#include <cstdint>

#include "dbmo/core/config_store.h"
#include "dbmo/core/event_bus.h"
#include "dbmo/core/hal/clock.h"
#include "dbmo/core/subsystem.h"
#include "dbmo/nfc/pn532.h"

namespace dbmo::nfc {

enum class NfcState : uint8_t { Off, Idle, Scanning, Fault };

const char *toString(NfcState state);

namespace nfc_event {
// Etiqueta nova no campo. a = tamanho do UID, b = CRC-32 do UID.
inline constexpr EventId kTagDetected = makeEventId(EventDomain::Nfc, 1);
// A etiqueta saiu do campo. b = CRC-32 do UID.
inline constexpr EventId kTagRemoved = makeEventId(EventDomain::Nfc, 2);
// a = NfcState.
inline constexpr EventId kStateChanged = makeEventId(EventDomain::Nfc, 3);
// O chip parou de responder. a = Status.
inline constexpr EventId kFault = makeEventId(EventDomain::Nfc, 4);
}  // namespace nfc_event

namespace config_key {
inline constexpr char kScanEnabled[] = "nfc.scan_enabled";
inline constexpr char kScanPeriodMs[] = "nfc.scan_period_ms";
}  // namespace config_key

class NfcSubsystem final : public ISubsystem {
 public:
  // Tempo máximo de cada comando da inicialização (o único trecho que espera).
  static constexpr uint32_t kInitCommandTimeoutMs = 150;
  static constexpr uint8_t kFaultThreshold = 3;
  static constexpr uint32_t kRetryPeriodMs = 5000;
  // Buscas vazias seguidas até a etiqueta ser dada como retirada.
  static constexpr uint8_t kMissesToRemove = 2;
  static constexpr uint8_t kExpectedIc = 0x32;

  NfcSubsystem(Pn532 &chip, EventBus &bus, ConfigStore &config, IClock &clock);

  static Status registerConfig(ConfigStore &config);

  const char *name() const override { return "nfc"; }
  // HardwareAbsent se o chip não responde ou não é um PN532.
  Status init() override;
  void tick(uint32_t nowMs) override;
  // Desliga o campo de RF.
  Status shutdown() override;

  NfcState state() const { return state_; }
  FirmwareVersion firmware() const { return firmware_; }

  Status startScan();
  Status stopScan();
  // Etiqueta que está no campo agora. false se não há.
  bool currentTag(TagInfo *out) const;

 private:
  enum class Step : uint8_t { None, Find, FieldOff };

  Status bringUp();
  Status runBlocking();  // espera limitada pelo comando em andamento (só na inicialização)
  void setState(NfcState state);
  void noteError(Status status, uint32_t nowMs);
  void handleScanResult(Status status);
  void dropTag();

  Pn532 &chip_;
  EventBus &bus_;
  ConfigStore &config_;
  IClock &clock_;
  NfcState state_ = NfcState::Off;
  bool initialized_ = false;
  bool wantScan_ = false;
  FirmwareVersion firmware_{};
  Step step_ = Step::None;
  uint32_t scanPeriodMs_ = 250;
  uint32_t lastScanMs_ = 0;
  uint32_t lastRetryMs_ = 0;
  uint8_t errors_ = 0;
  bool tagPresent_ = false;
  TagInfo tag_{};
  uint8_t misses_ = 0;
  bool fieldOffPending_ = false;
};

}  // namespace dbmo::nfc
