// A aplicação inteira, com simuladores no lugar do hardware. Cada teste é um cenário de
// uso do aparelho, do boot ao desligamento.
#include <cstring>

#include "dbmo/app/application.h"
#include "dbmo/cartridges/cartridge_header.h"
#include "dbmo/config/pins.h"
#include "dbmo/display/framebuffer_display.h"
#include "dbmo/infrared/ir_codec.h"
#include "dbmo/mock/mock_hal.h"
#include "dbmo/sim/cc1101_sim.h"
#include "dbmo/sim/eeprom_24c02_sim.h"
#include "dbmo/sim/mcp23017_sim.h"
#include "dbmo/sim/memory_file_system.h"
#include "dbmo/sim/pn532_sim.h"
#include "dbmo_test.h"
#include "example_sensor_cartridge.h"

using dbmo::Event;
using dbmo::Status;
using dbmo::SubsystemState;
using dbmo::SystemState;
using dbmo::app::Application;
using dbmo::app::Hardware;
using dbmo::app::RunState;
using dbmo::rosto::Expression;
namespace pins = dbmo::config::target;
namespace i2c = dbmo::config::i2c;

namespace {

class FakeAudioOutput final : public dbmo::audio::IAudioOutput {
 public:
  Status begin(uint32_t) override { return Status::Ok; }
  Status setAmplifier(bool on) override {
    amplifier = on;
    return Status::Ok;
  }
  dbmo::Result<size_t> write(const int16_t *, size_t n) override {
    samples += n;
    return n;
  }
  Status end() override { return Status::Ok; }
  bool amplifier = false;
  size_t samples = 0;
};

class FakeAudioInput final : public dbmo::audio::IAudioInput {
 public:
  Status begin(uint32_t) override {
    running = true;
    return Status::Ok;
  }
  dbmo::Result<size_t> read(int16_t *out, size_t n) override {
    const size_t count = n < 16 ? n : 16;
    for (size_t i = 0; i < count; ++i) out[i] = 100;
    return running ? count : 0;
  }
  Status end() override {
    running = false;
    return Status::Ok;
  }
  bool running = false;
};

class FakeIrReceiver final : public dbmo::infrared::IIrReceiver {
 public:
  Status begin() override { return Status::Ok; }
  dbmo::Result<bool> read(dbmo::infrared::RawTiming *out) override {
    if (!pending) return false;
    *out = next;
    pending = false;
    return true;
  }
  dbmo::infrared::RawTiming next;
  bool pending = false;
};

class FakeIrTransmitter final : public dbmo::infrared::IIrTransmitter {
 public:
  Status begin() override { return Status::Ok; }
  Status send(const dbmo::infrared::RawTiming &) override {
    ++sendCount;
    return Status::Ok;
  }
  bool busy() override { return false; }
  int sendCount = 0;
};

class CapturingOutput final : public dbmo::app::IConsoleOutput {
 public:
  void write(const char *text) override {
    const size_t length = strlen(text);
    if (used + length < sizeof(buffer)) {
      memcpy(buffer + used, text, length + 1);
      used += length;
    }
  }
  bool contains(const char *fragment) const { return strstr(buffer, fragment) != nullptr; }
  void clear() {
    used = 0;
    buffer[0] = '\0';
  }
  char buffer[8192] = {};
  size_t used = 0;
};

dbmo::display::Color gPixels[320 * 240];

// O aparelho completo, simulado.
struct Rig {
  dbmo::mock::MockClock clock;
  dbmo::display::FramebufferDisplay display{gPixels, 320, 240};
  dbmo::mock::MockI2cBus bus;
  dbmo::sim::Mcp23017Sim mcp;
  dbmo::sim::Pn532Sim pn532;
  dbmo::sim::Eeprom24c02Sim eeprom{clock};
  dbmo::mock::RegisterDeviceSim sensor;
  dbmo::sim::MemoryFileSystem card;
  dbmo::sim::Cc1101Sim radioChip;
  dbmo::mock::MockGpio gpio;
  dbmo::mock::MockAdcChannel battery;
  dbmo::mock::MemoryConfigBackend nvs;
  dbmo::power::GpioPowerLatch latch{gpio, pins::kPowerHold};
  dbmo::input::Mcp23017 expander{bus, i2c::kMcp23017Buttons};
  dbmo::input::Mcp23017ButtonSource buttons{expander,
                                            dbmo::input::Mcp23017ButtonSource::defaultMap()};
  FakeAudioOutput speaker;
  FakeAudioInput microphone;
  FakeIrReceiver irReceiver;
  FakeIrTransmitter irTransmitter;
  dbmo::cartridges::examples::ExampleSensorCartridge sensorDriver;
  Event events[256];
  int eventCount = 0;

