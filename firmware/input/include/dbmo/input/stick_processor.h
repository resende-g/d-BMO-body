// Um analógico (dois eixos de potenciômetro lidos pelo ADC): calibração, zona morta e
// normalização para -1000..+1000.
#pragma once

#include <cstdint>

#include "dbmo/core/hal/adc.h"
#include "dbmo/core/status.h"
#include "dbmo/input/types.h"

namespace dbmo::input {

// Valores brutos do ADC nos extremos e no repouso de um eixo.
struct AxisCalibration {
  uint16_t min;
  uint16_t center;
  uint16_t max;
};

class StickProcessor {
 public:
  static constexpr uint8_t kCenterSamples = 8;
  // Mudança mínima (em milésimos) para update() avisar que a posição mudou.
  static constexpr int16_t kChangeThreshold = 20;

  StickProcessor(IAdcChannel &x, IAdcChannel &y);

  // Mede o centro com o analógico solto (média de kCenterSamples leituras por eixo).
  // Corrupt se o centro medido ficar colado em um dos extremos (peça com defeito ou
  // analógico segurado).
  Status calibrateCenter();

  // Entre begin e end, update() aprende o mínimo e o máximo de cada eixo: girar o
  // analógico por toda a volta. end devolve Corrupt (e mantém a calibração anterior) se
  // o curso aprendido for pequeno demais.
  void beginRangeCalibration();
  Status endRangeCalibration();
  bool calibratingRange() const { return learning_; }

  // Aplica uma calibração guardada. InvalidArgument se min < center < max não vale ou
  // se algum lado do curso tem menos de 5 % da escala do ADC.
  Status setCalibration(const AxisCalibration &x, const AxisCalibration &y);
  AxisCalibration calibrationX() const { return calX_; }
  AxisCalibration calibrationY() const { return calY_; }

  // Raio da zona morta, em milésimos do curso (0 a 900).
  Status setDeadzonePermille(uint16_t permille);
  void setInverted(bool invertX, bool invertY);

  // Lê os dois eixos. Devolve true se a posição mudou além do limiar. Em erro do ADC
  // devolve o erro e mantém a última posição.
  Result<bool> update();

  StickPosition position() const { return position_; }
  uint16_t consecutiveErrors() const { return errors_; }

 private:
  bool validCalibration(const AxisCalibration &calibration, uint16_t maxRaw) const;
  static int16_t normalize(uint16_t raw, const AxisCalibration &calibration);

  IAdcChannel &x_;
  IAdcChannel &y_;
  AxisCalibration calX_;
  AxisCalibration calY_;
  AxisCalibration learnedX_ = {};
  AxisCalibration learnedY_ = {};
  uint16_t deadzone_ = 120;
  bool invertX_ = false;
  bool invertY_ = false;
  bool learning_ = false;
  StickPosition position_;
  uint16_t errors_ = 0;
};

// Raiz quadrada inteira (maior r com r*r <= value).
uint32_t integerSqrt(uint32_t value);

}  // namespace dbmo::input
