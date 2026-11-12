#include <cstring>

#include "dbmo/mock/mock_hal.h"
#include "dbmo/radio/cc1101_regs.h"
#include "dbmo/radio/radio_subsystem.h"
#include "dbmo/sim/cc1101_sim.h"
#include "dbmo_test.h"

using dbmo::ConfigStore;
using dbmo::Event;
using dbmo::EventBus;
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
  EventBus bus;
  ConfigStore config;
  RadioSubsystem radio{chip, bus, config, clock};
  Event events[64];
  int count = 0;

  Fixture() {
    TEST_ASSERT_OK(RadioSubsystem::registerConfig(config));
    TEST_ASSERT_OK(bus.subscribeAll(&Fixture::collect, this));
  }
  static void collect(const Event &event, void *context) {
    auto *self = static_cast<Fixture *>(context);
    if (self->count < 64) self->events[self->count++] = event;
  }
  void run(uint32_t durationMs) {
    for (uint32_t i = 0; i < durationMs; ++i) {
      clock.advanceMs(1);
      radio.tick(clock.nowMs());
      bus.dispatch();
    }
  }
  int countOf(dbmo::EventId id) const {
    int matches = 0;
    for (int i = 0; i < count; ++i)
      if (events[i].id == id) ++matches;
    return matches;
  }
  const Event *last(dbmo::EventId id) const {
    for (int i = count - 1; i >= 0; --i)
      if (events[i].id == id) return &events[i];
    return nullptr;
  }
  // O usuário habilita a transmissão e informa a faixa em que pode operar.
  void allowTransmit() {
    TEST_ASSERT_OK(config.setBool(config_key::kTxEnabled, true));
    TEST_ASSERT_OK(config.setInt(config_key::kTxMinKhz, 433050));
    TEST_ASSERT_OK(config.setInt(config_key::kTxMaxKhz, 434790));
    radio.applyConfig();
  }
};

const uint8_t kPayload[4] = {1, 2, 3, 4};

}  // namespace

void setUp() {}
void tearDown() {}

void test_init_identifies_and_leaves_the_chip_asleep() {
  Fixture f;
  TEST_ASSERT_OK(f.radio.init());
  TEST_ASSERT_EQUAL(RadioState::Idle, f.radio.state());
  TEST_ASSERT_EQUAL_HEX8(0x14, f.radio.chipId().version);
  TEST_ASSERT_EQUAL_HEX8(reg::kStateSleep, f.sim.state);  // baixo consumo por padrão
  TEST_ASSERT_EQUAL_HEX32(0x10B071, f.sim.frequencyWord());
}

void test_init_without_chip_is_hardware_absent() {
  Fixture f;
  f.sim.present = false;
  TEST_ASSERT_STATUS(Status::HardwareAbsent, f.radio.init());
  TEST_ASSERT_EQUAL(RadioState::Off, f.radio.state());
  TEST_ASSERT_STATUS(Status::NotInitialized, f.radio.startReceive());
  TEST_ASSERT_STATUS(Status::NotInitialized, f.radio.transmit(kPayload, 4));
  f.run(10000);  // sem atividade no barramento além da tentativa inicial
}

void test_receive_publishes_event_and_queues_the_packet() {
  Fixture f;
  TEST_ASSERT_OK(f.radio.init());
  TEST_ASSERT_OK(f.radio.startReceive());
  f.run(20);
  TEST_ASSERT_EQUAL(RadioState::Receiving, f.radio.state());

  TEST_ASSERT_TRUE(f.sim.injectPacket(kPayload, 4, 0xB0));
  f.run(20);
  const Event *event = f.last(radio_event::kPacketReceived);
  TEST_ASSERT_NOT_NULL(event);
  TEST_ASSERT_EQUAL_UINT32(4, event->a);
  TEST_ASSERT_EQUAL_INT16(-114, static_cast<int16_t>(event->b));

  RadioPacket packet;
  TEST_ASSERT_TRUE(f.radio.takePacket(&packet));
  TEST_ASSERT_EQUAL_MEMORY(kPayload, packet.data, 4);
  TEST_ASSERT_FALSE(f.radio.takePacket(&packet));
  // A recepção continua sozinha depois do pacote.
  TEST_ASSERT_EQUAL_HEX8(reg::kStateRx, f.sim.state);
}

