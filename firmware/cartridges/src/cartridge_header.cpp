#include "dbmo/cartridges/cartridge_header.h"

#include <cstring>

#include "dbmo/config/pins.h"
#include "dbmo/core/crc.h"

namespace dbmo::cartridges {

namespace {

// Posição de cada campo nos 64 bytes (little-endian).
constexpr size_t kOffMagic = 0;
constexpr size_t kOffFormat = 4;
constexpr size_t kOffSize = 5;
constexpr size_t kOffVendor = 6;
constexpr size_t kOffProduct = 8;
constexpr size_t kOffHardwareRevision = 10;
constexpr size_t kOffCartridgeVersion = 11;
constexpr size_t kOffMinFirmware = 14;
constexpr size_t kOffReserved17 = 17;
constexpr size_t kOffCapabilities = 18;
constexpr size_t kOffMaxCurrent = 20;
constexpr size_t kOffI2cCount = 22;
constexpr size_t kOffI2cAddresses = 23;
constexpr size_t kOffSpiMode = 27;
constexpr size_t kOffSpiMaxHz = 28;
constexpr size_t kOffUartBaud = 32;
constexpr size_t kOffName = 36;
constexpr size_t kOffReserved52 = 52;
constexpr size_t kOffCrc = 62;

void put16(uint8_t *out, uint16_t value) {
  out[0] = static_cast<uint8_t>(value & 0xFF);
  out[1] = static_cast<uint8_t>(value >> 8);
}
void put32(uint8_t *out, uint32_t value) {
  put16(out, static_cast<uint16_t>(value & 0xFFFF));
  put16(out + 2, static_cast<uint16_t>(value >> 16));
}
uint16_t get16(const uint8_t *in) { return static_cast<uint16_t>(in[0] | (in[1] << 8)); }
uint32_t get32(const uint8_t *in) {
  return static_cast<uint32_t>(get16(in)) | (static_cast<uint32_t>(get16(in + 2)) << 16);
}

bool validI2cAddress(uint8_t address) { return address >= 0x08 && address <= 0x77; }

// Nome: 1 a 16 caracteres ASCII imprimíveis; depois do fim, só zeros.
bool validNameField(const uint8_t *field) {
  size_t length = 0;
  while (length < kNameSize && field[length] != 0) {
    if (field[length] < 0x20 || field[length] > 0x7E) return false;
    ++length;
  }
  if (length == 0) return false;
  for (size_t i = length; i < kNameSize; ++i)
    if (field[i] != 0) return false;
  return true;
}

bool validFields(const CartridgeHeader &header) {
  if (header.i2cAddressCount > kMaxI2cAddresses) return false;
  for (uint8_t i = 0; i < header.i2cAddressCount; ++i)
    if (!validI2cAddress(header.i2cAddresses[i])) return false;
  if (header.spiMode > 3) return false;
  // Quem declara endereços usa I2C; quem declara parâmetros de SPI/UART usa o barramento.
  if (header.i2cAddressCount > 0 && (header.capabilities & capability::kI2c) == 0) return false;
  if ((header.capabilities & capability::kSpi) != 0 && header.spiMaxHz == 0) return false;
  if ((header.capabilities & capability::kUart) != 0 && header.uartBaud == 0) return false;
  return true;
}

}  // namespace

const uint8_t kDefaultBusyI2cAddresses[] = {
    config::i2c::kMcp23017Buttons,
    config::i2c::kMcp23017Aux,
    config::i2c::kPn532,
    0x50,
    0x51,
    0x52,
    0x53,
    0x54,
    0x55,
    0x56,
    0x57,
};
const size_t kDefaultBusyI2cCount = sizeof(kDefaultBusyI2cAddresses);

const char *toString(HeaderCheck check) {
  switch (check) {
    case HeaderCheck::Valid: return "Valid";
    case HeaderCheck::Blank: return "Blank";
    case HeaderCheck::BadMagic: return "BadMagic";
    case HeaderCheck::UnsupportedFormat: return "UnsupportedFormat";
    case HeaderCheck::BadCrc: return "BadCrc";
    case HeaderCheck::BadField: return "BadField";
  }
  return "Unknown";
}

const char *toString(Compatibility compatibility) {
  switch (compatibility) {
    case Compatibility::Compatible: return "Compatible";
    case Compatibility::FirmwareTooOld: return "FirmwareTooOld";
    case Compatibility::CapabilityUnavailable: return "CapabilityUnavailable";
    case Compatibility::I2cAddressConflict: return "I2cAddressConflict";
    case Compatibility::OverCurrentBudget: return "OverCurrentBudget";
  }
  return "Unknown";
}

Status encodeHeader(const CartridgeHeader &header, uint8_t out[kHeaderSize]) {
  if (out == nullptr || !validFields(header) || header.vendorId == 0)
    return Status::InvalidArgument;

  uint8_t name[kNameSize] = {};
  const size_t nameLength = strlen(header.name);
  if (nameLength == 0 || nameLength > kNameSize) return Status::InvalidArgument;
  memcpy(name, header.name, nameLength);
  if (!validNameField(name)) return Status::InvalidArgument;

  memset(out, 0, kHeaderSize);
  memcpy(out + kOffMagic, kHeaderMagic, sizeof(kHeaderMagic));
  out[kOffFormat] = kHeaderFormat;
  out[kOffSize] = static_cast<uint8_t>(kHeaderSize);
  put16(out + kOffVendor, header.vendorId);
  put16(out + kOffProduct, header.productId);
  out[kOffHardwareRevision] = header.hardwareRevision;
  out[kOffCartridgeVersion] = header.cartridgeVersion.major;
  out[kOffCartridgeVersion + 1] = header.cartridgeVersion.minor;
  out[kOffCartridgeVersion + 2] = header.cartridgeVersion.patch;
  out[kOffMinFirmware] = header.minHostFirmware.major;
  out[kOffMinFirmware + 1] = header.minHostFirmware.minor;
  out[kOffMinFirmware + 2] = header.minHostFirmware.patch;
  put16(out + kOffCapabilities, header.capabilities);
  put16(out + kOffMaxCurrent, header.maxCurrentMa);
  out[kOffI2cCount] = header.i2cAddressCount;
  memcpy(out + kOffI2cAddresses, header.i2cAddresses, header.i2cAddressCount);
  out[kOffSpiMode] = header.spiMode;
  put32(out + kOffSpiMaxHz, header.spiMaxHz);
  put32(out + kOffUartBaud, header.uartBaud);
  memcpy(out + kOffName, name, kNameSize);
  put16(out + kOffCrc, crc16Ccitt(out, kOffCrc));
  return Status::Ok;
}

HeaderCheck decodeHeader(const uint8_t *data, size_t length, CartridgeHeader *out) {
  if (data == nullptr || length < kHeaderSize) return HeaderCheck::BadMagic;

  bool blank = true;
  for (size_t i = 0; i < kHeaderSize && blank; ++i) blank = data[i] == 0xFF;
  if (blank) return HeaderCheck::Blank;

  if (memcmp(data + kOffMagic, kHeaderMagic, sizeof(kHeaderMagic)) != 0)
    return HeaderCheck::BadMagic;
  // O CRC vem antes da versão de formato: um formato futuro pode mudar os campos, mas a
  // posição da versão, do tamanho e do CRC de um cabeçalho de 64 bytes é fixa.
  if (data[kOffSize] != kHeaderSize) return HeaderCheck::UnsupportedFormat;
  if (get16(data + kOffCrc) != crc16Ccitt(data, kOffCrc)) return HeaderCheck::BadCrc;
  if (data[kOffFormat] != kHeaderFormat) return HeaderCheck::UnsupportedFormat;
  if (!validNameField(data + kOffName)) return HeaderCheck::BadField;
  // Fornecedor 0x0000 é inválido; bytes reservados e posições de endereço não usadas
  // precisam ser zero, para que um formato futuro possa dar significado a eles.
  if (get16(data + kOffVendor) == 0) return HeaderCheck::BadField;
  if (data[kOffReserved17] != 0) return HeaderCheck::BadField;
  for (size_t i = kOffReserved52; i < kOffCrc; ++i)
    if (data[i] != 0) return HeaderCheck::BadField;
  if (data[kOffI2cCount] <= kMaxI2cAddresses)
    for (size_t i = data[kOffI2cCount]; i < kMaxI2cAddresses; ++i)
      if (data[kOffI2cAddresses + i] != 0) return HeaderCheck::BadField;

  CartridgeHeader header;
  header.vendorId = get16(data + kOffVendor);
  header.productId = get16(data + kOffProduct);
  header.hardwareRevision = data[kOffHardwareRevision];
  header.cartridgeVersion = Version{data[kOffCartridgeVersion], data[kOffCartridgeVersion + 1],
                                    data[kOffCartridgeVersion + 2]};
  header.minHostFirmware =
      Version{data[kOffMinFirmware], data[kOffMinFirmware + 1], data[kOffMinFirmware + 2]};
  header.capabilities = get16(data + kOffCapabilities);
  header.maxCurrentMa = get16(data + kOffMaxCurrent);
  header.i2cAddressCount = data[kOffI2cCount];
  if (header.i2cAddressCount > kMaxI2cAddresses) return HeaderCheck::BadField;
  memcpy(header.i2cAddresses, data + kOffI2cAddresses, header.i2cAddressCount);
  header.spiMode = data[kOffSpiMode];
  header.spiMaxHz = get32(data + kOffSpiMaxHz);
  header.uartBaud = get32(data + kOffUartBaud);
  memcpy(header.name, data + kOffName, kNameSize);
  header.name[kNameSize] = '\0';
  if (!validFields(header)) return HeaderCheck::BadField;

  if (out != nullptr) *out = header;
  return HeaderCheck::Valid;
}

Compatibility checkCompatibility(const CartridgeHeader &header, const HostCapabilities &host) {
  if (host.firmware < header.minHostFirmware) return Compatibility::FirmwareTooOld;
  // Inclui bits que este firmware nem conhece: o aparelho não os oferece.
  if ((header.capabilities & ~host.capabilities) != 0) return Compatibility::CapabilityUnavailable;
  for (uint8_t i = 0; i < header.i2cAddressCount; ++i)
    for (size_t j = 0; j < host.busyI2cCount; ++j)
      if (header.i2cAddresses[i] == host.busyI2cAddresses[j])
        return Compatibility::I2cAddressConflict;
  if (header.maxCurrentMa > host.maxCurrentMa) return Compatibility::OverCurrentBudget;
  return Compatibility::Compatible;
}

}  // namespace dbmo::cartridges
