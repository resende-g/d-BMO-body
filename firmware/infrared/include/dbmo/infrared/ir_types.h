// Tipos do módulo de infravermelho e os contratos de hardware.
#pragma once

#include <cstddef>
#include <cstdint>

#include "dbmo/core/status.h"

namespace dbmo::infrared {

// Sinal bruto: durações em microssegundos, alternando marca (portadora ligada) e espaço,
// começando por uma marca.
struct RawTiming {
  static constexpr size_t kMaxEntries = 128;
  uint16_t durationsUs[kMaxEntries] = {};
  uint16_t count = 0;
  uint32_t carrierHz = 38000;

  uint32_t totalUs() const {
    uint32_t total = 0;
    for (uint16_t i = 0; i < count; ++i) total += durationsUs[i];
    return total;
  }
};

enum class IrProtocol : uint8_t { Unknown = 0, Nec = 1, Sony = 2 };

const char *toString(IrProtocol protocol);

struct IrCode {
  IrProtocol protocol = IrProtocol::Unknown;
  uint8_t bits = 0;      // NEC: 32; Sony: 12, 15 ou 20
  uint16_t address = 0;  // NEC: 8 bits ou 16 (estendido); Sony: 5, 8 ou 13 bits
  uint16_t command = 0;  // NEC: 8 bits; Sony: 7 bits
  // NEC com endereço de 16 bits (o segundo byte não é o inverso do primeiro). Sem esta
  // marca, um endereço estendido com byte alto zero se confundiria com o de 8 bits.
  bool extended = false;
};

constexpr bool operator==(const IrCode &a, const IrCode &b) {
  return a.protocol == b.protocol && a.bits == b.bits && a.address == b.address &&
         a.command == b.command && a.extended == b.extended;
}

// Um código guardado: decodificado (code) ou bruto (raw).
struct StoredCode {
  bool isRaw = false;
  IrCode code;
  RawTiming raw;
};

// Receptor demodulado (ex.: TSOP). O porte mede as durações.
class IIrReceiver {
 public:
  virtual ~IIrReceiver() = default;
  virtual Status begin() = 0;
  // true e *out preenchido quando há uma rajada completa. Não bloqueia.
  virtual Result<bool> read(RawTiming *out) = 0;
};

// Emissor com portadora. send() só inicia; busy() informa o andamento.
class IIrTransmitter {
 public:
  virtual ~IIrTransmitter() = default;
  virtual Status begin() = 0;
  virtual Status send(const RawTiming &timing) = 0;
  virtual bool busy() = 0;
};

}  // namespace dbmo::infrared
