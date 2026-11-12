// Implementação dos contratos de HAL do núcleo sobre o Arduino-ESP32 (core 3.x).
// É o único lugar do firmware próprio, junto com os demais arquivos desta pasta, que
// inclui cabeçalhos do Arduino. Não compila no computador.
#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <SPI.h>
#include <Wire.h>

#include "esp_timer.h"

#include "dbmo/core/config_store.h"
#include "dbmo/core/hal/adc.h"
#include "dbmo/core/hal/clock.h"
#include "dbmo/core/hal/critical_section.h"
#include "dbmo/core/hal/gpio.h"
#include "dbmo/core/hal/i2c.h"
#include "dbmo/core/hal/spi.h"
#include "dbmo/core/log.h"

namespace dbmo::platform::esp32 {

class ArduinoClock final : public IClock {
 public:
  uint32_t nowMs() override { return millis(); }
  uint64_t nowUs() override { return static_cast<uint64_t>(esp_timer_get_time()); }
  void delayMs(uint32_t ms) override { delay(ms); }
  void delayUs(uint32_t us) override { delayMicroseconds(us); }
};

class ArduinoGpio final : public IGpio {
 public:
  Status configure(int pin, PinMode mode) override {
    if (pin < 0 || pin > 48) return Status::InvalidArgument;
    switch (mode) {
      case PinMode::Input: pinMode(static_cast<uint8_t>(pin), INPUT); break;
      case PinMode::InputPullUp: pinMode(static_cast<uint8_t>(pin), INPUT_PULLUP); break;
      case PinMode::InputPullDown: pinMode(static_cast<uint8_t>(pin), INPUT_PULLDOWN); break;
      case PinMode::Output: pinMode(static_cast<uint8_t>(pin), OUTPUT); break;
      case PinMode::OutputOpenDrain: pinMode(static_cast<uint8_t>(pin), OUTPUT_OPEN_DRAIN); break;
    }
    return Status::Ok;
  }
  Status write(int pin, bool high) override {
    if (pin < 0 || pin > 48) return Status::InvalidArgument;
    digitalWrite(static_cast<uint8_t>(pin), high ? HIGH : LOW);
    return Status::Ok;
  }
  Result<bool> read(int pin) override {
    if (pin < 0 || pin > 48) return Status::InvalidArgument;
    return digitalRead(static_cast<uint8_t>(pin)) == HIGH;
  }
};

// Um dispositivo no SPI compartilhado. beginTransaction() do Arduino-ESP32 já trava o
// barramento, então tela, rádio e cartão não se atropelam.
class ArduinoSpiDevice final : public ISpiDevice {
 public:
  ArduinoSpiDevice(SPIClass &bus, int8_t csPin, int8_t misoPin, uint32_t clockHz,
                   uint8_t mode = SPI_MODE0)
      : bus_(bus), csPin_(csPin), misoPin_(misoPin), clockHz_(clockHz), mode_(mode) {}

  // Deixa o CS em alto. Chamar para todos os dispositivos antes de usar qualquer um.
  void begin() {
    pinMode(static_cast<uint8_t>(csPin_), OUTPUT);
    digitalWrite(static_cast<uint8_t>(csPin_), HIGH);
  }

  Status select() override {
    if (selected_) return Status::Busy;
    bus_.beginTransaction(SPISettings(clockHz_, SPI_MSBFIRST, mode_));
    digitalWrite(static_cast<uint8_t>(csPin_), LOW);
    selected_ = true;
    return Status::Ok;
  }

  Status transfer(const uint8_t *tx, uint8_t *rx, size_t n) override {
    if (!selected_) return Status::NotInitialized;
    if (n > 0) bus_.transferBytes(tx, rx, static_cast<uint32_t>(n));
    return Status::Ok;
  }

  void deselect() override {
    if (!selected_) return;
    digitalWrite(static_cast<uint8_t>(csPin_), HIGH);
    bus_.endTransaction();
    selected_ = false;
  }

  Result<bool> readMiso() override {
    if (misoPin_ < 0) return Status::Unsupported;
    return digitalRead(static_cast<uint8_t>(misoPin_)) == HIGH;
  }

 private:
  SPIClass &bus_;
  int8_t csPin_;
  int8_t misoPin_;
  uint32_t clockHz_;
  uint8_t mode_;
  bool selected_ = false;
};

class ArduinoI2cBus final : public II2cBus {
 public:
  explicit ArduinoI2cBus(TwoWire &wire) : wire_(wire) {}

  Status write(uint8_t address, const uint8_t *data, size_t n) override {
    wire_.beginTransmission(address);
    if (n > 0 && wire_.write(data, n) != n) {
      wire_.endTransmission(true);
      return Status::NoSpace;
    }
    return fromEndTransmission(wire_.endTransmission(true));
  }

  Status read(uint8_t address, uint8_t *data, size_t n) override {
    const size_t received = wire_.requestFrom(address, n, true);
    return drain(data, n, received);
  }

