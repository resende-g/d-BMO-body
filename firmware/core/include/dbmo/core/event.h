// Eventos trocados entre os subsistemas pelo EventBus.
//
// Um EventId tem dois bytes: domínio (quem publica) e código (definido pelo próprio
// módulo). Assim um módulo novo cria seus eventos sem alterar o núcleo.
#pragma once

#include <cstdint>

namespace dbmo {

using EventId = uint16_t;

enum class EventDomain : uint8_t {
  None = 0,
  System = 1,
  Input = 2,
  Power = 3,
  Audio = 4,
  Radio = 5,
  Nfc = 6,
  Infrared = 7,
  Storage = 8,
  Cartridge = 9,
  Face = 10,
  Display = 11,
  App = 12,
  // 0x80 em diante: módulos externos (cartuchos) registram o domínio em tempo de execução.
  ExternalBase = 0x80,
};

constexpr EventId makeEventId(EventDomain domain, uint8_t code) {
  return static_cast<EventId>((static_cast<uint16_t>(domain) << 8) | code);
}
constexpr EventDomain domainOf(EventId id) { return static_cast<EventDomain>(id >> 8); }
constexpr uint8_t codeOf(EventId id) { return static_cast<uint8_t>(id & 0xFF); }

// Carga pequena e copiável. Não guardar ponteiros: o evento é entregue depois, quando o
// dado original pode não existir mais.
struct Event {
  EventId id = 0;
  uint32_t timestampMs = 0;
  uint32_t a = 0;
  uint32_t b = 0;

  constexpr Event() = default;
  constexpr Event(EventId eventId, uint32_t argA = 0, uint32_t argB = 0)
      : id(eventId), a(argA), b(argB) {}
};

}  // namespace dbmo
