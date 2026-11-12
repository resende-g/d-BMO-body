// Modelo de driver de cartucho. Um cartucho novo entra no firmware como uma classe que
// implementa ICartridgeDriver e se registra no CartridgeRegistry: o núcleo e o
// CartridgeSubsystem não mudam.
#pragma once

#include <cstddef>
#include <cstdint>

#include "dbmo/cartridges/cartridge_header.h"
#include "dbmo/core/event_bus.h"
#include "dbmo/core/hal/i2c.h"
#include "dbmo/core/hal/spi.h"
#include "dbmo/core/resource_registry.h"
#include "dbmo/core/status.h"

namespace dbmo::cartridges {

// UART do conector do cartucho. Leitura e escrita não bloqueiam: devolvem quantos bytes
// foram transferidos.
class ISerialPort {
 public:
  virtual ~ISerialPort() = default;
  virtual Status configure(uint32_t baud) = 0;
  virtual Result<size_t> write(const uint8_t *data, size_t length) = 0;
  virtual Result<size_t> read(uint8_t *out, size_t capacity) = 0;
};

// O que um driver recebe ao iniciar. Só vêm preenchidos os barramentos que o cabeçalho
// do cartucho declarou; os demais ficam nullptr.
struct CartridgeContext {
  const CartridgeHeader *header = nullptr;
  II2cBus *i2c = nullptr;
  ISpiDevice *spi = nullptr;
  ISerialPort *uart = nullptr;
  EventBus *bus = nullptr;
  // Reservas feitas com name() como dono são liberadas quando o cartucho para.
  ResourceRegistry *resources = nullptr;
};

class ICartridgeDriver {
 public:
  virtual ~ICartridgeDriver() = default;
  // Também é o dono usado nas reservas do ResourceRegistry.
  virtual const char *name() const = 0;
  // Este driver sabe usar o cartucho descrito pelo cabeçalho?
  virtual bool matches(const CartridgeHeader &header) const = 0;
  virtual Status start(CartridgeContext &context) = 0;
  // Um passo de trabalho, sem bloquear. Devolver erro marca o cartucho como em falha.
  virtual Status tick(uint32_t nowMs) = 0;
  // Leva o cartucho a um estado seguro. Chamado na remoção, na falha e no desligamento.
  virtual void stop() = 0;
};

class CartridgeRegistry {
 public:
  static constexpr size_t kMaxDrivers = 8;

  Status add(ICartridgeDriver &driver);
  // Primeiro driver registrado que aceita o cabeçalho, ou nullptr.
  ICartridgeDriver *find(const CartridgeHeader &header) const;
  size_t size() const { return count_; }

 private:
  ICartridgeDriver *drivers_[kMaxDrivers] = {};
  size_t count_ = 0;
};

}  // namespace dbmo::cartridges
