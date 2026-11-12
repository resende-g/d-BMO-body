// Adaptadores dos contratos de hardware dos subsistemas sobre o Arduino-ESP32 (core 3.x):
// cartão SD, áudio I2S, infravermelho por RMT e sono profundo. Não compila no
// computador; a lógica que usa estes adaptadores está nas bibliotecas portáveis.
//
// Nada aqui foi executado em hardware ainda: a CI só garante que compila.
#pragma once

#include <Arduino.h>
#include <ESP_I2S.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>

#include "esp_sleep.h"

#include "dbmo/audio/audio_types.h"
#include "dbmo/infrared/ir_types.h"
#include "dbmo/power/power_hal.h"
#include "dbmo/storage/filesystem.h"

namespace dbmo::platform::esp32 {

// ---------------------------------------------------------------------------
// Cartão microSD pela biblioteca SD (SPI).
// ---------------------------------------------------------------------------
class SdFileSystem final : public storage::IFileSystem {
 public:
  static constexpr size_t kMaxOpen = 4;

  SdFileSystem(SPIClass &spi, uint8_t csPin, uint32_t clockHz)
      : spi_(spi), csPin_(csPin), clockHz_(clockHz) {}

  Status mount() override {
    if (mounted_) return Status::Ok;
    if (!SD.begin(csPin_, spi_, clockHz_)) return Status::HardwareAbsent;
    if (SD.cardType() == CARD_NONE) {
      SD.end();
      return Status::HardwareAbsent;
    }
    mounted_ = true;
    return Status::Ok;
  }

  void unmount() override {
    for (size_t i = 0; i < kMaxOpen; ++i) {
      if (used_[i]) files_[i].close();
      used_[i] = false;
    }
    if (mounted_) SD.end();
    mounted_ = false;
  }

  bool mounted() const override { return mounted_; }

  Result<storage::FileHandle> open(const char *path, storage::OpenMode mode) override {
    if (!mounted_) return Status::NotInitialized;
    int slot = -1;
    for (size_t i = 0; i < kMaxOpen; ++i) {
      if (!used_[i]) {
        slot = static_cast<int>(i);
        break;
      }
    }
    if (slot < 0) return Status::Busy;
    if (mode == storage::OpenMode::Read && !SD.exists(path)) return Status::NotFound;
    const char *arduinoMode = mode == storage::OpenMode::Read    ? FILE_READ
                              : mode == storage::OpenMode::Write ? FILE_WRITE
                                                                 : FILE_APPEND;
    File file = SD.open(path, arduinoMode);
    if (!file) return Status::IoError;
    if (file.isDirectory()) {
      file.close();
      return Status::InvalidArgument;
    }
    files_[slot] = file;
    used_[slot] = true;
    return static_cast<storage::FileHandle>(slot);
  }

  Result<size_t> read(storage::FileHandle file, void *buffer, size_t n) override {
    if (!valid(file)) return Status::InvalidArgument;
    return files_[file].read(static_cast<uint8_t *>(buffer), n);
  }

  Result<size_t> write(storage::FileHandle file, const void *data, size_t n) override {
    if (!valid(file)) return Status::InvalidArgument;
    if (n == 0) return static_cast<size_t>(0);
    const size_t written = files_[file].write(static_cast<const uint8_t *>(data), n);
    // A biblioteca não diferencia "cartão cheio" de erro: zero bytes é tratado como erro.
    if (written == 0) return Status::IoError;
    return written;
  }

  Status sync(storage::FileHandle file) override {
    if (!valid(file)) return Status::InvalidArgument;
    files_[file].flush();
    return Status::Ok;
  }

  Status close(storage::FileHandle file) override {
    if (!valid(file)) return Status::InvalidArgument;
    files_[file].close();
    used_[file] = false;
    return Status::Ok;
  }

  bool exists(const char *path) override { return mounted_ && SD.exists(path); }

  Result<uint32_t> fileSize(const char *path) override {
    if (!mounted_) return Status::NotInitialized;
    if (!SD.exists(path)) return Status::NotFound;
    File file = SD.open(path, FILE_READ);
    if (!file) return Status::IoError;
    const uint32_t size = static_cast<uint32_t>(file.size());
    file.close();
    return size;
  }