void test_bad_packets_are_counted_and_reception_continues() {
  Fixture f;
  TEST_ASSERT_OK(f.radio.init());
  TEST_ASSERT_OK(f.radio.startReceive());
  f.run(10);
  f.sim.injectPacket(kPayload, 4, 0x80, false);
  f.run(10);
  f.sim.forceOverflow();
  f.run(10);
  const uint8_t malformed[4] = {0, 1, 2, 3};
  f.sim.injectRaw(malformed, 4);
  f.run(10);
  TEST_ASSERT_EQUAL_UINT32(1, f.radio.counters().crcErrors);
  TEST_ASSERT_EQUAL_UINT32(1, f.radio.counters().overflows);
  TEST_ASSERT_EQUAL_UINT32(1, f.radio.counters().malformed);
  TEST_ASSERT_EQUAL_INT(0, f.countOf(radio_event::kPacketReceived));
  TEST_ASSERT_EQUAL_HEX8(reg::kStateRx, f.sim.state);
}

void test_full_queue_drops_new_packets() {
  Fixture f;
  TEST_ASSERT_OK(f.radio.init());
  TEST_ASSERT_OK(f.radio.startReceive());
  for (size_t i = 0; i < RadioSubsystem::kQueueSize + 2; ++i) {
    f.run(10);
    f.sim.injectPacket(kPayload, 4);
  }
  f.run(10);
  TEST_ASSERT_EQUAL_UINT32(2, f.radio.counters().droppedPackets);
  RadioPacket packet;
  size_t taken = 0;
  while (f.radio.takePacket(&packet)) ++taken;
  TEST_ASSERT_EQUAL_UINT(RadioSubsystem::kQueueSize, taken);
}

void test_stop_returns_to_sleep() {
  Fixture f;
  TEST_ASSERT_OK(f.radio.init());
  TEST_ASSERT_OK(f.radio.startReceive());
  TEST_ASSERT_OK(f.radio.stop());
  TEST_ASSERT_EQUAL(RadioState::Idle, f.radio.state());
  TEST_ASSERT_EQUAL_HEX8(reg::kStateSleep, f.sim.state);
}

void test_transmit_is_denied_by_default() {
  Fixture f;
  TEST_ASSERT_OK(f.radio.init());
  TEST_ASSERT_STATUS(Status::NotPermitted, f.radio.transmit(kPayload, 4));
  f.run(5);
  TEST_ASSERT_EQUAL(TxDenial::Disabled, f.radio.lastDenial());
  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(TxDenial::Disabled),
                           f.last(radio_event::kTxDenied)->a);
  TEST_ASSERT_EQUAL_UINT(0, f.sim.transmitCount);
  TEST_ASSERT_EQUAL_UINT(0, f.sim.strobeCount[reg::kStx]);
}

void test_enabling_without_a_band_is_not_enough() {
  Fixture f;
  TEST_ASSERT_OK(f.config.setBool(config_key::kTxEnabled, true));
  TEST_ASSERT_OK(f.radio.init());
  TEST_ASSERT_STATUS(Status::NotPermitted, f.radio.transmit(kPayload, 4));
  TEST_ASSERT_EQUAL(TxDenial::NoBandConfigured, f.radio.lastDenial());
  TEST_ASSERT_EQUAL_UINT(0, f.sim.transmitCount);
}

