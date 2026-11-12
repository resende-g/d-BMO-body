#include <cstring>

#include "dbmo/config/pins.h"
#include "dbmo/mock/mock_hal.h"
#include "dbmo/nfc/pn532.h"
#include "dbmo/sim/pn532_sim.h"
#include "dbmo_test.h"

using dbmo::Status;
using dbmo::mock::MockClock;
using dbmo::mock::MockI2cBus;
using dbmo::sim::Pn532Sim;
using namespace dbmo::nfc;

namespace {

constexpr uint8_t kAddress = dbmo::config::i2c::kPn532;

struct Fixture {
  MockI2cBus bus;
  MockClock clock;
  Pn532Sim sim;
  Pn532 chip{bus, clock, kAddress};

  Fixture() { bus.attach(kAddress, &sim); }
  // Consulta a cada 1 ms até o comando terminar (ou o limite do teste).
  CommandState finish(uint32_t limitMs = 1000) {
    for (uint32_t i = 0; i < limitMs; ++i) {
      const CommandState state = chip.poll();
      if (state == CommandState::Done || state == CommandState::Failed) return state;
      clock.advanceMs(1);
    }
    return chip.state();
  }
};

}  // namespace

void setUp() {}
void tearDown() {}

void test_probe() {
  Fixture f;
  TEST_ASSERT_OK(f.chip.probe());
  f.bus.detach(kAddress);
  TEST_ASSERT_STATUS(Status::HardwareAbsent, f.chip.probe());
}

void test_get_firmware_version() {
  Fixture f;
  TEST_ASSERT_OK(f.chip.startGetFirmwareVersion());
  TEST_ASSERT_EQUAL(CommandState::WaitingAck, f.chip.state());
  TEST_ASSERT_EQUAL(CommandState::WaitingResponse, f.chip.poll());
  TEST_ASSERT_EQUAL(CommandState::Done, f.chip.poll());
  FirmwareVersion version{};
  TEST_ASSERT_OK(Pn532::parseFirmwareVersion(f.chip.response(), f.chip.responseLength(), &version));
  TEST_ASSERT_EQUAL_HEX8(0x32, version.ic);
  TEST_ASSERT_EQUAL_UINT8(1, version.version);
  TEST_ASSERT_EQUAL_UINT8(6, version.revision);
}

void test_waits_while_the_chip_is_not_ready() {
  Fixture f;
  f.sim.readsBeforeAck = 3;
  f.sim.readsBeforeResponse = 5;
  TEST_ASSERT_OK(f.chip.startGetFirmwareVersion());
  for (int i = 0; i < 3; ++i) TEST_ASSERT_EQUAL(CommandState::WaitingAck, f.chip.poll());
  TEST_ASSERT_EQUAL(CommandState::WaitingResponse, f.chip.poll());
  for (int i = 0; i < 5; ++i) TEST_ASSERT_EQUAL(CommandState::WaitingResponse, f.chip.poll());
  TEST_ASSERT_EQUAL(CommandState::Done, f.chip.poll());
}

void test_second_command_while_busy_is_rejected() {
  Fixture f;
  TEST_ASSERT_OK(f.chip.startGetFirmwareVersion());
  TEST_ASSERT_STATUS(Status::Busy, f.chip.startSamConfiguration());
  TEST_ASSERT_EQUAL(CommandState::Done, f.finish());
  TEST_ASSERT_OK(f.chip.startSamConfiguration());
  TEST_ASSERT_EQUAL(CommandState::Done, f.finish());
  TEST_ASSERT_TRUE(f.sim.samConfigured);
}

void test_ack_timeout() {
  Fixture f;
  f.sim.readsBeforeAck = 100000;
  TEST_ASSERT_OK(f.chip.startGetFirmwareVersion());
  TEST_ASSERT_EQUAL(CommandState::Failed, f.finish());
  TEST_ASSERT_STATUS(Status::Timeout, f.chip.failure());
  TEST_ASSERT_TRUE(f.clock.nowMs() >= Pn532::kAckTimeoutMs);
  TEST_ASSERT_TRUE(f.clock.nowMs() < Pn532::kAckTimeoutMs + 5);
}

void test_response_timeout_aborts_the_command() {
  Fixture f;
  f.sim.neverRespond = true;
  TEST_ASSERT_OK(f.chip.startFindTagIso14443A());
  TEST_ASSERT_EQUAL(CommandState::Failed, f.finish());
  TEST_ASSERT_STATUS(Status::Timeout, f.chip.failure());
  TEST_ASSERT_EQUAL_UINT(1, f.sim.abortCount);  // o PN532 foi avisado para desistir
  // O driver fica livre para o próximo comando.
  f.sim.neverRespond = false;
  TEST_ASSERT_OK(f.chip.startGetFirmwareVersion());
  TEST_ASSERT_EQUAL(CommandState::Done, f.finish());
}

