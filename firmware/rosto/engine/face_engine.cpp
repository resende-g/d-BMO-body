#include "dbmo/rosto/face_engine.h"

#include "dbmo/core/hal/clock.h"

namespace dbmo::rosto {

namespace {

// Padrões por expressão, na ordem do enum. Persistentes: defaultHoldMs = 0.
constexpr ExpressionPolicy kDefaultPolicies[kExpressionCount] = {
    {0, 0, true},     // Idle
    {1, 0, true},     // Thinking
    {1, 0, true},     // Scanning
    {1, 0, true},     // Analyzing
    {2, 1500, true},  // Success
    {3, 3000, true},  // Alert
    {4, 4000, true},  // Error
    {2, 0, true},     // Protecting
    {1, 2000, true},  // Curious
    {0, 0, true},     // Offline
    {1, 2000, true},  // Happy
    {1, 2000, true},  // Sad
    {1, 2000, true},  // Angry
    {2, 0, true},     // Listening
    {2, 0, true},     // Speaking
    {3, 3000, true},  // Warning
    {0, 0, true},     // Sleep
};

bool valid(Expression expression) { return static_cast<uint8_t>(expression) < kExpressionCount; }

}  // namespace

FaceEngine::FaceEngine() {
  for (uint8_t i = 0; i < kExpressionCount; ++i) policies_[i] = kDefaultPolicies[i];
}

void FaceEngine::reset(uint32_t nowMs) {
  base_ = Expression::Idle;
  overlayActive_ = false;
  current_ = Expression::Idle;
  frameIndex_ = 0;
  frameStartMs_ = nowMs;
  ++revision_;
}

const ExpressionPolicy &FaceEngine::policy(Expression expression) const {
  return policies_[valid(expression) ? static_cast<uint8_t>(expression) : 0];
}

Status FaceEngine::setPolicy(Expression expression, const ExpressionPolicy &policy) {
  if (!valid(expression)) return Status::InvalidArgument;
  policies_[static_cast<uint8_t>(expression)] = policy;
  return Status::Ok;
}

void FaceEngine::show(Expression expression, uint32_t nowMs) {
  if (expression == current_) return;  // já na tela: a animação segue de onde está
  current_ = expression;
  frameIndex_ = 0;
  frameStartMs_ = nowMs;
  ++revision_;
}

Status FaceEngine::request(Expression expression, uint32_t nowMs, uint32_t holdMs) {
  if (!valid(expression)) return Status::InvalidArgument;
  const ExpressionPolicy &requested = policy(expression);
  const uint32_t hold = holdMs != 0 ? holdMs : requested.defaultHoldMs;

  if (hold == 0) {
    base_ = expression;
    if (!overlayActive_) show(expression, nowMs);
    return Status::Ok;
  }

  if (overlayActive_ && requested.priority < policy(overlay_).priority) return Status::NotPermitted;

  overlay_ = expression;
  overlayActive_ = true;
  overlayStartMs_ = nowMs;
  overlayHoldMs_ = hold;
  show(expression, nowMs);
  return Status::Ok;
}

void FaceEngine::clearOverlay(uint32_t nowMs) {
  if (!overlayActive_) return;
  overlayActive_ = false;
  show(base_, nowMs);
}

void FaceEngine::tick(uint32_t nowMs) {
  if (overlayActive_ && overlayHoldMs_ != kHoldForever &&
      elapsedMs(nowMs, overlayStartMs_) >= overlayHoldMs_)
    clearOverlay(nowMs);

  const uint8_t count = frameCount(current_);
  if (count <= 1) return;

  // No máximo uma volta completa por tick: se o laço principal atrasou muito, a animação
  // retoma do quadro em que parar, sem tentar recuperar o tempo perdido.
  for (uint8_t step = 0; step < count; ++step) {
    const uint16_t duration = frame(current_, frameIndex_).durationMs;
    if (elapsedMs(nowMs, frameStartMs_) < duration) return;
    const bool last = frameIndex_ + 1 == count;
    if (last && !policy(current_).loop) return;
    frameIndex_ = last ? 0 : static_cast<uint8_t>(frameIndex_ + 1);
    frameStartMs_ += duration;
    ++revision_;
  }
  frameStartMs_ = nowMs;
}

}  // namespace dbmo::rosto
