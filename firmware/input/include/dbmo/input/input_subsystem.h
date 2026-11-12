// Subsistema de controles: consulta as origens de botões e os analógicos em período
// fixo, aplica o tratamento e publica eventos. Funciona com qualquer subconjunto do
// hardware (na bancada de hoje, só três botões em GPIO).
#pragma once

#include <cstddef>
#include <cstdint>

#include "dbmo/core/config_store.h"
#include "dbmo/core/event_bus.h"
#include "dbmo/core/subsystem.h"
#include "dbmo/input/button_processor.h"
#include "dbmo/input/button_source.h"
#include "dbmo/input/stick_processor.h"
#include "dbmo/input/types.h"

namespace dbmo::input {

namespace input_event {
// a = Button, b = ButtonAction.
inline constexpr EventId kButton = makeEventId(EventDomain::Input, 1);
// a = id da combinação.
inline constexpr EventId kCombo = makeEventId(EventDomain::Input, 2);
// a = Stick, b = x nos 16 bits baixos e y nos 16 altos (int16 cada, -1000..+1000).
inline constexpr EventId kStick = makeEventId(EventDomain::Input, 3);
// Uma origem de botões parou de responder. a = índice da origem, b = Status.
inline constexpr EventId kSourceFault = makeEventId(EventDomain::Input, 4);
// A origem voltou a responder. a = índice da origem.
inline constexpr EventId kSourceRecovered = makeEventId(EventDomain::Input, 5);
}  // namespace input_event

namespace config_key {
inline constexpr char kDebounceMs[] = "input.debounce_ms";
inline constexpr char kLongPressMs[] = "input.long_press_ms";
inline constexpr char kRepeatDelayMs[] = "input.repeat_delay_ms";
inline constexpr char kRepeatMs[] = "input.repeat_ms";
inline constexpr char kDeadzone[] = "input.deadzone";  // milésimos do curso
}  // namespace config_key

constexpr uint32_t packStick(StickPosition position) {
  return static_cast<uint16_t>(position.x) |
         (static_cast<uint32_t>(static_cast<uint16_t>(position.y)) << 16);
}
constexpr StickPosition unpackStick(uint32_t packed) {
  return StickPosition{static_cast<int16_t>(packed & 0xFFFF), static_cast<int16_t>(packed >> 16)};
}

class InputSubsystem final : public ISubsystem {
 public:
  static constexpr size_t kMaxSources = 2;
  static constexpr uint32_t kPollPeriodMs = 5;
  // Leituras seguidas com erro até a origem ser dada como em falha.
  static constexpr uint8_t kFaultThreshold = 5;
  // Intervalo entre tentativas de reiniciar uma origem em falha.
  static constexpr uint32_t kRetryPeriodMs = 1000;

  InputSubsystem(EventBus &bus, ConfigStore &config);

  static Status registerConfig(ConfigStore &config);

  // Registrar antes de init(). As máscaras das origens são somadas.
  Status addButtonSource(IButtonSource &source);
  void setStick(Stick which, StickProcessor &stick);

  const char *name() const override { return "input"; }
  // HardwareAbsent se nenhuma origem de botões respondeu.
  Status init() override;
  void tick(uint32_t nowMs) override;
  Status shutdown() override;

  ButtonProcessor &buttons() { return buttons_; }
  bool isPressed(Button button) const { return buttons_.isPressed(button); }
  StickPosition stick(Stick which) const;
  // Quantas origens de botões estão respondendo.
  size_t healthySources() const;

  // Relê os tempos e a zona morta do ConfigStore (chamar depois de alterá-los).
  void applyConfig();

 private:
  struct Source {
    IButtonSource *source;
    bool healthy;
    uint8_t errors;
    uint32_t lastRetryMs;
  };

  static void onButton(const ButtonEvent &event, void *context);
  static void onCombo(uint8_t comboId, void *context);

  EventBus &bus_;
  ConfigStore &config_;
  ButtonProcessor buttons_;
  Source sources_[kMaxSources] = {};
  size_t sourceCount_ = 0;
  StickProcessor *sticks_[2] = {nullptr, nullptr};
  bool initialized_ = false;
  bool polled_ = false;
  uint32_t lastPollMs_ = 0;
};

}  // namespace dbmo::input