  Status writeRead(uint8_t address, const uint8_t *tx, size_t txLen, uint8_t *rx,
                   size_t rxLen) override {
    wire_.beginTransmission(address);
    if (txLen > 0 && wire_.write(tx, txLen) != txLen) {
      wire_.endTransmission(true);
      return Status::NoSpace;
    }
    const Status status = fromEndTransmission(wire_.endTransmission(false));
    if (!isOk(status)) return status;
    const size_t received = wire_.requestFrom(address, rxLen, true);
    return drain(rx, rxLen, received);
  }

  Status probe(uint8_t address) override {
    wire_.beginTransmission(address);
    return fromEndTransmission(wire_.endTransmission(true));
  }

 private:
  // Códigos de TwoWire::endTransmission(): 0 ok, 2 NACK, 5 timeout, demais = erro.
  static Status fromEndTransmission(uint8_t code) {
    switch (code) {
      case 0: return Status::Ok;
      case 2: return Status::HardwareAbsent;
      case 5: return Status::Timeout;
      default: return Status::IoError;
    }
  }

  Status drain(uint8_t *data, size_t wanted, size_t received) {
    size_t copied = 0;
    while (wire_.available() > 0 && copied < wanted)
      data[copied++] = static_cast<uint8_t>(wire_.read());
    while (wire_.available() > 0) (void)wire_.read();
    if (received == 0 && wanted > 0) return Status::HardwareAbsent;
    return copied == wanted ? Status::Ok : Status::IoError;
  }

  TwoWire &wire_;
};

// Canal do ADC1 com resolução de 12 bits. A atenuação é a padrão do core (11 dB).
class ArduinoAdcChannel final : public IAdcChannel {
 public:
  explicit ArduinoAdcChannel(uint8_t pin) : pin_(pin) {}
  Result<uint16_t> readRaw() override { return analogRead(pin_); }
  uint16_t maxRaw() const override { return 4095; }
  Result<uint16_t> readMillivolts() override {
    return static_cast<uint16_t>(analogReadMilliVolts(pin_));
  }

 private:
  uint8_t pin_;
};

class LedcPwmOutput final : public IPwmOutput {
 public:
  LedcPwmOutput(uint8_t pin, uint32_t frequencyHz) : pin_(pin), frequencyHz_(frequencyHz) {}

  bool begin() {
    attached_ = ledcAttach(pin_, frequencyHz_, kResolutionBits);
    return attached_;
  }

  Status setDutyPermille(uint16_t permille) override {
    if (!attached_) return Status::NotInitialized;
    if (permille > 1000) return Status::OutOfRange;
    const uint32_t maxDuty = (1u << kResolutionBits) - 1;
    return ledcWrite(pin_, maxDuty * permille / 1000) ? Status::Ok : Status::IoError;
  }

 private:
  static constexpr uint8_t kResolutionBits = 10;
  uint8_t pin_;
  uint32_t frequencyHz_;
  bool attached_ = false;
};

class PortCriticalSection final : public ICriticalSection {
 public:
  void enter() override { portENTER_CRITICAL(&mux_); }
  void exit() override { portEXIT_CRITICAL(&mux_); }

 private:
  portMUX_TYPE mux_ = portMUX_INITIALIZER_UNLOCKED;
};

// Log no console serial: "[   1234][I][tag] mensagem".
class SerialLogSink final : public ILogSink {
 public:
  explicit SerialLogSink(Print &out) : out_(out) {}
  void write(const LogRecord &record) override {
    out_.printf("[%8lu][%s][%s] %s\r\n", static_cast<unsigned long>(record.timestampMs),
                toString(record.level), record.tag, record.message);
  }

 private:
  Print &out_;
};

// Configurações na NVS (flash interna), para quando não há cartão. O texto inteiro vai
// em uma única chave.
class NvsConfigBackend final : public IConfigBackend {
 public:
  Status load(char *buffer, size_t capacity, size_t *length) override {
    Preferences prefs;
    if (!prefs.begin(kNamespace, true)) return Status::NotFound;
    const size_t stored = prefs.getBytesLength(kKey);
    if (stored == 0) {
      prefs.end();
      return Status::NotFound;
    }
    if (stored > capacity) {
      prefs.end();
      return Status::NoSpace;
    }
    const size_t read = prefs.getBytes(kKey, buffer, capacity);
    prefs.end();
    if (read != stored) return Status::IoError;
    *length = read;
    return Status::Ok;
  }

  Status save(const char *text, size_t length) override {
    Preferences prefs;
    if (!prefs.begin(kNamespace, false)) return Status::IoError;
    const size_t written = prefs.putBytes(kKey, text, length);
    prefs.end();
    return written == length ? Status::Ok : Status::IoError;
  }

 private:
  static constexpr char kNamespace[] = "dbmo";
  static constexpr char kKey[] = "config";
};

}  // namespace dbmo::platform::esp32