void test_transmit_inside_the_configured_band() {
  Fixture f;
  TEST_ASSERT_OK(f.radio.init());
  f.allowTransmit();
  TEST_ASSERT_OK(f.radio.transmit(kPayload, 4));
  TEST_ASSERT_EQUAL(RadioState::Transmitting, f.radio.state());
  TEST_ASSERT_STATUS(Status::Busy, f.radio.transmit(kPayload, 4));
  f.run(50);
  TEST_ASSERT_EQUAL_INT(1, f.countOf(radio_event::kTxDone));
  TEST_ASSERT_EQUAL_UINT(1, f.sim.transmitCount);
  TEST_ASSERT_EQUAL_MEMORY(kPayload, f.sim.lastTx, 4);
  TEST_ASSERT_EQUAL(RadioState::Idle, f.radio.state());
  TEST_ASSERT_EQUAL_HEX8(reg::kStateSleep, f.sim.state);
}

void test_transmit_outside_the_configured_band_is_denied() {
  Fixture f;
  TEST_ASSERT_OK(f.radio.init());
  f.allowTransmit();
  RadioConfig config;
  config.frequencyHz = 315000000;  // faixa do chip, mas não a informada pelo usuário
  TEST_ASSERT_OK(f.radio.configure(config));
  TEST_ASSERT_STATUS(Status::NotPermitted, f.radio.transmit(kPayload, 4));
  TEST_ASSERT_EQUAL(TxDenial::OutOfBand, f.radio.lastDenial());
  TEST_ASSERT_EQUAL_UINT(0, f.sim.transmitCount);
}

void test_airtime_limit_stops_repeated_transmission() {
  Fixture f;
  TEST_ASSERT_OK(f.radio.init());
  f.allowTransmit();
  // 4 bytes a 1200 baud = 87 ms no ar; o limite padrão é 600 ms por minuto.
  int sent = 0;
  int denied = 0;
  for (int i = 0; i < 40; ++i) {
    const Status status = f.radio.transmit(kPayload, 4);
    if (dbmo::isOk(status)) {
      ++sent;
    } else if (status == Status::NotPermitted) {
      ++denied;
    }
    f.run(200);
  }
  TEST_ASSERT_EQUAL_INT(6, sent);  // 6 × 87 = 522 ms; o sétimo passaria de 600
  TEST_ASSERT_EQUAL_INT(34, denied);
  TEST_ASSERT_EQUAL(TxDenial::AirtimeExceeded, f.radio.lastDenial());
  TEST_ASSERT_EQUAL_UINT(6, f.sim.transmitCount);
}

void test_transmit_resumes_reception_afterwards() {
  Fixture f;
  TEST_ASSERT_OK(f.radio.init());
  f.allowTransmit();
  TEST_ASSERT_OK(f.radio.startReceive());
  f.run(10);
  TEST_ASSERT_OK(f.radio.transmit(kPayload, 4));
  f.run(50);
  TEST_ASSERT_EQUAL(RadioState::Receiving, f.radio.state());
  TEST_ASSERT_EQUAL_HEX8(reg::kStateRx, f.sim.state);
}

void test_stuck_transmission_is_cut_off() {
  Fixture f;
  TEST_ASSERT_OK(f.radio.init());
  f.allowTransmit();
  f.sim.txNeverEnds = true;
  TEST_ASSERT_OK(f.radio.transmit(kPayload, 4));
  f.run(87 + RadioSubsystem::kTxMarginMs + 20);
  TEST_ASSERT_EQUAL_INT(1, f.countOf(radio_event::kTxFailed));
  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(Status::Timeout),
                           f.last(radio_event::kTxFailed)->a);
  TEST_ASSERT_NOT_EQUAL(reg::kStateTx, f.sim.state);  // o chip saiu do ar
  TEST_ASSERT_EQUAL(RadioState::Idle, f.radio.state());
}