  Status remove(const char *path) override {
    if (!mounted_) return Status::NotInitialized;
    if (!SD.exists(path)) return Status::NotFound;
    File file = SD.open(path, FILE_READ);
    const bool directory = file && file.isDirectory();
    if (file) file.close();
    const bool removed = directory ? SD.rmdir(path) : SD.remove(path);
    return removed ? Status::Ok : Status::IoError;
  }

  Status rename(const char *from, const char *to) override {
    if (!mounted_) return Status::NotInitialized;
    return SD.rename(from, to) ? Status::Ok : Status::IoError;
  }

  Status makeDir(const char *path) override {
    if (!mounted_) return Status::NotInitialized;
    return SD.mkdir(path) ? Status::Ok : Status::IoError;
  }

  Status list(const char *path, storage::DirVisitor visitor, void *context) override {
    if (!mounted_) return Status::NotInitialized;
    File directory = SD.open(path, FILE_READ);
    if (!directory || !directory.isDirectory()) {
      if (directory) directory.close();
      return Status::NotFound;
    }
    for (File entry = directory.openNextFile(); entry; entry = directory.openNextFile()) {
      const storage::DirEntry info{entry.name(), static_cast<bool>(entry.isDirectory()),
                                   static_cast<uint32_t>(entry.size())};
      const bool more = visitor(info, context);
      entry.close();
      if (!more) break;
    }
    directory.close();
    return Status::Ok;
  }

 private:
  bool valid(storage::FileHandle file) const {
    return mounted_ && file >= 0 && static_cast<size_t>(file) < kMaxOpen && used_[file];
  }

  SPIClass &spi_;
  uint8_t csPin_;
  uint32_t clockHz_;
  bool mounted_ = false;
  File files_[kMaxOpen];
  bool used_[kMaxOpen] = {};
};

// ---------------------------------------------------------------------------
// Áudio I2S: amplificador MAX98357A e microfone INMP441 no mesmo par de clocks.
// Quadros estéreo de 32 bits: são os 64 ciclos de BCLK por quadro que o microfone exige
// (MIC-04) e que o amplificador aceita (AMP-09).
// ---------------------------------------------------------------------------
class I2sAudio {
 public:
  static constexpr size_t kChunk = 64;  // amostras convertidas por vez (512 bytes na pilha)

  I2sAudio(int8_t bclkPin, int8_t wsPin, int8_t doutPin, int8_t dinPin, int8_t amplifierPin)
      : bclkPin_(bclkPin),
        wsPin_(wsPin),
        doutPin_(doutPin),
        dinPin_(dinPin),
        amplifierPin_(amplifierPin),
        output_(*this),
        input_(*this) {}

  audio::IAudioOutput &output() { return output_; }
  audio::IAudioInput &input() { return input_; }

 private:
  class Output final : public audio::IAudioOutput {
   public:
    explicit Output(I2sAudio &owner) : owner_(owner) {}

    Status begin(uint32_t sampleRateHz) override {
      pinMode(static_cast<uint8_t>(owner_.amplifierPin_), OUTPUT);
      digitalWrite(static_cast<uint8_t>(owner_.amplifierPin_), LOW);
      return owner_.start(sampleRateHz);
    }

    Status setAmplifier(bool on) override {
      // SD_MODE em nível alto liga o amplificador no canal esquerdo; em baixo, desliga
      // (AMP-06). Ver AMP-07 sobre acionar o pino direto com VDD abaixo de 3,3 V.
      digitalWrite(static_cast<uint8_t>(owner_.amplifierPin_), on ? HIGH : LOW);
      return Status::Ok;
    }

    Result<size_t> write(const int16_t *samples, size_t n) override {
      if (!owner_.started_) return Status::NotInitialized;
      size_t accepted = 0;
      int32_t frames[2 * kChunk];
      while (accepted < n) {
        const size_t count = n - accepted < kChunk ? n - accepted : kChunk;
        for (size_t i = 0; i < count; ++i) {
          const int32_t value = static_cast<int32_t>(samples[accepted + i]) * 65536;
          frames[2 * i] = value;      // esquerdo
          frames[2 * i + 1] = value;  // direito
        }
        size_t written = 0;
        // Tempo de espera zero: grava o que couber na fila e volta.
        const esp_err_t result =
            i2s_channel_write(owner_.i2s_.txChan(), frames, count * 8, &written, 0);
        if (result != ESP_OK && result != ESP_ERR_TIMEOUT) return Status::IoError;
        const size_t done = written / 8;
        accepted += done;
        if (done < count) break;
      }
      return accepted;
    }

