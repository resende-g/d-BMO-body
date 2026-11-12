// Máquina de estados do rosto. Decide qual expressão e qual quadro mostrar; não conhece
// tela nem os outros subsistemas, e nunca bloqueia: tudo anda por tick(nowMs).
//
// Há uma expressão de base, que fica até ser trocada (IDLE, LISTENING, OFFLINE...), e
// uma expressão transitória opcional por cima dela, que dura um tempo e devolve o lugar
// à base (SUCCESS, ERROR, HAPPY...). O rosto só muda por pedido explícito: não há humor
// autônomo.
#pragma once

#include <cstdint>

#include "dbmo/core/status.h"
#include "dbmo/rosto/expressions.h"

namespace dbmo::rosto {

struct ExpressionPolicy {
  // Uma transitória só substitui outra transitória de prioridade menor ou igual.
  uint8_t priority;
  // Duração padrão quando pedida sem tempo. 0 = persistente: o pedido troca a base.
  uint16_t defaultHoldMs;
  // false: a animação toca uma vez e para no último quadro.
  bool loop;
};

class FaceEngine {
 public:
  // Passado como holdMs: a transitória fica até clearOverlay() ou outro pedido.
  static constexpr uint32_t kHoldForever = 0xFFFFFFFFu;

  FaceEngine();

  // Volta ao estado inicial (base IDLE, sem transitória) no instante nowMs.
  void reset(uint32_t nowMs);

  // Pede uma expressão. holdMs == 0 usa o padrão dela. Se o resultado for persistente,
  // ela vira a base (visível assim que não houver transitória). Se for transitória, é
  // mostrada já, salvo se a transitória atual tiver prioridade maior (NotPermitted).
  Status request(Expression expression, uint32_t nowMs, uint32_t holdMs = 0);

  // Encerra a transitória atual, se houver.
  void clearOverlay(uint32_t nowMs);

  // Avança o tempo: encerra a transitória vencida e troca de quadro.
  void tick(uint32_t nowMs);

  Expression current() const { return current_; }
  Expression base() const { return base_; }
  bool overlayActive() const { return overlayActive_; }
  uint8_t frameIndex() const { return frameIndex_; }
  FrameView currentFrame() const { return frame(current_, frameIndex_); }
  // Muda a cada troca de expressão ou de quadro: o renderizador redesenha quando muda.
  uint32_t revision() const { return revision_; }

  const ExpressionPolicy &policy(Expression expression) const;
  Status setPolicy(Expression expression, const ExpressionPolicy &policy);

 private:
  void show(Expression expression, uint32_t nowMs);

  ExpressionPolicy policies_[kExpressionCount];
  Expression base_ = Expression::Idle;
  Expression overlay_ = Expression::Idle;
  Expression current_ = Expression::Idle;
  bool overlayActive_ = false;
  uint32_t overlayStartMs_ = 0;
  uint32_t overlayHoldMs_ = 0;
  uint8_t frameIndex_ = 0;
  uint32_t frameStartMs_ = 0;
  uint32_t revision_ = 0;
};

}  // namespace dbmo::rosto
