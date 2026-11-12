#include "dbmo/infrared/infrared_subsystem.h"
#include "dbmo/infrared/ir_codec.h"
#include "dbmo/mock/mock_hal.h"
#include "dbmo_test.h"

using dbmo::ConfigStore;
using dbmo::Event;
using dbmo::EventBus;
using dbmo::Status;
using dbmo::mock::MockClock;
using namespace dbmo::infrared;

namespace {

class FakeReceiver final : public IIrReceiver {
 public:
  Status begin() override {
    ++beginCalls;
    return beginStatus;
  }
  dbmo::Result<bool> read(RawTiming *out) override {
    if (!dbmo::isOk(readStatus)) return readStatus;
    if (!pending) return false;
    *out = next;
    pending = false;
    return true;
  }
  void deliver(const RawTiming &timing) {
    next = timing;
    pending = true;
  }
  RawTiming next;
  bool pending = false;
  Status beginStatus = Status::Ok;
  Status readStatus = Status::Ok;
  int beginCalls = 0;
};

class FakeTransmitter final : public IIrTransmitter {
 public:
  Status begin() override { return Status::Ok; }
  Status send(const RawTiming &timing) override {
    if (!dbmo::isOk(sendStatus)) return sendStatus;
    last = timing;
    ++sendCount;
    isBusy = true;
    return Status::Ok;
  }
  bool busy() override { return isBusy; }
  RawTiming last;
  int sendCount = 0;
  bool isBusy = false;
  Status sendStatus = Status::Ok;
};

IrCode necCode(uint16_t address, uint16_t command) {
  IrCode code;
  code.protocol = IrProtocol::Nec;
  code.bits = 32;
  code.address = address;
  code.command = command;
  return code;
}

RawTiming encoded(const IrCode &code, uint8_t frames = 1) {
  RawTiming timing;
  TEST_ASSERT_OK(encode(code, &timing, frames));
  return timing;
}

struct Fixture {
  EventBus bus;
  ConfigStore config;
  MockClock clock;
  FakeReceiver receiver;
  FakeTransmitter transmitter;
  InfraredSubsystem ir{bus, config, clock};
  Event events[64];
  int count = 0;