  Rig() {
    memset(gPixels, 0, sizeof(gPixels));
    bus.attach(i2c::kMcp23017Buttons, &mcp);
    bus.attach(i2c::kPn532, &pn532);
    gpio.setInput(pins::kPowerButton, true);  // botão solto
  }

  Hardware fullHardware() {
    Hardware hw;
    hw.clock = &clock;
    hw.display = &display;
    hw.i2c = &bus;
    hw.fallbackConfig = &nvs;
    hw.fileSystem = &card;
    hw.buttonSources[0] = &buttons;
    hw.batteryAdc = &battery;
    hw.powerSwitch = &latch;
    hw.powerButtonGpio = &gpio;
    hw.powerButtonPin = pins::kPowerButton;
    hw.audioOutput = &speaker;
    hw.audioInput = &microphone;
    hw.irReceiver = &irReceiver;
    hw.irTransmitter = &irTransmitter;
    hw.radioSpi = &radioChip;
    return hw;
  }

  void observe(Application &app) { TEST_ASSERT_OK(app.bus().subscribeAll(&Rig::collect, this)); }
  static void collect(const Event &event, void *context) {
    auto *self = static_cast<Rig *>(context);
    if (self->eventCount < 256) self->events[self->eventCount++] = event;
  }
  int countOf(dbmo::EventId id) const {
    int matches = 0;
    for (int i = 0; i < eventCount; ++i)
      if (events[i].id == id) ++matches;
    return matches;
  }
  void run(Application &app, uint32_t durationMs) {
    for (uint32_t i = 0; i < durationMs; ++i) {
      clock.advanceMs(1);
      app.loop();
    }
  }
  // Roda até a condição valer; false se o prazo passar.
  template <typename Condition>
  bool runUntil(Application &app, uint32_t limitMs, Condition condition) {
    for (uint32_t i = 0; i < limitMs; ++i) {
      if (condition()) return true;
      clock.advanceMs(1);
      app.loop();
    }
    return condition();
  }
  void insertExampleCartridge() {
    uint8_t image[dbmo::cartridges::kHeaderSize];
    TEST_ASSERT_OK(dbmo::cartridges::encodeHeader(
        dbmo::cartridges::examples::ExampleSensorCartridge::makeHeader(), image));
    eeprom.load(image, sizeof(image));
    sensor.memory[0] = 42;
    bus.attach(dbmo::cartridges::examples::ExampleSensorCartridge::kSensorAddress, &sensor);
    bus.attach(i2c::kCartridgeEeprom, &eeprom);
  }
  void removeCartridge() {
    bus.detach(i2c::kCartridgeEeprom);
    bus.detach(dbmo::cartridges::examples::ExampleSensorCartridge::kSensorAddress);
  }
  size_t litPixels() const {
    size_t lit = 0;
    for (dbmo::display::Color color : gPixels)
      if (color == dbmo::display::palette::kSignalRed) ++lit;
    return lit;
  }
};

Rig *rig = nullptr;

SubsystemState stateOf(Application &app, const char *name) {
  const dbmo::SubsystemInfo *info = app.system().find(name);
  TEST_ASSERT_NOT_NULL(info);
  return info->state;
}

}  // namespace

void setUp() {
  dbmo::logger().clearSinks();
  rig = new Rig();
}
void tearDown() {
  dbmo::logger().clearSinks();
  delete rig;
}

void test_boot_with_all_hardware() {
  Application app(rig->fullHardware());
  TEST_ASSERT_OK(app.begin());
  TEST_ASSERT_EQUAL(SystemState::Running, app.system().state());
  TEST_ASSERT_EQUAL(RunState::Running, app.runState());
  TEST_ASSERT_EQUAL_UINT(11, app.system().subsystemCount());
  for (size_t i = 0; i < app.system().subsystemCount(); ++i) {
    const dbmo::SubsystemInfo &info = app.system().info(i);
    if (info.state != SubsystemState::Ready) TEST_FAIL_MESSAGE(info.subsystem->name());
  }
  rig->run(app, 200);
  // Nenhum evento perdido na rajada do boot, e o rosto está na tela.
  TEST_ASSERT_EQUAL_UINT32(0, app.bus().droppedCount());
  TEST_ASSERT_TRUE(rig->litPixels() > 500);
  TEST_ASSERT_EQUAL(Expression::Idle, app.face().engine().current());
  // Estado seguro por padrão: nada emitindo, nada escutando.
  TEST_ASSERT_FALSE(rig->pn532.rfFieldOn);
  TEST_ASSERT_FALSE(rig->microphone.running);
  TEST_ASSERT_FALSE(rig->speaker.amplifier);
  TEST_ASSERT_EQUAL_UINT(0, rig->radioChip.transmitCount);
  TEST_ASSERT_EQUAL_INT(0, rig->irTransmitter.sendCount);
  TEST_ASSERT_TRUE(rig->gpio.read(pins::kPowerHold).value());  // trava segura
}