    Status end() override {
      digitalWrite(static_cast<uint8_t>(owner_.amplifierPin_), LOW);
      owner_.stop();
      return Status::Ok;
    }

   private:
    I2sAudio &owner_;
  };

  class Input final : public audio::IAudioInput {
   public:
    explicit Input(I2sAudio &owner) : owner_(owner) {}

    Status begin(uint32_t sampleRateHz) override {
      DBMO_TRY(owner_.start(sampleRateHz));
      if (!owner_.rxEnabled_) {
        if (i2s_channel_enable(owner_.i2s_.rxChan()) != ESP_OK) return Status::IoError;
        owner_.rxEnabled_ = true;
      }
      return Status::Ok;
    }

    Result<size_t> read(int16_t *samples, size_t n) override {
      if (!owner_.started_ || !owner_.rxEnabled_) return Status::NotInitialized;
      int32_t frames[2 * kChunk];
      const size_t count = n < kChunk ? n : kChunk;
      size_t got = 0;
      const esp_err_t result = i2s_channel_read(owner_.i2s_.rxChan(), frames, count * 8, &got, 0);
      if (result != ESP_OK && result != ESP_ERR_TIMEOUT) return Status::IoError;
      const size_t done = got / 8;
      // O INMP441 com L/R em nível baixo entrega no canal esquerdo (MIC-06), 24 bits
      // alinhados à esquerda da palavra de 32.
      for (size_t i = 0; i < done; ++i) samples[i] = static_cast<int16_t>(frames[2 * i] >> 16);
      return done;
    }

    Status end() override {
      if (owner_.started_ && owner_.rxEnabled_) {
        (void)i2s_channel_disable(owner_.i2s_.rxChan());
        owner_.rxEnabled_ = false;
      }
      return Status::Ok;
    }

   private:
    I2sAudio &owner_;
  };

  Status start(uint32_t sampleRateHz) {
    if (started_ && rate_ == sampleRateHz) return Status::Ok;
    stop();
    i2s_.setPins(bclkPin_, wsPin_, doutPin_, dinPin_);
    if (!i2s_.begin(I2S_MODE_STD, sampleRateHz, I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO))
      return Status::IoError;
    // begin() liga a recepção junto; ela só deve ficar ligada durante uma gravação.
    if (i2s_.rxChan() != nullptr) (void)i2s_channel_disable(i2s_.rxChan());
    rxEnabled_ = false;
    started_ = true;
    rate_ = sampleRateHz;
    return Status::Ok;
  }

  void stop() {
    if (started_) (void)i2s_.end();
    started_ = false;
    rxEnabled_ = false;
  }

  int8_t bclkPin_;
  int8_t wsPin_;
  int8_t doutPin_;
  int8_t dinPin_;
  int8_t amplifierPin_;
  I2SClass i2s_;
  bool started_ = false;
  bool rxEnabled_ = false;
  uint32_t rate_ = 0;
  Output output_;
  Input input_;
};

// ---------------------------------------------------------------------------
// Infravermelho pelo periférico RMT, com 1 µs por tique.
// ---------------------------------------------------------------------------
class RmtIrReceiver final : public infrared::IIrReceiver {
 public:
  // activeLow: receptores demodulados (TSOP) baixam a saída enquanto há portadora.
  explicit RmtIrReceiver(int pin, bool activeLow = true) : pin_(pin), activeLow_(activeLow) {}

  Status begin() override {
    if (!rmtInit(pin_, RMT_RX_MODE, RMT_MEM_NUM_BLOCKS_2, 1000000)) return Status::IoError;
    // Filtro de ruído de 2 µs (o limite do periférico é de poucos microssegundos) e fim
    // de rajada depois de 12 ms sem borda.
    (void)rmtSetRxMinThreshold(pin_, 2);
    if (!rmtSetRxMaxThreshold(pin_, 12000)) return Status::IoError;
    return arm();
  }

