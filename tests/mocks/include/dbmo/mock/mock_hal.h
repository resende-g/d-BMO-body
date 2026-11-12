// Mocks dos contratos de HAL do núcleo, para os testes no computador.
// Só cabeçalho: cada teste inclui e instancia o que precisa.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "dbmo/core/config_store.h"
#include "dbmo/core/hal/adc.h"
#include "dbmo/core/hal/clock.h"
#include "dbmo/core/hal/critical_section.h"
#include "dbmo/core/hal/gpio.h"
#include "dbmo/core/hal/i2c.h"
#include "dbmo/core/hal/spi.h"
#include "dbmo/core/log.h"
#include "dbmo/core/status.h"

namespace dbmo::mock {

// Relógio manual: o tempo só anda com advance*() ou com delay*().
class MockClock final : public IClock {
 public:
  uint32_t nowMs() override { return static_cast<uint32_t>(us_ / 1000); }
  uint64_t nowUs() override { return us_; }
  void delayMs(uint32_t ms) override {
    us_ += static_cast<uint64_t>(ms) * 1000;
    ++delayCalls;
  }
  void delayUs(uint32_t us) override {
    us_ += us;
    ++delayCalls;
  }

  void advanceMs(uint32_t ms) { us_ += static_cast<uint64_t>(ms) * 1000; }
  void advanceUs(uint64_t us) { us_ += us; }
  void setMs(uint32_t ms) { us_ = static_cast<uint64_t>(ms) * 1000; }

  unsigned delayCalls = 0;

 private:
  uint64_t us_ = 0;
};

// GPIOs 0..kPinCount-1. Entradas valem o que setInput() definiu (pull-up sem sinal = alto).
class MockGpio final : public IGpio {
 public:
  static constexpr int kPinCount = 64;

  Status configure(int pin, PinMode mode) override {
    if (!valid(pin)) return Status::InvalidArgument;
    if (!isOk(nextStatus_)) return takeFault();
    configured_[pin] = true;
    modes_[pin] = mode;
    ++configureCount;
    return Status::Ok;
  }

  Status write(int pin, bool high) override {
    if (!valid(pin)) return Status::InvalidArgument;
    if (!isOk(nextStatus_)) return takeFault();
    if (!configured_[pin] ||
        (modes_[pin] != PinMode::Output && modes_[pin] != PinMode::OutputOpenDrain))
      return Status::NotInitialized;
    outputs_[pin] = high;
    ++writeCount;
    return Status::Ok;
  }

  Result<bool> read(int pin) override {
    if (!valid(pin)) return Status::InvalidArgument;
    if (!isOk(nextStatus_)) return takeFault();
    if (!configured_[pin]) return Status::NotInitialized;
    if (modes_[pin] == PinMode::Output || modes_[pin] == PinMode::OutputOpenDrain)
      return outputs_[pin];
    if (inputDriven_[pin]) return inputs_[pin];
    return modes_[pin] == PinMode::InputPullUp;
  }

  // Nível aplicado de fora ao pino (botão, sensor).
  void setInput(int pin, bool high) {
    inputs_[pin] = high;
    inputDriven_[pin] = true;
  }
  void floatInput(int pin) { inputDriven_[pin] = false; }

  bool isConfigured(int pin) const { return valid(pin) && configured_[pin]; }
  PinMode mode(int pin) const { return modes_[pin]; }
  bool output(int pin) const { return outputs_[pin]; }

  // A próxima chamada devolve este erro (uma vez).
  void failNext(Status status) { nextStatus_ = status; }

  unsigned configureCount = 0;
  unsigned writeCount = 0;

 private:
  static bool valid(int pin) { return pin >= 0 && pin < kPinCount; }
  Status takeFault() {
    const Status status = nextStatus_;
    nextStatus_ = Status::Ok;
    return status;
  }

  bool configured_[kPinCount] = {};
  PinMode modes_[kPinCount] = {};
  bool outputs_[kPinCount] = {};
  bool inputs_[kPinCount] = {};
  bool inputDriven_[kPinCount] = {};
  Status nextStatus_ = Status::Ok;
};

// Um dispositivo simulado pendurado no barramento I2C falso.
class II2cDeviceSim {
 public:
  virtual ~II2cDeviceSim() = default;
  virtual Status onWrite(const uint8_t *data, size_t n) = 0;
  virtual Status onRead(uint8_t *data, size_t n) = 0;
};

// Barramento I2C falso: endereços sem dispositivo respondem HardwareAbsent (NACK).
class MockI2cBus final : public II2cBus {
 public:
  void attach(uint8_t address, II2cDeviceSim *device) { devices_[address & 0x7F] = device; }
  void detach(uint8_t address) { devices_[address & 0x7F] = nullptr; }