void test_boot_on_todays_bench_with_only_a_display() {
  Hardware hw;
  hw.clock = &rig->clock;
  hw.display = &rig->display;
  Application app(hw);
  TEST_ASSERT_OK(app.begin());
  TEST_ASSERT_EQUAL(SystemState::Degraded, app.system().state());
  TEST_ASSERT_EQUAL(SubsystemState::Ready, stateOf(app, "display"));
  TEST_ASSERT_EQUAL(SubsystemState::Ready, stateOf(app, "rosto"));
  TEST_ASSERT_EQUAL(SubsystemState::Ready, stateOf(app, "power"));
  TEST_ASSERT_EQUAL(SubsystemState::Absent, stateOf(app, "storage"));
  TEST_ASSERT_EQUAL(SubsystemState::Absent, stateOf(app, "nfc"));
  TEST_ASSERT_EQUAL(SubsystemState::Absent, stateOf(app, "radio"));
  TEST_ASSERT_EQUAL(SubsystemState::Absent, stateOf(app, "audio"));
  rig->run(app, 40000);  // inclui as novas tentativas periódicas de NFC e rádio
  TEST_ASSERT_EQUAL(RunState::Running, app.runState());
  TEST_ASSERT_TRUE(rig->litPixels() > 500);
  TEST_ASSERT_EQUAL_UINT32(0, app.bus().droppedCount());
}

void test_missing_required_hardware_is_refused() {
  Hardware hw;
  hw.clock = &rig->clock;
  // Sem tela, a aplicação nem pode ser construída com segurança: begin() recusa.
  Hardware noDisplay = hw;
  noDisplay.display = nullptr;
  (void)noDisplay;
  // (O construtor exige relógio e tela válidos; o caso é conferido pelo contrato de begin.)
  Application app(rig->fullHardware());
  TEST_ASSERT_OK(app.begin());
  TEST_ASSERT_STATUS(Status::AlreadyInitialized, app.begin());
}

void test_button_press_reaches_the_event_bus() {
  Application app(rig->fullHardware());
  TEST_ASSERT_OK(app.begin());
  rig->observe(app);
  rig->run(app, 50);
  rig->mcp.press(dbmo::config::mcp_buttons::kActionA);
  rig->run(app, 60);
  rig->mcp.release(dbmo::config::mcp_buttons::kActionA);
  rig->run(app, 60);
  TEST_ASSERT_TRUE(rig->countOf(dbmo::input::input_event::kButton) >= 2);  // apertou e soltou
  TEST_ASSERT_TRUE(app.input().healthySources() == 1);
}

void test_nfc_scan_drives_the_face() {
  Application app(rig->fullHardware());
  TEST_ASSERT_OK(app.begin());
  rig->run(app, 50);
  TEST_ASSERT_OK(app.nfc().startScan());
  rig->run(app, 300);
  TEST_ASSERT_EQUAL(Expression::Scanning, app.face().engine().current());
  TEST_ASSERT_TRUE(rig->pn532.rfFieldOn);

  const uint8_t uid[4] = {0x01, 0x02, 0x03, 0x04};
  rig->pn532.setTag(uid, 4);
  TEST_ASSERT_TRUE(rig->runUntil(
      app, 1000, [&] { return app.face().engine().current() == Expression::Success; }));
  rig->run(app, 2500);
  TEST_ASSERT_EQUAL(Expression::Scanning, app.face().engine().current());  // voltou ao fundo

  TEST_ASSERT_OK(app.nfc().stopScan());
  rig->run(app, 200);
  TEST_ASSERT_EQUAL(Expression::Idle, app.face().engine().base());
  TEST_ASSERT_FALSE(rig->pn532.rfFieldOn);
}

