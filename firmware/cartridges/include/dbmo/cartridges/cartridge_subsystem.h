// Gerência do conector de cartucho: detecta inserção e remoção consultando a EEPROM de
// identificação, valida o cabeçalho, confere a compatibilidade e entrega o cartucho ao
// driver registrado para ele.
#pragma once

#include <cstddef>
#include <cstdint>

#include "dbmo/cartridges/cartridge_driver.h"
#include "dbmo/cartridges/cartridge_header.h"
#include "dbmo/cartridges/eeprom_24c02.h"
#include "dbmo/core/config_store.h"
#include "dbmo/core/event_bus.h"
#include "dbmo/core/subsystem.h"

namespace dbmo::cartridges {

enum class SlotState : uint8_t {
  Empty,       // nada responde no endereço da EEPROM
  Unknown,     // há algo no conector, mas sem cabeçalho válido (EEPROM em branco ou inválida)
  Rejected,    // identificado, mas incompatível com este aparelho
  Identified,  // válido e compatível, sem driver registrado para ele
  Active,      // driver em execução
  Faulted,     // o driver falhou e foi parado
};

const char *toString(SlotState state);

namespace cartridge_event {
// O estado do conector mudou. a = (vendorId << 16) | productId (0 se desconhecido),
// b = SlotState novo.
inline constexpr EventId kStateChanged = makeEventId(EventDomain::Cartridge, 1);
// Cartucho recusado. a = ids como acima, b = Compatibility.
inline constexpr EventId kRejected = makeEventId(EventDomain::Cartridge, 2);
// Driver parado por falha. a = ids, b = Status.
inline constexpr EventId kFault = makeEventId(EventDomain::Cartridge, 3);
}  // namespace cartridge_event

namespace config_key {
// Corrente máxima que o conector fornece ao cartucho, em mA. PROVISÓRIO: o circuito de
// proteção do conector ainda não foi definido.
inline constexpr char kMaxCurrentMa[] = "cartridge.max_ma";
}  // namespace config_key

class CartridgeSubsystem final : public ISubsystem {
 public:
  static constexpr uint32_t kPollPeriodMs = 500;
  // Tentativas de leitura do cabeçalho antes de desistir desta inserção.
  static constexpr uint8_t kReadAttempts = 3;
  // Consultas seguidas sem resposta até considerar o cartucho removido.
  static constexpr uint8_t kMissesToRemove = 2;

  CartridgeSubsystem(Eeprom24c02 &eeprom, CartridgeRegistry &registry, EventBus &bus,
                     ConfigStore &config, ResourceRegistry &resources);

  static Status registerConfig(ConfigStore &config);

  // Barramentos do conector que podem ser entregues a um driver (nullptr = indisponível).
  void setBuses(II2cBus *i2c, ISpiDevice *spi, ISerialPort *uart);
  // O que o aparelho oferece. Padrão: I2C, SPI, UART e os dois GPIOs; sem linha de 5 V.
  void setHostCapabilities(uint16_t capabilities) { hostCapabilities_ = capabilities; }

  const char *name() const override { return "cartridges"; }
  // Conector vazio é o estado normal: init() devolve Ok com o estado Empty.
  Status init() override;
  void tick(uint32_t nowMs) override;
  Status shutdown() override;

  SlotState state() const { return state_; }
  // Cabeçalho do cartucho presente; nullptr em Empty e Unknown.
  const CartridgeHeader *header() const;
  HeaderCheck lastHeaderCheck() const { return lastCheck_; }
  Compatibility lastCompatibility() const { return lastCompatibility_; }

 private:
  void poll(uint32_t nowMs);
  void identify();
  void remove();
  void stopDriver();
  void setState(SlotState state);
  uint32_t ids() const;

  Eeprom24c02 &eeprom_;
  CartridgeRegistry &registry_;
  EventBus &bus_;
  ConfigStore &config_;
  ResourceRegistry &resources_;
  II2cBus *i2c_ = nullptr;
  ISpiDevice *spi_ = nullptr;
  ISerialPort *uart_ = nullptr;
  uint16_t hostCapabilities_;

  SlotState state_ = SlotState::Empty;
  CartridgeHeader header_;
  bool headerValid_ = false;
  HeaderCheck lastCheck_ = HeaderCheck::Blank;
  Compatibility lastCompatibility_ = Compatibility::Compatible;
  ICartridgeDriver *driver_ = nullptr;
  CartridgeContext context_;
  bool initialized_ = false;
  bool polled_ = false;
  uint32_t lastPollMs_ = 0;
  uint8_t misses_ = 0;
  uint8_t readFailures_ = 0;
};

}  // namespace dbmo::cartridges
