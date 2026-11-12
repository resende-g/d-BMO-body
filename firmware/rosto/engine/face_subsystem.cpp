#include "dbmo/rosto/face_subsystem.h"

#include "dbmo/core/log.h"

namespace dbmo::rosto {

namespace {
constexpr char kTag[] = "rosto";
}

FaceSubsystem::FaceSubsystem(display::IDisplay &display, EventBus &bus, IClock &clock)
    : bus_(bus), clock_(clock), renderer_(display) {}

Status FaceSubsystem::bind(EventId id, Expression expression, uint32_t holdMs) {
  if (domainOf(id) == EventDomain::Face) return Status::InvalidArgument;
  if (static_cast<uint8_t>(expression) >= kExpressionCount) return Status::InvalidArgument;
  for (size_t i = 0; i < bindingCount_; ++i) {
    if (bindings_[i].id == id) {
      bindings_[i] = Binding{id, expression, holdMs};
      return Status::Ok;
    }
  }
  if (bindingCount_ == kMaxBindings) return Status::NoSpace;
  bindings_[bindingCount_++] = Binding{id, expression, holdMs};
  return Status::Ok;
}

Status FaceSubsystem::init() {
  if (!subscribed_) {
    DBMO_TRY(bus_.subscribeAll(&FaceSubsystem::onEvent, this));
    subscribed_ = true;
  }
  const uint32_t now = clock_.nowMs();
  engine_.reset(now);
  announced_ = engine_.current();
  renderer_.invalidate();
  redrawPending_ = false;
  drawFailed_ = false;
  // O primeiro desenho acontece aqui para que a falha da tela apareça já no boot.
  const Status status = renderer_.draw(engine_.currentFrame());
  if (isOk(status)) drawnRevision_ = engine_.revision();
  return status;
}

void FaceSubsystem::onEvent(const Event &event, void *context) {
  static_cast<FaceSubsystem *>(context)->handle(event);
}

void FaceSubsystem::handle(const Event &event) {
  if (event.id == face_event::kRequest) {
    const Result<Expression> expression = expressionFromIndex(event.a);
    if (!expression.ok()) {
      DBMO_LOGW(kTag, "pedido de expressao invalida: %lu", static_cast<unsigned long>(event.a));
      return;
    }
    // NotPermitted aqui só significa que há algo mais prioritário na tela.
    (void)engine_.request(expression.value(), clock_.nowMs(), event.b);
    return;
  }
  for (size_t i = 0; i < bindingCount_; ++i) {
    if (bindings_[i].id == event.id) {
      (void)engine_.request(bindings_[i].expression, clock_.nowMs(), bindings_[i].holdMs);
      return;
    }
  }
}

void FaceSubsystem::tick(uint32_t nowMs) {
  engine_.tick(nowMs);

  if (engine_.current() != announced_) {
    (void)bus_.publish(face_event::kChanged, static_cast<uint32_t>(engine_.current()),
                       static_cast<uint32_t>(announced_));
    announced_ = engine_.current();
  }

  if (!redrawPending_ && engine_.revision() == drawnRevision_) return;
  if (drawFailed_ && elapsedMs(nowMs, lastAttemptMs_) < kRedrawRetryMs) return;

  lastAttemptMs_ = nowMs;
  const uint32_t revision = engine_.revision();
  const Status status = renderer_.draw(engine_.currentFrame());
  if (isOk(status)) {
    drawnRevision_ = revision;
    redrawPending_ = false;
    drawFailed_ = false;
    return;
  }
  if (!drawFailed_) DBMO_LOGE(kTag, "falha ao desenhar: %s", toString(status));
  drawFailed_ = true;
  redrawPending_ = true;
}

Status FaceSubsystem::shutdown() {
  if (subscribed_) {
    (void)bus_.unsubscribe(&FaceSubsystem::onEvent, this);
    subscribed_ = false;
  }
  return Status::Ok;
}

}  // namespace dbmo::rosto
