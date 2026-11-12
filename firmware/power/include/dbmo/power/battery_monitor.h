// Leitura da tensão da bateria por um canal de ADC com divisor resistivo.
//
// Regra do projeto: nenhuma leitura é inventada. Sem sensor, com a leitura desabilitada,
// com o ADC saturado ou com um valor que uma célula de lítio não pode ter, o nível é
// Unknown e millivolts() devolve erro. Não há estimativa de porcentagem: ela dependeria
// de uma curva da célula real, que ainda não foi medida.
#pragma once

#include <cstddef>
#include <cstdint>

#include "dbmo/core/hal/adc.h"
#include "dbmo/core/status.h"

namespace dbmo::power {

enum class BatteryLevel : uint8_t { Unknown, Critical, Low, Normal };

enum class BatteryFault : uint8_t {
  None,
  Disabled,     // power.vbat_enabled = false
  NoSensor,     // nenhum canal de ADC foi ligado ao monitor
  ReadError,    // o ADC devolveu erro
  Saturated,    // tensão no pino acima da faixa calibrada do ADC
  Implausible,  // tensão que uma célula 1S ligada não pode ter (sensor solto, sem bateria)
};

const char *toString(BatteryLevel level);
const char *toString(BatteryFault fault);

// Todos os valores são PROVISÓRIOS: o divisor, o regulador e a célula ainda não foram
// definidos nem medidos (docs/hardware/POWER_ARCHITECTURE.md).
struct BatteryConfig {
  // Vbat = Vpino × ratioPermille / 1000. O README prevê divisor 100k/100k (×2).
  uint16_t ratioPermille = 2000;
  uint16_t lowMv = 3500;
  uint16_t criticalMv = 3300;
  uint16_t hysteresisMv = 100;
  // Acima disto no PINO a leitura do ESP32-S3 não é confiável (faixa calibrada com a
  // atenuação máxima: docs/hardware/sources/ESP32-S3.md).
  uint16_t adcCeilingMv = 2900;
  // Faixa em que uma célula 1S alimentando o aparelho pode estar.
  uint16_t minPlausibleMv = 2500;
  uint16_t maxPlausibleMv = 4350;
};

class BatteryMonitor {
 public:
  static constexpr size_t kWindow = 8;
  // Amostras inválidas seguidas até a leitura anterior ser descartada.
  static constexpr uint8_t kFaultSamples = 3;

  void setAdc(IAdcChannel *adc) { adc_ = adc; }
  void configure(const BatteryConfig &config) { config_ = config; }
  const BatteryConfig &config() const { return config_; }
  void setEnabled(bool enabled);
  bool enabled() const { return enabled_; }

  // Colhe uma amostra e atualiza o nível. Devolve o diagnóstico desta amostra.
  BatteryFault sample();

  bool hasReading() const { return count_ > 0; }
  // Média das últimas amostras válidas. NotInitialized enquanto não houver leitura.
  Result<uint16_t> millivolts() const;
  BatteryLevel level() const { return level_; }
  BatteryFault fault() const { return fault_; }

 private:
  void discard();
  void classify(uint16_t mv);

  IAdcChannel *adc_ = nullptr;
  BatteryConfig config_;
  bool enabled_ = false;
  uint16_t window_[kWindow] = {};
  size_t count_ = 0;
  size_t next_ = 0;
  uint8_t invalidStreak_ = 0;
  BatteryLevel level_ = BatteryLevel::Unknown;
  BatteryFault fault_ = BatteryFault::Disabled;
};

}  // namespace dbmo::power
