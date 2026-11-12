#include "dbmo/app/application.h"

#include <cstdio>

#include "dbmo/config/pins.h"
#include "dbmo/core/log.h"
#include "dbmo/storage/file_utils.h"
#include "dbmo/storage/path.h"

namespace dbmo::app {

namespace {

constexpr char kTag[] = "app";

using rosto::Expression;

}  // namespace

const char *toString(RunState state) {
  switch (state) {
    case RunState::NotStarted: return "NotStarted";
    case RunState::Running: return "Running";
    case RunState::ShuttingDown: return "ShuttingDown";
    case RunState::PoweredOff: return "PoweredOff";
    case RunState::Halted: return "Halted";
  }
  return "?";
}

// ---- ConfigPersistence ----

ConfigPersistence::ConfigPersistence(ConfigStore &config, storage::StorageSubsystem &storage,
                                     IConfigBackend *fallback)
    : config_(config), storage_(storage), fallback_(fallback) {}

IConfigBackend *ConfigPersistence::backend() {
  if (storage_.available()) return &storage_.configBackend();
  return fallback_;
}

Status ConfigPersistence::init() {
  loadedFromCard_ = false;
  pending_ = false;
  retrying_ = false;
  IConfigBackend *source = backend();
  if (source == nullptr) {
    DBMO_LOGW(kTag, "sem onde guardar a configuracao: usando os padroes");
    return Status::Ok;
  }
  ConfigLoadReport report;
  const Status status = config_.load(*source, &report);
  loadedFromCard_ = storage_.available() && (isOk(status) || status == Status::Corrupt);
  if (status == Status::NotFound) {
    DBMO_LOGI(kTag, "nenhuma configuracao salva: usando os padroes");
    return Status::Ok;
  }
  if (status == Status::Corrupt) {
    // As linhas válidas foram aplicadas; o resto fica no padrão e o arquivo é regravado.
    DBMO_LOGW(kTag, "configuracao com %u linhas invalidas e %u valores fora da faixa",
              static_cast<unsigned>(report.malformedLines),
              static_cast<unsigned>(report.invalidValues));
    return Status::Ok;
  }
  if (!isOk(status)) {
    DBMO_LOGW(kTag, "falha ao ler a configuracao: %s", toString(status));
    return Status::Ok;  // segue com os padrões; não é motivo para parar o aparelho
  }
  DBMO_LOGI(kTag, "configuracao carregada (%u chaves, %s)", static_cast<unsigned>(report.applied),
            loadedFromCard_ ? "cartao" : "memoria interna");
  return Status::Ok;
}

Status ConfigPersistence::saveNow() {
  IConfigBackend *target = backend();
  if (target == nullptr) return Status::NotFound;
  const Status status = config_.save(*target);
  if (isOk(status)) pending_ = false;
  return status;
}

void ConfigPersistence::tick(uint32_t nowMs) {
  if (!config_.dirty()) {
    pending_ = false;
    return;
  }
  if (!pending_) {
    pending_ = true;
    retrying_ = false;
    dirtySinceMs_ = nowMs;
    return;
  }
  if (retrying_) {
    if (elapsedMs(nowMs, retryAtMs_) < kRetryDelayMs) return;
  } else if (elapsedMs(nowMs, dirtySinceMs_) < kSaveDelayMs) {
    return;
  }
  const Status status = saveNow();
  if (isOk(status)) {
    retrying_ = false;
    return;
  }
  if (!retrying_) DBMO_LOGW(kTag, "falha ao gravar a configuracao: %s", toString(status));
  retrying_ = true;
  retryAtMs_ = nowMs;
}

Status ConfigPersistence::shutdown() {
  if (!config_.dirty()) return Status::Ok;
  const Status status = saveNow();
  return status == Status::NotFound ? Status::Ok : status;
}

// ---- FileIrCodeStorage ----

Status FileIrCodeStorage::pathFor(const char *name, char *out, size_t capacity) const {
  char file[storage::kMaxComponent + 1];
  const int written = snprintf(file, sizeof(file), "%s.ir", name);
  if (written < 0 || static_cast<size_t>(written) >= sizeof(file)) return Status::InvalidArgument;
  return storage::joinPath(out, capacity, storage::kIrDir, file);
}

Status FileIrCodeStorage::save(const char *name, const char *text, size_t length) {
  if (!storage_.available()) return Status::NotInitialized;
  char path[storage::kMaxPath];
  DBMO_TRY(pathFor(name, path, sizeof(path)));
  return storage::writeFileAtomic(storage_.fileSystem(), path, text, length, false);
}

Result<size_t> FileIrCodeStorage::load(const char *name, char *buffer, size_t capacity) {
  if (!storage_.available()) return Status::NotInitialized;
  char path[storage::kMaxPath];
  const Status status = pathFor(name, path, sizeof(path));
  if (!isOk(status)) return status;
  return storage::readFile(storage_.fileSystem(), path, buffer, capacity);
}

Status FileIrCodeStorage::remove(const char *name) {
  if (!storage_.available()) return Status::NotInitialized;
  char path[storage::kMaxPath];
  DBMO_TRY(pathFor(name, path, sizeof(path)));
  if (!storage_.fileSystem().exists(path)) return Status::NotFound;
  return storage_.fileSystem().remove(path);
}

// ---- Application ----

Application::Application(const Hardware &hardware)
    : hw_(hardware),
      bus_(hardware.clock, hardware.eventLock),
      system_(*hardware.clock, bus_),
      power_(bus_, config_),
      storage_(hardware.fileSystem != nullptr ? *hardware.fileSystem : nullFs_, bus_, config_),
      persistence_(config_, storage_, hardware.fallbackConfig),
      display_(*hardware.display, bus_, config_),
      face_(*hardware.display, bus_, *hardware.clock),
      input_(bus_, config_),
      audio_(bus_, config_, *hardware.clock),
      infrared_(bus_, config_, *hardware.clock),
      pn532_(hardware.i2c != nullptr ? *hardware.i2c : nullI2c_, *hardware.clock,
             config::i2c::kPn532),
      nfc_(pn532_, bus_, config_, *hardware.clock),
      cc1101_(hardware.radioSpi != nullptr ? *hardware.radioSpi : nullSpi_, *hardware.clock),
      radio_(cc1101_, bus_, config_, *hardware.clock),
      cartridgeEeprom_(hardware.i2c != nullptr ? *hardware.i2c : nullI2c_, *hardware.clock,
                       config::i2c::kCartridgeEeprom),
      cartridges_(cartridgeEeprom_, cartridgeDrivers_, bus_, config_, resources_),
      irStorage_(storage_),
      irLibrary_(irStorage_) {}

Status Application::registerConfig() {
  DBMO_TRY(display::DisplaySubsystem::registerConfig(config_));
  DBMO_TRY(input::InputSubsystem::registerConfig(config_));
  DBMO_TRY(power::PowerSubsystem::registerConfig(config_));
  DBMO_TRY(storage::StorageSubsystem::registerConfig(config_));
  DBMO_TRY(audio::AudioSubsystem::registerConfig(config_));
  DBMO_TRY(infrared::InfraredSubsystem::registerConfig(config_));
  DBMO_TRY(nfc::NfcSubsystem::registerConfig(config_));
  DBMO_TRY(radio::RadioSubsystem::registerConfig(config_));
  DBMO_TRY(cartridges::CartridgeSubsystem::registerConfig(config_));
  return Status::Ok;
}

// Reações do rosto que não dependem do argumento do evento. As que dependem (nível da
// bateria, estado do cartucho) estão em handle().
Status Application::bindFace() {
  DBMO_TRY(face_.bind(nfc::nfc_event::kTagDetected, Expression::Success, 1500));
  DBMO_TRY(face_.bind(radio::radio_event::kPacketReceived, Expression::Success, 800));
  DBMO_TRY(face_.bind(radio::radio_event::kTxDenied, Expression::Alert, 1200));
  DBMO_TRY(face_.bind(infrared::infrared_event::kCodeReceived, Expression::Success, 600));
  DBMO_TRY(face_.bind(infrared::infrared_event::kLearned, Expression::Success, 1000));
  DBMO_TRY(face_.bind(infrared::infrared_event::kLearnTimeout, Expression::Alert, 1000));
  DBMO_TRY(face_.bind(infrared::infrared_event::kTxDenied, Expression::Alert, 1200));
  DBMO_TRY(face_.bind(storage::storage_event::kUnavailable, Expression::Warning, 2000));
  DBMO_TRY(face_.bind(system_event::kFault, Expression::Error, 2500));
  return Status::Ok;
}

// Registra o que o próprio aparelho ocupa, para um cartucho não poder reivindicar.
Status Application::claimResources() {
  DBMO_TRY(resources_.claim(ResourceKind::I2cAddress, config::i2c::kMcp23017Buttons, "input"));
  DBMO_TRY(resources_.claim(ResourceKind::I2cAddress, config::i2c::kMcp23017Aux, "input"));
  DBMO_TRY(resources_.claim(ResourceKind::I2cAddress, config::i2c::kPn532, "nfc"));
  DBMO_TRY(resources_.claim(ResourceKind::I2cAddress, config::i2c::kCartridgeEeprom, "cartridges"));
  return Status::Ok;
}

Status Application::begin() {
  if (runState_ != RunState::NotStarted) return Status::AlreadyInitialized;
  if (hw_.clock == nullptr || hw_.display == nullptr) return Status::InvalidArgument;

  DBMO_TRY(registerConfig());
  // A energia sobe antes do cartão; por isso a configuração da guarda alternativa é lida
  // já aqui. A do cartão, se houver, é aplicada pelo subsistema "config".
  if (hw_.fallbackConfig != nullptr) (void)config_.load(*hw_.fallbackConfig);
  DBMO_TRY(bindFace());
  DBMO_TRY(claimResources());
  DBMO_TRY(bus_.subscribeAll(&Application::onEvent, this));
  config_.setChangeObserver(&Application::onConfigChanged, this);
  (void)logger().addSink(&storage_.logSink());

  if (hw_.batteryAdc != nullptr) power_.setBatteryAdc(*hw_.batteryAdc);
  if (hw_.charger != nullptr) power_.setCharger(*hw_.charger);
  if (hw_.powerSwitch != nullptr) power_.setPowerSwitch(*hw_.powerSwitch);
  if (hw_.sleep != nullptr) power_.setSleepControl(*hw_.sleep);
  if (hw_.powerButtonGpio != nullptr && hw_.powerButtonPin >= 0)
    power_.setPowerButton(*hw_.powerButtonGpio, hw_.powerButtonPin);

  for (input::IButtonSource *source : hw_.buttonSources)
    if (source != nullptr) DBMO_TRY(input_.addButtonSource(*source));
  if (hw_.stickLeft != nullptr) input_.setStick(input::Stick::Left, *hw_.stickLeft);
  if (hw_.stickRight != nullptr) input_.setStick(input::Stick::Right, *hw_.stickRight);

  if (hw_.audioOutput != nullptr) audio_.setOutput(*hw_.audioOutput);
  if (hw_.audioInput != nullptr) audio_.setInput(*hw_.audioInput);
  if (hw_.irReceiver != nullptr) infrared_.setReceiver(*hw_.irReceiver);
  if (hw_.irTransmitter != nullptr) infrared_.setTransmitter(*hw_.irTransmitter);

  uint16_t capabilities = 0;
  if (hw_.i2c != nullptr) capabilities |= cartridges::capability::kI2c;
  if (hw_.cartridgeSpi != nullptr) capabilities |= cartridges::capability::kSpi;
  if (hw_.cartridgeUart != nullptr) capabilities |= cartridges::capability::kUart;
  cartridges_.setHostCapabilities(capabilities);
  cartridges_.setBuses(hw_.i2c, hw_.cartridgeSpi, hw_.cartridgeUart);

  // Ordem de inicialização: energia (segura a trava), cartão, configuração, tela e
  // rosto (obrigatórios), depois os periféricos opcionais.
  DBMO_TRY(system_.add(power_));
  DBMO_TRY(system_.add(storage_));
  DBMO_TRY(system_.add(persistence_));
  SubsystemOptions required;
  required.required = true;
  DBMO_TRY(system_.add(display_, required));
  SubsystemOptions faceOptions;
  faceOptions.required = true;
  faceOptions.dependsOn = &display_;
  DBMO_TRY(system_.add(face_, faceOptions));
  DBMO_TRY(system_.add(input_));
  DBMO_TRY(system_.add(audio_));
  DBMO_TRY(system_.add(infrared_));
  DBMO_TRY(system_.add(nfc_));
  DBMO_TRY(system_.add(radio_));
  DBMO_TRY(system_.add(cartridges_));

  const Status status = system_.boot();
  // A energia leu a configuração antes de o cartão ser montado.
  power_.applyConfig();
  lastRetryMs_ = hw_.clock->nowMs();
  runState_ = RunState::Running;
  DBMO_LOGI(kTag, "boot: %s, sistema %s", toString(status), toString(system_.state()));
  return status;
}

void Application::onEvent(const Event &event, void *context) {
  static_cast<Application *>(context)->handle(event);
}

void Application::onConfigChanged(const char *, void *context) {
  // Só marca: os subsistemas releem a configuração na próxima volta do laço, fora de
  // quem estava alterando a chave.
  static_cast<Application *>(context)->configChanged_ = true;
}

void Application::applyConfig() {
  power_.applyConfig();
  storage_.applyConfig();
  input_.applyConfig();
  audio_.applyConfig();
  infrared_.applyConfig();
  radio_.applyConfig();
}

void Application::handle(const Event &event) {
  const uint32_t now = event.timestampMs;
  switch (domainOf(event.id)) {
    case EventDomain::Input:
      // Qualquer uso dos controles conta como atividade.
      display_.notifyActivity(now);
      power_.notifyActivity(now);
      break;

    case EventDomain::Power:
      if (event.id == power::power_event::kShutdownRequested) {
        shutdownPending_ = true;
      } else if (event.id == power::power_event::kButtonPress) {
        display_.notifyActivity(now);
      } else if (event.id == power::power_event::kIdleTimeout) {
        // Aparelho parado pelo tempo configurado: desliga para poupar a bateria.
        power_.requestShutdown(power::ShutdownReason::Command);
      } else if (event.id == power::power_event::kBatteryLevel) {
        const auto level = static_cast<power::BatteryLevel>(event.a);
        if (level == power::BatteryLevel::Low) {
          (void)face_.engine().request(Expression::Warning, now, 3000);
        } else if (level == power::BatteryLevel::Critical) {
          (void)face_.engine().request(Expression::Alert, now, 3000);
        }
      }
      break;

    case EventDomain::Cartridge:
      if (event.id == cartridges::cartridge_event::kStateChanged) {
        const auto state = static_cast<cartridges::SlotState>(event.b);
        if (state == cartridges::SlotState::Active || state == cartridges::SlotState::Identified) {
          (void)face_.engine().request(Expression::Happy, now, 1500);
        } else if (state == cartridges::SlotState::Rejected ||
                   state == cartridges::SlotState::Faulted) {
          (void)face_.engine().request(Expression::Error, now, 2000);
        } else if (state == cartridges::SlotState::Unknown) {
          (void)face_.engine().request(Expression::Curious, now, 1500);
        }
      }
      break;

    default: break;
  }
}

// Expressão de fundo, decidida pelo que o aparelho está fazendo agora.
void Application::updateBaseExpression(uint32_t nowMs) {
  Expression wanted = Expression::Idle;
  if (shutdownPending_) {
    wanted = Expression::Sleep;
  } else if (system_.state() == SystemState::Fault) {
    wanted = Expression::Error;
  } else if (audio_.recording()) {
    wanted = Expression::Listening;
  } else if (infrared_.learning() || nfc_.state() == nfc::NfcState::Scanning ||
             radio_.state() == radio::RadioState::Receiving) {
    wanted = Expression::Scanning;
  }
  if (wanted == baseExpression_) return;
  baseExpression_ = wanted;
  (void)face_.engine().request(wanted, nowMs);
}

void Application::startShutdown() {
  DBMO_LOGI(kTag, "desligando: %s", power::toString(power_.shutdownReason()));
  const Status status = system_.shutdown();  // grava a configuração e o log pendentes
  if (!isOk(status)) DBMO_LOGW(kTag, "erro ao encerrar um subsistema: %s", toString(status));
  runState_ = RunState::ShuttingDown;
}

void Application::retryAbsent(uint32_t nowMs) {
  if (elapsedMs(nowMs, lastRetryMs_) < kRetryPeriodMs) return;
  lastRetryMs_ = nowMs;
  // Só os periféricos cuja nova tentativa é rápida. O cartão fica fora: montar um cartão
  // ausente pode levar mais de um segundo (retryStorage() faz isso a pedido).
  ISubsystem *const candidates[] = {&nfc_, &radio_};
  for (ISubsystem *candidate : candidates) {
    const SubsystemInfo *info = system_.find(*candidate);
    if (info == nullptr) continue;
    if (info->state == SubsystemState::Absent || info->state == SubsystemState::Failed)
      (void)system_.retry(*candidate);
  }
}

Status Application::retryStorage() {
  if (runState_ != RunState::Running) return Status::NotInitialized;
  if (storage_.available()) return Status::Ok;
  return system_.retry(storage_);
}

void Application::loop() {
  switch (runState_) {
    case RunState::Running:
      if (configChanged_) {
        configChanged_ = false;
        applyConfig();
      }
      system_.tick();
      // O fundo do rosto acompanha o que o aparelho está fazendo, inclusive o que começa
      // sem evento (aprendizado de infravermelho).
      updateBaseExpression(hw_.clock->nowMs());
      if (shutdownPending_) {
        startShutdown();
      } else {
        retryAbsent(hw_.clock->nowMs());
      }
      break;

    case RunState::ShuttingDown: {
      const Status status = power_.completeShutdown();
      if (isOk(status)) {
        runState_ = RunState::PoweredOff;
      } else if (status != Status::Busy) {
        // Sem trava e sem sono profundo nesta montagem: fica parado, com tudo encerrado.
        DBMO_LOGW(kTag, "nao foi possivel cortar a energia: %s", toString(status));
        runState_ = RunState::Halted;
      }
      break;
    }

    case RunState::NotStarted:
    case RunState::PoweredOff:
    case RunState::Halted: break;
  }
}

}  // namespace dbmo::app
