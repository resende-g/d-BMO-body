// Simulador do CC1101 no nível do SPI: cabeçalho (leitura/rajada/endereço), comandos,
// registradores de configuração e de status, FIFOs de RX e TX e a máquina de estados
// principal. Modela o que o driver usa; não modela o rádio em si.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "dbmo/core/hal/spi.h"
#include "dbmo/radio/cc1101_regs.h"

namespace dbmo::sim {

class Cc1101Sim final : public ISpiDevice {
 public:
  Cc1101Sim() { resetChip(); }

  Status select() override {
    ++selectCount;
    if (!isOk(selectStatus)) return selectStatus;
    selected_ = true;
    headerPending_ = true;
    if (state == radio::cc1101::kStateSleep && present) state = radio::cc1101::kStateIdle;
    return Status::Ok;
  }

  Status transfer(const uint8_t *tx, uint8_t *rx, size_t n) override {
    if (!selected_) return Status::NotInitialized;
    if (failTransfers > 0) {
      --failTransfers;
      return Status::IoError;
    }
    for (size_t i = 0; i < n; ++i) {
      const uint8_t out = present ? step(tx != nullptr ? tx[i] : 0) : absentValue;
      if (rx != nullptr) rx[i] = out;
    }
    return Status::Ok;
  }

  void deselect() override {
    selected_ = false;
    if (sleepPending_) {
      sleepPending_ = false;
      state = radio::cc1101::kStateSleep;
    }
  }

  Result<bool> readMiso() override {
    if (!misoReadable) return Status::Unsupported;
    return !present;  // chip presente e acordado: MISO baixo ("pronto")
  }

  // ---- Controle do teste ----

  // Entrega um pacote recebido pelo ar. false se o chip não está em recepção.
  bool injectPacket(const uint8_t *data, uint8_t length, uint8_t packetRssi = 0x80,
                    bool crcOk = true) {
    if (state != radio::cc1101::kStateRx) return false;
    const size_t needed = static_cast<size_t>(length) + 3;
    if (rxCount + needed > radio::cc1101::kFifoSize) {
      rxOverflow = true;
      state = radio::cc1101::kStateRxOverflow;
      return true;
    }
    rxFifo[rxCount++] = length;
    memcpy(rxFifo + rxCount, data, length);
    rxCount += length;
    rxFifo[rxCount++] = packetRssi;
    rxFifo[rxCount++] = static_cast<uint8_t>((crcOk ? 0x80 : 0x00) | 0x2A);
    state = radio::cc1101::kStateIdle;  // RXOFF_MODE = IDLE
    return true;
  }
  // Coloca bytes crus na FIFO de RX (para testar pacotes malformados).
  void injectRaw(const uint8_t *bytes, size_t n) {
    memcpy(rxFifo + rxCount, bytes, n);
    rxCount += n;
    state = radio::cc1101::kStateIdle;
  }
  void forceOverflow() {
    rxOverflow = true;
    state = radio::cc1101::kStateRxOverflow;
  }
  uint32_t frequencyWord() const {
    return (static_cast<uint32_t>(registers[radio::cc1101::kFreq2]) << 16) |
           (static_cast<uint32_t>(registers[radio::cc1101::kFreq1]) << 8) |
           registers[radio::cc1101::kFreq0];
  }

  bool present = true;
  uint8_t absentValue = 0xFF;  // o que um MISO sem chip devolve
  bool misoReadable = true;
  Status selectStatus = Status::Ok;
  unsigned failTransfers = 0;
  uint8_t partnum = 0x00;
  uint8_t version = 0x14;
  uint8_t rssiRaw = 0x80;

  uint8_t state = radio::cc1101::kStateIdle;
  uint8_t registers[0x2F] = {};
  uint8_t patable[8] = {};
  uint8_t rxFifo[128] = {};
  size_t rxCount = 0;
  bool rxOverflow = false;
  uint8_t txFifo[128] = {};
  size_t txCount = 0;
  bool txUnderflow = false;

  // Quantas consultas de MARCSTATE ainda devolvem TX depois de um STX.
  unsigned txBusyPolls = 2;
  bool txNeverEnds = false;
  // Último pacote "transmitido" (sem o byte de tamanho) e contadores.
  uint8_t lastTx[64] = {};
  uint8_t lastTxLength = 0;
  unsigned transmitCount = 0;
  unsigned resetCount = 0;
  unsigned selectCount = 0;
  unsigned strobeCount[0x3E] = {};

