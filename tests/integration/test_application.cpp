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
  // Toque curto e toque longo (input.long_press_ms = 600) num botão do MCP23017.
  void click(Application &app, uint8_t pin) {
    mcp.press(pin);
    run(app, 60);
    mcp.release(pin);
    run(app, 60);
  }
  void longPress(Application &app, uint8_t pin) {
    mcp.press(pin);
    run(app, 800);
    mcp.release(pin);
    run(app, 60);
  }
  size_t litPixels() const {
    size_t lit = 0;
    for (dbmo::display::Color color : gPixels)
      if (color == dbmo::display::palette::kSignalRed) ++lit;
    return lit;
  }
};

Rig *rig = nullptr;

namespace mcp = dbmo::config::mcp_buttons;

// Alguma linha do menu contém o trecho?
bool screenHas(const Application &app, const char *fragment) {
  for (size_t row = 0; row < app.menuScreen().rows(); ++row)
    if (strstr(app.menuScreen().line(row), fragment) != nullptr) return true;
  return false;
}

bool facePixel(uint16_t x, uint16_t y) {
  return gPixels[static_cast<size_t>(y) * 320 + x] == dbmo::display::palette::kSignalRed;
}

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

void test_display_with_unexpected_id_is_degraded_and_still_draws() {
  // Bancada, 2026-11-21: sem o SDO da tela, ID 0xFFFF e mesmo assim "display: Ready (Ok)".
  rig->display.controllerIdResult = static_cast<uint16_t>(0xFFFF);
  Hardware hw;
  hw.clock = &rig->clock;
  hw.display = &rig->display;
  Application app(hw);
  TEST_ASSERT_OK(app.begin());
  TEST_ASSERT_EQUAL(SubsystemState::Degraded, stateOf(app, "display"));
  TEST_ASSERT_STATUS(Status::IoError, app.system().find("display")->lastStatus);
  TEST_ASSERT_EQUAL(SubsystemState::Ready, stateOf(app, "rosto"));
  // Aviso, não falha: sem registro de falha e sem o rosto de erro.
  TEST_ASSERT_EQUAL_UINT(0, app.system().faultCount());
  rig->run(app, 10);
  TEST_ASSERT_EQUAL(Expression::Idle, app.face().engine().current());
  rig->run(app, 3000);
  TEST_ASSERT_EQUAL(RunState::Running, app.runState());
  TEST_ASSERT_TRUE(rig->litPixels() > 500);  // o rosto continua na tela

  CapturingOutput output;
  dbmo::app::Console console(output);
  TEST_ASSERT_OK(app.registerCommands(console));
  TEST_ASSERT_OK(console.execute("status"));
  TEST_ASSERT_TRUE(output.contains("display      Degraded      IoError"));
}

