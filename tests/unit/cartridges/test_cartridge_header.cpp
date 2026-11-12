#include <cstring>

#include "dbmo/cartridges/cartridge_header.h"
#include "dbmo/core/crc.h"
#include "dbmo_test.h"
#include "example_sensor_cartridge.h"

using dbmo::Status;
using dbmo::Version;
using namespace dbmo::cartridges;

namespace {

CartridgeHeader sample() {
  CartridgeHeader header;
  header.vendorId = 0x1234;
  header.productId = 0xABCD;
  header.hardwareRevision = 2;
  header.cartridgeVersion = Version{1, 2, 3};
  header.minHostFirmware = Version{0, 1, 0};
  header.capabilities = capability::kI2c | capability::kSpi | capability::kGpio0;
  header.maxCurrentMa = 60;
  header.i2cAddressCount = 2;
  header.i2cAddresses[0] = 0x3C;
  header.i2cAddresses[1] = 0x3D;
  header.spiMode = 0;
  header.spiMaxHz = 8000000;
  strcpy(header.name, "LORA 915");
  return header;
}

HostCapabilities host() {
  return HostCapabilities{capability::kI2c | capability::kSpi | capability::kUart |
                              capability::kGpio0 | capability::kGpio1,
                          100, Version{0, 1, 0}, kDefaultBusyI2cAddresses, kDefaultBusyI2cCount};
}

void recomputeCrc(uint8_t *raw) {
  const uint16_t crc = dbmo::crc16Ccitt(raw, 62);
  raw[62] = static_cast<uint8_t>(crc & 0xFF);
  raw[63] = static_cast<uint8_t>(crc >> 8);
}

}  // namespace

void setUp() {}
void tearDown() {}

static void test_byte_layout_is_the_documented_one() {
  uint8_t raw[kHeaderSize];
  TEST_ASSERT_OK(encodeHeader(sample(), raw));
  // Conferência campo a campo contra a tabela da especificação (little-endian).
  TEST_ASSERT_EQUAL_UINT8_ARRAY("DBMC", raw, 4);
  TEST_ASSERT_EQUAL_UINT8(1, raw[4]);    // versão do formato
  TEST_ASSERT_EQUAL_UINT8(64, raw[5]);   // tamanho do cabeçalho
  TEST_ASSERT_EQUAL_HEX8(0x34, raw[6]);  // vendorId 0x1234
  TEST_ASSERT_EQUAL_HEX8(0x12, raw[7]);
  TEST_ASSERT_EQUAL_HEX8(0xCD, raw[8]);  // productId 0xABCD
  TEST_ASSERT_EQUAL_HEX8(0xAB, raw[9]);
  TEST_ASSERT_EQUAL_UINT8(2, raw[10]);  // revisão de hardware
  TEST_ASSERT_EQUAL_UINT8(1, raw[11]);  // versão 1.2.3
  TEST_ASSERT_EQUAL_UINT8(2, raw[12]);
  TEST_ASSERT_EQUAL_UINT8(3, raw[13]);
  TEST_ASSERT_EQUAL_UINT8(0, raw[14]);  // firmware mínimo 0.1.0
  TEST_ASSERT_EQUAL_UINT8(1, raw[15]);
  TEST_ASSERT_EQUAL_UINT8(0, raw[16]);
  TEST_ASSERT_EQUAL_HEX8(0x0B, raw[18]);  // capacidades: i2c | spi | gpio0 = 0x000B
  TEST_ASSERT_EQUAL_HEX8(0x00, raw[19]);
  TEST_ASSERT_EQUAL_UINT8(60, raw[20]);  // 60 mA
  TEST_ASSERT_EQUAL_UINT8(0, raw[21]);
  TEST_ASSERT_EQUAL_UINT8(2, raw[22]);  // dois endereços I2C
  TEST_ASSERT_EQUAL_HEX8(0x3C, raw[23]);
  TEST_ASSERT_EQUAL_HEX8(0x3D, raw[24]);
  TEST_ASSERT_EQUAL_UINT8(0, raw[27]);    // modo SPI
  TEST_ASSERT_EQUAL_HEX8(0x00, raw[28]);  // 8 000 000 = 0x007A1200
  TEST_ASSERT_EQUAL_HEX8(0x12, raw[29]);
  TEST_ASSERT_EQUAL_HEX8(0x7A, raw[30]);
  TEST_ASSERT_EQUAL_HEX8(0x00, raw[31]);
  TEST_ASSERT_EQUAL_UINT8_ARRAY("LORA 915", raw + 36, 8);
  TEST_ASSERT_EQUAL_UINT8(0, raw[44]);  // nome preenchido com zeros
  const uint16_t crc = dbmo::crc16Ccitt(raw, 62);
  TEST_ASSERT_EQUAL_HEX8(crc & 0xFF, raw[62]);
  TEST_ASSERT_EQUAL_HEX8(crc >> 8, raw[63]);
}

