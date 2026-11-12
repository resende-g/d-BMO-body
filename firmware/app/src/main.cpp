// Firmware próprio do d-BMO v1: ponto de entrada no ESP32-S3 (Arduino).
// Aqui só se criam os objetos do porte e se ligam uns aos outros; a lógica está nas
// bibliotecas portáveis de firmware/, testadas no computador.
#include <Adafruit_ILI9341.h>
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

#include "dbmo/app/console.h"
#include "dbmo/config/pins.h"
#include "dbmo/core/event_bus.h"
#include "dbmo/core/log.h"
#include "dbmo/core/system.h"
#include "dbmo/core/version.h"
#include "dbmo/display/display.h"
#include "dbmo/platform/esp32/hal.h"
#include "dbmo/platform/esp32/ili9341_display.h"

namespace {

namespace pins = dbmo::config::target;
namespace esp32 = dbmo::platform::esp32;

constexpr char kTag[] = "app";
// Tela na horizontal (320 x 240), como nos sketches de bancada.
constexpr uint8_t kDisplayRotation = 1;

class SerialConsoleOutput final : public dbmo::app::IConsoleOutput {
 public:
  void write(const char *text) override { Serial.print(text); }
};

esp32::ArduinoClock gClock;
esp32::ArduinoGpio gGpio;
esp32::PortCriticalSection gEventLock;
esp32::SerialLogSink gSerialLog(Serial);
dbmo::LogHistory<32> gLogHistory;

dbmo::EventBus gBus(&gClock, &gEventLock);
dbmo::System gSystem(gClock, gBus);

Adafruit_ILI9341 gTft(&SPI, pins::kDisplayDc, pins::kDisplayCs, pins::kDisplayReset);
// Na bancada de hoje o LED da tela está direto no 3V3: sem controle de backlight.
esp32::Ili9341Display gDisplay(gTft, gClock, nullptr);

SerialConsoleOutput gConsoleOutput;
dbmo::app::Console gConsole(gConsoleOutput);

void versionCommand(dbmo::app::Console &console, int, const char *const *, void *) {
  char version[12];
  dbmo::formatVersion(dbmo::kFirmwareVersion, version, sizeof(version));
  console.printf("%s firmware %s\r\n", dbmo::kHardwareName, version);
}

void statusCommand(dbmo::app::Console &console, int, const char *const *, void *) {
  console.printf("sistema: %s\r\n", dbmo::toString(gSystem.state()));
  for (size_t i = 0; i < gSystem.subsystemCount(); ++i) {
    const dbmo::SubsystemInfo &info = gSystem.info(i);
    console.printf("  %-12s %-13s %s\r\n", info.subsystem->name(), dbmo::toString(info.state),
                   dbmo::toString(info.lastStatus));
  }
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
  for (int8_t cs : {pins::kDisplayCs, pins::kCc1101Cs, pins::kSdCs}) {
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

  gDisplay.begin(dbmo::config::spi::kDisplayHz, kDisplayRotation);
  (void)gDisplay.fillRect(0, 0, static_cast<int16_t>(gDisplay.width()),
                          static_cast<int16_t>(gDisplay.height()), dbmo::display::palette::kBlack);
  DBMO_LOGI(kTag, "tela: id 0x%04X, %ux%u", gDisplay.readId(), gDisplay.width(), gDisplay.height());

  (void)gConsole.registerCommand({"version", "versao do firmware", versionCommand, nullptr});
  (void)gConsole.registerCommand({"status", "estado dos subsistemas", statusCommand, nullptr});
  (void)gConsole.registerCommand({"log", "ultimas mensagens de log", logCommand, nullptr});
  gConsole.setEcho(true);

  const dbmo::Status status = gSystem.boot();
  DBMO_LOGI(kTag, "boot: %s, sistema %s", dbmo::toString(status), dbmo::toString(gSystem.state()));
}

void loop() {
  while (Serial.available() > 0) gConsole.feed(static_cast<char>(Serial.read()));
  gSystem.tick();
}