  Result<bool> read(infrared::RawTiming *out) override {
    if (!armed_) {
      DBMO_TRY(arm());
      return false;
    }
    if (!rmtReceiveCompleted(pin_)) return false;

    const uint32_t markLevel = activeLow_ ? 0 : 1;
    out->count = 0;
    out->carrierHz = 38000;  // o receptor entrega o sinal já demodulado
    bool done = false;
    for (size_t i = 0; i < symbolCount_ && !done; ++i) {
      const uint32_t durations[2] = {data_[i].duration0, data_[i].duration1};
      const uint32_t levels[2] = {data_[i].level0, data_[i].level1};
      for (int half = 0; half < 2; ++half) {
        if (durations[half] == 0) {  // fim da rajada
          done = true;
          break;
        }
        // A lista começa por uma marca: descarta o que vier antes.
        if (out->count == 0 && levels[half] != markLevel) continue;
        if (out->count >= infrared::RawTiming::kMaxEntries) {
          done = true;
          break;
        }
        out->durationsUs[out->count++] = static_cast<uint16_t>(durations[half]);
      }
    }
    armed_ = false;
    DBMO_TRY(arm());
    return out->count > 0;
  }

 private:
  static constexpr size_t kSymbols = infrared::RawTiming::kMaxEntries / 2;

  Status arm() {
    symbolCount_ = kSymbols;
    armed_ = rmtReadAsync(pin_, data_, &symbolCount_);
    return armed_ ? Status::Ok : Status::IoError;
  }

  int pin_;
  bool activeLow_;
  bool armed_ = false;
  rmt_data_t data_[kSymbols] = {};
  size_t symbolCount_ = 0;
};

class RmtIrTransmitter final : public infrared::IIrTransmitter {
 public:
  explicit RmtIrTransmitter(int pin) : pin_(pin) {}

  Status begin() override {
    if (!rmtInit(pin_, RMT_TX_MODE, RMT_MEM_NUM_BLOCKS_2, 1000000)) return Status::IoError;
    (void)rmtSetEOT(pin_, 0);  // LED apagado ao terminar
    return Status::Ok;
  }

  Status send(const infrared::RawTiming &timing) override {
    if (busy()) return Status::Busy;
    if (timing.count == 0 || timing.count > infrared::RawTiming::kMaxEntries)
      return Status::InvalidArgument;
    size_t symbols = 0;
    for (uint16_t i = 0; i < timing.count; i += 2) {
      const uint32_t mark = timing.durationsUs[i];
      const uint32_t space = i + 1 < timing.count ? timing.durationsUs[i + 1] : 0;
      // Cada metade de um símbolo RMT tem 15 bits de duração.
      if (mark > kMaxDurationUs || space > kMaxDurationUs) return Status::InvalidArgument;
      data_[symbols].duration0 = mark;
      data_[symbols].level0 = 1;
      data_[symbols].duration1 = space;
      data_[symbols].level1 = 0;
      ++symbols;
    }
    if (!rmtSetCarrier(pin_, true, true, timing.carrierHz, 0.33f)) return Status::IoError;
    if (!rmtWriteAsync(pin_, data_, symbols)) return Status::IoError;
    sending_ = true;
    return Status::Ok;
  }

  bool busy() override {
    if (sending_ && rmtTransmitCompleted(pin_)) sending_ = false;
    return sending_;
  }

 private:
  static constexpr uint32_t kMaxDurationUs = 32767;
  static constexpr size_t kSymbols = infrared::RawTiming::kMaxEntries / 2;

  int pin_;
  bool sending_ = false;
  rmt_data_t data_[kSymbols] = {};
};

// ---------------------------------------------------------------------------
// Sono profundo com despertar por um pino (precisa ser GPIO de RTC: 0 a 21 no S3).
// ---------------------------------------------------------------------------
class DeepSleepControl final : public power::ISleepControl {
 public:
  Status deepSleep(const power::WakeSource &wake) override {
    // Sem pino de despertar o aparelho só voltaria com um reset: recusa.
    if (wake.pin < 0) return Status::InvalidArgument;
    if (esp_sleep_enable_ext0_wakeup(static_cast<gpio_num_t>(wake.pin), wake.activeLow ? 0 : 1) !=
        ESP_OK)
      return Status::InvalidArgument;
    Serial.flush();
    esp_deep_sleep_start();  // não retorna: o despertar é um novo boot
    return Status::Ok;
  }
};

}  // namespace dbmo::platform::esp32