  // As próximas count operações devolvem este erro, sem chegar ao dispositivo.
  void failNext(Status status, unsigned count = 1) {
    faultStatus_ = status;
    faultCount_ = count;
  }

  Status write(uint8_t address, const uint8_t *data, size_t n) override {
    ++writeCount;
    lastAddress = address;
    if (faultCount_ > 0) return takeFault();
    II2cDeviceSim *device = find(address);
    if (device == nullptr) return Status::HardwareAbsent;
    return device->onWrite(data, n);
  }

  Status read(uint8_t address, uint8_t *data, size_t n) override {
    ++readCount;
    lastAddress = address;
    if (faultCount_ > 0) return takeFault();
    II2cDeviceSim *device = find(address);
    if (device == nullptr) return Status::HardwareAbsent;
    return device->onRead(data, n);
  }

  Status writeRead(uint8_t address, const uint8_t *tx, size_t txLen, uint8_t *rx,
                   size_t rxLen) override {
    ++writeReadCount;
    lastAddress = address;
    if (faultCount_ > 0) return takeFault();
    II2cDeviceSim *device = find(address);
    if (device == nullptr) return Status::HardwareAbsent;
    const Status status = device->onWrite(tx, txLen);
    if (!isOk(status)) return status;
    return device->onRead(rx, rxLen);
  }

  Status probe(uint8_t address) override {
    ++probeCount;
    if (faultCount_ > 0) return takeFault();
    return find(address) != nullptr ? Status::Ok : Status::HardwareAbsent;
  }

  unsigned writeCount = 0;
  unsigned readCount = 0;
  unsigned writeReadCount = 0;
  unsigned probeCount = 0;
  uint8_t lastAddress = 0;

 private:
  II2cDeviceSim *find(uint8_t address) { return address < 0x80 ? devices_[address] : nullptr; }
  Status takeFault() {
    --faultCount_;
    return faultStatus_;
  }

  II2cDeviceSim *devices_[128] = {};
  Status faultStatus_ = Status::Ok;
  unsigned faultCount_ = 0;
};

// Dispositivo I2C genérico de registradores de 8 bits: o primeiro byte escrito é o
// endereço do registrador; escritas e leituras seguintes avançam o ponteiro.
class RegisterDeviceSim : public II2cDeviceSim {
 public:
  Status onWrite(const uint8_t *data, size_t n) override {
    if (n == 0) return Status::Ok;
    pointer_ = data[0];
    for (size_t i = 1; i < n; ++i) {
      writeRegister(pointer_, data[i]);
      ++pointer_;
    }
    return Status::Ok;
  }

  Status onRead(uint8_t *data, size_t n) override {
    for (size_t i = 0; i < n; ++i) {
      data[i] = readRegister(pointer_);
      ++pointer_;
    }
    return Status::Ok;
  }

  uint8_t memory[256] = {};

 protected:
  virtual void writeRegister(uint8_t reg, uint8_t value) { memory[reg] = value; }
  virtual uint8_t readRegister(uint8_t reg) { return memory[reg]; }

 private:
  uint8_t pointer_ = 0;
};

// Dispositivo SPI falso: registra o que foi enviado e responde por um handler.
class MockSpiDevice final : public ISpiDevice {
 public:
  static constexpr size_t kLogCapacity = 512;
  // Calcula a resposta de uma chamada de transfer(). tx/rx podem ser nullptr.
  using Handler = void (*)(const uint8_t *tx, uint8_t *rx, size_t n, void *context);

  void setHandler(Handler handler, void *context) {
    handler_ = handler;
    handlerContext_ = context;
  }

  Status select() override {
    ++selectCount;
    if (!isOk(selectStatus)) return selectStatus;
    if (selected_) return Status::Busy;
    selected_ = true;
    return Status::Ok;
  }

  Status transfer(const uint8_t *tx, uint8_t *rx, size_t n) override {
    if (!selected_) return Status::NotInitialized;
    if (!isOk(transferStatus)) return transferStatus;
    ++transferCount;
    for (size_t i = 0; i < n && sentLength < kLogCapacity; ++i)
      sent[sentLength++] = tx != nullptr ? tx[i] : 0x00;
    if (rx != nullptr) memset(rx, 0, n);
    if (handler_ != nullptr) handler_(tx, rx, n, handlerContext_);
    return Status::Ok;
  }

