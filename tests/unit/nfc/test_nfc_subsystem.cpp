#include <cstring>

#include "dbmo/config/pins.h"
#include "dbmo/core/crc.h"
#include "dbmo/core/log.h"
#include "dbmo/mock/mock_hal.h"
#include "dbmo/nfc/nfc_subsystem.h"
#include "dbmo/sim/pn532_sim.h"
#include "dbmo_test.h"

using dbmo::ConfigStore;
using dbmo::Event;
using dbmo::EventBus;
using dbmo::Status;
using dbmo::mock::MockClock;
using dbmo::mock::MockI2cBus;
using dbmo::sim::Pn532Sim;
using namespace dbmo::nfc;

namespace {

constexpr uint8_t kAddress = dbmo::config::i2c::kPn532;
constexpr uint8_t kFind = command::kInListPassiveTarget;
const uint8_t kUidA[4] = {0xDE, 0xAD, 0xBE, 0xEF};
const uint8_t kUidB[7] = {0x04, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66};

struct Fixture {
  MockI2cBus bus;
  MockClock clock;
  Pn532Sim sim;
  Pn532 chip{bus, clock, kAddress};
  EventBus events;
  ConfigStore config;
  NfcSubsystem nfc{chip, events, config, clock};
  Event log[64];
  int count = 0;

  Fixture() {
    bus.attach(kAddress, &sim);
    TEST_ASSERT_OK(NfcSubsystem::registerConfig(config));
    TEST_ASSERT_OK(events.subscribeAll(&Fixture::collect, this));
  }
  static void collect(const Event &event, void *context) {
    auto *self = static_cast<Fixture *>(context);
    if (self->count < 64) self->log[self->count++] = event;
  }
  void run(uint32_t durationMs) {
    for (uint32_t i = 0; i < durationMs; ++i) {
      clock.advanceMs(1);
      nfc.tick(clock.nowMs());
      events.dispatch();
    }
  }
  int countOf(dbmo::EventId id) const {
    int matches = 0;
    for (int i = 0; i < count; ++i)
      if (log[i].id == id) ++matches;
    return matches;
  }
  const Event *last(dbmo::EventId id) const {
    for (int i = count - 1; i >= 0; --i)
      if (log[i].id == id) return &log[i];
    return nullptr;
  }
};

}  // namespace

void setUp() {}
void tearDown() {}

void test_init_identifies_the_chip_and_leaves_the_field_off() {
  Fixture f;
  TEST_ASSERT_OK(f.nfc.init());
  TEST_ASSERT_EQUAL(NfcState::Idle, f.nfc.state());
  TEST_ASSERT_EQUAL_HEX8(0x32, f.nfc.firmware().ic);
  TEST_ASSERT_TRUE(f.sim.samConfigured);
  TEST_ASSERT_EQUAL_UINT8(1, f.sim.passiveRetries);
  TEST_ASSERT_FALSE(f.sim.rfFieldOn);
}

void test_scanning_is_off_by_default() {
  Fixture f;
  f.sim.setTag(kUidA, 4);
  TEST_ASSERT_OK(f.nfc.init());
  f.run(5000);
  TEST_ASSERT_EQUAL_UINT(0, f.sim.commandCount[kFind]);  // nenhuma busca, campo desligado
  TEST_ASSERT_FALSE(f.sim.rfFieldOn);
  TEST_ASSERT_EQUAL_INT(0, f.countOf(nfc_event::kTagDetected));
}

void test_init_without_chip_is_hardware_absent() {
  Fixture f;
  f.bus.detach(kAddress);
  TEST_ASSERT_STATUS(Status::HardwareAbsent, f.nfc.init());
  TEST_ASSERT_STATUS(Status::NotInitialized, f.nfc.startScan());
}

void test_init_with_another_chip_at_the_address() {
  Fixture f;
  f.sim.ic = 0x33;
  TEST_ASSERT_STATUS(Status::HardwareAbsent, f.nfc.init());
}

void test_init_with_silent_chip_is_bounded() {
  Fixture f;
  f.sim.readsBeforeAck = 1000000;  // responde ao endereço, mas nunca ao comando
  TEST_ASSERT_STATUS(Status::HardwareAbsent, f.nfc.init());
  TEST_ASSERT_TRUE(f.clock.nowMs() <= 2 * NfcSubsystem::kInitCommandTimeoutMs);
}