void test_nack_instead_of_ack_is_a_protocol_error() {
  Fixture f;
  f.sim.nackInsteadOfAck = true;
  TEST_ASSERT_OK(f.chip.startGetFirmwareVersion());
  TEST_ASSERT_EQUAL(CommandState::Failed, f.finish());
  TEST_ASSERT_STATUS(Status::ProtocolError, f.chip.failure());
}

void test_corrupted_response_is_detected() {
  Fixture f;
  f.sim.corruptNextResponse = true;
  TEST_ASSERT_OK(f.chip.startGetFirmwareVersion());
  TEST_ASSERT_EQUAL(CommandState::Failed, f.finish());
  TEST_ASSERT_STATUS(Status::CrcError, f.chip.failure());
}

void test_response_to_another_command_is_rejected() {
  Fixture f;
  f.sim.wrongResponseCode = true;
  TEST_ASSERT_OK(f.chip.startGetFirmwareVersion());
  TEST_ASSERT_EQUAL(CommandState::Failed, f.finish());
  TEST_ASSERT_STATUS(Status::ProtocolError, f.chip.failure());
}

void test_error_frame_is_a_protocol_error() {
  Fixture f;
  f.sim.errorFrameResponse = true;
  TEST_ASSERT_OK(f.chip.startGetFirmwareVersion());
  TEST_ASSERT_EQUAL(CommandState::Failed, f.finish());
  TEST_ASSERT_STATUS(Status::ProtocolError, f.chip.failure());
}

void test_bus_errors() {
  Fixture f;
  f.bus.failNext(Status::IoError);
  TEST_ASSERT_STATUS(Status::IoError, f.chip.startGetFirmwareVersion());
  TEST_ASSERT_EQUAL(CommandState::Failed, f.chip.state());

  TEST_ASSERT_OK(f.chip.startGetFirmwareVersion());
  f.bus.failNext(Status::IoError);
  TEST_ASSERT_EQUAL(CommandState::Failed, f.chip.poll());
  TEST_ASSERT_STATUS(Status::IoError, f.chip.failure());
}

void test_abort_sends_ack_only_when_a_command_is_pending() {
  Fixture f;
  f.chip.abort();
  TEST_ASSERT_EQUAL_UINT(0, f.sim.abortCount);
  f.sim.neverRespond = true;
  TEST_ASSERT_OK(f.chip.startFindTagIso14443A());
  f.chip.abort();
  TEST_ASSERT_EQUAL_UINT(1, f.sim.abortCount);
  TEST_ASSERT_EQUAL(CommandState::Idle, f.chip.state());
}

void test_only_identification_commands_can_be_sent() {
  Fixture f;
  // InDataExchange (0x40, usado para autenticar e ler MIFARE), InCommunicateThru (0x42),
  // TgInitAsTarget (0x8C, emulação de cartão), WriteRegister (0x08) e PowerDown (0x16).
  const uint8_t forbidden[] = {0x40, 0x42, 0x8C, 0x08, 0x16, 0x00, 0xFF};
  for (uint8_t code : forbidden) {
    const uint8_t payload[] = {code, 0x01, 0x60, 0x04};
    TEST_ASSERT_STATUS(Status::NotPermitted, f.chip.start(payload, sizeof(payload), 100));
  }
  TEST_ASSERT_EQUAL_UINT(0, f.bus.writeCount);  // nada chegou ao barramento
  TEST_ASSERT_EQUAL(CommandState::Idle, f.chip.state());
  TEST_ASSERT_STATUS(Status::InvalidArgument, f.chip.start(nullptr, 1, 100));
  const uint8_t empty[1] = {command::kGetFirmwareVersion};
  TEST_ASSERT_STATUS(Status::InvalidArgument, f.chip.start(empty, 0, 100));
}

void test_find_tag() {
  Fixture f;
  const uint8_t uid[7] = {0x04, 0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6};
  f.sim.setTag(uid, 7, 0x0044, 0x00);
  TEST_ASSERT_OK(f.chip.startFindTagIso14443A());
  TEST_ASSERT_EQUAL(CommandState::Done, f.finish());
  TagInfo tag{};
  TEST_ASSERT_OK(Pn532::parseTag(f.chip.response(), f.chip.responseLength(), &tag));
  TEST_ASSERT_EQUAL_UINT8(7, tag.uidLength);
  TEST_ASSERT_EQUAL_MEMORY(uid, tag.uid, 7);
  TEST_ASSERT_EQUAL_HEX16(0x0044, tag.atqa);
  TEST_ASSERT_EQUAL_HEX8(0x00, tag.sak);
}

