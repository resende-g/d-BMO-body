// Subsistema de energia: bateria, carregador, botão de energia, trava de alimentação e
// desligamento ordenado. Cada peça de hardware é opcional; o que não existe aparece como
// Unknown, nunca como um valor de fantasia.
//
// Desligamento em duas etapas:
//   1. alguém pede (toque longo no botão, bateria crítica ou comando): o subsistema
//      publica kShutdownRequested e fica em ShutdownRequested;
//   2. a aplicação encerra os outros subsistemas e chama completeShutdown(), que solta a
//      trava (ou entra em sono profundo, se não há trava).
#pragma once

#include <cstdint>

#include "dbmo/core/config_store.h"
#include "dbmo/core/event_bus.h"
#include "dbmo/core/hal/adc.h"
#include "dbmo/core/hal/gpio.h"
#include "dbmo/core/subsystem.h"
#include "dbmo/power/battery_monitor.h"
#include "dbmo/power/power_hal.h"

namespace dbmo::power {

enum class ShutdownReason : uint8_t { None, UserButton, BatteryCritical, Command };
enum class PowerPhase : uint8_t { Running, ShutdownRequested, Off };

const char *toString(ShutdownReason reason);

namespace power_event {
// a = BatteryLevel, b = tensão em mV (0 quando o nível é Unknown).
inline constexpr EventId kBatteryLevel = makeEventId(EventDomain::Power, 1);
// a = ChargeState.
inline constexpr EventId kChargeState = makeEventId(EventDomain::Power, 2);
// Toque curto no botão de energia.
inline constexpr EventId kButtonPress = makeEventId(EventDomain::Power, 3);
// a = ShutdownReason.
inline constexpr EventId kShutdownRequested = makeEventId(EventDomain::Power, 4);
// A leitura da bateria passou a falhar ou voltou. a = BatteryFault.
inline constexpr EventId kBatteryFault = makeEventId(EventDomain::Power, 5);
// Sem atividade pelo tempo de power.sleep_after_s.
inline constexpr EventId kIdleTimeout = makeEventId(EventDomain::Power, 6);
}  // namespace power_event

namespace config_key {
inline constexpr char kVbatEnabled[] = "power.vbat_enabled";
inline constexpr char kVbatRatio[] = "power.vbat_ratio_permille";
inline constexpr char kLowMv[] = "power.low_mv";
inline constexpr char kCriticalMv[] = "power.critical_mv";
inline constexpr char kHysteresisMv[] = "power.hysteresis_mv";
inline constexpr char kOffHoldMs[] = "power.off_hold_ms";
inline constexpr char kAutoOffCritical[] = "power.auto_off_critical";
inline constexpr char kSleepAfterS[] = "power.sleep_after_s";
}  // namespace config_key

class PowerSubsystem final : public ISubsystem {
 public:
  static constexpr uint32_t kButtonPollMs = 10;
  static constexpr uint8_t kButtonDebounceSamples = 3;
  static constexpr uint32_t kBatteryPeriodMs = 1000;
  // Amostras seguidas em nível crítico até o pedido de desligamento.
  static constexpr uint8_t kCriticalConfirmSamples = 5;

  PowerSubsystem(EventBus &bus, ConfigStore &config);

  static Status registerConfig(ConfigStore &config);

  // Ligações opcionais, antes de init().
  void setBatteryAdc(IAdcChannel &adc) { battery_.setAdc(&adc); }
  void setCharger(IChargerStatus &charger) { charger_ = &charger; }
  void setPowerSwitch(IPowerSwitch &powerSwitch) { switch_ = &powerSwitch; }
  void setSleepControl(ISleepControl &sleep) { sleep_ = &sleep; }
  void setPowerButton(IGpio &gpio, int pin, bool activeLow = true);

  const char *name() const override { return "power"; }
  // Segura a trava de alimentação antes de qualquer outra coisa.
  Status init() override;
  void tick(uint32_t nowMs) override;
  Status shutdown() override { return Status::Ok; }

  const BatteryMonitor &battery() const { return battery_; }
  ChargeState chargeState() const { return chargeState_; }
  PowerPhase phase() const { return phase_; }
  ShutdownReason shutdownReason() const { return reason_; }
  bool buttonPressed() const { return buttonStable_; }

  // Reinicia a contagem de inatividade.
  void notifyActivity(uint32_t nowMs);

  // Etapa 1 do desligamento. Sem efeito se já foi pedido.
  void requestShutdown(ShutdownReason reason);
  // Volta a Running (ex.: o carregador foi ligado antes de completeShutdown()).
  void cancelShutdown();
  // Etapa 2. Busy enquanto o botão de energia ainda está apertado (soltar a trava com o
  // botão fechado religaria o aparelho); Unsupported sem trava e sem controle de sono;
  // NotPermitted se o desligamento não foi pedido.
  Status completeShutdown();

  // Relê a configuração (chamar depois de alterá-la).
  void applyConfig();

 private:
  void pollButton(uint32_t nowMs);
  void pollBattery();

  EventBus &bus_;
  ConfigStore &config_;
  BatteryMonitor battery_;
  IChargerStatus *charger_ = nullptr;
  IPowerSwitch *switch_ = nullptr;
  ISleepControl *sleep_ = nullptr;
  IGpio *buttonGpio_ = nullptr;
  int buttonPin_ = -1;
  bool buttonActiveLow_ = true;

  bool initialized_ = false;
  PowerPhase phase_ = PowerPhase::Running;
  ShutdownReason reason_ = ShutdownReason::None;
  ChargeState chargeState_ = ChargeState::Unknown;
  BatteryLevel reportedLevel_ = BatteryLevel::Unknown;
  BatteryFault reportedFault_ = BatteryFault::Disabled;
  uint8_t criticalStreak_ = 0;

  // Botão: o toque que ligou o aparelho não conta. O botão só "arma" depois de solto.
  bool buttonArmed_ = false;
  bool buttonStable_ = false;
  uint8_t buttonCounter_ = 0;
  bool longFired_ = false;
  uint32_t pressStartMs_ = 0;
  uint32_t lastButtonPollMs_ = 0;
  uint32_t lastBatteryMs_ = 0;
  bool polledOnce_ = false;

  uint32_t offHoldMs_ = 1500;
  bool autoOffCritical_ = true;
  uint32_t sleepAfterMs_ = 0;
  uint32_t lastActivityMs_ = 0;
  bool idleReported_ = false;
};

}  // namespace dbmo::power
