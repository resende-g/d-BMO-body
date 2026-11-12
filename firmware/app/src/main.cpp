// Firmware próprio do d-BMO v1: ponto de entrada no ESP32-S3 (Arduino).
// Aqui só se criam os objetos do porte e se diz à aplicação que hardware existe; a
// lógica está em dbmo::app::Application e nas bibliotecas portáveis de firmware/,
// testadas no computador.
//
// Dois perfis (platformio.ini):
//   DBMO_PROFILE_BENCH   a protoboard de hoje: tela, três botões em GPIO, IR;
//   DBMO_PROFILE_TARGET  a pinagem proposta do v1 (pins.h, namespace target).
// Periférico que não estiver ligado aparece como ausente no comando "status".
#include <Adafruit_ILI9341.h>
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

#include "dbmo/app/application.h"
#include "dbmo/app/console.h"
#include "dbmo/config/pins.h"
#include "dbmo/core/log.h"
#include "dbmo/core/version.h"
#include "dbmo/display/display.h"
#include "dbmo/platform/esp32/hal.h"
#include "dbmo/platform/esp32/ili9341_display.h"
#include "dbmo/platform/esp32/peripherals.h"

#if !defined(DBMO_PROFILE_BENCH) && !defined(DBMO_PROFILE_TARGET)
#error "Defina DBMO_PROFILE_BENCH ou DBMO_PROFILE_TARGET (ver platformio.ini)"
#endif

