// Um dispositivo em um barramento SPI compartilhado (clock, modo e CS próprios).
#pragma once

#include <cstddef>
#include <cstdint>

#include "dbmo/core/status.h"

namespace dbmo {

class ISpiDevice {
 public:
  virtual ~ISpiDevice() = default;
  // Abre a transação: trava o barramento, aplica clock/modo deste dispositivo e baixa o CS.
  virtual Status select() = 0;
  // Troca n bytes. tx == nullptr envia 0x00; rx == nullptr descarta o que chegar.
  virtual Status transfer(const uint8_t *tx, uint8_t *rx, size_t n) = 0;
  // Sobe o CS e libera o barramento. Seguro chamar mesmo se select() falhou.
  virtual void deselect() = 0;
  // Nível do MISO com o CS baixo (o CC1101 sinaliza "pronto" por ele).
  // Unsupported quando o porte não consegue ler o pino.
  virtual Result<bool> readMiso() { return Status::Unsupported; }
};

// Abre a transação no construtor e fecha no destrutor.
class SpiTransaction {
 public:
  explicit SpiTransaction(ISpiDevice &device) : device_(device), status_(device.select()) {}
  ~SpiTransaction() { device_.deselect(); }
  SpiTransaction(const SpiTransaction &) = delete;
  SpiTransaction &operator=(const SpiTransaction &) = delete;

  Status status() const { return status_; }

 private:
  ISpiDevice &device_;
  Status status_;
};

}  // namespace dbmo
