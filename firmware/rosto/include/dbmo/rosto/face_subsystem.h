// O rosto como subsistema: junta a máquina de estados e o renderizador, recebe pedidos
// pelo EventBus e redesenha a tela quando o quadro muda.
#pragma once

#include <cstddef>
#include <cstdint>

#include "dbmo/core/event_bus.h"
#include "dbmo/core/hal/clock.h"
#include "dbmo/core/subsystem.h"
#include "dbmo/display/display.h"
#include "dbmo/rosto/face_engine.h"
#include "dbmo/rosto/face_renderer.h"

namespace dbmo::rosto {

namespace face_event {
// Pedido de expressão. a = Expression, b = duração em ms (0 = padrão da expressão).
inline constexpr EventId kRequest = makeEventId(EventDomain::Face, 1);
// A expressão na tela mudou. a = expressão nova, b = expressão anterior.
inline constexpr EventId kChanged = makeEventId(EventDomain::Face, 2);
}  // namespace face_event

class FaceSubsystem final : public ISubsystem {
 public:
  static constexpr size_t kMaxBindings = 24;
  // Depois de um erro da tela, espera isto antes de tentar desenhar de novo.
  static constexpr uint32_t kRedrawRetryMs = 250;

  FaceSubsystem(display::IDisplay &display, EventBus &bus, IClock &clock);

  // Liga um evento de outro módulo a uma expressão: quando o evento chegar, o rosto
  // recebe request(expression, holdMs). Assim o rosto não precisa conhecer os outros
  // módulos. Eventos do próprio domínio Face não podem ser ligados (InvalidArgument).
  Status bind(EventId id, Expression expression, uint32_t holdMs = 0);

  const char *name() const override { return "rosto"; }
  Status init() override;
  void tick(uint32_t nowMs) override;
  Status shutdown() override;

  FaceEngine &engine() { return engine_; }
  // Força o redesenho completo no próximo tick (a tela foi usada por outra coisa).
  void invalidate() {
    renderer_.invalidate();
    redrawPending_ = true;
  }

 private:
  struct Binding {
    EventId id;
    Expression expression;
    uint32_t holdMs;
  };

  static void onEvent(const Event &event, void *context);
  void handle(const Event &event);

  EventBus &bus_;
  IClock &clock_;
  FaceEngine engine_;
  FaceRenderer renderer_;
  Binding bindings_[kMaxBindings] = {};
  size_t bindingCount_ = 0;
  bool subscribed_ = false;
  bool redrawPending_ = true;
  bool drawFailed_ = false;
  uint32_t lastAttemptMs_ = 0;
  uint32_t drawnRevision_ = 0;
  Expression announced_ = Expression::Idle;
};

}  // namespace dbmo::rosto