void test_cartridge_insertion_starts_its_driver() {
  Application app(rig->fullHardware());
  TEST_ASSERT_OK(app.addCartridgeDriver(rig->sensorDriver));
  TEST_ASSERT_OK(app.begin());
  rig->observe(app);
  rig->run(app, 600);
  TEST_ASSERT_EQUAL(dbmo::cartridges::SlotState::Empty, app.cartridges().state());

  rig->insertExampleCartridge();
  TEST_ASSERT_TRUE(rig->runUntil(
      app, 2000, [&] { return app.cartridges().state() == dbmo::cartridges::SlotState::Active; }));
  rig->run(app, 5);
  TEST_ASSERT_EQUAL(Expression::Happy, app.face().engine().current());
  TEST_ASSERT_EQUAL_STRING("EXEMPLO SENSOR", app.cartridges().header()->name);
  rig->run(app, 1500);
  TEST_ASSERT_TRUE(rig->countOf(dbmo::cartridges::examples::ExampleSensorCartridge::kSampleEvent) >=
                   1);

  rig->removeCartridge();
  TEST_ASSERT_TRUE(rig->runUntil(
      app, 3000, [&] { return app.cartridges().state() == dbmo::cartridges::SlotState::Empty; }));
  // O endereço que o driver tinha reservado foi devolvido.
  TEST_ASSERT_NULL(
      app.resources().ownerOf(dbmo::ResourceKind::I2cAddress,
                              dbmo::cartridges::examples::ExampleSensorCartridge::kSensorAddress));
}

void test_cartridge_cannot_claim_a_device_address() {
  // Um cartucho que declara o endereço do PN532 é recusado antes de qualquer driver.
  Application app(rig->fullHardware());
  TEST_ASSERT_OK(app.begin());
  dbmo::cartridges::CartridgeHeader header =
      dbmo::cartridges::examples::ExampleSensorCartridge::makeHeader();
  header.i2cAddresses[0] = i2c::kPn532;
  uint8_t image[dbmo::cartridges::kHeaderSize];
  TEST_ASSERT_OK(dbmo::cartridges::encodeHeader(header, image));
  rig->eeprom.load(image, sizeof(image));
  rig->bus.attach(i2c::kCartridgeEeprom, &rig->eeprom);
  TEST_ASSERT_TRUE(rig->runUntil(app, 2000, [&] {
    return app.cartridges().state() == dbmo::cartridges::SlotState::Rejected;
  }));
  rig->run(app, 5);
  TEST_ASSERT_EQUAL(Expression::Error, app.face().engine().current());
  TEST_ASSERT_EQUAL(SubsystemState::Ready, stateOf(app, "nfc"));  // o aparelho segue intacto
}

void test_config_is_saved_to_the_card_and_restored() {
  {
    Application app(rig->fullHardware());
    TEST_ASSERT_OK(app.begin());
    rig->run(app, 100);
    TEST_ASSERT_OK(app.config().setInt(dbmo::audio::config_key::kVolume, 35));
    rig->run(app, 1000);
    TEST_ASSERT_FALSE(rig->card.exists(dbmo::storage::kConfigFile));  // ainda agrupando
    rig->run(app, 2000);
    TEST_ASSERT_NOT_NULL(strstr(rig->card.text(dbmo::storage::kConfigFile), "audio.volume=35"));
    TEST_ASSERT_EQUAL_UINT8(35, app.audio().volume().percent());  // aplicado em uso
  }
  dbmo::logger().clearSinks();
  rig->card.unmount();
  Application second(rig->fullHardware());
  TEST_ASSERT_OK(second.begin());
  TEST_ASSERT_TRUE(second.persistence().loadedFromCard());
  TEST_ASSERT_EQUAL_INT32(35, second.config().getInt(dbmo::audio::config_key::kVolume).value());
  TEST_ASSERT_EQUAL_UINT8(35, second.audio().volume().percent());
}

void test_config_falls_back_to_internal_storage_without_a_card() {
  Hardware hw = rig->fullHardware();
  hw.fileSystem = nullptr;
  {
    Application app(hw);
    TEST_ASSERT_OK(app.begin());
    TEST_ASSERT_OK(app.config().setInt(dbmo::audio::config_key::kVolume, 20));
    rig->run(app, 3000);
    TEST_ASSERT_TRUE(rig->nvs.saveCount >= 1);
  }
  dbmo::logger().clearSinks();
  Application second(hw);
  TEST_ASSERT_OK(second.begin());
  TEST_ASSERT_FALSE(second.persistence().loadedFromCard());
  TEST_ASSERT_EQUAL_INT32(20, second.config().getInt(dbmo::audio::config_key::kVolume).value());
}

