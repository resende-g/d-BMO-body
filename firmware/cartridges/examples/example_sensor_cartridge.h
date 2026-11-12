// Exemplo de driver de cartucho: um sensor I2C com um registrador de 8 bits. O sensor é
// fictício: este arquivo serve de modelo para drivers reais e é exercitado nos testes
// com um dispositivo simulado, sem hardware.
//
// Para criar um cartucho novo: (1) grave na EEPROM dele um cabeçalho com vendorId e
// productId próprios (ferramenta dbmo_cartridge_image); (2) escreva uma classe como
// esta; (3) registre-a no CartridgeRegistry na inicialização. Nada mais muda.
#pragma once

#include <cstdint>

#include "dbmo/cartridges/cartridge_driver.h"
#include "dbmo/core/hal/clock.h"

namespace dbmo::cartridges::examples {

class ExampleSensorCartridge final : public ICartridgeDriver {
 public:
  static constexpr uint16_t kVendorId = 0xFFFF;  // faixa de experimentos (ver a especificação)
  static constexpr uint16_t kProductId = 0x0001;
  static constexpr uint8_t kSensorAddress = 0x48;
  static constexpr uint8_t kValueRegister = 0x00;
  static constexpr uint32_t kSamplePeriodMs = 1000;
  // Evento com a leitura: a = valor lido.
  static constexpr EventId kSampleEvent = makeEventId(EventDomain::Cartridge, 0x40);

  // O cabeçalho que este cartucho traz gravado.
  static CartridgeHeader makeHeader() {
    CartridgeHeader header;
    header.vendorId = kVendorId;
    header.productId = kProductId;
    header.hardwareRevision = 1;
    header.cartridgeVersion = Version{1, 0, 0};
    header.minHostFirmware = Version{0, 1, 0};
    header.capabilities = capability::kI2c;
    header.maxCurrentMa = 5;
    header.i2cAddressCount = 1;
    header.i2cAddresses[0] = kSensorAddress;
    const char name[] = "EXEMPLO SENSOR";
    for (size_t i = 0; i < sizeof(name); ++i) header.name[i] = name[i];
    return header;
  }

  const char *name() const override { return "example-sensor"; }

  bool matches(const CartridgeHeader &header) const override {
    return header.vendorId == kVendorId && header.productId == kProductId;
  }

  Status start(CartridgeContext &context) override {
    if (context.i2c == nullptr || context.bus == nullptr) return Status::InvalidArgument;
    context_ = &context;
    if (context.resources != nullptr)
      DBMO_TRY(context.resources->claim(ResourceKind::I2cAddress, kSensorAddress, name()));
    DBMO_TRY(context.i2c->probe(kSensorAddress));
    sampled_ = false;
    return Status::Ok;
  }

  Status tick(uint32_t nowMs) override {
    if (context_ == nullptr) return Status::NotInitialized;
    if (sampled_ && elapsedMs(nowMs, lastSampleMs_) < kSamplePeriodMs) return Status::Ok;
    sampled_ = true;
    lastSampleMs_ = nowMs;
    const uint8_t reg = kValueRegister;
    uint8_t value = 0;
    DBMO_TRY(context_->i2c->writeRead(kSensorAddress, &reg, 1, &value, 1));
    lastValue_ = value;
    (void)context_->bus->publish(kSampleEvent, value);
    return Status::Ok;
  }

  void stop() override { context_ = nullptr; }

  uint8_t lastValue() const { return lastValue_; }
  bool running() const { return context_ != nullptr; }

 private:
  CartridgeContext *context_ = nullptr;
  bool sampled_ = false;
  uint32_t lastSampleMs_ = 0;
  uint8_t lastValue_ = 0;
};

}  // namespace dbmo::cartridges::examples
