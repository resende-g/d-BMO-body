#include "dbmo/radio/tx_policy.h"
#include "dbmo_test.h"

using namespace dbmo::radio;

namespace {

constexpr uint32_t kFrequency = 433920000;
constexpr uint32_t kOccupied = 12000;

TxLimits openLimits() {
  TxLimits limits;
  limits.enabled = true;
  limits.minHz = 433050000;
  limits.maxHz = 434790000;
  limits.maxAirtimeMsPerMinute = 600;
  return limits;
}

}  // namespace

void setUp() {}
void tearDown() {}

void test_factory_default_denies_everything() {
  TxPolicy policy;
  TEST_ASSERT_EQUAL(TxDenial::Disabled, policy.check(kFrequency, kOccupied, 10, 0));
  const TxDecision decision = policy.authorize(kFrequency, kOccupied, 10, 0);
  TEST_ASSERT_FALSE(decision.allowed());
  TEST_ASSERT_FALSE(decision.grant.valid());  // decisão negada não carrega autorização
  TEST_ASSERT_EQUAL_UINT32(0, policy.airtimeUsedMs(0));
}

void test_enabled_without_band_is_still_denied() {
  TxPolicy policy;
  TxLimits limits;
  limits.enabled = true;
  policy.setLimits(limits);
  TEST_ASSERT_EQUAL(TxDenial::NoBandConfigured, policy.check(kFrequency, kOccupied, 10, 0));
  limits.minHz = 434000000;
  limits.maxHz = 433000000;  // invertida
  policy.setLimits(limits);
  TEST_ASSERT_EQUAL(TxDenial::NoBandConfigured, policy.check(kFrequency, kOccupied, 10, 0));
}

void test_band_edges_count_the_occupied_bandwidth() {
  TxPolicy policy;
  policy.setLimits(openLimits());
  TEST_ASSERT_EQUAL(TxDenial::None, policy.check(kFrequency, kOccupied, 10, 0));
  // Portadora dentro da faixa, mas a metade da banda ocupada cruza a borda.
  TEST_ASSERT_EQUAL(TxDenial::OutOfBand, policy.check(433052000, kOccupied, 10, 0));
  TEST_ASSERT_EQUAL(TxDenial::OutOfBand, policy.check(434788000, kOccupied, 10, 0));
  TEST_ASSERT_EQUAL(TxDenial::None, policy.check(433056000, kOccupied, 10, 0));
  TEST_ASSERT_EQUAL(TxDenial::OutOfBand, policy.check(315000000, kOccupied, 10, 0));
}

void test_frequency_outside_chip_bands() {
  TxPolicy policy;
  TxLimits limits = openLimits();
  limits.minHz = 100000000;
  limits.maxHz = 900000000;
  policy.setLimits(limits);
  TEST_ASSERT_EQUAL(TxDenial::OutsideChipBand, policy.check(500000000, kOccupied, 10, 0));
}

void test_single_packet_longer_than_the_budget() {
  TxPolicy policy;
  policy.setLimits(openLimits());
  TEST_ASSERT_EQUAL(TxDenial::PacketTooLong, policy.check(kFrequency, kOccupied, 601, 0));
  TEST_ASSERT_EQUAL(TxDenial::None, policy.check(kFrequency, kOccupied, 600, 0));
}

void test_airtime_budget_is_enforced_over_a_sliding_minute() {
  TxPolicy policy;
  policy.setLimits(openLimits());
  uint32_t now = 1000;
  // 5 pacotes de 100 ms: 500 ms gastos.
  for (int i = 0; i < 5; ++i) {
    const TxDecision decision = policy.authorize(kFrequency, kOccupied, 100, now);
    TEST_ASSERT_TRUE(decision.allowed());
    TEST_ASSERT_TRUE(decision.grant.valid());
    TEST_ASSERT_EQUAL_UINT32(kFrequency, decision.grant.frequencyHz());
    now += 1000;
  }
  TEST_ASSERT_EQUAL_UINT32(500, policy.airtimeUsedMs(now));
  TEST_ASSERT_TRUE(policy.authorize(kFrequency, kOccupied, 100, now).allowed());  // 600: no limite
  TEST_ASSERT_EQUAL(TxDenial::AirtimeExceeded,
                    policy.authorize(kFrequency, kOccupied, 1, now).denial);
  // Negado não debita.
  TEST_ASSERT_EQUAL_UINT32(600, policy.airtimeUsedMs(now));

  // 30 s depois ainda está tudo na janela.
  now += 30000;
  TEST_ASSERT_EQUAL(TxDenial::AirtimeExceeded, policy.check(kFrequency, kOccupied, 100, now));
  // Depois de mais de um minuto desde os primeiros pacotes, o orçamento volta.
  now += 45000;
  TEST_ASSERT_EQUAL(TxDenial::None, policy.check(kFrequency, kOccupied, 100, now));
  now += 60000;
  TEST_ASSERT_EQUAL_UINT32(0, policy.airtimeUsedMs(now));
}

void test_continuous_transmission_is_impossible() {
  // Tentando transmitir sem parar por 10 minutos, o tempo no ar não passa de 1 %
  // (mais a granularidade de um intervalo de 10 s).
  TxPolicy policy;
  policy.setLimits(openLimits());
  uint32_t granted = 0;
  for (uint32_t now = 0; now < 600000; now += 50)
    if (policy.authorize(kFrequency, kOccupied, 50, now).allowed()) granted += 50;
  TEST_ASSERT_TRUE(granted <= 600 * 10 + 600);
  TEST_ASSERT_TRUE(granted >= 600 * 8);
}

void test_zero_budget_blocks_transmission() {
  TxPolicy policy;
  TxLimits limits = openLimits();
  limits.maxAirtimeMsPerMinute = 0;
  policy.setLimits(limits);
  TEST_ASSERT_EQUAL(TxDenial::PacketTooLong, policy.check(kFrequency, kOccupied, 1, 0));
}

void test_clock_wraparound_does_not_reopen_the_budget() {
  TxPolicy policy;
  policy.setLimits(openLimits());
  uint32_t now = 0xFFFFFFFFu - 5000;
  TEST_ASSERT_TRUE(policy.authorize(kFrequency, kOccupied, 600, now).allowed());
  now += 10000;  // o contador de milissegundos estourou
  TEST_ASSERT_EQUAL(TxDenial::AirtimeExceeded, policy.check(kFrequency, kOccupied, 100, now));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_factory_default_denies_everything);
  RUN_TEST(test_enabled_without_band_is_still_denied);
  RUN_TEST(test_band_edges_count_the_occupied_bandwidth);
  RUN_TEST(test_frequency_outside_chip_bands);
  RUN_TEST(test_single_packet_longer_than_the_budget);
  RUN_TEST(test_airtime_budget_is_enforced_over_a_sliding_minute);
  RUN_TEST(test_continuous_transmission_is_impossible);
  RUN_TEST(test_zero_budget_blocks_transmission);
  RUN_TEST(test_clock_wraparound_does_not_reopen_the_budget);
  return UNITY_END();
}