void test_scan_detects_a_tag_once() {
  Fixture f;
  TEST_ASSERT_OK(f.nfc.init());
  TEST_ASSERT_OK(f.nfc.startScan());
  f.run(600);
  TEST_ASSERT_EQUAL_INT(0, f.countOf(nfc_event::kTagDetected));
  TEST_ASSERT_TRUE(f.sim.commandCount[kFind] >= 2);

  f.sim.setTag(kUidA, 4);
  f.run(2000);
  TEST_ASSERT_EQUAL_INT(1, f.countOf(nfc_event::kTagDetected));  // uma vez, não a cada busca
  const Event *event = f.last(nfc_event::kTagDetected);
  TEST_ASSERT_EQUAL_UINT32(4, event->a);
  TEST_ASSERT_EQUAL_HEX32(dbmo::crc32(kUidA, 4), event->b);

  TagInfo tag{};
  TEST_ASSERT_TRUE(f.nfc.currentTag(&tag));
  TEST_ASSERT_EQUAL_MEMORY(kUidA, tag.uid, 4);
}

void test_tag_removal_needs_two_empty_scans() {
  Fixture f;
  TEST_ASSERT_OK(f.nfc.init());
  TEST_ASSERT_OK(f.nfc.startScan());
  f.sim.setTag(kUidA, 4);
  f.run(600);
  f.sim.tagPresent = false;
  f.run(300);  // uma busca vazia: ainda não saiu
  TEST_ASSERT_EQUAL_INT(0, f.countOf(nfc_event::kTagRemoved));
  f.run(600);
  TEST_ASSERT_EQUAL_INT(1, f.countOf(nfc_event::kTagRemoved));
  TEST_ASSERT_EQUAL_HEX32(dbmo::crc32(kUidA, 4), f.last(nfc_event::kTagRemoved)->b);
  TagInfo tag{};
  TEST_ASSERT_FALSE(f.nfc.currentTag(&tag));
}

void test_swapping_tags_reports_removal_and_detection() {
  Fixture f;
  TEST_ASSERT_OK(f.nfc.init());
  TEST_ASSERT_OK(f.nfc.startScan());
  f.sim.setTag(kUidA, 4);
  f.run(600);
  f.sim.setTag(kUidB, 7);
  f.run(600);
  TEST_ASSERT_EQUAL_INT(2, f.countOf(nfc_event::kTagDetected));
  TEST_ASSERT_EQUAL_INT(1, f.countOf(nfc_event::kTagRemoved));
  TEST_ASSERT_EQUAL_UINT32(7, f.last(nfc_event::kTagDetected)->a);
}

void test_stop_scan_turns_the_field_off_and_forgets_the_tag() {
  Fixture f;
  TEST_ASSERT_OK(f.nfc.init());
  TEST_ASSERT_OK(f.nfc.startScan());
  f.sim.setTag(kUidA, 4);
  f.run(600);
  TEST_ASSERT_TRUE(f.sim.rfFieldOn);
  TEST_ASSERT_OK(f.nfc.stopScan());
  f.run(100);
  TEST_ASSERT_EQUAL(NfcState::Idle, f.nfc.state());
  TEST_ASSERT_FALSE(f.sim.rfFieldOn);
  TEST_ASSERT_EQUAL_INT(1, f.countOf(nfc_event::kTagRemoved));
  const unsigned searches = f.sim.commandCount[kFind];
  f.run(3000);
  TEST_ASSERT_EQUAL_UINT(searches, f.sim.commandCount[kFind]);
}

void test_scan_enabled_by_config_starts_at_boot() {
  Fixture f;
  TEST_ASSERT_OK(f.config.setBool(config_key::kScanEnabled, true));
  TEST_ASSERT_OK(f.config.setInt(config_key::kScanPeriodMs, 1000));
  f.sim.setTag(kUidA, 4);
  TEST_ASSERT_OK(f.nfc.init());
  TEST_ASSERT_EQUAL(NfcState::Scanning, f.nfc.state());
  f.run(900);
  TEST_ASSERT_EQUAL_UINT(0, f.sim.commandCount[kFind]);  // respeita o período configurado
  f.run(300);
  TEST_ASSERT_EQUAL_INT(1, f.countOf(nfc_event::kTagDetected));
}

void test_chip_without_finite_retries_still_scans_by_timeout() {
  // Se o chip ignorar a configuração de tentativas, a busca não responde sem etiqueta:
  // o driver desiste no tempo limite e segue.
  Fixture f;
  TEST_ASSERT_OK(f.nfc.init());
  f.sim.passiveRetries = 0xFF;
  TEST_ASSERT_OK(f.nfc.startScan());
  f.run(3000);
  TEST_ASSERT_EQUAL(NfcState::Scanning, f.nfc.state());
  TEST_ASSERT_TRUE(f.sim.abortCount >= 2);
  f.sim.setTag(kUidA, 4);
  f.run(1000);
  TEST_ASSERT_EQUAL_INT(1, f.countOf(nfc_event::kTagDetected));
}

