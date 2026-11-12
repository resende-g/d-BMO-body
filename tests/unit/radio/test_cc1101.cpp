#include <cstring>

#include "dbmo/mock/mock_hal.h"
#include "dbmo/radio/cc1101.h"
#include "dbmo/radio/cc1101_regs.h"
#include "dbmo/sim/cc1101_sim.h"
#include "dbmo_test.h"

using dbmo::Status;
using dbmo::mock::MockClock;
using dbmo::sim::Cc1101Sim;
using namespace dbmo::radio;
namespace reg = dbmo::radio::cc1101;

namespace {

struct Fixture {
  Cc1101Sim sim;
  MockClock clock;
  Cc1101 chip{sim, clock};
  TxPolicy policy;

  Fixture() {
    TxLimits limits;
    limits.enabled = true;
    limits.minHz = 433050000;
    limits.maxHz = 434790000;
    limits.maxAirtimeMsPerMinute = 60000;  // os testes do driver pedem várias autorizações
    policy.setLimits(limits);
  }
  TxGrant grant(uint32_t frequencyHz = 433920000) {
    return policy.authorize(frequencyHz, 12000, 600, clock.nowMs()).grant;
  }
  void configure() { TEST_ASSERT_OK(chip.configure(RadioConfig(), 0x12)); }
};

}  // namespace

void setUp() {}
void tearDown() {}

void test_reset_and_identify() {
  Fixture f;
  TEST_ASSERT_OK(f.chip.reset());
  TEST_ASSERT_EQUAL_UINT(1, f.sim.resetCount);
  const dbmo::Result<ChipId> id = f.chip.identify();
  TEST_ASSERT_TRUE(id.ok());
  TEST_ASSERT_EQUAL_HEX8(0x00, id.value().partnum);
  TEST_ASSERT_EQUAL_HEX8(0x14, id.value().version);
}

void test_identify_accepts_other_version_values() {
  Fixture f;
  f.sim.version = 0x04;
  TEST_ASSERT_EQUAL_HEX8(0x04, f.chip.identify().value().version);
  f.sim.version = 0x18;  // "pode mudar sem aviso": o driver não recusa
  TEST_ASSERT_TRUE(f.chip.identify().ok());
}

void test_absent_chip_with_miso_high() {
  Fixture f;
  f.sim.present = false;  // MISO em nível alto: nunca fica "pronto"
  TEST_ASSERT_STATUS(Status::Timeout, f.chip.reset());
  TEST_ASSERT_STATUS(Status::HardwareAbsent, f.chip.identify().status());
}

void test_absent_chip_when_miso_cannot_be_read() {
  // Porte que não lê o MISO: a ausência aparece nos valores lidos.
  Fixture f;
  f.sim.present = false;
  f.sim.misoReadable = false;
  f.sim.absentValue = 0xFF;
  TEST_ASSERT_STATUS(Status::HardwareAbsent, f.chip.identify().status());
  f.sim.absentValue = 0x00;  // MISO preso em zero imita PARTNUM = 0x00
  TEST_ASSERT_STATUS(Status::HardwareAbsent, f.chip.identify().status());
}

void test_wrong_partnum_is_not_a_cc1101() {
  Fixture f;
  f.sim.partnum = 0x80;
  TEST_ASSERT_STATUS(Status::HardwareAbsent, f.chip.identify().status());
}