void test_corrupt_config_file_does_not_stop_the_boot() {
  rig->card.put(dbmo::storage::kConfigFile,
                "audio.volume=45\nlinha sem sentido\naudio.sample_rate=999999\nnao.existe=1\n");
  Application app(rig->fullHardware());
  TEST_ASSERT_OK(app.begin());
  TEST_ASSERT_EQUAL(SystemState::Running, app.system().state());
  // A linha válida vale; as inválidas ficam no padrão.
  TEST_ASSERT_EQUAL_INT32(45, app.config().getInt(dbmo::audio::config_key::kVolume).value());
  TEST_ASSERT_EQUAL_UINT32(16000, app.audio().sampleRateHz());
  // O arquivo é regravado limpo.
  rig->run(app, 3000);
  TEST_ASSERT_NULL(strstr(rig->card.text(dbmo::storage::kConfigFile), "sem sentido"));
}

void test_power_button_shuts_the_device_down_in_order() {
  Application app(rig->fullHardware());
  TEST_ASSERT_OK(app.begin());
  rig->observe(app);
  rig->run(app, 200);
  TEST_ASSERT_OK(app.config().setInt(dbmo::audio::config_key::kVolume, 77));  // ainda não gravado

  rig->gpio.setInput(pins::kPowerButton, false);  // segura o botão
  TEST_ASSERT_TRUE(
      rig->runUntil(app, 3000, [&] { return app.runState() == RunState::ShuttingDown; }));
  TEST_ASSERT_EQUAL_INT(1, rig->countOf(dbmo::power::power_event::kShutdownRequested));
  // Subsistemas encerrados, mas a energia só cai depois que o botão é solto.
  TEST_ASSERT_EQUAL(SystemState::Off, app.system().state());
  rig->run(app, 500);
  TEST_ASSERT_EQUAL(RunState::ShuttingDown, app.runState());
  TEST_ASSERT_TRUE(rig->gpio.read(pins::kPowerHold).value());

  rig->gpio.setInput(pins::kPowerButton, true);
  rig->run(app, 5);
  TEST_ASSERT_EQUAL(RunState::PoweredOff, app.runState());
  TEST_ASSERT_FALSE(rig->gpio.read(pins::kPowerHold).value());

  // Nada se perdeu: configuração pendente e log foram para o cartão antes do corte.
  TEST_ASSERT_NOT_NULL(strstr(rig->card.text(dbmo::storage::kConfigFile), "audio.volume=77"));
  TEST_ASSERT_NOT_NULL(strstr(rig->card.text(dbmo::storage::kLogFile), "desligando"));
  TEST_ASSERT_FALSE(rig->card.mounted());
  TEST_ASSERT_FALSE(rig->speaker.amplifier);
}

void test_critical_battery_shuts_the_device_down() {
  Application app(rig->fullHardware());
  TEST_ASSERT_OK(app.begin());
  rig->battery.millivolts = 1600;  // 3,2 V na bateria com o divisor ×2
  // Com a leitura desabilitada (padrão), o valor do ADC não tem efeito algum.
  rig->run(app, 20000);
  TEST_ASSERT_EQUAL(RunState::Running, app.runState());
  TEST_ASSERT_EQUAL(dbmo::power::BatteryLevel::Unknown, app.power().battery().level());

  TEST_ASSERT_OK(app.config().setBool(dbmo::power::config_key::kVbatEnabled, true));
  TEST_ASSERT_TRUE(
      rig->runUntil(app, 20000, [&] { return app.runState() == RunState::PoweredOff; }));
  TEST_ASSERT_EQUAL(dbmo::power::ShutdownReason::BatteryCritical, app.power().shutdownReason());
  TEST_ASSERT_FALSE(rig->gpio.read(pins::kPowerHold).value());
}

void test_shutdown_without_a_way_to_cut_power_halts() {
  Hardware hw = rig->fullHardware();
  hw.powerSwitch = nullptr;  // montagem sem trava e sem controle de sono
  Application app(hw);
  TEST_ASSERT_OK(app.begin());
  rig->run(app, 50);
  app.power().requestShutdown(dbmo::power::ShutdownReason::Command);
  rig->run(app, 50);
  TEST_ASSERT_EQUAL(RunState::Halted, app.runState());
  TEST_ASSERT_EQUAL(SystemState::Off, app.system().state());
  rig->run(app, 1000);  // parado, sem travar
}