  void deselect() override {
    if (selected_) ++deselectCount;
    selected_ = false;
  }

  Result<bool> readMiso() override {
    if (!misoReadable) return Status::Unsupported;
    return misoLevel;
  }

  bool selected() const { return selected_; }
  void clearLog() { sentLength = 0; }

  Status selectStatus = Status::Ok;
  Status transferStatus = Status::Ok;
  bool misoReadable = true;
  bool misoLevel = false;
  unsigned selectCount = 0;
  unsigned deselectCount = 0;
  unsigned transferCount = 0;
  uint8_t sent[kLogCapacity] = {};
  size_t sentLength = 0;

 private:
  Handler handler_ = nullptr;
  void *handlerContext_ = nullptr;
  bool selected_ = false;
};

class MockAdcChannel final : public IAdcChannel {
 public:
  Result<uint16_t> readRaw() override {
    ++readCount;
    if (!isOk(status)) return status;
    return raw;
  }
  uint16_t maxRaw() const override { return max; }
  Result<uint16_t> readMillivolts() override {
    ++readCount;
    if (!isOk(status)) return status;
    return millivolts;
  }

  uint16_t raw = 0;
  uint16_t max = 4095;
  uint16_t millivolts = 0;
  Status status = Status::Ok;
  unsigned readCount = 0;
};

class MockPwmOutput final : public IPwmOutput {
 public:
  Status setDutyPermille(uint16_t permille) override {
    if (!isOk(status)) return status;
    if (permille > 1000) return Status::OutOfRange;
    duty = permille;
    ++setCount;
    return Status::Ok;
  }

  uint16_t duty = 0;
  Status status = Status::Ok;
  unsigned setCount = 0;
};

class CountingCriticalSection final : public ICriticalSection {
 public:
  void enter() override {
    ++enterCount;
    ++depth;
  }
  void exit() override { --depth; }

  unsigned enterCount = 0;
  int depth = 0;
};

// Guarda as últimas mensagens de log para as asserções.
class RecordingLogSink final : public ILogSink {
 public:
  static constexpr size_t kCapacity = 32;

  void write(const LogRecord &record) override {
    Entry &entry = entries_[count % kCapacity];
    entry.level = record.level;
    snprintf(entry.tag, sizeof(entry.tag), "%s", record.tag);
    snprintf(entry.text, sizeof(entry.text), "%s", record.message);
    ++count;
  }

  // Alguma das mensagens guardadas contém este trecho?
  bool contains(const char *fragment) const {
    const size_t stored = count < kCapacity ? count : kCapacity;
    for (size_t i = 0; i < stored; ++i)
      if (strstr(entries_[i].text, fragment) != nullptr) return true;
    return false;
  }

  size_t countAtLevel(LogLevel level) const {
    const size_t stored = count < kCapacity ? count : kCapacity;
    size_t matches = 0;
    for (size_t i = 0; i < stored; ++i)
      if (entries_[i].level == level) ++matches;
    return matches;
  }

  void clear() { count = 0; }

  size_t count = 0;

 private:
  struct Entry {
    LogLevel level = LogLevel::Info;
    char tag[16] = {};
    char text[Logger::kMessageMax] = {};
  };
  Entry entries_[kCapacity];
};

// Persistência de configuração em memória.
class MemoryConfigBackend final : public IConfigBackend {
 public:
  Status load(char *buffer, size_t capacity, size_t *length) override {
    ++loadCount;
    if (!isOk(loadStatus)) return loadStatus;
    if (!exists) return Status::NotFound;
    if (stored > capacity) return Status::NoSpace;
    memcpy(buffer, text, stored);
    *length = stored;
    return Status::Ok;
  }

  Status save(const char *data, size_t length) override {
    ++saveCount;
    if (!isOk(saveStatus)) return saveStatus;
    if (length > sizeof(text)) return Status::NoSpace;
    memcpy(text, data, length);
    stored = length;
    exists = true;
    return Status::Ok;
  }

  void setText(const char *value) {
    stored = strlen(value);
    memcpy(text, value, stored);
    exists = true;
  }

  char text[ConfigStore::kSerializedMax] = {};
  size_t stored = 0;
  bool exists = false;
  Status loadStatus = Status::Ok;
  Status saveStatus = Status::Ok;
  unsigned loadCount = 0;
  unsigned saveCount = 0;
};

}  // namespace dbmo::mock