  Fixture() {
    TEST_ASSERT_OK(InfraredSubsystem::registerConfig(config));
    TEST_ASSERT_OK(bus.subscribeAll(&Fixture::collect, this));
    ir.setReceiver(receiver);
    ir.setTransmitter(transmitter);
  }
  static void collect(const Event &event, void *context) {
    auto *self = static_cast<Fixture *>(context);
    if (self->count < 64) self->events[self->count++] = event;
  }
  void run(uint32_t durationMs) {
    for (uint32_t i = 0; i < durationMs; ++i) {
      clock.advanceMs(1);
      ir.tick(clock.nowMs());
      bus.dispatch();
    }
  }
  void receive(const RawTiming &timing) {
    receiver.deliver(timing);
    run(2);
  }
  // Termina o envio em andamento.
  void finishSend() {
    transmitter.isBusy = false;
    run(2);
  }
  void enableTx() {
    TEST_ASSERT_OK(config.setBool(config_key::kTxEnabled, true));
    ir.applyConfig();
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
};

}  // namespace

void setUp() {}
void tearDown() {}

void test_init_requires_some_hardware() {
  EventBus bus;
  ConfigStore config;
  MockClock clock;
  TEST_ASSERT_OK(InfraredSubsystem::registerConfig(config));
  InfraredSubsystem ir(bus, config, clock);
  TEST_ASSERT_STATUS(Status::HardwareAbsent, ir.init());

  Fixture f;
  TEST_ASSERT_OK(f.ir.init());
  f.receiver.beginStatus = Status::IoError;
  TEST_ASSERT_STATUS(Status::IoError, f.ir.init());
}

void test_received_nec_code_is_published() {
  Fixture f;
  TEST_ASSERT_OK(f.ir.init());
  f.receive(encoded(necCode(0x04, 0x08)));
  const Event *event = f.last(infrared_event::kCodeReceived);
  TEST_ASSERT_NOT_NULL(event);
  TEST_ASSERT_EQUAL_HEX32(static_cast<uint32_t>(IrProtocol::Nec) | (32u << 8), event->a);
  TEST_ASSERT_EQUAL_HEX32((0x04u << 16) | 0x08u, event->b);

  StoredCode last;
  TEST_ASSERT_TRUE(f.ir.lastReceived(&last));
  TEST_ASSERT_FALSE(last.isRaw);
  TEST_ASSERT_TRUE(last.code == necCode(0x04, 0x08));
}

void test_nec_repeat_codes_follow_the_last_frame() {
  Fixture f;
  TEST_ASSERT_OK(f.ir.init());
  RawTiming repeat;
  repeat.durationsUs[0] = 9000;
  repeat.durationsUs[1] = 2250;
  repeat.durationsUs[2] = 562;
  repeat.count = 3;

  f.receive(repeat);  // repetição sem quadro antes: ignorada
  TEST_ASSERT_EQUAL_INT(0, f.countOf(infrared_event::kRepeat));

  f.receive(encoded(necCode(0x04, 0x08)));
  for (int i = 0; i < 3; ++i) {
    f.run(100);
    f.receive(repeat);
  }
  TEST_ASSERT_EQUAL_INT(1, f.countOf(infrared_event::kCodeReceived));
  TEST_ASSERT_EQUAL_INT(3, f.countOf(infrared_event::kRepeat));
  TEST_ASSERT_EQUAL_HEX32((0x04u << 16) | 0x08u, f.last(infrared_event::kRepeat)->b);

  f.run(1000);  // tecla solta há muito tempo: a repetição já não se refere a nada
  f.receive(repeat);
  TEST_ASSERT_EQUAL_INT(3, f.countOf(infrared_event::kRepeat));
}

void test_sony_repeated_frames_count_as_one_press() {
  Fixture f;
  TEST_ASSERT_OK(f.ir.init());
  IrCode code;
  code.protocol = IrProtocol::Sony;
  code.bits = 12;
  code.address = 0x01;
  code.command = 0x15;
  for (int i = 0; i < 3; ++i) {
    f.receive(encoded(code));
    f.run(43);
  }
  TEST_ASSERT_EQUAL_INT(1, f.countOf(infrared_event::kCodeReceived));
  TEST_ASSERT_EQUAL_INT(2, f.countOf(infrared_event::kRepeat));
  f.run(500);
  f.receive(encoded(code));  // nova pressão
  TEST_ASSERT_EQUAL_INT(2, f.countOf(infrared_event::kCodeReceived));
}

void test_unknown_protocol_is_reported_as_raw() {
  Fixture f;
  TEST_ASSERT_OK(f.ir.init());
  RawTiming rc5;
  for (uint16_t i = 0; i < 26; ++i) rc5.durationsUs[i] = 889;
  rc5.count = 26;
  rc5.carrierHz = 36000;
  f.receive(rc5);
  TEST_ASSERT_EQUAL_INT(0, f.countOf(infrared_event::kCodeReceived));
  TEST_ASSERT_EQUAL_UINT32(26, f.last(infrared_event::kRawReceived)->a);
  StoredCode last;
  TEST_ASSERT_TRUE(f.ir.lastReceived(&last));
  TEST_ASSERT_TRUE(last.isRaw);
  TEST_ASSERT_EQUAL_UINT16(26, last.raw.count);
}

void test_corrupted_nec_frame_is_dropped() {
  Fixture f;
  TEST_ASSERT_OK(f.ir.init());
  RawTiming timing = encoded(necCode(0x20, 0x10));
  timing.durationsUs[3 + 2 * 16] = nec::kOneSpaceUs;
  f.receive(timing);
  TEST_ASSERT_EQUAL_INT(0, f.count);
  StoredCode last;
  TEST_ASSERT_FALSE(f.ir.lastReceived(&last));
}

void test_reception_can_be_disabled() {
  Fixture f;
  TEST_ASSERT_OK(f.config.setBool(config_key::kRxEnabled, false));
  TEST_ASSERT_OK(f.ir.init());
  f.receive(encoded(necCode(0x04, 0x08)));
  TEST_ASSERT_EQUAL_INT(0, f.count);
  TEST_ASSERT_STATUS(Status::Unsupported, f.ir.startLearning());
}

void test_learning_captures_the_next_signal() {
  Fixture f;
  TEST_ASSERT_OK(f.ir.init());
  StoredCode learned;
  TEST_ASSERT_FALSE(f.ir.learned(&learned));
  TEST_ASSERT_OK(f.ir.startLearning());
  TEST_ASSERT_TRUE(f.ir.learning());
  f.run(500);
  const RawTiming timing = encoded(necCode(0x04, 0x08));
  f.receive(timing);
  TEST_ASSERT_FALSE(f.ir.learning());
  TEST_ASSERT_EQUAL_UINT32(0, f.last(infrared_event::kLearned)->a);
  TEST_ASSERT_TRUE(f.ir.learned(&learned));
  TEST_ASSERT_TRUE(learned.code == necCode(0x04, 0x08));
  TEST_ASSERT_EQUAL_UINT16(timing.count, learned.raw.count);  // o bruto vem junto

  // Só o primeiro sinal é aprendido.
  f.run(500);
  f.receive(encoded(necCode(0x04, 0x09)));
  TEST_ASSERT_EQUAL_INT(1, f.countOf(infrared_event::kLearned));
  TEST_ASSERT_TRUE(f.ir.learned(&learned));
  TEST_ASSERT_EQUAL_HEX16(0x08, learned.code.command);
}

void test_learning_unknown_protocol_keeps_raw() {
  Fixture f;
  TEST_ASSERT_OK(f.ir.init());
  TEST_ASSERT_OK(f.ir.startLearning());
  RawTiming rc5;
  for (uint16_t i = 0; i < 26; ++i) rc5.durationsUs[i] = 889;
  rc5.count = 26;
  f.receive(rc5);
  TEST_ASSERT_EQUAL_UINT32(1, f.last(infrared_event::kLearned)->a);
  StoredCode learned;
  TEST_ASSERT_TRUE(f.ir.learned(&learned));
  TEST_ASSERT_TRUE(learned.isRaw);
}

void test_learning_times_out_and_can_be_cancelled() {
  Fixture f;
  TEST_ASSERT_OK(f.ir.init());
  TEST_ASSERT_OK(f.ir.startLearning(2000));
  f.run(1900);
  TEST_ASSERT_EQUAL_INT(0, f.countOf(infrared_event::kLearnTimeout));
  f.run(200);
  TEST_ASSERT_EQUAL_INT(1, f.countOf(infrared_event::kLearnTimeout));
  TEST_ASSERT_FALSE(f.ir.learning());

  TEST_ASSERT_OK(f.ir.startLearning(2000));
  f.ir.cancelLearning();
  f.run(3000);
  TEST_ASSERT_EQUAL_INT(1, f.countOf(infrared_event::kLearnTimeout));
}

void test_send_is_denied_by_default() {
  Fixture f;
  TEST_ASSERT_OK(f.ir.init());
  TEST_ASSERT_STATUS(Status::NotPermitted, f.ir.send(necCode(0x04, 0x08)));
  f.run(2);
  TEST_ASSERT_EQUAL(IrTxDenial::Disabled, f.ir.lastDenial());
  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(IrTxDenial::Disabled),
                           f.last(infrared_event::kTxDenied)->a);
  TEST_ASSERT_EQUAL_INT(0, f.transmitter.sendCount);
}