static void test_round_trip() {
  uint8_t raw[kHeaderSize];
  TEST_ASSERT_OK(encodeHeader(sample(), raw));
  CartridgeHeader decoded;
  TEST_ASSERT_TRUE(decodeHeader(raw, sizeof(raw), &decoded) == HeaderCheck::Valid);
  TEST_ASSERT_EQUAL_HEX16(0x1234, decoded.vendorId);
  TEST_ASSERT_EQUAL_HEX16(0xABCD, decoded.productId);
  TEST_ASSERT_TRUE(decoded.cartridgeVersion == (Version{1, 2, 3}));
  TEST_ASSERT_EQUAL_HEX16(capability::kI2c | capability::kSpi | capability::kGpio0,
                          decoded.capabilities);
  TEST_ASSERT_EQUAL_UINT16(60, decoded.maxCurrentMa);
  TEST_ASSERT_EQUAL_UINT8(2, decoded.i2cAddressCount);
  TEST_ASSERT_EQUAL_HEX8(0x3D, decoded.i2cAddresses[1]);
  TEST_ASSERT_EQUAL_UINT32(8000000, decoded.spiMaxHz);
  TEST_ASSERT_EQUAL_STRING("LORA 915", decoded.name);
}

static void test_any_single_corrupted_byte_is_detected() {
  uint8_t raw[kHeaderSize];
  TEST_ASSERT_OK(encodeHeader(sample(), raw));
  for (size_t i = 0; i < kHeaderSize; ++i) {
    uint8_t damaged[kHeaderSize];
    memcpy(damaged, raw, sizeof(raw));
    damaged[i] ^= 0x01;
    TEST_ASSERT_FALSE_MESSAGE(decodeHeader(damaged, sizeof(damaged), nullptr) == HeaderCheck::Valid,
                              "byte corrompido aceito");
  }
}

static void test_blank_garbage_and_truncated_data_are_distinct_outcomes() {
  uint8_t raw[kHeaderSize];
  memset(raw, 0xFF, sizeof(raw));
  TEST_ASSERT_TRUE(decodeHeader(raw, sizeof(raw), nullptr) == HeaderCheck::Blank);
  memset(raw, 0x00, sizeof(raw));
  TEST_ASSERT_TRUE(decodeHeader(raw, sizeof(raw), nullptr) == HeaderCheck::BadMagic);
  TEST_ASSERT_OK(encodeHeader(sample(), raw));
  TEST_ASSERT_TRUE(decodeHeader(raw, 63, nullptr) == HeaderCheck::BadMagic);  // truncado
  TEST_ASSERT_TRUE(decodeHeader(nullptr, 64, nullptr) == HeaderCheck::BadMagic);
  raw[63] ^= 0xFF;
  TEST_ASSERT_TRUE(decodeHeader(raw, sizeof(raw), nullptr) == HeaderCheck::BadCrc);
}

static void test_future_format_is_reported_as_unsupported() {
  uint8_t raw[kHeaderSize];
  TEST_ASSERT_OK(encodeHeader(sample(), raw));
  raw[4] = 2;  // formato 2, com CRC correto
  recomputeCrc(raw);
  TEST_ASSERT_TRUE(decodeHeader(raw, sizeof(raw), nullptr) == HeaderCheck::UnsupportedFormat);
  TEST_ASSERT_OK(encodeHeader(sample(), raw));
  raw[5] = 128;  // outro tamanho de cabeçalho
  TEST_ASSERT_TRUE(decodeHeader(raw, sizeof(raw), nullptr) == HeaderCheck::UnsupportedFormat);
}

static void test_fields_with_valid_crc_but_invalid_values_are_rejected() {
  uint8_t raw[kHeaderSize];
  auto withByte = [&raw](size_t offset, uint8_t value) {
    TEST_ASSERT_OK(encodeHeader(sample(), raw));
    raw[offset] = value;
    recomputeCrc(raw);
    return decodeHeader(raw, sizeof(raw), nullptr);
  };
  TEST_ASSERT_TRUE(withByte(22, 5) == HeaderCheck::BadField);     // mais endereços que o máximo
  TEST_ASSERT_TRUE(withByte(23, 0x03) == HeaderCheck::BadField);  // endereço I2C reservado
  TEST_ASSERT_TRUE(withByte(23, 0x78) == HeaderCheck::BadField);
  TEST_ASSERT_TRUE(withByte(27, 4) == HeaderCheck::BadField);     // modo SPI inexistente
  TEST_ASSERT_TRUE(withByte(36, 0x07) == HeaderCheck::BadField);  // caractere de controle no nome
  TEST_ASSERT_TRUE(withByte(36, 0x00) == HeaderCheck::BadField);  // nome vazio
  TEST_ASSERT_TRUE(withByte(50, 'X') == HeaderCheck::BadField);   // lixo depois do fim do nome
  TEST_ASSERT_TRUE(withByte(18, 0x02) == HeaderCheck::BadField);  // endereços I2C sem declarar I2C
}

