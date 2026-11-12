// Contratos de hardware do módulo de energia e as implementações que só dependem de GPIO.
// Carregador, trava de alimentação e modo de sono ainda não têm circuito definido: cada
// peça é opcional e o subsistema funciona sem ela.
#pragma once

#include <cstdint>

#include "dbmo/core/hal/gpio.h"
#include "dbmo/core/status.h"

namespace dbmo::power {

enum class ChargeState : uint8_t { Unknown, NotCharging, Charging, Full };

const char *toString(ChargeState state);

class IChargerStatus {
 public:
  virtual ~IChargerStatus() = default;
  virtual Result<ChargeState> read() = 0;
};

// Sem carregador conhecido: o estado é sempre Unknown, nunca um palpite.
class NullChargerStatus final : public IChargerStatus {
 public:
  Result<ChargeState> read() override { return ChargeState::Unknown; }
};

// Carregador com saídas de estado em dreno aberto (padrão em CIs de carga lineares):
// uma indica "carregando" e outra, opcional, "carga completa". Os pinos e o nível ativo
// dependem do CI que for escolhido.
class GpioChargerStatus final : public IChargerStatus {
 public:
  GpioChargerStatus(IGpio &gpio, int chargingPin, int fullPin = -1, bool activeLow = true)
      : gpio_(gpio), chargingPin_(chargingPin), fullPin_(fullPin), activeLow_(activeLow) {}

  Status begin();
  Result<ChargeState> read() override;

 private:
  IGpio &gpio_;
  int chargingPin_;
  int fullPin_;
  bool activeLow_;
};

// Trava que mantém o aparelho ligado depois que o botão é solto.
class IPowerSwitch {
 public:
  virtual ~IPowerSwitch() = default;
  virtual Status hold() = 0;     // manter ligado
  virtual Status release() = 0;  // cortar a alimentação
};

class GpioPowerLatch final : public IPowerSwitch {
 public:
  GpioPowerLatch(IGpio &gpio, int pin, bool activeHigh = true)
      : gpio_(gpio), pin_(pin), activeHigh_(activeHigh) {}

  Status hold() override;
  Status release() override;

 private:
  IGpio &gpio_;
  int pin_;
  bool activeHigh_;
};

struct WakeSource {
  int pin = -1;  // -1: sem pino de despertar
  bool activeLow = true;
};

// Sono profundo do microcontrolador. No hardware real, deepSleep() não retorna.
class ISleepControl {
 public:
  virtual ~ISleepControl() = default;
  virtual Status deepSleep(const WakeSource &wake) = 0;
};

}  // namespace dbmo::power