void test_configure_writes_the_documented_fields() {
  Fixture f;
  RadioConfig config;
  config.frequencyHz = 433920000;
  config.dataRateBaud = 1200;
  config.bandwidthHz = 58000;
  config.deviationHz = 5157;
  config.syncWord = 0xA55A;
  TEST_ASSERT_OK(f.chip.configure(config, 0x12));

  TEST_ASSERT_EQUAL_HEX32(0x10B071, f.sim.frequencyWord());
  TEST_ASSERT_EQUAL_HEX8(0xF5, f.sim.registers[reg::kMdmcfg4]);  // banda E=3 M=3, taxa E=5
  TEST_ASSERT_EQUAL_HEX8(0x83, f.sim.registers[reg::kMdmcfg3]);
  TEST_ASSERT_EQUAL_HEX8(0x02, f.sim.registers[reg::kMdmcfg2]);  // 2-FSK, sincronismo 16 bits
  TEST_ASSERT_EQUAL_HEX8(0x15, f.sim.registers[reg::kDeviatn]);
  TEST_ASSERT_EQUAL_HEX8(0xA5, f.sim.registers[reg::kSync1]);
  TEST_ASSERT_EQUAL_HEX8(0x5A, f.sim.registers[reg::kSync0]);
  TEST_ASSERT_EQUAL_HEX8(0x05, f.sim.registers[reg::kPktctrl0]);  // tamanho variável + CRC
  TEST_ASSERT_EQUAL_HEX8(0x04, f.sim.registers[reg::kPktctrl1]);  // anexa RSSI/LQI
  TEST_ASSERT_EQUAL_HEX8(61, f.sim.registers[reg::kPktlen]);
  TEST_ASSERT_EQUAL_HEX8(0x10, f.sim.registers[reg::kFrend0]);
  TEST_ASSERT_EQUAL_HEX8(0x12, f.sim.patable[0]);
}

void test_configure_ook_uses_two_patable_entries() {
  Fixture f;
  RadioConfig config;
  config.modulation = Modulation::Ook;
  config.crc = false;
  config.whitening = true;
  TEST_ASSERT_OK(f.chip.configure(config, 0x34));
  TEST_ASSERT_EQUAL_HEX8(0x32, f.sim.registers[reg::kMdmcfg2]);  // ASK/OOK
  TEST_ASSERT_EQUAL_HEX8(0x11, f.sim.registers[reg::kFrend0]);
  TEST_ASSERT_EQUAL_HEX8(0x00, f.sim.patable[0]);  // bit 0: portadora desligada
  TEST_ASSERT_EQUAL_HEX8(0x34, f.sim.patable[1]);
  TEST_ASSERT_EQUAL_HEX8(0x41, f.sim.registers[reg::kPktctrl0]);  // whitening, sem CRC
}

void test_configure_rejects_invalid_parameters_without_touching_the_chip() {
  Fixture f;
  RadioConfig config;
  config.frequencyHz = 500000000;
  const unsigned before = f.sim.selectCount;
  TEST_ASSERT_STATUS(Status::OutOfRange, f.chip.configure(config, 0x12));
  TEST_ASSERT_EQUAL_UINT(before, f.sim.selectCount);
}

void test_configure_detects_failed_write_by_read_back() {
  Fixture f;
  f.sim.present = false;       // nada é gravado...
  f.sim.misoReadable = false;  // ...e o porte não percebe pelo MISO
  f.sim.absentValue = 0x00;
  TEST_ASSERT_STATUS(Status::IoError, f.chip.configure(RadioConfig(), 0x12));
}

void test_bus_error_is_propagated() {
  Fixture f;
  f.sim.failTransfers = 1;
  TEST_ASSERT_STATUS(Status::IoError, f.chip.reset());
  f.sim.selectStatus = Status::Busy;
  TEST_ASSERT_STATUS(Status::Busy, f.chip.idle());
}

void test_operations_require_configuration() {
  Fixture f;
  RadioPacket packet;
  TEST_ASSERT_STATUS(Status::NotInitialized, f.chip.startReceive());
  TEST_ASSERT_STATUS(Status::NotInitialized, f.chip.pollReceive(&packet).status());
  const uint8_t data[1] = {1};
  TEST_ASSERT_STATUS(Status::NotInitialized, f.chip.startTransmit(f.grant(), data, 1));
}