static void test_encode_validates_its_input() {
  uint8_t raw[kHeaderSize];
  CartridgeHeader header = sample();
  header.name[0] = '\0';
  TEST_ASSERT_STATUS(Status::InvalidArgument, encodeHeader(header, raw));
  header = sample();
  header.spiMaxHz = 0;  // declara SPI sem dizer a velocidade
  TEST_ASSERT_STATUS(Status::InvalidArgument, encodeHeader(header, raw));
  header = sample();
  header.capabilities |= capability::kUart;  // declara UART sem dizer a taxa
  TEST_ASSERT_STATUS(Status::InvalidArgument, encodeHeader(header, raw));
  header = sample();
  header.i2cAddresses[0] = 0x00;
  TEST_ASSERT_STATUS(Status::InvalidArgument, encodeHeader(header, raw));
  TEST_ASSERT_STATUS(Status::InvalidArgument, encodeHeader(sample(), nullptr));

  header = sample();
  strcpy(header.name, "DEZESSEIS LETRAS");  // exatamente 16 caracteres
  TEST_ASSERT_OK(encodeHeader(header, raw));
  CartridgeHeader decoded;
  TEST_ASSERT_TRUE(decodeHeader(raw, sizeof(raw), &decoded) == HeaderCheck::Valid);
  TEST_ASSERT_EQUAL_STRING("DEZESSEIS LETRAS", decoded.name);
}

static void test_compatibility_matrix() {
  TEST_ASSERT_TRUE(checkCompatibility(sample(), host()) == Compatibility::Compatible);

  CartridgeHeader header = sample();
  header.minHostFirmware = Version{0, 2, 0};
  TEST_ASSERT_TRUE(checkCompatibility(header, host()) == Compatibility::FirmwareTooOld);

  header = sample();
  header.capabilities |= capability::kNeeds5V;  // a linha de 5 V não é oferecida
  TEST_ASSERT_TRUE(checkCompatibility(header, host()) == Compatibility::CapabilityUnavailable);

  header = sample();
  header.capabilities |= 0x0100;  // capacidade que este firmware nem conhece
  TEST_ASSERT_TRUE(checkCompatibility(header, host()) == Compatibility::CapabilityUnavailable);

  header = sample();
  header.maxCurrentMa = 101;
  TEST_ASSERT_TRUE(checkCompatibility(header, host()) == Compatibility::OverCurrentBudget);
  header.maxCurrentMa = 100;
  TEST_ASSERT_TRUE(checkCompatibility(header, host()) == Compatibility::Compatible);
}

static void test_i2c_addresses_of_the_device_are_protected() {
  // MCP23017 (0x20, 0x21), PN532 (0x24) e toda a faixa da EEPROM de identificação.
  for (uint8_t busy : {uint8_t{0x20}, uint8_t{0x21}, uint8_t{0x24}, uint8_t{0x50}, uint8_t{0x57}}) {
    CartridgeHeader header = sample();
    header.i2cAddresses[1] = busy;
    TEST_ASSERT_TRUE_MESSAGE(
        checkCompatibility(header, host()) == Compatibility::I2cAddressConflict,
        "endereco ocupado aceito");
  }
  CartridgeHeader header = sample();
  header.i2cAddresses[1] = 0x22;  // livre
  TEST_ASSERT_TRUE(checkCompatibility(header, host()) == Compatibility::Compatible);
}

static void test_example_cartridge_header_is_valid_and_compatible() {
  uint8_t raw[kHeaderSize];
  const CartridgeHeader header = examples::ExampleSensorCartridge::makeHeader();
  TEST_ASSERT_OK(encodeHeader(header, raw));
  TEST_ASSERT_TRUE(decodeHeader(raw, sizeof(raw), nullptr) == HeaderCheck::Valid);
  TEST_ASSERT_TRUE(checkCompatibility(header, host()) == Compatibility::Compatible);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_byte_layout_is_the_documented_one);
  RUN_TEST(test_round_trip);
  RUN_TEST(test_any_single_corrupted_byte_is_detected);
  RUN_TEST(test_blank_garbage_and_truncated_data_are_distinct_outcomes);
  RUN_TEST(test_future_format_is_reported_as_unsupported);
  RUN_TEST(test_fields_with_valid_crc_but_invalid_values_are_rejected);
  RUN_TEST(test_encode_validates_its_input);
  RUN_TEST(test_compatibility_matrix);
  RUN_TEST(test_i2c_addresses_of_the_device_are_protected);
  RUN_TEST(test_example_cartridge_header_is_valid_and_compatible);
  return UNITY_END();
}