void test_find_tag_with_finite_retries_reports_no_tag() {
  Fixture f;
  TEST_ASSERT_OK(f.chip.startSetPassiveActivationRetries(1));
  TEST_ASSERT_EQUAL(CommandState::Done, f.finish());
  TEST_ASSERT_EQUAL_UINT8(1, f.sim.passiveRetries);
  TEST_ASSERT_OK(f.chip.startFindTagIso14443A());
  TEST_ASSERT_EQUAL(CommandState::Done, f.finish());
  TagInfo tag{};
  TEST_ASSERT_STATUS(Status::NotFound,
                     Pn532::parseTag(f.chip.response(), f.chip.responseLength(), &tag));
}

void test_rf_field_control() {
  Fixture f;
  TEST_ASSERT_OK(f.chip.startRfField(true));
  TEST_ASSERT_EQUAL(CommandState::Done, f.finish());
  TEST_ASSERT_TRUE(f.sim.rfFieldOn);
  TEST_ASSERT_OK(f.chip.startRfField(false));
  TEST_ASSERT_EQUAL(CommandState::Done, f.finish());
  TEST_ASSERT_FALSE(f.sim.rfFieldOn);
}

void test_parse_tag_rejects_malformed_responses() {
  TagInfo tag{};
  const uint8_t empty[1] = {0};
  TEST_ASSERT_STATUS(Status::ProtocolError, Pn532::parseTag(empty, 0, &tag));
  const uint8_t twoTargets[] = {2, 1, 0, 4, 8, 4, 1, 2, 3, 4};
  TEST_ASSERT_STATUS(Status::ProtocolError, Pn532::parseTag(twoTargets, sizeof(twoTargets), &tag));
  const uint8_t badLength[] = {1, 1, 0, 4, 8, 5, 1, 2, 3, 4, 5};  // UID de 5 bytes não existe
  TEST_ASSERT_STATUS(Status::ProtocolError, Pn532::parseTag(badLength, sizeof(badLength), &tag));
  const uint8_t truncated[] = {1, 1, 0, 4, 8, 7, 1, 2, 3};  // diz 7, vieram 3
  TEST_ASSERT_STATUS(Status::ProtocolError, Pn532::parseTag(truncated, sizeof(truncated), &tag));
  const uint8_t header[] = {1, 1, 0};
  TEST_ASSERT_STATUS(Status::ProtocolError, Pn532::parseTag(header, sizeof(header), &tag));
  TEST_ASSERT_STATUS(Status::InvalidArgument, Pn532::parseTag(nullptr, 4, &tag));
}

void test_parse_firmware_rejects_wrong_size() {
  FirmwareVersion version{};
  const uint8_t data[5] = {0x32, 1, 6, 7, 0};
  TEST_ASSERT_STATUS(Status::ProtocolError, Pn532::parseFirmwareVersion(data, 3, &version));
  TEST_ASSERT_STATUS(Status::ProtocolError, Pn532::parseFirmwareVersion(data, 5, &version));
  TEST_ASSERT_OK(Pn532::parseFirmwareVersion(data, 4, &version));
}

void test_same_tag() {
  TagInfo a{};
  TagInfo b{};
  a.uidLength = b.uidLength = 4;
  memcpy(a.uid, "\x01\x02\x03\x04", 4);
  memcpy(b.uid, "\x01\x02\x03\x04", 4);
  TEST_ASSERT_TRUE(sameTag(a, b));
  b.uid[3] = 5;
  TEST_ASSERT_FALSE(sameTag(a, b));
  b.uid[3] = 4;
  b.uidLength = 7;
  TEST_ASSERT_FALSE(sameTag(a, b));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_probe);
  RUN_TEST(test_get_firmware_version);
  RUN_TEST(test_waits_while_the_chip_is_not_ready);
  RUN_TEST(test_second_command_while_busy_is_rejected);
  RUN_TEST(test_ack_timeout);
  RUN_TEST(test_response_timeout_aborts_the_command);
  RUN_TEST(test_nack_instead_of_ack_is_a_protocol_error);
  RUN_TEST(test_corrupted_response_is_detected);
  RUN_TEST(test_response_to_another_command_is_rejected);
  RUN_TEST(test_error_frame_is_a_protocol_error);
  RUN_TEST(test_bus_errors);
  RUN_TEST(test_abort_sends_ack_only_when_a_command_is_pending);
  RUN_TEST(test_only_identification_commands_can_be_sent);
  RUN_TEST(test_find_tag);
  RUN_TEST(test_find_tag_with_finite_retries_reports_no_tag);
  RUN_TEST(test_rf_field_control);
  RUN_TEST(test_parse_tag_rejects_malformed_responses);
  RUN_TEST(test_parse_firmware_rejects_wrong_size);
  RUN_TEST(test_same_tag);
  return UNITY_END();
}