void test_receive_packet() {
  Fixture f;
  f.configure();
  TEST_ASSERT_OK(f.chip.startReceive());
  TEST_ASSERT_EQUAL_HEX8(reg::kStateRx, f.sim.state);

  RadioPacket packet;
  TEST_ASSERT_EQUAL(RxPoll::Nothing, f.chip.pollReceive(&packet).value());

  const uint8_t data[5] = {'d', '-', 'B', 'M', 'O'};
  TEST_ASSERT_TRUE(f.sim.injectPacket(data, 5, 0xB0));
  TEST_ASSERT_EQUAL(RxPoll::Packet, f.chip.pollReceive(&packet).value());
  TEST_ASSERT_EQUAL_UINT8(5, packet.length);
  TEST_ASSERT_EQUAL_MEMORY(data, packet.data, 5);
  // RSSI bruto 0xB0 = 176 → (176 − 256) / 2 − 74 = −114 dBm.
  TEST_ASSERT_EQUAL_INT16(-114, packet.rssiDbm);
  TEST_ASSERT_EQUAL_UINT8(0x2A, packet.lqi);

  // Depois de um pacote o rádio está parado até um novo startReceive().
  TEST_ASSERT_EQUAL(RxPoll::Stopped, f.chip.pollReceive(&packet).value());
  TEST_ASSERT_OK(f.chip.startReceive());
  TEST_ASSERT_EQUAL(RxPoll::Nothing, f.chip.pollReceive(&packet).value());
}

void test_receive_maximum_length_packet() {
  Fixture f;
  f.configure();
  TEST_ASSERT_OK(f.chip.startReceive());
  uint8_t data[kMaxPayload];
  for (uint8_t i = 0; i < kMaxPayload; ++i) data[i] = i;
  TEST_ASSERT_TRUE(f.sim.injectPacket(data, kMaxPayload));
  RadioPacket packet;
  TEST_ASSERT_EQUAL(RxPoll::Packet, f.chip.pollReceive(&packet).value());
  TEST_ASSERT_EQUAL_UINT8(kMaxPayload, packet.length);
  TEST_ASSERT_EQUAL_MEMORY(data, packet.data, kMaxPayload);
}

void test_receive_crc_error_is_reported_and_discarded() {
  Fixture f;
  f.configure();
  TEST_ASSERT_OK(f.chip.startReceive());
  const uint8_t data[3] = {1, 2, 3};
  f.sim.injectPacket(data, 3, 0x80, false);
  RadioPacket packet;
  TEST_ASSERT_EQUAL(RxPoll::CrcError, f.chip.pollReceive(&packet).value());
}

void test_receive_without_crc_accepts_any_status() {
  Fixture f;
  RadioConfig config;
  config.crc = false;
  TEST_ASSERT_OK(f.chip.configure(config, 0x12));
  TEST_ASSERT_OK(f.chip.startReceive());
  const uint8_t data[3] = {1, 2, 3};
  f.sim.injectPacket(data, 3, 0x80, false);
  RadioPacket packet;
  TEST_ASSERT_EQUAL(RxPoll::Packet, f.chip.pollReceive(&packet).value());
}

void test_receive_overflow_flushes_the_fifo() {
  Fixture f;
  f.configure();
  TEST_ASSERT_OK(f.chip.startReceive());
  f.sim.forceOverflow();
  RadioPacket packet;
  TEST_ASSERT_EQUAL(RxPoll::Overflow, f.chip.pollReceive(&packet).value());
  TEST_ASSERT_FALSE(f.sim.rxOverflow);
  TEST_ASSERT_EQUAL_UINT(0, f.sim.rxCount);
  TEST_ASSERT_OK(f.chip.startReceive());
  TEST_ASSERT_EQUAL_HEX8(reg::kStateRx, f.sim.state);
}

