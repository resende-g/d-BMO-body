// Gerência da tela acima do IDisplay: brilho configurado e economia por inatividade
// (acesa -> esmaecida -> apagada). Não desenha nada além de limpar a tela.
#pragma once

#include <cstdint>

#include "dbmo/core/config_store.h"
#include "dbmo/core/event_bus.h"
#include "dbmo/core/subsystem.h"
#include "dbmo/display/display.h"

namespace dbmo::display {

enum class ScreenPower : uint8_t { On, Dimmed, Off };

namespace display_event {
// a = estado novo, b = estado anterior (ScreenPower).
inline constexpr EventId kPowerChanged = makeEventId(EventDomain::Display, 1);
}  // namespace display_event

namespace config_key {
inline constexpr char kBrightness[] = "display.brightness";  // 0 a 100 %
inline constexpr char kDimAfterS[] = "display.dim_after_s";  // 0 desliga o esmaecimento
inline constexpr char kOffAfterS[] = "display.off_after_s";  // 0 nunca apaga
}  // namespace config_key

class DisplaySubsystem final : public ISubsystem {
 public:
  // Brilho usado no estado Dimmed, em % do brilho configurado.
  static constexpr uint8_t kDimFactorPercent = 25;

  DisplaySubsystem(IDisplay &display, EventBus &bus, ConfigStore &config);

  // Declara as chaves de configuração deste módulo. Chamar antes de carregar o arquivo.
  static Status registerConfig(ConfigStore &config);

  const char *name() const override { return "display"; }
  Status init() override;
  void tick(uint32_t nowMs) override;
  Status shutdown() override;

  // Houve interação do usuário: acorda a tela e reinicia a contagem de inatividade.
  void notifyActivity(uint32_t nowMs);

  // Altera o brilho configurado (0 a 100) e aplica na hora.
  Status setBrightness(uint8_t percent);

  ScreenPower power() const { return power_; }
  // Falso quando o hardware não controla o backlight (LED da tela direto no 3V3).
  bool backlightControlled() const { return backlightControlled_; }
  IDisplay &display() { return display_; }

 private:
  Status applyBacklight(uint8_t percent);
  void setPower(ScreenPower power);
  uint8_t configuredBrightness() const;

  IDisplay &display_;
  EventBus &bus_;
  ConfigStore &config_;
  ScreenPower power_ = ScreenPower::On;
  bool backlightControlled_ = true;
  bool initialized_ = false;
  bool activitySeen_ = false;
  uint32_t lastActivityMs_ = 0;
};

}  // namespace dbmo::display