void test_send_when_enabled() {
  Fixture f;
  TEST_ASSERT_OK(f.ir.init());
  f.enableTx();
  TEST_ASSERT_OK(f.ir.send(necCode(0x04, 0x08)));
  TEST_ASSERT_TRUE(f.ir.transmitting());
  TEST_ASSERT_EQUAL_INT(1, f.transmitter.sendCount);
  TEST_ASSERT_EQUAL_UINT32(38000, f.transmitter.last.carrierHz);
  IrCode sent;
  TEST_ASSERT_OK(decode(f.transmitter.last, &sent));
  TEST_ASSERT_TRUE(sent == necCode(0x04, 0x08));

  TEST_ASSERT_STATUS(Status::NotPermitted, f.ir.send(necCode(0x04, 0x09)));  // ainda enviando
  TEST_ASSERT_EQUAL(IrTxDenial::Busy, f.ir.lastDenial());
  f.finishSend();
  TEST_ASSERT_EQUAL_INT(1, f.countOf(infrared_event::kTxDone));
  TEST_ASSERT_FALSE(f.ir.transmitting());
}

void test_sony_is_sent_three_times() {
  Fixture f;
  TEST_ASSERT_OK(f.ir.init());
  f.enableTx();
  IrCode code;
  code.protocol = IrProtocol::Sony;
  code.bits = 12;
  code.address = 0x01;
  code.command = 0x15;
  TEST_ASSERT_OK(f.ir.send(code));
  TEST_ASSERT_EQUAL_UINT16(77, f.transmitter.last.count);
  TEST_ASSERT_EQUAL_UINT32(40000, f.transmitter.last.carrierHz);
}

