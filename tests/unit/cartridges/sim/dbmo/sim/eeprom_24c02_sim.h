// Simulador de uma EEPROM 24C02, escrito a partir dos datasheets da Microchip 24LC02B e
// da Atmel AT24C02 (docs/hardware/sources/PERIPHERALS.md, EE-01 a EE-09):
//   - o primeiro byte de uma escrita é o endereço interno; uma leitura continua dele;
//   - uma escrita que passa do fim da página volta ao início da MESMA página;
//   - depois de uma escrita o chip fica sem responder durante o ciclo de gravação;
//   - com a proteção de escrita ligada, o comando é aceito e nada é gravado.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "dbmo/core/hal/clock.h"
#include "dbmo/mock/mock_hal.h"

namespace dbmo::sim {

class Eeprom24c02Sim final : public mock::II2cDeviceSim {
 public:
  explicit Eeprom24c02Sim(IClock &clock, uint8_t pageSize = 8)
      : clock_(clock), pageSize_(pageSize) {
    memset(memory, 0xFF, sizeof(memory));  // como sai de fábrica
  }

  Status onWrite(const uint8_t *data, size_t n) override {
    if (busy()) return Status::HardwareAbsent;  // em ciclo de gravação: sem ACK
    if (n == 0) return Status::Ok;
    pointer_ = data[0];
    if (n == 1) return Status::Ok;
    const uint8_t pageBase = static_cast<uint8_t>(pointer_ - (pointer_ % pageSize_));
    for (size_t i = 1; i < n; ++i) {
      const uint8_t address = static_cast<uint8_t>(pageBase + ((pointer_ - pageBase) % pageSize_));
      if (!writeProtect) memory[address] = data[i];
      pointer_ = static_cast<uint8_t>(pageBase + ((address - pageBase + 1) % pageSize_));
    }
    if (!writeProtect) {
      writing_ = true;
      writeStartMs_ = clock_.nowMs();
      ++writeCycles;
    }
    return Status::Ok;
  }

  Status onRead(uint8_t *data, size_t n) override {
    if (busy()) return Status::HardwareAbsent;
    for (size_t i = 0; i < n; ++i) data[i] = memory[pointer_++];  // volta a 0 depois de 255
    return Status::Ok;
  }

  void load(const uint8_t *image, size_t n) { memcpy(memory, image, n < 256 ? n : 256); }

  uint8_t memory[256];
  bool writeProtect = false;
  uint32_t writeCycleMs = 5;
  unsigned writeCycles = 0;

 private:
  bool busy() {
    if (writing_ && elapsedMs(clock_.nowMs(), writeStartMs_) >= writeCycleMs) writing_ = false;
    return writing_;
  }

  IClock &clock_;
  uint8_t pageSize_;
  uint8_t pointer_ = 0;
  bool writing_ = false;
  uint32_t writeStartMs_ = 0;
};

}  // namespace dbmo::sim
