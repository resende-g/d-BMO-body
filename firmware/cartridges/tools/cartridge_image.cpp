// Gera e confere a imagem de 256 bytes da EEPROM de identificação de um cartucho.
//
//   dbmo_cartridge_image make saida.bin vendor=0x1234 product=1 name="LORA 915"
//       caps=spi,gpio0 ma=60 spi_mode=0 spi_hz=8000000 [hwrev=1] [version=1.0.0]
//       [min_fw=0.1.0] [i2c=0x3C,0x3D] [baud=115200]     (tudo em uma linha só)
//   dbmo_cartridge_image check arquivo.bin
//
// Roda no computador. A gravação na EEPROM física é feita com um gravador comum de 24C02.
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "dbmo/cartridges/cartridge_header.h"

namespace {

using namespace dbmo::cartridges;

bool parseCapabilities(const char *text, uint16_t *out) {
  char buffer[96];
  snprintf(buffer, sizeof(buffer), "%s", text);
  uint16_t mask = 0;
  for (char *token = strtok(buffer, ","); token != nullptr; token = strtok(nullptr, ",")) {
    if (strcmp(token, "i2c") == 0)
      mask |= capability::kI2c;
    else if (strcmp(token, "spi") == 0)
      mask |= capability::kSpi;
    else if (strcmp(token, "uart") == 0)
      mask |= capability::kUart;
    else if (strcmp(token, "gpio0") == 0)
      mask |= capability::kGpio0;
    else if (strcmp(token, "gpio1") == 0)
      mask |= capability::kGpio1;
    else if (strcmp(token, "5v") == 0)
      mask |= capability::kNeeds5V;
    else
      return false;
  }
  *out = mask;
  return true;
}

bool applyOption(CartridgeHeader &header, const char *key, const char *value) {
  if (strcmp(key, "vendor") == 0)
    header.vendorId = static_cast<uint16_t>(strtoul(value, nullptr, 0));
  else if (strcmp(key, "product") == 0)
    header.productId = static_cast<uint16_t>(strtoul(value, nullptr, 0));
  else if (strcmp(key, "hwrev") == 0)
    header.hardwareRevision = static_cast<uint8_t>(strtoul(value, nullptr, 0));
  else if (strcmp(key, "ma") == 0)
    header.maxCurrentMa = static_cast<uint16_t>(strtoul(value, nullptr, 0));
  else if (strcmp(key, "spi_mode") == 0)
    header.spiMode = static_cast<uint8_t>(strtoul(value, nullptr, 0));
  else if (strcmp(key, "spi_hz") == 0)
    header.spiMaxHz = static_cast<uint32_t>(strtoul(value, nullptr, 0));
  else if (strcmp(key, "baud") == 0)
    header.uartBaud = static_cast<uint32_t>(strtoul(value, nullptr, 0));
  else if (strcmp(key, "name") == 0)
    snprintf(header.name, sizeof(header.name), "%s", value);
  else if (strcmp(key, "caps") == 0)
    return parseCapabilities(value, &header.capabilities);
  else if (strcmp(key, "version") == 0 || strcmp(key, "min_fw") == 0) {
    const dbmo::Result<dbmo::Version> version = dbmo::parseVersion(value);
    if (!version.ok()) return false;
    (strcmp(key, "version") == 0 ? header.cartridgeVersion : header.minHostFirmware) =
        version.value();
  } else if (strcmp(key, "i2c") == 0) {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%s", value);
    header.i2cAddressCount = 0;
    for (char *token = strtok(buffer, ","); token != nullptr; token = strtok(nullptr, ",")) {
      if (header.i2cAddressCount == kMaxI2cAddresses) return false;
      header.i2cAddresses[header.i2cAddressCount++] =
          static_cast<uint8_t>(strtoul(token, nullptr, 0));
    }
  } else {
    return false;
  }
  return true;
}

void print(const CartridgeHeader &header) {
  printf("fornecedor/produto : %04X:%04X (revisao de hardware %u)\n", header.vendorId,
         header.productId, header.hardwareRevision);
  printf("nome               : %s\n", header.name);
  printf("versao do cartucho : %u.%u.%u\n", header.cartridgeVersion.major,
         header.cartridgeVersion.minor, header.cartridgeVersion.patch);
  printf("firmware minimo    : %u.%u.%u\n", header.minHostFirmware.major,
         header.minHostFirmware.minor, header.minHostFirmware.patch);
  printf("capacidades        : 0x%04X%s%s%s%s%s%s\n", header.capabilities,
         (header.capabilities & capability::kI2c) ? " i2c" : "",
         (header.capabilities & capability::kSpi) ? " spi" : "",
         (header.capabilities & capability::kUart) ? " uart" : "",
         (header.capabilities & capability::kGpio0) ? " gpio0" : "",
         (header.capabilities & capability::kGpio1) ? " gpio1" : "",
         (header.capabilities & capability::kNeeds5V) ? " 5v" : "");
  printf("corrente maxima    : %u mA\n", header.maxCurrentMa);
  printf("enderecos I2C      :");
  for (uint8_t i = 0; i < header.i2cAddressCount; ++i) printf(" 0x%02X", header.i2cAddresses[i]);
  printf("%s\n", header.i2cAddressCount == 0 ? " nenhum" : "");
  printf("SPI                : modo %u, ate %lu Hz\n", header.spiMode,
         static_cast<unsigned long>(header.spiMaxHz));
  printf("UART               : %lu baud\n", static_cast<unsigned long>(header.uartBaud));
}

int check(const char *path) {
  uint8_t image[256];
  FILE *file = fopen(path, "rb");
  if (file == nullptr) {
    fprintf(stderr, "nao foi possivel abrir %s\n", path);
    return 1;
  }
  const size_t read = fread(image, 1, sizeof(image), file);
  fclose(file);
  CartridgeHeader header;
  const HeaderCheck result = decodeHeader(image, read, &header);
  printf("%s: %zu bytes, cabecalho %s\n", path, read, toString(result));
  if (result != HeaderCheck::Valid) return 1;
  print(header);
  const HostCapabilities host{capability::kI2c | capability::kSpi | capability::kUart |
                                  capability::kGpio0 | capability::kGpio1,
                              100, dbmo::kFirmwareVersion, kDefaultBusyI2cAddresses,
                              kDefaultBusyI2cCount};
  printf("compatibilidade com o d-BMO v1 (firmware %u.%u.%u, 100 mA): %s\n",
         dbmo::kFirmwareVersion.major, dbmo::kFirmwareVersion.minor, dbmo::kFirmwareVersion.patch,
         toString(checkCompatibility(header, host)));
  return 0;
}

int make(int argc, char **argv) {
  CartridgeHeader header;
  header.hardwareRevision = 1;
  header.cartridgeVersion = dbmo::Version{1, 0, 0};
  header.minHostFirmware = dbmo::kFirmwareVersion;
  for (int i = 3; i < argc; ++i) {
    char option[128];
    snprintf(option, sizeof(option), "%s", argv[i]);
    char *equals = strchr(option, '=');
    if (equals == nullptr) {
      fprintf(stderr, "opcao sem '=': %s\n", argv[i]);
      return 2;
    }
    *equals = '\0';
    if (!applyOption(header, option, equals + 1)) {
      fprintf(stderr, "opcao invalida: %s\n", argv[i]);
      return 2;
    }
  }
  uint8_t image[256];
  memset(image, 0xFF, sizeof(image));  // o resto da EEPROM fica como sai de fábrica
  if (!dbmo::isOk(encodeHeader(header, image))) {
    fprintf(stderr, "cabecalho invalido (nome, enderecos I2C, modo SPI ou parametros ausentes)\n");
    return 1;
  }
  FILE *file = fopen(argv[2], "wb");
  if (file == nullptr || fwrite(image, 1, sizeof(image), file) != sizeof(image)) {
    fprintf(stderr, "nao foi possivel gravar %s\n", argv[2]);
    if (file != nullptr) fclose(file);
    return 1;
  }
  fclose(file);
  return check(argv[2]);
}

}  // namespace

int main(int argc, char **argv) {
  if (argc == 3 && strcmp(argv[1], "check") == 0) return check(argv[2]);
  if (argc >= 3 && strcmp(argv[1], "make") == 0) return make(argc, argv);
  fprintf(stderr,
          "uso: %s make saida.bin chave=valor...   (vendor, product, name, caps, ma, hwrev,\n"
          "         version, min_fw, i2c, spi_mode, spi_hz, baud)\n"
          "     %s check arquivo.bin\n",
          argv[0], argv[0]);
  return 2;
}