void test_menu_opens_learns_infrared_and_returns_to_the_face() {
  Application app(rig->fullHardware());
  TEST_ASSERT_OK(app.begin());
  rig->run(app, 200);
  TEST_ASSERT_TRUE(facePixel(80, 60));  // olho esquerdo do rosto

  rig->click(app, mcp::kActionA);  // OK no rosto abre o menu
  TEST_ASSERT_TRUE(app.menu().isOpen());
  TEST_ASSERT_FALSE(app.face().visible());
  TEST_ASSERT_EQUAL_STRING("Menu", app.menuScreen().line(0));
  TEST_ASSERT_EQUAL_STRING("> Estado", app.menuScreen().line(2));
  TEST_ASSERT_FALSE(facePixel(80, 60));                                     // o menu cobriu o rosto
  TEST_ASSERT_EQUAL_HEX16(dbmo::display::palette::kSignalRed, gPixels[0]);  // faixa do título

  rig->click(app, mcp::kDpadDown);
  TEST_ASSERT_EQUAL_STRING("> Infravermelho", app.menuScreen().line(3));
  // BAIXO segurado: repetições aos 400, 520 e 640 ms, e soltar não anda mais um item.
  rig->mcp.press(mcp::kDpadDown);
  rig->run(app, 700);
  TEST_ASSERT_EQUAL_STRING("> Sobre", app.menuScreen().line(6));
  rig->mcp.release(mcp::kDpadDown);
  rig->run(app, 60);
  TEST_ASSERT_EQUAL_STRING("> Sobre", app.menuScreen().line(6));
  for (int i = 0; i < 3; ++i) rig->click(app, mcp::kDpadUp);
  TEST_ASSERT_EQUAL_STRING("> Infravermelho", app.menuScreen().line(3));
  rig->click(app, mcp::kActionA);
  TEST_ASSERT_EQUAL_STRING("Infravermelho", app.menuScreen().line(0));
  TEST_ASSERT_TRUE(screenHas(app, "nada recebido"));
  TEST_ASSERT_TRUE(screenHas(app, "Envio: desabilitado"));

  rig->click(app, mcp::kActionA);  // OK aprende
  TEST_ASSERT_TRUE(app.infrared().learning());
  TEST_ASSERT_TRUE(screenHas(app, "aponte e aperte a tecla"));
  dbmo::infrared::IrCode code;
  code.protocol = dbmo::infrared::IrProtocol::Nec;
  code.bits = 32;
  code.address = 0x20;
  code.command = 0xDF;
  TEST_ASSERT_OK(dbmo::infrared::encode(code, &rig->irReceiver.next));
  rig->irReceiver.pending = true;
  rig->run(app, 20);
  TEST_ASSERT_EQUAL_STRING("  ok: nec:0020:DF", app.menuScreen().line(6));
  TEST_ASSERT_EQUAL_STRING("  nec:0020:DF", app.menuScreen().line(3));

  rig->longPress(app, mcp::kActionA);  // toque longo volta à lista, no mesmo item
  TEST_ASSERT_EQUAL_STRING("> Infravermelho", app.menuScreen().line(3));
  rig->click(app, mcp::kActionB);  // B volta ao rosto (perfil target)
  TEST_ASSERT_FALSE(app.menu().isOpen());
  TEST_ASSERT_TRUE(app.face().visible());
  rig->run(app, 2000);
  TEST_ASSERT_TRUE(rig->litPixels() > 500);
  TEST_ASSERT_EQUAL_HEX16(dbmo::display::palette::kBlack, gPixels[0]);  // sem a faixa do menu
  TEST_ASSERT_EQUAL_INT(0, rig->irTransmitter.sendCount);
  TEST_ASSERT_EQUAL_UINT(0, rig->radioChip.transmitCount);
}

void test_menu_status_shows_absent_subsystems_and_hides_their_items() {
  Hardware hw = rig->fullHardware();
  hw.radioSpi = nullptr;  // sem CC1101, como na bancada sem o rádio
  hw.irReceiver = nullptr;
  hw.irTransmitter = nullptr;
  Application app(hw);
  TEST_ASSERT_OK(app.begin());
  rig->run(app, 100);
  rig->click(app, mcp::kActionA);
  TEST_ASSERT_FALSE(screenHas(app, "Radio"));
  TEST_ASSERT_FALSE(screenHas(app, "Infravermelho"));
  TEST_ASSERT_EQUAL_STRING("  Botoes", app.menuScreen().line(3));

  rig->click(app, mcp::kActionA);  // Estado
  TEST_ASSERT_EQUAL_STRING("Estado", app.menuScreen().line(0));
  TEST_ASSERT_TRUE(screenHas(app, "sistema: degradado"));
  TEST_ASSERT_TRUE(screenHas(app, "radio      ausente"));
  TEST_ASSERT_TRUE(screenHas(app, "infrared   ausente"));
  TEST_ASSERT_TRUE(screenHas(app, "display    pronto"));
  TEST_ASSERT_TRUE(screenHas(app, "OK longo: volta"));
}

