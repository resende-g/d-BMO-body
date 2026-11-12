#include "dbmo/radio/radio_config.h"
#include "dbmo_test.h"

using dbmo::Status;
using namespace dbmo::radio;

namespace {
constexpr uint32_t kXtal = 26000000;
}

void setUp() {}
void tearDown() {}

void test_chip_bands() {
  TEST_ASSERT_TRUE(inChipBand(300000000));
  TEST_ASSERT_TRUE(inChipBand(315000000));
  TEST_ASSERT_TRUE(inChipBand(348000000));
  TEST_ASSERT_FALSE(inChipBand(348000001));
  TEST_ASSERT_FALSE(inChipBand(386999999));
  TEST_ASSERT_TRUE(inChipBand(433920000));
  TEST_ASSERT_TRUE(inChipBand(464000000));
  TEST_ASSERT_FALSE(inChipBand(500000000));
  TEST_ASSERT_TRUE(inChipBand(868000000));
  TEST_ASSERT_TRUE(inChipBand(915000000));
  TEST_ASSERT_FALSE(inChipBand(928000001));
  TEST_ASSERT_FALSE(inChipBand(2400000000u));
  TEST_ASSERT_FALSE(inChipBand(0));
}

void test_frequency_word() {
  // 433,92 MHz com cristal de 26 MHz: 433,92e6 × 65536 / 26e6 = 1093745,3 → 0x10B071.
  TEST_ASSERT_EQUAL_HEX32(0x10B071, frequencyWord(kXtal, 433920000));
  // 868 MHz: 868e6 × 65536 / 26e6 = 2187894,2 → 0x216276.
  TEST_ASSERT_EQUAL_HEX32(0x216276, frequencyWord(kXtal, 868000000));
  // Ida e volta com erro menor que um passo (26e6 / 65536 ≈ 397 Hz).
  const uint32_t back = frequencyFromWord(kXtal, frequencyWord(kXtal, 433920000));
  TEST_ASSERT_UINT32_WITHIN(400, 433920000, back);
  TEST_ASSERT_EQUAL_UINT32(0, frequencyWord(0, 433920000));
}

void test_data_rate_fields() {
  // 1200 baud: (256 + 131) × 2^5 / 2^28 × 26e6 = 1199,5.
  dbmo::Result<FieldPair> fields = computeDataRate(kXtal, 1200);
  TEST_ASSERT_TRUE(fields.ok());
  TEST_ASSERT_EQUAL_UINT8(5, fields.value().exponent);
  TEST_ASSERT_EQUAL_UINT8(131, fields.value().mantissa);
  TEST_ASSERT_UINT32_WITHIN(2, 1200, dataRateFromFields(kXtal, fields.value()));

  // 38400 baud: (256 + 131) × 2^10 / 2^28 × 26e6 = 38383,5.
  fields = computeDataRate(kXtal, 38400);
  TEST_ASSERT_EQUAL_UINT8(10, fields.value().exponent);
  TEST_ASSERT_EQUAL_UINT8(131, fields.value().mantissa);

  // Em toda a faixa, o erro fica abaixo de 0,2 %.
  for (uint32_t baud = 600; baud <= 500000; baud = baud * 5 / 4 + 1) {
    fields = computeDataRate(kXtal, baud);
    TEST_ASSERT_TRUE(fields.ok());
    TEST_ASSERT_TRUE(fields.value().exponent < 16);
    TEST_ASSERT_UINT32_WITHIN(baud / 500 + 1, baud, dataRateFromFields(kXtal, fields.value()));
  }

  TEST_ASSERT_STATUS(Status::OutOfRange, computeDataRate(kXtal, 599).status());
  TEST_ASSERT_STATUS(Status::OutOfRange, computeDataRate(kXtal, 500001).status());
  TEST_ASSERT_STATUS(Status::InvalidArgument, computeDataRate(0, 1200).status());
}

