// Raiz de composição do firmware do d-BMO: cria os subsistemas, liga uns aos outros e
// conduz o laço principal. Só depende de interfaces, então é a mesma classe no ESP32-S3
// (firmware/app/src/main.cpp) e nos testes de integração, com simuladores no lugar do
// hardware.
#pragma once

#include <cstdint>

#include "dbmo/app/console.h"
#include "dbmo/app/null_hal.h"
#include "dbmo/audio/audio_subsystem.h"
#include "dbmo/cartridges/cartridge_subsystem.h"
#include "dbmo/core/config_store.h"
#include "dbmo/core/event_bus.h"
#include "dbmo/core/hal/adc.h"
#include "dbmo/core/hal/clock.h"
#include "dbmo/core/hal/critical_section.h"
#include "dbmo/core/hal/gpio.h"
#include "dbmo/core/resource_registry.h"
#include "dbmo/core/system.h"
#include "dbmo/display/display_subsystem.h"
#include "dbmo/infrared/infrared_subsystem.h"
#include "dbmo/infrared/ir_library.h"
#include "dbmo/input/input_subsystem.h"
#include "dbmo/nfc/nfc_subsystem.h"
#include "dbmo/power/power_subsystem.h"
#include "dbmo/radio/radio_subsystem.h"
#include "dbmo/rosto/face_subsystem.h"
#include "dbmo/storage/storage_subsystem.h"

namespace dbmo::app {

// O hardware que existe nesta montagem. Só o relógio e a tela são obrigatórios; todo o
// resto pode faltar (na bancada de hoje falta quase tudo).
struct Hardware {
  IClock *clock = nullptr;
  display::IDisplay *display = nullptr;
  ICriticalSection *eventLock = nullptr;
  II2cBus *i2c = nullptr;

  // Configuração: no cartão quando há cartão; senão, nesta guarda (NVS no ESP32).
  IConfigBackend *fallbackConfig = nullptr;
  storage::IFileSystem *fileSystem = nullptr;

  input::IButtonSource *buttonSources[input::InputSubsystem::kMaxSources] = {};
  input::StickProcessor *stickLeft = nullptr;
  input::StickProcessor *stickRight = nullptr;

  IAdcChannel *batteryAdc = nullptr;
  power::IChargerStatus *charger = nullptr;
  power::IPowerSwitch *powerSwitch = nullptr;
  power::ISleepControl *sleep = nullptr;
  IGpio *powerButtonGpio = nullptr;
  int powerButtonPin = -1;

  audio::IAudioOutput *audioOutput = nullptr;
  audio::IAudioInput *audioInput = nullptr;
  infrared::IIrReceiver *irReceiver = nullptr;
  infrared::IIrTransmitter *irTransmitter = nullptr;
  ISpiDevice *radioSpi = nullptr;
  ISpiDevice *cartridgeSpi = nullptr;
  cartridges::ISerialPort *cartridgeUart = nullptr;
};

// Guarda da configuração como subsistema: carrega logo depois do armazenamento, antes
// dos subsistemas que leem a configuração, e grava quando algo muda.
class ConfigPersistence final : public ISubsystem {
 public:
  // Espera depois da última alteração antes de gravar (várias mudanças viram uma gravação).
  static constexpr uint32_t kSaveDelayMs = 2000;
  static constexpr uint32_t kRetryDelayMs = 10000;

  ConfigPersistence(ConfigStore &config, storage::StorageSubsystem &storage,
                    IConfigBackend *fallback);

  const char *name() const override { return "config"; }
  Status init() override;
  void tick(uint32_t nowMs) override;
  Status shutdown() override;

  // Grava já, no cartão se houver, senão na guarda alternativa. NotFound se não há onde.
  Status saveNow();
  bool loadedFromCard() const { return loadedFromCard_; }

 private:
  IConfigBackend *backend();

  ConfigStore &config_;
  storage::StorageSubsystem &storage_;
  IConfigBackend *fallback_;
  bool loadedFromCard_ = false;
  bool pending_ = false;
  uint32_t dirtySinceMs_ = 0;
  uint32_t retryAtMs_ = 0;
  bool retrying_ = false;
};

// Códigos de infravermelho em /dbmo/ir/<nome>.ir.
class FileIrCodeStorage final : public infrared::IIrCodeStorage {
 public:
  explicit FileIrCodeStorage(storage::StorageSubsystem &storage) : storage_(storage) {}
  Status save(const char *name, const char *text, size_t length) override;
  Result<size_t> load(const char *name, char *buffer, size_t capacity) override;
  Status remove(const char *name) override;