void test_own_transmission_is_not_received() {
  Fixture f;
  TEST_ASSERT_OK(f.ir.init());
  f.enableTx();
  TEST_ASSERT_OK(f.ir.send(necCode(0x04, 0x08)));
  f.receive(f.transmitter.last);  // reflexo do próprio emissor
  TEST_ASSERT_EQUAL_INT(0, f.countOf(infrared_event::kCodeReceived));
  f.finishSend();
  f.receive(encoded(necCode(0x04, 0x08)));
  TEST_ASSERT_EQUAL_INT(1, f.countOf(infrared_event::kCodeReceived));
}

void test_invalid_signals_are_denied() {
  Fixture f;
  TEST_ASSERT_OK(f.ir.init());
  f.enableTx();

  RawTiming empty;
  TEST_ASSERT_STATUS(Status::NotPermitted, f.ir.sendRaw(empty));
  TEST_ASSERT_EQUAL(IrTxDenial::InvalidSignal, f.ir.lastDenial());

  RawTiming timing = encoded(necCode(1, 2));
  timing.carrierHz = 10000;
  TEST_ASSERT_STATUS(Status::NotPermitted, f.ir.sendRaw(timing));
  timing.carrierHz = 455000;
  TEST_ASSERT_STATUS(Status::NotPermitted, f.ir.sendRaw(timing));
  timing.carrierHz = 38000;
  timing.durationsUs[5] = 0;
  TEST_ASSERT_STATUS(Status::NotPermitted, f.ir.sendRaw(timing));
  TEST_ASSERT_EQUAL(IrTxDenial::InvalidSignal, f.ir.lastDenial());

  // Um único envio longo demais (seria "portadora ligada" disfarçada de código).
  RawTiming flood;
  flood.count = RawTiming::kMaxEntries;
  for (uint16_t i = 0; i < flood.count; ++i) flood.durationsUs[i] = 60000;
  TEST_ASSERT_STATUS(Status::NotPermitted, f.ir.sendRaw(flood));
  TEST_ASSERT_EQUAL(IrTxDenial::TooLong, f.ir.lastDenial());

  IrCode unknown;
  TEST_ASSERT_STATUS(Status::InvalidArgument, f.ir.send(unknown));
  TEST_ASSERT_EQUAL_INT(0, f.transmitter.sendCount);
}

void test_airtime_budget_prevents_flooding() {
  Fixture f;
  TEST_ASSERT_OK(f.ir.init());
  f.enableTx();
  // Um quadro NEC dura 68 ms; o orçamento padrão é 6000 ms por minuto → 88 quadros.
  int sent = 0;
  for (int i = 0; i < 300; ++i) {
    if (dbmo::isOk(f.ir.send(necCode(0x04, 0x08)))) ++sent;
    f.finishSend();
    f.run(100);
  }
  TEST_ASSERT_EQUAL_INT(88, sent);
  TEST_ASSERT_EQUAL(IrTxDenial::BudgetExceeded, f.ir.lastDenial());
  TEST_ASSERT_TRUE(f.ir.airtimeUsedMs() <= 6000);

  f.run(60000);  // minuto seguinte: o orçamento volta
  TEST_ASSERT_OK(f.ir.send(necCode(0x04, 0x08)));
}

void test_budget_cannot_be_doubled_across_a_minute_boundary() {
  Fixture f;
  TEST_ASSERT_OK(f.ir.init());
  f.enableTx();
  f.run(54000);
  int sent = 0;
  // Do segundo 54 ao 66, tentando sem parar: uma janela fixa aceitaria o dobro.
  for (int i = 0; i < 600; ++i) {
    if (dbmo::isOk(f.ir.send(necCode(0x04, 0x08)))) ++sent;
    f.finishSend();
    f.run(18);
  }
  TEST_ASSERT_EQUAL_INT(88, sent);  // 88 × 68 ms = 6 s: o orçamento de um minuto, não dois
}