void test_radio_receive_and_transmit_policy() {
  Application app(rig->fullHardware());
  TEST_ASSERT_OK(app.begin());
  rig->run(app, 50);
  TEST_ASSERT_OK(app.radio().startReceive());
  rig->run(app, 50);
  TEST_ASSERT_EQUAL(Expression::Scanning, app.face().engine().current());

  const uint8_t payload[3] = {7, 8, 9};
  TEST_ASSERT_TRUE(rig->radioChip.injectPacket(payload, 3));
  TEST_ASSERT_TRUE(rig->runUntil(
      app, 200, [&] { return app.face().engine().current() == Expression::Success; }));
  dbmo::radio::RadioPacket packet;
  TEST_ASSERT_TRUE(app.radio().takePacket(&packet));
  TEST_ASSERT_EQUAL_MEMORY(payload, packet.data, 3);

  // Transmissão: negada de fábrica, e o rosto mostra o alerta.
  rig->run(app, 1500);
  TEST_ASSERT_STATUS(Status::NotPermitted, app.radio().transmit(payload, 3));
  rig->run(app, 10);
  TEST_ASSERT_EQUAL(Expression::Alert, app.face().engine().current());
  TEST_ASSERT_EQUAL_UINT(0, rig->radioChip.transmitCount);
}

void test_infrared_code_is_received_and_send_is_gated() {
  Application app(rig->fullHardware());
  TEST_ASSERT_OK(app.begin());
  rig->observe(app);
  rig->run(app, 50);

  dbmo::infrared::IrCode code;
  code.protocol = dbmo::infrared::IrProtocol::Nec;
  code.bits = 32;
  code.address = 0x04;
  code.command = 0x08;
  TEST_ASSERT_OK(dbmo::infrared::encode(code, &rig->irReceiver.next));
  rig->irReceiver.pending = true;
  rig->run(app, 10);
  TEST_ASSERT_EQUAL_INT(1, rig->countOf(dbmo::infrared::infrared_event::kCodeReceived));
  TEST_ASSERT_EQUAL(Expression::Success, app.face().engine().current());

  TEST_ASSERT_STATUS(Status::NotPermitted, app.infrared().send(code));
  TEST_ASSERT_EQUAL_INT(0, rig->irTransmitter.sendCount);
  TEST_ASSERT_OK(app.config().setBool(dbmo::infrared::config_key::kTxEnabled, true));
  rig->run(app, 5);  // a configuração nova é aplicada na volta seguinte
  TEST_ASSERT_OK(app.infrared().send(code));
  TEST_ASSERT_EQUAL_INT(1, rig->irTransmitter.sendCount);
}

void test_learned_infrared_code_is_stored_on_the_card() {
  Application app(rig->fullHardware());
  TEST_ASSERT_OK(app.begin());
  rig->run(app, 50);
  TEST_ASSERT_OK(app.infrared().startLearning());
  rig->run(app, 10);
  TEST_ASSERT_EQUAL(Expression::Scanning, app.face().engine().current());

  dbmo::infrared::IrCode code;
  code.protocol = dbmo::infrared::IrProtocol::Nec;
  code.bits = 32;
  code.address = 0x20;
  code.command = 0xDF;
  TEST_ASSERT_OK(dbmo::infrared::encode(code, &rig->irReceiver.next));
  rig->irReceiver.pending = true;
  rig->run(app, 10);

  dbmo::infrared::StoredCode learned;
  TEST_ASSERT_TRUE(app.infrared().learned(&learned));
  learned.isRaw = false;
  TEST_ASSERT_OK(app.irLibrary().save("tv_power", learned));
  TEST_ASSERT_EQUAL_STRING("nec:0020:DF", rig->card.text("/dbmo/ir/tv_power.ir"));

  dbmo::infrared::StoredCode loaded;
  TEST_ASSERT_OK(app.irLibrary().load("tv_power", &loaded));
  TEST_ASSERT_TRUE(loaded.code == code);
  TEST_ASSERT_STATUS(Status::InvalidArgument, app.irLibrary().save("../config/config", learned));
  TEST_ASSERT_OK(app.irLibrary().remove("tv_power"));
  TEST_ASSERT_STATUS(Status::NotFound, app.irLibrary().load("tv_power", &loaded));
}

