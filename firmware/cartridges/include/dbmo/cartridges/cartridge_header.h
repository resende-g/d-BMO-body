// Cabeçalho de identificação do cartucho, gravado no início da EEPROM do cartucho.
// O formato byte a byte está em docs/cartridges/CARTRIDGE_SPECIFICATION.md.
//
// O conteúdo da EEPROM é entrada não confiável: tudo é validado antes de ser usado, e o
// cabeçalho só declara o que o cartucho precisa. Nenhum código é carregado do cartucho.
#pragma once

#include <cstddef>
#include <cstdint>

#include "dbmo/core/status.h"
#include "dbmo/core/version.h"

namespace dbmo::cartridges {

inline constexpr size_t kHeaderSize = 64;
inline constexpr uint8_t kHeaderFormat = 1;
inline constexpr uint8_t kHeaderMagic[4] = {'D', 'B', 'M', 'C'};
inline constexpr size_t kNameSize = 16;
inline constexpr size_t kMaxI2cAddresses = 4;

// O que o cartucho usa do conector.
namespace capability {
inline constexpr uint16_t kI2c = 1u << 0;
inline constexpr uint16_t kSpi = 1u << 1;
inline constexpr uint16_t kUart = 1u << 2;
inline constexpr uint16_t kGpio0 = 1u << 3;
inline constexpr uint16_t kGpio1 = 1u << 4;
inline constexpr uint16_t kNeeds5V = 1u << 5;
inline constexpr uint16_t kAllKnown = 0x003F;
}  // namespace capability

struct CartridgeHeader {
  uint16_t vendorId = 0;
  uint16_t productId = 0;
  uint8_t hardwareRevision = 0;
  Version cartridgeVersion;   // versão dos dados/do projeto do cartucho
  Version minHostFirmware;    // firmware mínimo do d-BMO que sabe usar este cartucho
  uint16_t capabilities = 0;  // bits de capability::
  uint16_t maxCurrentMa = 0;  // consumo máximo declarado na linha de 3,3 V
  uint8_t i2cAddressCount = 0;
  uint8_t i2cAddresses[kMaxI2cAddresses] = {};  // endereços de 7 bits que o cartucho ocupa
  uint8_t spiMode = 0;
  uint32_t spiMaxHz = 0;
  uint32_t uartBaud = 0;
  char name[kNameSize + 1] = {};  // ASCII imprimível
};

enum class HeaderCheck : uint8_t {
  Valid,
  Blank,              // EEPROM nunca gravada (tudo 0xFF): cartucho sem identificação
  BadMagic,           // não é um cabeçalho do d-BMO
  UnsupportedFormat,  // versão de formato que este firmware não conhece
  BadCrc,
  BadField,  // CRC confere, mas algum campo está fora da faixa
};

const char *toString(HeaderCheck check);

// Monta os 64 bytes do cabeçalho. InvalidArgument se algum campo é inválido.
Status encodeHeader(const CartridgeHeader &header, uint8_t out[kHeaderSize]);

// Valida e decodifica. out só é preenchido quando o resultado é Valid.
HeaderCheck decodeHeader(const uint8_t *data, size_t length, CartridgeHeader *out);

// O que o aparelho oferece a um cartucho.
struct HostCapabilities {
  uint16_t capabilities;
  uint16_t maxCurrentMa;
  Version firmware;
  // Endereços I2C de 7 bits já ocupados no barramento do aparelho.
  const uint8_t *busyI2cAddresses;
  size_t busyI2cCount;
};

enum class Compatibility : uint8_t {
  Compatible,
  FirmwareTooOld,         // o cartucho pede um firmware mais novo
  CapabilityUnavailable,  // pede algo que o aparelho não oferece (ex.: linha de 5 V)
  I2cAddressConflict,     // endereço I2C já usado no aparelho
  OverCurrentBudget,      // consumo declarado acima do que o conector pode fornecer
};

const char *toString(Compatibility compatibility);

Compatibility checkCompatibility(const CartridgeHeader &header, const HostCapabilities &host);

// Endereços ocupados no barramento I2C do d-BMO v1: os dois MCP23017, o PN532 e toda a
// faixa 0x50–0x57, porque uma 24LC02B responde em qualquer endereço dela.
extern const uint8_t kDefaultBusyI2cAddresses[];
extern const size_t kDefaultBusyI2cCount;

}  // namespace dbmo::cartridges