void test_receive_malformed_lengths() {
  Fixture f;
  f.configure();
  RadioPacket packet;

  TEST_ASSERT_OK(f.chip.startReceive());
  const uint8_t zeroLength[4] = {0, 0xAA, 0x80, 0x80};
  f.sim.injectRaw(zeroLength, 4);
  TEST_ASSERT_EQUAL(RxPoll::Malformed, f.chip.pollReceive(&packet).value());
  TEST_ASSERT_EQUAL_UINT(0, f.sim.rxCount);

  TEST_ASSERT_OK(f.chip.startReceive());
  const uint8_t tooLong[4] = {62, 0xAA, 0x80, 0x80};  // maior que o máximo configurado
  f.sim.injectRaw(tooLong, 4);
  TEST_ASSERT_EQUAL(RxPoll::Malformed, f.chip.pollReceive(&packet).value());

  TEST_ASSERT_OK(f.chip.startReceive());
  const uint8_t truncated[4] = {5, 1, 2, 3};  // diz 5 bytes, chegaram menos
  f.sim.injectRaw(truncated, 4);
  TEST_ASSERT_EQUAL(RxPoll::Malformed, f.chip.pollReceive(&packet).value());
}

void test_transmit_packet() {
  Fixture f;
  f.configure();
  const uint8_t data[4] = {0xDE, 0xAD, 0xBE, 0xEF};
  f.sim.txBusyPolls = 4;  // cada consulta do driver lê MARCSTATE duas vezes
  TEST_ASSERT_OK(f.chip.startTransmit(f.grant(), data, 4));
  TEST_ASSERT_EQUAL_UINT(1, f.sim.transmitCount);
  TEST_ASSERT_EQUAL_UINT8(4, f.sim.lastTxLength);
  TEST_ASSERT_EQUAL_MEMORY(data, f.sim.lastTx, 4);

  TEST_ASSERT_EQUAL(TxPoll::Busy, f.chip.pollTransmit().value());
  TEST_ASSERT_EQUAL(TxPoll::Busy, f.chip.pollTransmit().value());
  TEST_ASSERT_EQUAL(TxPoll::Done, f.chip.pollTransmit().value());
}

void test_transmit_requires_a_valid_grant_for_the_configured_frequency() {
  Fixture f;
  f.configure();
  const uint8_t data[1] = {1};

  // Política que nega: a autorização vem inválida e o chip não transmite.
  TxPolicy closed;
  const TxDecision denied = closed.authorize(433920000, 12000, 10, 0);
  TEST_ASSERT_STATUS(Status::NotPermitted, f.chip.startTransmit(denied.grant, data, 1));

  // Autorização válida, mas para outra frequência.
  TEST_ASSERT_STATUS(Status::NotPermitted, f.chip.startTransmit(f.grant(434000000), data, 1));
  TEST_ASSERT_EQUAL_UINT(0, f.sim.transmitCount);
  TEST_ASSERT_EQUAL_UINT(0, f.sim.strobeCount[reg::kStx]);
}

void test_transmit_rejects_bad_lengths() {
  Fixture f;
  f.configure();
  uint8_t data[kMaxPayload + 1] = {};
  TEST_ASSERT_STATUS(Status::InvalidArgument, f.chip.startTransmit(f.grant(), data, 0));
  TEST_ASSERT_STATUS(Status::InvalidArgument,
                     f.chip.startTransmit(f.grant(), data, kMaxPayload + 1));
  TEST_ASSERT_STATUS(Status::InvalidArgument, f.chip.startTransmit(f.grant(), nullptr, 4));
  TEST_ASSERT_EQUAL_UINT(0, f.sim.transmitCount);
}

void test_transmit_underflow_is_recovered() {
  Fixture f;
  f.configure();
  f.sim.txUnderflow = true;
  f.sim.state = reg::kStateTxUnderflow;
  TEST_ASSERT_EQUAL(TxPoll::Underflow, f.chip.pollTransmit().value());
  TEST_ASSERT_EQUAL_HEX8(reg::kStateIdle, f.sim.state);
}

void test_sleep_and_wake() {
  Fixture f;
  f.configure();
  TEST_ASSERT_OK(f.chip.sleep());
  TEST_ASSERT_EQUAL_HEX8(reg::kStateSleep, f.sim.state);
  // A próxima transação acorda o chip.
  TEST_ASSERT_OK(f.chip.startReceive());
  TEST_ASSERT_EQUAL_HEX8(reg::kStateRx, f.sim.state);
}