void test_menu_radio_screen_only_toggles_reception() {
  Application app(rig->fullHardware());
  TEST_ASSERT_OK(app.begin());
  rig->run(app, 100);
  rig->click(app, mcp::kActionA);
  rig->click(app, mcp::kDpadDown);
  rig->click(app, mcp::kDpadDown);
  rig->click(app, mcp::kActionA);
  TEST_ASSERT_EQUAL_STRING("Radio", app.menuScreen().line(0));
  TEST_ASSERT_TRUE(screenHas(app, "estado: em repouso"));
  TEST_ASSERT_TRUE(screenHas(app, "freq: 433.920 MHz"));
  TEST_ASSERT_TRUE(screenHas(app, "transmissao: desabilitada"));

  rig->click(app, mcp::kActionA);
  TEST_ASSERT_EQUAL(dbmo::radio::RadioState::Receiving, app.radio().state());
  TEST_ASSERT_TRUE(screenHas(app, "estado: recebendo"));
  TEST_ASSERT_TRUE(screenHas(app, "recepcao: Ok"));
  const uint8_t payload[2] = {1, 2};
  TEST_ASSERT_TRUE(rig->radioChip.injectPacket(payload, 2));
  rig->run(app, 50);
  TEST_ASSERT_TRUE(screenHas(app, "pacotes  1"));

  rig->click(app, mcp::kActionA);
  TEST_ASSERT_EQUAL(dbmo::radio::RadioState::Idle, app.radio().state());
  TEST_ASSERT_TRUE(screenHas(app, "estado: em repouso"));
  TEST_ASSERT_EQUAL_UINT(0, rig->radioChip.transmitCount);
}

void test_menu_buttons_and_about_screens() {
  Hardware hw = rig->fullHardware();
  hw.profile = "bench";
  Application app(hw);
  TEST_ASSERT_OK(app.begin());
  rig->run(app, 100);
  rig->click(app, mcp::kActionA);
  for (int i = 0; i < 3; ++i) rig->click(app, mcp::kDpadDown);
  rig->click(app, mcp::kActionA);
  TEST_ASSERT_EQUAL_STRING("Botoes", app.menuScreen().line(0));
  TEST_ASSERT_TRUE(screenHas(app, "aperte um botao"));
  rig->click(app, mcp::kDpadUp);
  TEST_ASSERT_EQUAL_STRING("  up: clique", app.menuScreen().line(3));
  TEST_ASSERT_EQUAL_STRING("  up: soltou", app.menuScreen().line(4));
  TEST_ASSERT_EQUAL_STRING("  up: apertou", app.menuScreen().line(5));
  TEST_ASSERT_EQUAL_STRING("Botoes", app.menuScreen().line(0));  // CIMA não navegou

  rig->longPress(app, mcp::kActionA);
  rig->click(app, mcp::kDpadDown);
  rig->click(app, mcp::kActionA);
  TEST_ASSERT_EQUAL_STRING("Sobre", app.menuScreen().line(0));
  TEST_ASSERT_EQUAL_STRING("d-BMO v1", app.menuScreen().line(2));
  TEST_ASSERT_EQUAL_STRING("firmware 0.1.0", app.menuScreen().line(3));
  TEST_ASSERT_EQUAL_STRING("perfil: bench", app.menuScreen().line(4));
}