void test_fault_is_reported_once_and_recovered() {
  Fixture f;
  TEST_ASSERT_OK(f.nfc.init());
  TEST_ASSERT_OK(f.nfc.startScan());
  f.sim.setTag(kUidA, 4);
  f.run(600);
  f.bus.detach(kAddress);
  f.run(2000);
  TEST_ASSERT_EQUAL(NfcState::Fault, f.nfc.state());
  TEST_ASSERT_EQUAL_INT(1, f.countOf(nfc_event::kFault));
  TEST_ASSERT_EQUAL_INT(1, f.countOf(nfc_event::kTagRemoved));  // sem chip não há etiqueta
  f.run(12000);
  TEST_ASSERT_EQUAL_INT(1, f.countOf(nfc_event::kFault));

  f.bus.attach(kAddress, &f.sim);
  f.run(6500);
  TEST_ASSERT_EQUAL(NfcState::Scanning, f.nfc.state());
  TEST_ASSERT_EQUAL_INT(2, f.countOf(nfc_event::kTagDetected));
}

void test_one_corrupted_response_is_tolerated() {
  Fixture f;
  TEST_ASSERT_OK(f.nfc.init());
  TEST_ASSERT_OK(f.nfc.startScan());
  f.sim.corruptNextResponse = true;
  f.run(1500);
  TEST_ASSERT_EQUAL(NfcState::Scanning, f.nfc.state());
  TEST_ASSERT_EQUAL_INT(0, f.countOf(nfc_event::kFault));
}

void test_shutdown_turns_the_field_off() {
  Fixture f;
  TEST_ASSERT_OK(f.nfc.init());
  TEST_ASSERT_OK(f.nfc.startScan());
  f.run(600);
  TEST_ASSERT_TRUE(f.sim.rfFieldOn);
  TEST_ASSERT_OK(f.nfc.shutdown());
  TEST_ASSERT_FALSE(f.sim.rfFieldOn);
  TEST_ASSERT_EQUAL(NfcState::Off, f.nfc.state());
}

void test_uid_is_not_written_to_the_log() {
  Fixture f;
  dbmo::mock::RecordingLogSink sink;
  TEST_ASSERT_OK(dbmo::logger().addSink(&sink));
  TEST_ASSERT_OK(f.nfc.init());
  TEST_ASSERT_OK(f.nfc.startScan());
  f.sim.setTag(kUidA, 4);
  f.run(600);
  dbmo::logger().removeSink(&sink);
  TEST_ASSERT_TRUE(sink.contains("etiqueta detectada"));
  TEST_ASSERT_FALSE(sink.contains("DEAD"));
  TEST_ASSERT_FALSE(sink.contains("dead"));
  TEST_ASSERT_FALSE(sink.contains("DE:AD"));
}

void test_only_identification_commands_are_ever_sent() {
  // O módulo não tem código para autenticar, ler ou gravar memória, nem para emular
  // cartão. Depois de um ciclo completo de uso, só estes comandos chegaram ao chip.
  Fixture f;
  TEST_ASSERT_OK(f.nfc.init());
  TEST_ASSERT_OK(f.nfc.startScan());
  f.sim.setTag(kUidA, 4);
  f.run(2000);
  TEST_ASSERT_OK(f.nfc.stopScan());
  f.run(200);
  TEST_ASSERT_OK(f.nfc.shutdown());
  for (unsigned code = 0; code < 256; ++code) {
    const bool allowed = code == command::kGetFirmwareVersion ||
                         code == command::kSamConfiguration || code == command::kRfConfiguration ||
                         code == command::kInListPassiveTarget;
    if (!allowed) TEST_ASSERT_EQUAL_UINT(0, f.sim.commandCount[code]);
  }
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_init_identifies_the_chip_and_leaves_the_field_off);
  RUN_TEST(test_scanning_is_off_by_default);
  RUN_TEST(test_init_without_chip_is_hardware_absent);
  RUN_TEST(test_init_with_another_chip_at_the_address);
  RUN_TEST(test_init_with_silent_chip_is_bounded);
  RUN_TEST(test_scan_detects_a_tag_once);
  RUN_TEST(test_tag_removal_needs_two_empty_scans);
  RUN_TEST(test_swapping_tags_reports_removal_and_detection);
  RUN_TEST(test_stop_scan_turns_the_field_off_and_forgets_the_tag);
  RUN_TEST(test_scan_enabled_by_config_starts_at_boot);
  RUN_TEST(test_chip_without_finite_retries_still_scans_by_timeout);
  RUN_TEST(test_fault_is_reported_once_and_recovered);
  RUN_TEST(test_one_corrupted_response_is_tolerated);
  RUN_TEST(test_shutdown_turns_the_field_off);
  RUN_TEST(test_uid_is_not_written_to_the_log);
  RUN_TEST(test_only_identification_commands_are_ever_sent);
  return UNITY_END();
}