void test_wake_without_readable_miso_waits_a_fixed_time() {
  Fixture f;
  f.sim.misoReadable = false;
  f.configure();
  TEST_ASSERT_OK(f.chip.sleep());
  const uint64_t before = f.clock.nowUs();
  TEST_ASSERT_OK(f.chip.idle());
  TEST_ASSERT_TRUE(f.clock.nowUs() - before >= 150);  // CC-10: 150 µs no mínimo
}

void test_overrides_cannot_change_frequency_or_packet_mode() {
  Fixture f;
  f.configure();
  const RegisterSetting tuning[] = {{0x1B, 0x43}, {0x21, 0x56}};  // AGCCTRL2, FREND1
  TEST_ASSERT_OK(f.chip.applyOverrides(tuning, 2));
  TEST_ASSERT_EQUAL_HEX8(0x43, f.sim.registers[0x1B]);

  const RegisterSetting frequency[] = {{0x1B, 0x07}, {reg::kFreq1, 0x00}};
  TEST_ASSERT_STATUS(Status::NotPermitted, f.chip.applyOverrides(frequency, 2));
  TEST_ASSERT_EQUAL_HEX8(0x43, f.sim.registers[0x1B]);  // nada do lote foi aplicado
  TEST_ASSERT_EQUAL_HEX32(0x10B071, f.sim.frequencyWord());

  const RegisterSetting async[] = {{reg::kPktctrl0, 0x32}};  // modo serial assíncrono
  TEST_ASSERT_STATUS(Status::NotPermitted, f.chip.applyOverrides(async, 1));
  const RegisterSetting power[] = {{reg::kPatable, 0xC0}};
  TEST_ASSERT_STATUS(Status::NotPermitted, f.chip.applyOverrides(power, 1));
}

void test_low_level_write_cannot_touch_what_goes_on_air() {
  Fixture f;
  f.configure();
  const uint8_t before[] = {f.sim.registers[reg::kMcsm1], f.sim.registers[reg::kMdmcfg2]};
  // Cada um destes mudaria o que é transmitido sem passar pela política.
  const uint8_t forbidden[] = {reg::kFreq2,    reg::kFreq1,    reg::kFreq0,   reg::kPktlen,
                               reg::kPktctrl1, reg::kPktctrl0, reg::kMdmcfg4, reg::kMdmcfg3,
                               reg::kMdmcfg2,  reg::kDeviatn,  reg::kMcsm1,   reg::kFrend0};
  for (uint8_t address : forbidden)
    TEST_ASSERT_STATUS(Status::NotPermitted, f.chip.writeRegister(address, 0xFF));
  TEST_ASSERT_EQUAL_HEX32(0x10B071, f.sim.frequencyWord());
  TEST_ASSERT_EQUAL_HEX8(before[0], f.sim.registers[reg::kMcsm1]);  // não fica "sempre em TX"
  TEST_ASSERT_EQUAL_HEX8(before[1], f.sim.registers[reg::kMdmcfg2]);
  TEST_ASSERT_EQUAL_HEX8(0x05, f.sim.registers[reg::kPktctrl0]);  // sem modo aleatório/assíncrono

  // PATABLE, FIFO e comandos não são registradores de configuração.
  TEST_ASSERT_STATUS(Status::InvalidArgument, f.chip.writeRegister(reg::kPatable, 0xC0));
  TEST_ASSERT_STATUS(Status::InvalidArgument, f.chip.writeRegister(reg::kFifo, 0xAA));
  TEST_ASSERT_STATUS(Status::InvalidArgument, f.chip.writeRegister(reg::kStx, 0));
  TEST_ASSERT_EQUAL_UINT(0, f.sim.transmitCount);

  // Registradores de ajuste continuam acessíveis.
  TEST_ASSERT_OK(f.chip.writeRegister(0x1B, 0x43));  // AGCCTRL2
  TEST_ASSERT_EQUAL_HEX8(0x43, f.sim.registers[0x1B]);
}