namespace {

namespace pins = dbmo::config::target;
namespace esp32 = dbmo::platform::esp32;

constexpr char kTag[] = "main";
// Tela na horizontal (320 x 240), como nos sketches de bancada.
constexpr uint8_t kDisplayRotation = 1;
// Frequência do cartão depois de montado (padrão da biblioteca SD).
constexpr uint32_t kSdHz = 4000000;

class SerialConsoleOutput final : public dbmo::app::IConsoleOutput {
 public:
  void write(const char *text) override { Serial.print(text); }
};

// ---- Comum aos dois perfis ----
esp32::ArduinoClock gClock;
esp32::ArduinoGpio gGpio;
esp32::PortCriticalSection gEventLock;
esp32::SerialLogSink gSerialLog(Serial);
dbmo::LogHistory<32> gLogHistory;
esp32::ArduinoI2cBus gI2c(Wire);
esp32::NvsConfigBackend gNvs;
esp32::SdFileSystem gCard(SPI, static_cast<uint8_t>(pins::kSdCs), kSdHz);
esp32::ArduinoSpiDevice gRadioSpi(SPI, pins::kCc1101Cs, pins::kSpiMiso,
                                  dbmo::config::spi::kCc1101Hz);
esp32::DeepSleepControl gSleep;
Adafruit_ILI9341 gTft(&SPI, pins::kDisplayDc, pins::kDisplayCs, pins::kDisplayReset);

#if defined(DBMO_PROFILE_TARGET)
// ---- Pinagem proposta do v1 ----
esp32::LedcPwmOutput gBacklight(static_cast<uint8_t>(pins::kDisplayBacklight), 5000);
esp32::Ili9341Display gDisplay(gTft, gClock, &gBacklight);
dbmo::input::Mcp23017 gExpander(gI2c, dbmo::config::i2c::kMcp23017Buttons);
dbmo::input::Mcp23017ButtonSource gButtons(gExpander,
                                           dbmo::input::Mcp23017ButtonSource::defaultMap());
esp32::ArduinoAdcChannel gStickLeftX(static_cast<uint8_t>(pins::kStickLeftX));
esp32::ArduinoAdcChannel gStickLeftY(static_cast<uint8_t>(pins::kStickLeftY));
esp32::ArduinoAdcChannel gStickRightX(static_cast<uint8_t>(pins::kStickRightX));
esp32::ArduinoAdcChannel gStickRightY(static_cast<uint8_t>(pins::kStickRightY));
dbmo::input::StickProcessor gStickLeft(gStickLeftX, gStickLeftY);
dbmo::input::StickProcessor gStickRight(gStickRightX, gStickRightY);
esp32::ArduinoAdcChannel gBatteryAdc(static_cast<uint8_t>(pins::kVbatSense));
dbmo::power::GpioPowerLatch gPowerLatch(gGpio, pins::kPowerHold);
esp32::I2sAudio gAudio(pins::kI2sBclk, pins::kI2sLrclk, pins::kI2sSpeakerData, pins::kI2sMicData,
                       pins::kAmpShutdown);
esp32::ArduinoSpiDevice gCartridgeSpi(SPI, pins::kCartridgeCs, pins::kSpiMiso, 1000000);
esp32::RmtIrReceiver gIrReceiver(pins::kIrRx);
esp32::RmtIrTransmitter gIrTransmitter(pins::kIrTx);
#else
// ---- Protoboard de hoje ----
namespace bench = dbmo::config::bench;
// O LED da tela está direto no 3V3: sem controle de backlight.
esp32::Ili9341Display gDisplay(gTft, gClock, nullptr);
const dbmo::input::GpioButtonSource::Entry kBenchButtons[] = {
    {bench::kButtonUp, dbmo::input::Button::DpadUp},
    {bench::kButtonOk, dbmo::input::Button::A},
    {bench::kButtonDown, dbmo::input::Button::DpadDown},
};
dbmo::input::GpioButtonSource gButtons(gGpio, kBenchButtons,
                                       sizeof(kBenchButtons) / sizeof(kBenchButtons[0]));
esp32::RmtIrReceiver gIrReceiver(bench::kIrRx);
esp32::RmtIrTransmitter gIrTransmitter(bench::kIrTx);
#endif

dbmo::app::Hardware makeHardware() {
  dbmo::app::Hardware hw;
  hw.clock = &gClock;
  hw.display = &gDisplay;
  hw.eventLock = &gEventLock;
  hw.i2c = &gI2c;
  hw.fallbackConfig = &gNvs;
  hw.fileSystem = &gCard;
  hw.buttonSources[0] = &gButtons;
  hw.radioSpi = &gRadioSpi;
  hw.irReceiver = &gIrReceiver;
  hw.irTransmitter = &gIrTransmitter;
  hw.sleep = &gSleep;
  // Botão de energia no GPIO 0 (na DevKitC, o botão BOOT). Pino de strap: risco aberto
  // registrado em docs/hardware/GPIO_MAP.md.
  hw.powerButtonGpio = &gGpio;
  hw.powerButtonPin = pins::kPowerButton;
#if defined(DBMO_PROFILE_TARGET)
  hw.stickLeft = &gStickLeft;
  hw.stickRight = &gStickRight;
  hw.batteryAdc = &gBatteryAdc;
  hw.powerSwitch = &gPowerLatch;
  hw.audioOutput = &gAudio.output();
  hw.audioInput = &gAudio.input();
  hw.cartridgeSpi = &gCartridgeSpi;
#endif
  return hw;
}

dbmo::app::Application gApp(makeHardware());
SerialConsoleOutput gConsoleOutput;
dbmo::app::Console gConsole(gConsoleOutput);

void versionCommand(dbmo::app::Console &console, int, const char *const *, void *) {
  char version[12];
  dbmo::formatVersion(dbmo::kFirmwareVersion, version, sizeof(version));
  console.printf("%s firmware %s\r\n", dbmo::kHardwareName, version);
}

void logCommand(dbmo::app::Console &console, int, const char *const *, void *) {
  for (size_t i = 0; i < gLogHistory.size(); ++i) {
    const dbmo::LogEntry &entry = gLogHistory.at(i);
    console.printf("[%8lu][%s][%s] %s\r\n", static_cast<unsigned long>(entry.timestampMs),
                   dbmo::toString(entry.level), entry.tag, entry.text);
  }
}

// Todo CS do barramento compartilhado começa em alto, para ninguém responder fora da vez.
void parkChipSelects() {
  const int8_t chipSelects[] = {
      pins::kDisplayCs,
      pins::kCc1101Cs,
      pins::kSdCs,
#if defined(DBMO_PROFILE_TARGET)
      pins::kCartridgeCs,
#endif
  };
  for (int8_t cs : chipSelects) {
    pinMode(static_cast<uint8_t>(cs), OUTPUT);
    digitalWrite(static_cast<uint8_t>(cs), HIGH);
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);

  dbmo::logger().setClock(&gClock);
  (void)dbmo::logger().addSink(&gSerialLog);
  (void)dbmo::logger().addSink(&gLogHistory);

  parkChipSelects();
  SPI.begin(pins::kSpiSck, pins::kSpiMiso, pins::kSpiMosi);
  Wire.begin(pins::kI2cSda, pins::kI2cScl, dbmo::config::i2c::kBusHz);

  // O cartão precisa ser o primeiro dispositivo iniciado no SPI compartilhado
  // (docs/hardware/sources/PERIPHERALS.md). Sem cartão, segue sem ele.
  const dbmo::Status card = gCard.mount();
  DBMO_LOGI(kTag, "cartao: %s", dbmo::toString(card));

#if defined(DBMO_PROFILE_TARGET)
  if (!gBacklight.begin()) DBMO_LOGW(kTag, "PWM do backlight nao iniciou");
#endif
  gDisplay.begin(dbmo::config::spi::kDisplayHz, kDisplayRotation);
  (void)gDisplay.fillRect(0, 0, static_cast<int16_t>(gDisplay.width()),
                          static_cast<int16_t>(gDisplay.height()), dbmo::display::palette::kBlack);
  DBMO_LOGI(kTag, "tela: id 0x%04X, %ux%u", gDisplay.readId(), gDisplay.width(), gDisplay.height());

  (void)gConsole.registerCommand({"version", "versao do firmware", versionCommand, nullptr});
  (void)gConsole.registerCommand({"log", "ultimas mensagens de log", logCommand, nullptr});
  (void)gApp.registerCommands(gConsole);
  gConsole.setEcho(true);

  const dbmo::Status status = gApp.begin();
  if (!dbmo::isOk(status)) DBMO_LOGE(kTag, "inicializacao falhou: %s", dbmo::toString(status));
}

void loop() {
  while (Serial.available() > 0) gConsole.feed(static_cast<char>(Serial.read()));
  gApp.loop();
}