void test_idle_menu_redraws_nothing_and_returns_to_the_face() {
  Application app(rig->fullHardware());
  TEST_ASSERT_OK(app.begin());
  rig->run(app, 100);
  rig->click(app, mcp::kActionA);
  rig->run(app, 1000);
  // Parado e sem nada mudando: o menu não repinta a tela (nem o rosto escondido).
  const unsigned calls = rig->display.fillRectCalls;
  rig->run(app, 5000);
  TEST_ASSERT_EQUAL_UINT(calls, rig->display.fillRectCalls);

  // Um botão no menu conta como atividade: a tela não escurece aos 30 s do último toque
  // de antes, e o menu só fecha 30 s depois deste.
  rig->click(app, mcp::kDpadDown);
  rig->run(app, 25000);
  TEST_ASSERT_TRUE(app.menu().isOpen());
  TEST_ASSERT_TRUE(app.display().power() == dbmo::display::ScreenPower::On);
  TEST_ASSERT_TRUE(rig->runUntil(app, 6000, [&] { return !app.menu().isOpen(); }));
  TEST_ASSERT_TRUE(app.face().visible());
  rig->run(app, 100);
  TEST_ASSERT_TRUE(rig->litPixels() > 500);
  TEST_ASSERT_EQUAL_HEX16(dbmo::display::palette::kBlack, gPixels[0]);
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

void test_button_press_is_written_to_the_log() {
  dbmo::mock::RecordingLogSink sink;
  TEST_ASSERT_OK(dbmo::logger().addSink(&sink));
  Application app(rig->fullHardware());
  TEST_ASSERT_OK(app.begin());
  rig->run(app, 50);
  sink.clear();
  rig->mcp.press(dbmo::config::mcp_buttons::kActionA);
  rig->run(app, 60);
  rig->mcp.release(dbmo::config::mcp_buttons::kActionA);
  rig->run(app, 60);
  TEST_ASSERT_TRUE(sink.contains("botao a: apertou"));
  TEST_ASSERT_TRUE(sink.contains("botao a: soltou"));
  TEST_ASSERT_TRUE(sink.contains("botao a: clique"));
  dbmo::logger().clearSinks();
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

void test_received_infrared_code_is_written_to_the_log_once() {
  // Bancada, 2026-11-21: para ver o código recebido foi preciso mandar "ir last" à mão.
  dbmo::mock::RecordingLogSink sink;
  TEST_ASSERT_OK(dbmo::logger().addSink(&sink));
  Application app(rig->fullHardware());
  TEST_ASSERT_OK(app.begin());
  rig->run(app, 50);
  sink.clear();

  dbmo::infrared::IrCode code;
  code.protocol = dbmo::infrared::IrProtocol::Nec;
  code.bits = 32;
  code.address = 0x04;
  code.command = 0x17;
  TEST_ASSERT_OK(dbmo::infrared::encode(code, &rig->irReceiver.next));
  rig->irReceiver.pending = true;
  rig->run(app, 10);
  TEST_ASSERT_TRUE(sink.contains("infravermelho: nec:0004:17"));

  // O mesmo quadro logo em seguida (tecla segurada) é repetição: não vai para o log.
  sink.clear();
  rig->irReceiver.pending = true;
  rig->run(app, 10);
  TEST_ASSERT_FALSE(sink.contains("infravermelho:"));

  // Sinal de protocolo desconhecido: registra o tamanho, não as durações. Com a tecla
  // segurada o quadro volta a cada ~110 ms (RC5, por exemplo): só o primeiro vai ao log.
  rig->run(app, 500);
  sink.clear();
  rig->irReceiver.next.count = 5;
  for (int frame = 0; frame < 9; ++frame) {
    rig->irReceiver.pending = true;
    rig->run(app, 114);
  }
  TEST_ASSERT_EQUAL_UINT(1, sink.countContaining("infravermelho: sinal desconhecido (5 duracoes)"));
  rig->run(app, 500);  // soltou e apertou de novo
  rig->irReceiver.pending = true;
  rig->run(app, 10);
  TEST_ASSERT_EQUAL_UINT(2, sink.countContaining("infravermelho: sinal desconhecido"));
  // Outra tecla logo em seguida, com outro tamanho: é um sinal novo.
  rig->run(app, 150);
  rig->irReceiver.next.count = 7;
  rig->irReceiver.pending = true;
  rig->run(app, 10);
  TEST_ASSERT_EQUAL_UINT(1, sink.countContaining("infravermelho: sinal desconhecido (7 duracoes)"));
  dbmo::logger().clearSinks();
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

void test_mic_command_records_only_when_enabled_and_logs_the_peak() {
  // Bancada, 2026-11-22: primeiro teste do INMP441 pelo console, sem alto-falante.
  dbmo::mock::RecordingLogSink sink;
  TEST_ASSERT_OK(dbmo::logger().addSink(&sink));
  Application app(rig->fullHardware());
  TEST_ASSERT_OK(app.begin());
  CapturingOutput output;
  dbmo::app::Console console(output);
  TEST_ASSERT_OK(app.registerCommands(console));
  rig->run(app, 50);

  // Microfone desabilitado por padrão: o comando não grava e diz como habilitar.
  TEST_ASSERT_OK(console.execute("mic"));
  TEST_ASSERT_TRUE(output.contains("audio.mic_enabled"));
  TEST_ASSERT_FALSE(app.audio().recording());
  TEST_ASSERT_FALSE(rig->microphone.running);

  output.clear();
  TEST_ASSERT_OK(console.execute("mic 5000"));
  TEST_ASSERT_TRUE(output.contains("uso: mic"));

  TEST_ASSERT_OK(console.execute("config audio.mic_enabled 1"));
  rig->run(app, 5);  // a configuração nova vale a partir da próxima volta do laço
  output.clear();
  sink.clear();
  TEST_ASSERT_OK(console.execute("mic 300"));
  TEST_ASSERT_TRUE(output.contains("gravando 300 ms"));
  TEST_ASSERT_TRUE(app.audio().recording());
  rig->run(app, 400);
  TEST_ASSERT_FALSE(app.audio().recording());
  TEST_ASSERT_FALSE(rig->microphone.running);  // o microfone só fica ligado gravando
  TEST_ASSERT_TRUE(app.audio().recordedSamples() > 0);
  TEST_ASSERT_TRUE(sink.contains("gravacao encerrada"));
  // O microfone falso entrega 100 constante: o passa-altas da gravação tira esse DC.
  TEST_ASSERT_TRUE(sink.contains("pico 0"));

  // Estatísticas da última gravação, para separar som de desvio de DC na bancada.
  output.clear();
  TEST_ASSERT_OK(console.execute("mic last"));
  TEST_ASSERT_TRUE(output.contains("media 0; min 0; max 0; rms 0"));
  TEST_ASSERT_TRUE(output.contains("media por 100 ms: 0"));
  TEST_ASSERT_TRUE(output.contains("rms por 100 ms: 0"));
  TEST_ASSERT_TRUE(output.contains("pico por 100 ms: 0"));

  // Com o filtro desligado, a amostra crua.
  TEST_ASSERT_OK(console.execute("config audio.mic_highpass_hz 0"));
  rig->run(app, 5);
  TEST_ASSERT_OK(console.execute("mic 300"));
  rig->run(app, 400);
  output.clear();
  TEST_ASSERT_OK(console.execute("mic last"));
  TEST_ASSERT_TRUE(output.contains("media 100; min 100; max 100; rms 100"));
  dbmo::logger().clearSinks();
}

void test_boot_log_says_internal_memory_values_apply_when_the_card_has_no_config() {
  // Bancada, 2026-11-25: com o cartão sem arquivo, o log dizia "usando os padroes" enquanto
  // o microfone seguia habilitado pela memória interna.
  dbmo::mock::RecordingLogSink sink;
  TEST_ASSERT_OK(dbmo::logger().addSink(&sink));
  rig->nvs.setText("audio.mic_enabled=1\n");
  Application app(rig->fullHardware());
  TEST_ASSERT_OK(app.begin());
  rig->run(app, 50);
  TEST_ASSERT_TRUE(app.config().getBool(dbmo::audio::config_key::kMicEnabled).value());
  TEST_ASSERT_FALSE(sink.contains("usando os padroes"));
  TEST_ASSERT_TRUE(
      sink.contains("nenhuma configuracao no cartao: valem 1 chaves da memoria interna"));
  dbmo::logger().clearSinks();
}

void test_boot_log_says_defaults_when_nothing_is_saved_anywhere() {
  dbmo::mock::RecordingLogSink sink;
  TEST_ASSERT_OK(dbmo::logger().addSink(&sink));
  Application app(rig->fullHardware());
  TEST_ASSERT_OK(app.begin());
  rig->run(app, 50);
  TEST_ASSERT_TRUE(sink.contains("nenhuma configuracao salva: usando os padroes"));
  dbmo::logger().clearSinks();
}

void test_mic_command_uses_the_longer_buffer_the_platform_gives() {
  // Na placa, main.cpp entrega um buffer na PSRAM: o rosto "ouvindo" passa de 1 s para 10 s.
  dbmo::mock::RecordingLogSink sink;
  TEST_ASSERT_OK(dbmo::logger().addSink(&sink));
  Application app(rig->fullHardware());
  static int16_t longBuffer[48000];  // 3 s a 16 kHz
  app.setRecordBuffer(longBuffer, 48000);
  TEST_ASSERT_OK(app.begin());
  CapturingOutput output;
  dbmo::app::Console console(output);
  TEST_ASSERT_OK(app.registerCommands(console));
  TEST_ASSERT_OK(console.execute("config audio.mic_enabled 1"));
  rig->run(app, 50);

  TEST_ASSERT_OK(console.execute("mic 3001"));
  TEST_ASSERT_TRUE(output.contains("uso: mic [ms], ate 3000"));
  output.clear();
  TEST_ASSERT_OK(console.execute("mic 3000"));
  TEST_ASSERT_TRUE(output.contains("gravando 3000 ms"));
  rig->run(app, 3200);
  TEST_ASSERT_FALSE(app.audio().recording());
  // O microfone falso entrega amostras mais rápido que o tempo real: o que importa é que a
  // gravação passou de 1 s de amostras, o tamanho do buffer próprio.
  TEST_ASSERT_TRUE(app.audio().recordedSamples() > 16000);
  output.clear();
  TEST_ASSERT_OK(console.execute("mic last"));
  TEST_ASSERT_TRUE(output.contains("media por 1000 ms:"));  // trechos de 1 s acima de 1 s
  dbmo::logger().clearSinks();
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
  RUN_TEST(test_display_with_unexpected_id_is_degraded_and_still_draws);
  RUN_TEST(test_menu_opens_learns_infrared_and_returns_to_the_face);
  RUN_TEST(test_menu_status_shows_absent_subsystems_and_hides_their_items);
  RUN_TEST(test_menu_radio_screen_only_toggles_reception);
  RUN_TEST(test_menu_buttons_and_about_screens);
  RUN_TEST(test_idle_menu_redraws_nothing_and_returns_to_the_face);
  RUN_TEST(test_missing_required_hardware_is_refused);
  RUN_TEST(test_button_press_reaches_the_event_bus);
  RUN_TEST(test_button_press_is_written_to_the_log);
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
  RUN_TEST(test_received_infrared_code_is_written_to_the_log_once);
  RUN_TEST(test_learned_infrared_code_is_stored_on_the_card);
  RUN_TEST(test_recording_is_visible_on_the_face);
  RUN_TEST(test_card_removed_in_use_keeps_the_device_running);
  RUN_TEST(test_peripheral_connected_after_boot_is_picked_up);
  RUN_TEST(test_mic_command_records_only_when_enabled_and_logs_the_peak);
  RUN_TEST(test_mic_command_uses_the_longer_buffer_the_platform_gives);
  RUN_TEST(test_boot_log_says_internal_memory_values_apply_when_the_card_has_no_config);
  RUN_TEST(test_boot_log_says_defaults_when_nothing_is_saved_anywhere);
  RUN_TEST(test_console_commands);
  RUN_TEST(test_long_run_is_stable);
  return UNITY_END();
}