void test_bandwidth_fields() {
  // Menor banda: 26e6 / (8 × 7 × 8) = 58035 Hz.
  dbmo::Result<FieldPair> fields = computeBandwidth(kXtal, 58000);
  TEST_ASSERT_EQUAL_UINT8(3, fields.value().exponent);
  TEST_ASSERT_EQUAL_UINT8(3, fields.value().mantissa);
  TEST_ASSERT_EQUAL_UINT32(58035, bandwidthFromFields(kXtal, fields.value()));

  // Pedido entre dois degraus: sobe para o próximo (nunca estreita o que foi pedido).
  fields = computeBandwidth(kXtal, 60000);
  TEST_ASSERT_TRUE(bandwidthFromFields(kXtal, fields.value()) >= 60000);
  TEST_ASSERT_EQUAL_UINT32(67708, bandwidthFromFields(kXtal, fields.value()));

  // Maior banda: 26e6 / (8 × 4 × 1) = 812500 Hz.
  fields = computeBandwidth(kXtal, 812500);
  TEST_ASSERT_EQUAL_UINT8(0, fields.value().exponent);
  TEST_ASSERT_EQUAL_UINT8(0, fields.value().mantissa);
  TEST_ASSERT_STATUS(Status::OutOfRange, computeBandwidth(kXtal, 900000).status());
  TEST_ASSERT_STATUS(Status::InvalidArgument, computeBandwidth(kXtal, 0).status());
}

void test_deviation_fields() {
  // 5157 Hz: 26e6 / 2^17 × (8 + 5) × 2^1 = 5157,5.
  FieldPair fields = computeDeviation(kXtal, 5157);
  TEST_ASSERT_EQUAL_UINT8(1, fields.exponent);
  TEST_ASSERT_EQUAL_UINT8(5, fields.mantissa);
  // 47607 Hz: (8 + 7) × 2^4 → 47607,4.
  fields = computeDeviation(kXtal, 47607);
  TEST_ASSERT_EQUAL_UINT8(4, fields.exponent);
  TEST_ASSERT_EQUAL_UINT8(7, fields.mantissa);
  TEST_ASSERT_UINT32_WITHIN(2, 47607, deviationFromFields(kXtal, fields));
}

void test_validate() {
  RadioConfig config;
  TEST_ASSERT_OK(validate(config, kXtal));

  RadioConfig bad = config;
  bad.frequencyHz = 500000000;  // entre as faixas do chip
  TEST_ASSERT_STATUS(Status::OutOfRange, validate(bad, kXtal));
  bad = config;
  bad.dataRateBaud = 100;
  TEST_ASSERT_STATUS(Status::OutOfRange, validate(bad, kXtal));
  bad = config;
  bad.bandwidthHz = 2000000;
  TEST_ASSERT_STATUS(Status::OutOfRange, validate(bad, kXtal));
  bad = config;
  bad.deviationHz = 500000;
  TEST_ASSERT_STATUS(Status::OutOfRange, validate(bad, kXtal));
  bad.modulation = Modulation::Ook;  // OOK ignora o desvio
  TEST_ASSERT_OK(validate(bad, kXtal));
  bad = config;
  bad.maxPayload = 62;
  TEST_ASSERT_STATUS(Status::InvalidArgument, validate(bad, kXtal));
  bad = config;
  bad.modulation = static_cast<Modulation>(7);
  TEST_ASSERT_STATUS(Status::InvalidArgument, validate(bad, kXtal));
  TEST_ASSERT_STATUS(Status::InvalidArgument, validate(config, 16000000));
}

void test_airtime() {
  RadioConfig config;  // 1200 baud, CRC ligado
  // (4 + 2 + 1 + 10 + 2) × 8 = 152 bits → 126,7 ms → 127.
  TEST_ASSERT_EQUAL_UINT32(127, airtimeMs(config, 10));
  config.crc = false;
  // 136 bits → 113,3 ms → 114.
  TEST_ASSERT_EQUAL_UINT32(114, airtimeMs(config, 10));
  config.dataRateBaud = 38400;
  TEST_ASSERT_EQUAL_UINT32(4, airtimeMs(config, 10));  // 3,54 ms, arredondado para cima
  config.dataRateBaud = 0;
  TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, airtimeMs(config, 10));
}

void test_occupied_bandwidth() {
  RadioConfig config;  // FSK, desvio 5157, 1200 baud
  TEST_ASSERT_EQUAL_UINT32(2 * 5157 + 1200, occupiedBandwidthHz(config));
  config.modulation = Modulation::Ook;
  TEST_ASSERT_EQUAL_UINT32(2400, occupiedBandwidthHz(config));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_chip_bands);
  RUN_TEST(test_frequency_word);
  RUN_TEST(test_data_rate_fields);
  RUN_TEST(test_bandwidth_fields);
  RUN_TEST(test_deviation_fields);
  RUN_TEST(test_validate);
  RUN_TEST(test_airtime);
  RUN_TEST(test_occupied_bandwidth);
  return UNITY_END();
}
