// Contrato que todo subsistema implementa para ser gerenciado pelo System.
#pragma once

#include <cstdint>

#include "dbmo/core/status.h"

namespace dbmo {

enum class SubsystemState : uint8_t {
  Uninitialized,
  Ready,
  Degraded,  // funcionando com restrição (ex.: sem cartão, usando padrões)
  Absent,    // o hardware não respondeu; o sistema segue sem ele
  Failed,    // erro na inicialização ou em operação
  Stopped,   // desligado por shutdown()
};

const char *toString(SubsystemState state);

class ISubsystem {
 public:
  virtual ~ISubsystem() = default;

  virtual const char *name() const = 0;

  // Inicializa o módulo e o periférico. Devolver HardwareAbsent quando o periférico não
  // responde: o sistema registra a ausência e continua. Pode ser chamado de novo depois
  // de uma falha (retry), então não deve deixar estado pela metade.
  virtual Status init() = 0;

  // Um passo de trabalho, chamado a cada volta do laço principal. Nunca bloqueia.
  virtual void tick(uint32_t nowMs) = 0;

  // Leva o periférico a um estado seguro e de baixo consumo.
  virtual Status shutdown() = 0;
};

}  // namespace dbmo