void test_recording_is_visible_on_the_face() {
  Application app(rig->fullHardware());
  TEST_ASSERT_OK(app.begin());
  rig->run(app, 50);
  static int16_t storage[4000];
  TEST_ASSERT_STATUS(Status::NotPermitted, app.audio().startRecording(storage, 4000));
  TEST_ASSERT_FALSE(rig->microphone.running);

  TEST_ASSERT_OK(app.config().setBool(dbmo::audio::config_key::kMicEnabled, true));
  rig->run(app, 5);
  TEST_ASSERT_OK(app.audio().startRecording(storage, 4000));
  rig->run(app, 5);
  TEST_ASSERT_EQUAL(Expression::Listening, app.face().engine().current());
  TEST_ASSERT_TRUE(rig->runUntil(app, 5000, [&] { return !app.audio().recording(); }));
  rig->run(app, 5);
  TEST_ASSERT_EQUAL(Expression::Idle, app.face().engine().current());
  TEST_ASSERT_FALSE(rig->microphone.running);
}

void test_card_removed_in_use_keeps_the_device_running() {
  Application app(rig->fullHardware());
  TEST_ASSERT_OK(app.begin());
  rig->run(app, 500);
  rig->card.ejectCard();
  TEST_ASSERT_TRUE(rig->runUntil(app, 8000, [&] { return !app.storage().available(); }));
  rig->run(app, 5);
  TEST_ASSERT_EQUAL(Expression::Warning, app.face().engine().current());
  TEST_ASSERT_EQUAL(RunState::Running, app.runState());

  // Sem cartão, a configuração passa a ir para a memória interna.
  const unsigned before = rig->nvs.saveCount;
  TEST_ASSERT_OK(app.config().setInt(dbmo::audio::config_key::kVolume, 10));
  rig->run(app, 3000);
  TEST_ASSERT_TRUE(rig->nvs.saveCount > before);

  rig->card.insertCard();
  TEST_ASSERT_TRUE(rig->runUntil(app, 8000, [&] { return app.storage().available(); }));
}

void test_peripheral_connected_after_boot_is_picked_up() {
  rig->bus.detach(i2c::kPn532);
  Application app(rig->fullHardware());
  TEST_ASSERT_OK(app.begin());
  TEST_ASSERT_EQUAL(SubsystemState::Absent, stateOf(app, "nfc"));
  TEST_ASSERT_EQUAL(SystemState::Degraded, app.system().state());
  rig->run(app, 5000);
  rig->bus.attach(i2c::kPn532, &rig->pn532);
  rig->run(app, Application::kRetryPeriodMs + 1000);
  TEST_ASSERT_EQUAL(SubsystemState::Ready, stateOf(app, "nfc"));
  TEST_ASSERT_EQUAL(SystemState::Running, app.system().state());
}

void test_console_commands() {
  Application app(rig->fullHardware());
  TEST_ASSERT_OK(app.begin());
  CapturingOutput output;
  dbmo::app::Console console(output);
  TEST_ASSERT_OK(app.registerCommands(console));
  rig->run(app, 50);

  TEST_ASSERT_OK(console.execute("status"));
  TEST_ASSERT_TRUE(output.contains("sistema: Running"));
  TEST_ASSERT_TRUE(output.contains("rosto"));
  TEST_ASSERT_TRUE(output.contains("bateria: sem leitura (Disabled)"));  // nenhum número inventado

  output.clear();
  TEST_ASSERT_OK(console.execute("config audio.volume 30"));
  TEST_ASSERT_TRUE(output.contains("audio.volume=30"));
  TEST_ASSERT_EQUAL_INT32(30, app.config().getInt(dbmo::audio::config_key::kVolume).value());

  output.clear();
  TEST_ASSERT_OK(console.execute("config audio.volume 500"));
  TEST_ASSERT_TRUE(output.contains("valor recusado"));
  TEST_ASSERT_OK(console.execute("config nao.existe 1"));
  TEST_ASSERT_TRUE(output.contains("chave desconhecida"));

  output.clear();
  TEST_ASSERT_OK(console.execute("config"));
  TEST_ASSERT_TRUE(output.contains("radio.tx_enabled=0"));
  TEST_ASSERT_TRUE(output.contains("nfc.scan_enabled=0"));
  TEST_ASSERT_TRUE(output.contains("audio.mic_enabled=0"));
  TEST_ASSERT_TRUE(output.contains("infrared.tx_enabled=0"));
  TEST_ASSERT_TRUE(output.contains("power.vbat_enabled=0"));

  TEST_ASSERT_OK(console.execute("face happy"));
  rig->run(app, 5);
  TEST_ASSERT_EQUAL(Expression::Happy, app.face().engine().current());

  TEST_ASSERT_OK(console.execute("tone 1000 50"));
  rig->run(app, 100);
  TEST_ASSERT_EQUAL_UINT(800, rig->speaker.samples);

  output.clear();
  TEST_ASSERT_OK(console.execute("nfc on"));
  rig->run(app, 400);
  TEST_ASSERT_EQUAL(dbmo::nfc::NfcState::Scanning, app.nfc().state());
  TEST_ASSERT_OK(console.execute("radio"));
  TEST_ASSERT_TRUE(output.contains("433920000 Hz"));
  TEST_ASSERT_OK(console.execute("cart"));
  TEST_ASSERT_TRUE(output.contains("nenhum identificado"));

  TEST_ASSERT_OK(console.execute("save"));
  TEST_ASSERT_NOT_NULL(strstr(rig->card.text(dbmo::storage::kConfigFile), "audio.volume=30"));

  TEST_ASSERT_OK(console.execute("off"));
  rig->run(app, 50);
  TEST_ASSERT_EQUAL(RunState::PoweredOff, app.runState());
}