 private:
  void resetChip() {
    memset(registers, 0, sizeof(registers));
    registers[radio::cc1101::kIocfg2] = 0x29;
    registers[radio::cc1101::kIocfg0] = 0x3F;
    registers[radio::cc1101::kPktlen] = 0xFF;
    registers[radio::cc1101::kMcsm1] = 0x30;
    registers[radio::cc1101::kFrend0] = 0x10;
    memset(patable, 0, sizeof(patable));
    patable[0] = 0xC6;
    rxCount = txCount = 0;
    rxOverflow = txUnderflow = false;
    state = radio::cc1101::kStateIdle;
  }

  void strobe(uint8_t command) {
    using namespace radio::cc1101;
    ++strobeCount[command];
    switch (command) {
      case kSres:
        ++resetCount;
        resetChip();
        break;
      case kSrx:
        if (!rxOverflow) state = kStateRx;
        break;
      case kStx:
        if (txCount == 0) {
          txUnderflow = true;
          state = kStateTxUnderflow;
        } else {
          lastTxLength = txFifo[0];
          memcpy(lastTx, txFifo + 1, lastTxLength);
          ++transmitCount;
          txPollsLeft_ = txBusyPolls;
          state = kStateTx;
        }
        break;
      case kSidle:
        if (state != kStateRxOverflow && state != kStateTxUnderflow) state = kStateIdle;
        break;
      case kSfrx:
        rxCount = 0;
        rxOverflow = false;
        if (state == kStateRxOverflow) state = kStateIdle;
        break;
      case kSftx:
        txCount = 0;
        txUnderflow = false;
        if (state == kStateTxUnderflow) state = kStateIdle;
        break;
      case kSpwd: sleepPending_ = true; break;
      default: break;
    }
  }

  uint8_t statusRegister(uint8_t address) {
    using namespace radio::cc1101;
    switch (address) {
      case kPartnum: return partnum;
      case kVersion: return version;
      case kRssi: return rssiRaw;
      case kMarcstate:
        if (state == kStateTx && !txNeverEnds) {
          if (txPollsLeft_ == 0) {
            state = kStateIdle;  // TXOFF_MODE = IDLE
            txCount = 0;
          } else {
            --txPollsLeft_;
          }
        }
        return state;
      case kTxbytes: return static_cast<uint8_t>(txCount | (txUnderflow ? 0x80 : 0));
      case kRxbytes: return static_cast<uint8_t>(rxCount | (rxOverflow ? 0x80 : 0));
      default: return 0;
    }
  }

  uint8_t chipStatus() const { return static_cast<uint8_t>((state & 0x07) << 4); }

  uint8_t step(uint8_t in) {
    using namespace radio::cc1101;
    if (headerPending_) {
      headerPending_ = false;
      read_ = (in & kRead) != 0;
      burst_ = (in & kBurst) != 0;
      address_ = in & 0x3F;
      index_ = 0;
      if (address_ >= kSres && address_ <= kSnop && !burst_) {
        strobe(address_);
        headerPending_ = true;  // cada byte de comando é um cabeçalho novo
      }
      return chipStatus();
    }

    uint8_t out = chipStatus();
    if (read_) {
      if (address_ <= kLastConfigRegister) {
        out = registers[address_];
      } else if (address_ == kPatable) {
        out = patable[index_++ % 8];
      } else if (address_ == kFifo) {
        if (rxCount > 0) {
          out = rxFifo[0];
          memmove(rxFifo, rxFifo + 1, --rxCount);
        } else {
          out = 0;
        }
      } else {
        out = statusRegister(address_);
      }
    } else {
      if (address_ <= kLastConfigRegister) {
        registers[address_] = in;
      } else if (address_ == kPatable) {
        patable[index_++ % 8] = in;
      } else if (address_ == kFifo && txCount < sizeof(txFifo)) {
        txFifo[txCount++] = in;
      }
    }
    if (burst_ && address_ < kLastConfigRegister) {
      ++address_;
    } else if (!burst_) {
      headerPending_ = true;
    }
    return out;
  }

  bool selected_ = false;
  bool headerPending_ = true;
  bool read_ = false;
  bool burst_ = false;
  uint8_t address_ = 0;
  size_t index_ = 0;
  bool sleepPending_ = false;
  unsigned txPollsLeft_ = 0;
};

}  // namespace dbmo::sim
