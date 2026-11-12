#include "dbmo/cartridges/cartridge_subsystem.h"

#include "dbmo/core/hal/clock.h"
#include "dbmo/core/log.h"

namespace dbmo::cartridges {

namespace {
constexpr char kTag[] = "cartucho";
// Posição de vendorId, productId e hardwareRevision no cabeçalho.
constexpr size_t kIdentityOffset = 6;
constexpr size_t kIdentitySize = 5;
}  // namespace

const char *toString(SlotState state) {
  switch (state) {
    case SlotState::Empty: return "Empty";
    case SlotState::Unknown: return "Unknown";
    case SlotState::Rejected: return "Rejected";
    case SlotState::Identified: return "Identified";
    case SlotState::Active: return "Active";
    case SlotState::Faulted: return "Faulted";
  }
  return "?";
}

Status CartridgeRegistry::add(ICartridgeDriver &driver) {
  for (size_t i = 0; i < count_; ++i)
    if (drivers_[i] == &driver) return Status::AlreadyInitialized;
  if (count_ == kMaxDrivers) return Status::NoSpace;
  drivers_[count_++] = &driver;
  return Status::Ok;
}

ICartridgeDriver *CartridgeRegistry::find(const CartridgeHeader &header) const {
  for (size_t i = 0; i < count_; ++i)
    if (drivers_[i]->matches(header)) return drivers_[i];
  return nullptr;
}

CartridgeSubsystem::CartridgeSubsystem(Eeprom24c02 &eeprom, CartridgeRegistry &registry,
                                       EventBus &bus, ConfigStore &config,
                                       ResourceRegistry &resources)
    : eeprom_(eeprom),
      registry_(registry),
      bus_(bus),
      config_(config),
      resources_(resources),
      hostCapabilities_(capability::kI2c | capability::kSpi | capability::kUart |
                        capability::kGpio0 | capability::kGpio1) {}

Status CartridgeSubsystem::registerConfig(ConfigStore &config) {
  return config.define(ConfigSpec::integer(config_key::kMaxCurrentMa, 100, 0, 500));
}

void CartridgeSubsystem::setBuses(II2cBus *i2c, ISpiDevice *spi, ISerialPort *uart) {
  i2c_ = i2c;
  spi_ = spi;
  uart_ = uart;
}

const CartridgeHeader *CartridgeSubsystem::header() const {
  return headerValid_ ? &header_ : nullptr;
}

uint32_t CartridgeSubsystem::ids() const {
  if (!headerValid_) return 0;
  return (static_cast<uint32_t>(header_.vendorId) << 16) | header_.productId;
}

void CartridgeSubsystem::setState(SlotState state) {
  if (state == state_) return;
  state_ = state;
  DBMO_LOGI(kTag, "conector: %s", toString(state));
  (void)bus_.publish(cartridge_event::kStateChanged, ids(), static_cast<uint32_t>(state));
}

Status CartridgeSubsystem::init() {
  state_ = SlotState::Empty;
  headerValid_ = false;
  driver_ = nullptr;
  misses_ = 0;
  readFailures_ = 0;
  polled_ = false;
  initialized_ = true;
  return Status::Ok;
}

void CartridgeSubsystem::stopDriver() {
  if (driver_ == nullptr) return;
  driver_->stop();
  // Devolve o que o driver reservou (pinos, endereços).
  resources_.releaseAll(driver_->name());
  driver_ = nullptr;
}

void CartridgeSubsystem::remove() {
  stopDriver();
  setState(SlotState::Empty);  // publica ainda com os ids do cartucho que saiu
  headerValid_ = false;
  misses_ = 0;
  readFailures_ = 0;
  unknownPolls_ = 0;
}

void CartridgeSubsystem::identify() {
  uint8_t raw[kHeaderSize];
  const Status status = eeprom_.read(0, raw, sizeof(raw));
  if (!isOk(status)) {
    // Leitura falhou (mau contato durante a inserção): tenta de novo nas próximas voltas.
    if (++readFailures_ >= kReadAttempts) {
      DBMO_LOGW(kTag, "identificacao ilegivel: %s", toString(status));
      lastCheck_ = HeaderCheck::BadCrc;
      setState(SlotState::Unknown);
    }
    return;
  }
  readFailures_ = 0;

  lastCheck_ = decodeHeader(raw, sizeof(raw), &header_);
  if (lastCheck_ != HeaderCheck::Valid) {
    headerValid_ = false;
    DBMO_LOGW(kTag, "sem identificacao valida: %s", toString(lastCheck_));
    setState(SlotState::Unknown);
    return;
  }
  headerValid_ = true;
  DBMO_LOGI(kTag, "cartucho %04X:%04X \"%s\"", header_.vendorId, header_.productId, header_.name);

  const HostCapabilities host{
      hostCapabilities_,
      static_cast<uint16_t>(config_.getInt(config_key::kMaxCurrentMa).valueOr(0)),
      kFirmwareVersion,
      kDefaultBusyI2cAddresses,
      kDefaultBusyI2cCount,
  };
  lastCompatibility_ = checkCompatibility(header_, host);
  if (lastCompatibility_ != Compatibility::Compatible) {
    DBMO_LOGW(kTag, "cartucho recusado: %s", toString(lastCompatibility_));
    (void)bus_.publish(cartridge_event::kRejected, ids(),
                       static_cast<uint32_t>(lastCompatibility_));
    setState(SlotState::Rejected);
    return;
  }

  ICartridgeDriver *driver = registry_.find(header_);
  if (driver == nullptr) {
    setState(SlotState::Identified);  // conhecido e compatível, mas ninguém sabe usá-lo
    return;
  }

  context_ = CartridgeContext{};
  context_.header = &header_;
  context_.bus = &bus_;
  context_.resources = &resources_;
  if ((header_.capabilities & capability::kI2c) != 0) context_.i2c = i2c_;
  if ((header_.capabilities & capability::kSpi) != 0) context_.spi = spi_;
  if ((header_.capabilities & capability::kUart) != 0) context_.uart = uart_;

  driver_ = driver;
  const Status started = driver->start(context_);
  if (isOk(started)) {
    setState(SlotState::Active);
    return;
  }
  DBMO_LOGE(kTag, "driver %s nao iniciou: %s", driver->name(), toString(started));
  stopDriver();
  (void)bus_.publish(cartridge_event::kFault, ids(), static_cast<uint32_t>(started));
  setState(SlotState::Faulted);
}

void CartridgeSubsystem::poll(uint32_t) {
  const bool present = isOk(eeprom_.probe());
  if (!present) {
    if (state_ == SlotState::Empty) {
      readFailures_ = 0;
      return;
    }
    if (++misses_ >= kMissesToRemove) remove();
    return;
  }
  misses_ = 0;

  if (state_ == SlotState::Empty) {
    identify();
    return;
  }
  if (!headerValid_) {
    // Há uma EEPROM, mas a leitura não trouxe um cabeçalho válido. Pode ter sido mau
    // contato durante a inserção: relê de tempos em tempos em vez de desistir.
    if (state_ == SlotState::Unknown && ++unknownPolls_ >= kUnknownRetryPolls) {
      unknownPolls_ = 0;
      identify();
    }
    return;
  }

  // Troca rápida de cartucho entre duas consultas: a identidade gravada mudou.
  uint8_t identity[kIdentitySize];
  if (!isOk(eeprom_.read(kIdentityOffset, identity, sizeof(identity)))) return;
  const uint16_t vendor = static_cast<uint16_t>(identity[0] | (identity[1] << 8));
  const uint16_t product = static_cast<uint16_t>(identity[2] | (identity[3] << 8));
  if (vendor != header_.vendorId || product != header_.productId ||
      identity[4] != header_.hardwareRevision) {
    remove();
    identify();
  }
}

void CartridgeSubsystem::tick(uint32_t nowMs) {
  if (!initialized_) return;

  if (!polled_ || elapsedMs(nowMs, lastPollMs_) >= kPollPeriodMs) {
    polled_ = true;
    lastPollMs_ = nowMs;
    poll(nowMs);
  }

  if (state_ == SlotState::Active && driver_ != nullptr) {
    const Status status = driver_->tick(nowMs);
    if (!isOk(status) && !isOk(eeprom_.probe())) {
      // O cartucho está saindo do conector: o erro do driver é consequência da remoção,
      // não uma falha dele. A consulta periódica conclui a remoção.
      return;
    }
    if (!isOk(status)) {
      // Falha do driver fica contida no cartucho: ele para e o resto do sistema segue.
      DBMO_LOGE(kTag, "driver %s falhou: %s", driver_->name(), toString(status));
      stopDriver();
      (void)bus_.publish(cartridge_event::kFault, ids(), static_cast<uint32_t>(status));
      setState(SlotState::Faulted);
    }
  }
}

Status CartridgeSubsystem::shutdown() {
  stopDriver();
  initialized_ = false;
  return Status::Ok;
}

}  // namespace dbmo::cartridges