void test_send_without_transmitter() {
  EventBus bus;
  ConfigStore config;
  MockClock clock;
  FakeReceiver receiver;
  TEST_ASSERT_OK(InfraredSubsystem::registerConfig(config));
  TEST_ASSERT_OK(config.setBool(config_key::kTxEnabled, true));
  InfraredSubsystem ir(bus, config, clock);
  ir.setReceiver(receiver);
  TEST_ASSERT_OK(ir.init());
  TEST_ASSERT_STATUS(Status::NotPermitted, ir.send(necCode(1, 2)));
  TEST_ASSERT_EQUAL(IrTxDenial::NoTransmitter, ir.lastDenial());
}

void test_transmitter_failure_is_reported_and_not_charged() {
  Fixture f;
  TEST_ASSERT_OK(f.ir.init());
  f.enableTx();
  f.transmitter.sendStatus = Status::IoError;
  TEST_ASSERT_STATUS(Status::IoError, f.ir.send(necCode(1, 2)));
  f.run(2);
  TEST_ASSERT_EQUAL_INT(1, f.countOf(infrared_event::kFault));
  TEST_ASSERT_FALSE(f.ir.transmitting());
  TEST_ASSERT_EQUAL_UINT32(0, f.ir.airtimeUsedMs());
}

void test_receiver_failure_is_reported_once() {
  Fixture f;
  TEST_ASSERT_OK(f.ir.init());
  f.receiver.readStatus = Status::IoError;
  f.run(500);
  TEST_ASSERT_EQUAL_INT(1, f.countOf(infrared_event::kFault));
  f.receiver.readStatus = Status::Ok;
  f.receive(encoded(necCode(0x04, 0x08)));
  TEST_ASSERT_EQUAL_INT(1, f.countOf(infrared_event::kCodeReceived));
}

void test_stored_code_replay() {
  Fixture f;
  TEST_ASSERT_OK(f.ir.init());
  f.enableTx();
  // Aprende um código de protocolo desconhecido e o reenvia exatamente como chegou.
  TEST_ASSERT_OK(f.ir.startLearning());
  RawTiming rc5;
  for (uint16_t i = 0; i < 26; ++i) rc5.durationsUs[i] = 889;
  rc5.count = 26;
  rc5.carrierHz = 36000;
  f.receive(rc5);
  StoredCode learned;
  TEST_ASSERT_TRUE(f.ir.learned(&learned));
  TEST_ASSERT_OK(f.ir.send(learned));
  TEST_ASSERT_EQUAL_UINT16(26, f.transmitter.last.count);
  TEST_ASSERT_EQUAL_UINT32(36000, f.transmitter.last.carrierHz);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_init_requires_some_hardware);
  RUN_TEST(test_received_nec_code_is_published);
  RUN_TEST(test_nec_repeat_codes_follow_the_last_frame);
  RUN_TEST(test_sony_repeated_frames_count_as_one_press);
  RUN_TEST(test_unknown_protocol_is_reported_as_raw);
  RUN_TEST(test_corrupted_nec_frame_is_dropped);
  RUN_TEST(test_reception_can_be_disabled);
  RUN_TEST(test_learning_captures_the_next_signal);
  RUN_TEST(test_learning_unknown_protocol_keeps_raw);
  RUN_TEST(test_learning_times_out_and_can_be_cancelled);
  RUN_TEST(test_send_is_denied_by_default);
  RUN_TEST(test_send_when_enabled);
  RUN_TEST(test_sony_is_sent_three_times);
  RUN_TEST(test_own_transmission_is_not_received);
  RUN_TEST(test_invalid_signals_are_denied);
  RUN_TEST(test_airtime_budget_prevents_flooding);
  RUN_TEST(test_budget_cannot_be_doubled_across_a_minute_boundary);
  RUN_TEST(test_send_without_transmitter);
  RUN_TEST(test_transmitter_failure_is_reported_and_not_charged);
  RUN_TEST(test_receiver_failure_is_reported_once);
  RUN_TEST(test_stored_code_replay);
  return UNITY_END();
}