void test_overrides_cannot_change_bandwidth_or_tx_behaviour() {
  Fixture f;
  f.configure();
  const RegisterSetting rate[] = {{reg::kMdmcfg3, 0xFF}};
  TEST_ASSERT_STATUS(Status::NotPermitted, f.chip.applyOverrides(rate, 1));
  const RegisterSetting stayInTx[] = {{reg::kMcsm1, 0x32}};  // TXOFF_MODE = permanecer em TX
  TEST_ASSERT_STATUS(Status::NotPermitted, f.chip.applyOverrides(stayInTx, 1));
  const RegisterSetting length[] = {{reg::kPktlen, 0xFF}};
  TEST_ASSERT_STATUS(Status::NotPermitted, f.chip.applyOverrides(length, 1));
}

void test_channel_and_offset_registers_are_protected_and_reset_by_configure() {
  Fixture f;
  f.configure();
  // CHANNR, FSCTRL0 e o espaçamento de canal deslocariam a portadora sem a política ver.
  const uint8_t shifting[] = {reg::kChannr, reg::kFsctrl0, reg::kMdmcfg1, reg::kMdmcfg0};
  for (uint8_t address : shifting) {
    TEST_ASSERT_STATUS(Status::NotPermitted, f.chip.writeRegister(address, 0x7F));
    const RegisterSetting setting[] = {{address, 0x7F}};
    TEST_ASSERT_STATUS(Status::NotPermitted, f.chip.applyOverrides(setting, 1));
  }
  // Mesmo que algo os tenha alterado antes, configure() os devolve ao valor conhecido.
  f.sim.registers[reg::kChannr] = 100;
  f.sim.registers[reg::kFsctrl0] = 0x7F;
  f.configure();
  TEST_ASSERT_EQUAL_HEX8(0x00, f.sim.registers[reg::kChannr]);
  TEST_ASSERT_EQUAL_HEX8(0x00, f.sim.registers[reg::kFsctrl0]);
  TEST_ASSERT_EQUAL_HEX8(0x22, f.sim.registers[reg::kMdmcfg1]);
  TEST_ASSERT_EQUAL_HEX8(0xF8, f.sim.registers[reg::kMdmcfg0]);
}

void test_grant_is_single_use_and_bound_to_what_was_evaluated() {
  Fixture f;
  f.configure();
  const uint8_t data[61] = {};
  const TxGrant grant = f.grant();
  TEST_ASSERT_OK(f.chip.startTransmit(grant, data, 4));
  TEST_ASSERT_STATUS(Status::NotPermitted, f.chip.startTransmit(grant, data, 4));  // reuso
  TEST_ASSERT_EQUAL_UINT(1, f.sim.transmitCount);

  // Autorização para um pacote curto não serve para um longo (mais tempo no ar).
  const TxGrant shortGrant = f.policy.authorize(433920000, 12000, 70, 0).grant;
  TEST_ASSERT_STATUS(Status::NotPermitted, f.chip.startTransmit(shortGrant, data, 61));

  // Nem para uma configuração mais larga feita depois.
  const TxGrant narrow = f.grant();
  RadioConfig wide;
  wide.modulation = Modulation::Gfsk;
  wide.dataRateBaud = 250000;
  wide.deviationHz = 127000;
  wide.bandwidthHz = 540000;
  TEST_ASSERT_OK(f.chip.configure(wide, 0x12));
  TEST_ASSERT_STATUS(Status::NotPermitted, f.chip.startTransmit(narrow, data, 4));
  TEST_ASSERT_EQUAL_UINT(1, f.sim.transmitCount);
}

void test_failed_reconfiguration_blocks_the_radio() {
  Fixture f;
  f.configure();
  RadioConfig other;
  other.frequencyHz = 868300000;
  f.sim.failTransfers = 1;  // o barramento falha no meio da nova configuração
  TEST_ASSERT_FALSE(dbmo::isOk(f.chip.configure(other, 0x12)));
  const uint8_t data[4] = {};
  // Com os registradores em estado desconhecido, nada de transmitir nem receber.
  TEST_ASSERT_STATUS(Status::NotInitialized, f.chip.startTransmit(f.grant(), data, 4));
  TEST_ASSERT_STATUS(Status::NotInitialized, f.chip.startReceive());
  TEST_ASSERT_EQUAL_UINT(0, f.sim.transmitCount);
}