 private:
  Status pathFor(const char *name, char *out, size_t capacity) const;
  storage::StorageSubsystem &storage_;
};

enum class RunState : uint8_t {
  NotStarted,
  Running,
  ShuttingDown,  // subsistemas encerrados; esperando o botão ser solto para cortar a energia
  PoweredOff,    // a trava foi solta (ou o chip entrou em sono profundo)
  Halted,        // desligamento pedido, mas esta montagem não tem como cortar a energia
};

const char *toString(RunState state);

class Application {
 public:
  // Intervalo entre tentativas de reiniciar NFC e rádio ausentes.
  static constexpr uint32_t kRetryPeriodMs = 15000;

  explicit Application(const Hardware &hardware);

  // Registra configuração, ligações e subsistemas e inicializa tudo. Só devolve erro se
  // falta hardware obrigatório ou se um subsistema obrigatório (tela, rosto) falhou.
  Status begin();
  // Uma volta do laço principal. Nunca bloqueia.
  void loop();

  RunState runState() const { return runState_; }

  // Drivers de cartucho: registrar antes de begin().
  Status addCartridgeDriver(cartridges::ICartridgeDriver &driver) {
    return cartridgeDrivers_.add(driver);
  }
  // Comandos do console serial.
  Status registerCommands(Console &console);
  // Tenta montar o cartão de novo (pode demorar: só a pedido do usuário).
  Status retryStorage();

  System &system() { return system_; }
  EventBus &bus() { return bus_; }
  ConfigStore &config() { return config_; }
  ResourceRegistry &resources() { return resources_; }
  ConfigPersistence &persistence() { return persistence_; }
  power::PowerSubsystem &power() { return power_; }
  storage::StorageSubsystem &storage() { return storage_; }
  display::DisplaySubsystem &display() { return display_; }
  rosto::FaceSubsystem &face() { return face_; }
  input::InputSubsystem &input() { return input_; }
  audio::AudioSubsystem &audio() { return audio_; }
  infrared::InfraredSubsystem &infrared() { return infrared_; }
  infrared::IrLibrary &irLibrary() { return irLibrary_; }
  nfc::NfcSubsystem &nfc() { return nfc_; }
  radio::RadioSubsystem &radio() { return radio_; }
  cartridges::CartridgeSubsystem &cartridges() { return cartridges_; }

 private:
  static void onEvent(const Event &event, void *context);
  static void onConfigChanged(const char *key, void *context);
  void handle(const Event &event);
  void applyConfig();
  Status registerConfig();
  Status bindFace();
  Status claimResources();
  void updateBaseExpression(uint32_t nowMs);
  void startShutdown();
  void retryAbsent(uint32_t nowMs);

  Hardware hw_;
  NullI2cBus nullI2c_;
  NullSpiDevice nullSpi_;
  NullFileSystem nullFs_;

  EventBus bus_;
  System system_;
  ConfigStore config_;
  ResourceRegistry resources_;

  power::PowerSubsystem power_;
  storage::StorageSubsystem storage_;
  ConfigPersistence persistence_;
  display::DisplaySubsystem display_;
  rosto::FaceSubsystem face_;
  input::InputSubsystem input_;
  audio::AudioSubsystem audio_;
  infrared::InfraredSubsystem infrared_;
  nfc::Pn532 pn532_;
  nfc::NfcSubsystem nfc_;
  radio::Cc1101 cc1101_;
  radio::RadioSubsystem radio_;
  cartridges::Eeprom24c02 cartridgeEeprom_;
  cartridges::CartridgeRegistry cartridgeDrivers_;
  cartridges::CartridgeSubsystem cartridges_;
  FileIrCodeStorage irStorage_;
  infrared::IrLibrary irLibrary_;

  RunState runState_ = RunState::NotStarted;
  bool shutdownPending_ = false;
  bool configChanged_ = false;
  rosto::Expression baseExpression_ = rosto::Expression::Idle;
  uint32_t lastRetryMs_ = 0;
};

}  // namespace dbmo::app