void test_chip_failure_is_reported_once_and_recovered() {
  Fixture f;
  TEST_ASSERT_OK(f.radio.init());
  TEST_ASSERT_OK(f.radio.startReceive());
  f.run(20);
  f.sim.present = false;
  f.run(200);
  TEST_ASSERT_EQUAL(RadioState::Fault, f.radio.state());
  TEST_ASSERT_EQUAL_INT(1, f.countOf(radio_event::kFault));
  TEST_ASSERT_STATUS(Status::HardwareAbsent, f.radio.transmit(kPayload, 4));

  f.run(12000);
  TEST_ASSERT_EQUAL_INT(1, f.countOf(radio_event::kFault));  // sem repetir a cada tentativa

  f.sim.present = true;
  f.run(6000);
  TEST_ASSERT_EQUAL(RadioState::Receiving, f.radio.state());  // retoma o que estava fazendo
  TEST_ASSERT_EQUAL_HEX8(reg::kStateRx, f.sim.state);
  TEST_ASSERT_EQUAL_HEX32(0x10B071, f.sim.frequencyWord());  // reconfigurado depois do reset
}

void test_configure_while_receiving_restarts_reception() {
  Fixture f;
  TEST_ASSERT_OK(f.radio.init());
  TEST_ASSERT_OK(f.radio.startReceive());
  RadioConfig config;
  config.frequencyHz = 868000000;
  config.modulation = Modulation::Gfsk;
  config.dataRateBaud = 38400;
  config.deviationHz = 20000;
  config.bandwidthHz = 100000;
  TEST_ASSERT_OK(f.radio.configure(config));
  TEST_ASSERT_EQUAL_HEX32(0x216276, f.sim.frequencyWord());
  TEST_ASSERT_EQUAL_HEX8(reg::kStateRx, f.sim.state);

  RadioConfig bad = config;
  bad.frequencyHz = 100000000;
  TEST_ASSERT_STATUS(Status::OutOfRange, f.radio.configure(bad));
  TEST_ASSERT_EQUAL_HEX32(0x216276, f.sim.frequencyWord());  // configuração anterior mantida
}

void test_shutdown_puts_the_chip_to_sleep() {
  Fixture f;
  TEST_ASSERT_OK(f.radio.init());
  TEST_ASSERT_OK(f.radio.startReceive());
  TEST_ASSERT_OK(f.radio.shutdown());
  TEST_ASSERT_EQUAL(RadioState::Off, f.radio.state());
  TEST_ASSERT_EQUAL_HEX8(reg::kStateSleep, f.sim.state);
}

void test_invalid_transmit_arguments() {
  Fixture f;
  TEST_ASSERT_OK(f.radio.init());
  f.allowTransmit();
  uint8_t big[kMaxPayload + 1] = {};
  TEST_ASSERT_STATUS(Status::InvalidArgument, f.radio.transmit(nullptr, 4));
  TEST_ASSERT_STATUS(Status::InvalidArgument, f.radio.transmit(big, 0));
  TEST_ASSERT_STATUS(Status::InvalidArgument, f.radio.transmit(big, sizeof(big)));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_init_identifies_and_leaves_the_chip_asleep);
  RUN_TEST(test_init_without_chip_is_hardware_absent);
  RUN_TEST(test_receive_publishes_event_and_queues_the_packet);
  RUN_TEST(test_bad_packets_are_counted_and_reception_continues);
  RUN_TEST(test_full_queue_drops_new_packets);
  RUN_TEST(test_stop_returns_to_sleep);
  RUN_TEST(test_transmit_is_denied_by_default);
  RUN_TEST(test_enabling_without_a_band_is_not_enough);
  RUN_TEST(test_transmit_inside_the_configured_band);
  RUN_TEST(test_transmit_outside_the_configured_band_is_denied);
  RUN_TEST(test_airtime_limit_stops_repeated_transmission);
  RUN_TEST(test_transmit_resumes_reception_afterwards);
  RUN_TEST(test_stuck_transmission_is_cut_off);
  RUN_TEST(test_chip_failure_is_reported_once_and_recovered);
  RUN_TEST(test_configure_while_receiving_restarts_reception);
  RUN_TEST(test_shutdown_puts_the_chip_to_sleep);
  RUN_TEST(test_invalid_transmit_arguments);
  return UNITY_END();
}
