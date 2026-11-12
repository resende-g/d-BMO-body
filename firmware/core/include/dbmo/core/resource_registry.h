// Registro de posse de recursos compartilhados (GPIOs, endereços I2C, chip-selects).
// Cada subsistema reserva o que usa na inicialização; uma segunda reserva do mesmo
// recurso por outro dono falha, o que revela conflito de pinagem em tempo de execução.
#pragma once

#include <cstddef>
#include <cstdint>

#include "dbmo/core/status.h"

namespace dbmo {

enum class ResourceKind : uint8_t { Gpio, I2cAddress, SpiChipSelect, Peripheral };

const char *toString(ResourceKind kind);

class ResourceRegistry {
 public:
  static constexpr size_t kMaxClaims = 64;

  // Reserva o recurso para owner. Ok se estava livre ou já era do mesmo dono; Busy se é
  // de outro dono; NoSpace se a tabela encheu. owner precisa ter duração estática.
  Status claim(ResourceKind kind, uint16_t id, const char *owner);
  // NotFound se não estava reservado; NotPermitted se pertence a outro dono.
  Status release(ResourceKind kind, uint16_t id, const char *owner);
  // Libera tudo o que pertence a owner; devolve quantos recursos liberou.
  size_t releaseAll(const char *owner);

  // Dono atual, ou nullptr se livre.
  const char *ownerOf(ResourceKind kind, uint16_t id) const;
  size_t count() const { return count_; }

 private:
  struct Claim {
    ResourceKind kind;
    uint16_t id;
    const char *owner;
  };

  int find(ResourceKind kind, uint16_t id) const;

  Claim claims_[kMaxClaims] = {};
  size_t count_ = 0;
};

}  // namespace dbmo