void test_ook_power_table_is_rewritten_before_each_transmission() {
  Fixture f;
  RadioConfig ook;
  ook.modulation = Modulation::Ook;
  TEST_ASSERT_OK(f.chip.configure(ook, 0x34));
  TEST_ASSERT_OK(f.chip.sleep());
  f.sim.patable[1] = 0x00;  // o que o chip real perde ao dormir
  const uint8_t data[4] = {};
  TEST_ASSERT_OK(f.chip.startTransmit(f.policy.authorize(433920000, 12000, 600, 0).grant, data, 4));
  TEST_ASSERT_EQUAL_HEX8(0x00, f.sim.patable[0]);
  TEST_ASSERT_EQUAL_HEX8(0x34, f.sim.patable[1]);
}

void test_rssi_conversion() {
  TEST_ASSERT_EQUAL_INT16(-74, rssiToDbm(0));
  TEST_ASSERT_EQUAL_INT16(-11, rssiToDbm(127));   // 63 − 74
  TEST_ASSERT_EQUAL_INT16(-138, rssiToDbm(128));  // −64 − 74
  TEST_ASSERT_EQUAL_INT16(-74, rssiToDbm(255));   // −0,5 arredonda para 0
  Fixture f;
  f.sim.rssiRaw = 0xB0;
  TEST_ASSERT_EQUAL_INT16(-114, f.chip.rssiDbm().value());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_reset_and_identify);
  RUN_TEST(test_identify_accepts_other_version_values);
  RUN_TEST(test_absent_chip_with_miso_high);
  RUN_TEST(test_absent_chip_when_miso_cannot_be_read);
  RUN_TEST(test_wrong_partnum_is_not_a_cc1101);
  RUN_TEST(test_configure_writes_the_documented_fields);
  RUN_TEST(test_configure_ook_uses_two_patable_entries);
  RUN_TEST(test_configure_rejects_invalid_parameters_without_touching_the_chip);
  RUN_TEST(test_configure_detects_failed_write_by_read_back);
  RUN_TEST(test_bus_error_is_propagated);
  RUN_TEST(test_operations_require_configuration);
  RUN_TEST(test_receive_packet);
  RUN_TEST(test_receive_maximum_length_packet);
  RUN_TEST(test_receive_crc_error_is_reported_and_discarded);
  RUN_TEST(test_receive_without_crc_accepts_any_status);
  RUN_TEST(test_receive_overflow_flushes_the_fifo);
  RUN_TEST(test_receive_malformed_lengths);
  RUN_TEST(test_transmit_packet);
  RUN_TEST(test_transmit_requires_a_valid_grant_for_the_configured_frequency);
  RUN_TEST(test_transmit_rejects_bad_lengths);
  RUN_TEST(test_transmit_underflow_is_recovered);
  RUN_TEST(test_sleep_and_wake);
  RUN_TEST(test_wake_without_readable_miso_waits_a_fixed_time);
  RUN_TEST(test_overrides_cannot_change_frequency_or_packet_mode);
  RUN_TEST(test_low_level_write_cannot_touch_what_goes_on_air);
  RUN_TEST(test_overrides_cannot_change_bandwidth_or_tx_behaviour);
  RUN_TEST(test_channel_and_offset_registers_are_protected_and_reset_by_configure);
  RUN_TEST(test_grant_is_single_use_and_bound_to_what_was_evaluated);
  RUN_TEST(test_failed_reconfiguration_blocks_the_radio);
  RUN_TEST(test_ook_power_table_is_rewritten_before_each_transmission);
  RUN_TEST(test_rssi_conversion);
  return UNITY_END();
}