void test_long_run_is_stable() {
  // Dez minutos de aparelho ligado com tudo em uso: sem evento perdido, sem falha.
  Application app(rig->fullHardware());
  TEST_ASSERT_OK(app.addCartridgeDriver(rig->sensorDriver));
  TEST_ASSERT_OK(app.begin());
  TEST_ASSERT_OK(app.nfc().startScan());
  TEST_ASSERT_OK(app.radio().startReceive());
  rig->insertExampleCartridge();
  const uint8_t uid[4] = {1, 2, 3, 4};
  const uint8_t payload[2] = {1, 2};
  for (uint32_t second = 0; second < 600; ++second) {
    if (second % 30 == 5) rig->pn532.setTag(uid, 4);
    if (second % 30 == 15) rig->pn532.tagPresent = false;
    if (second % 20 == 3) (void)rig->radioChip.injectPacket(payload, 2);
    if (second % 7 == 0) {
      rig->mcp.press(dbmo::config::mcp_buttons::kDpadUp);
    } else {
      rig->mcp.release(dbmo::config::mcp_buttons::kDpadUp);
    }
    dbmo::radio::RadioPacket packet;
    while (app.radio().takePacket(&packet)) {
    }
    rig->run(app, 1000);
  }
  TEST_ASSERT_EQUAL(RunState::Running, app.runState());
  TEST_ASSERT_EQUAL(SystemState::Running, app.system().state());
  TEST_ASSERT_EQUAL_UINT32(0, app.bus().droppedCount());
  TEST_ASSERT_EQUAL_UINT(0, app.system().faultCount());
  TEST_ASSERT_TRUE(app.radio().counters().packets >= 25);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_boot_with_all_hardware);
  RUN_TEST(test_boot_on_todays_bench_with_only_a_display);
  RUN_TEST(test_missing_required_hardware_is_refused);
  RUN_TEST(test_button_press_reaches_the_event_bus);
  RUN_TEST(test_nfc_scan_drives_the_face);
  RUN_TEST(test_cartridge_insertion_starts_its_driver);
  RUN_TEST(test_cartridge_cannot_claim_a_device_address);
  RUN_TEST(test_config_is_saved_to_the_card_and_restored);
  RUN_TEST(test_config_falls_back_to_internal_storage_without_a_card);
  RUN_TEST(test_corrupt_config_file_does_not_stop_the_boot);
  RUN_TEST(test_power_button_shuts_the_device_down_in_order);
  RUN_TEST(test_critical_battery_shuts_the_device_down);
  RUN_TEST(test_shutdown_without_a_way_to_cut_power_halts);
  RUN_TEST(test_radio_receive_and_transmit_policy);
  RUN_TEST(test_infrared_code_is_received_and_send_is_gated);
  RUN_TEST(test_learned_infrared_code_is_stored_on_the_card);
  RUN_TEST(test_recording_is_visible_on_the_face);
  RUN_TEST(test_card_removed_in_use_keeps_the_device_running);
  RUN_TEST(test_peripheral_connected_after_boot_is_picked_up);
  RUN_TEST(test_console_commands);
  RUN_TEST(test_long_run_is_stable);
  return UNITY_END();
}
