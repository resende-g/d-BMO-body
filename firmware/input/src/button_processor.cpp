#include "dbmo/input/button_processor.h"

#include "dbmo/core/hal/clock.h"

namespace dbmo::input {

namespace {
int bitCount(ButtonMask mask) {
  int count = 0;
  for (; mask != 0; mask &= mask - 1) ++count;
  return count;
}
}  // namespace

void ButtonProcessor::setSinks(EventSink onButton, ComboSink onCombo, void *context) {
  onButton_ = onButton;
  onCombo_ = onCombo;
  context_ = context;
}

Status ButtonProcessor::addCombo(const Combo &combo) {
  if (bitCount(combo.mask) < 2 || (combo.mask >> kButtonCount) != 0) return Status::InvalidArgument;
  for (size_t i = 0; i < comboCount_; ++i)
    if (combos_[i].combo.id == combo.id) return Status::AlreadyInitialized;
  if (comboCount_ == kMaxCombos) return Status::NoSpace;
  combos_[comboCount_++] = ComboState{combo, false, 0, 0, 0};
  return Status::Ok;
}

void ButtonProcessor::emit(Button button, ButtonAction action) {
  if (onButton_ != nullptr) onButton_(ButtonEvent{button, action}, context_);
}

void ButtonProcessor::update(ButtonMask raw, uint32_t nowMs) {
  raw &= (1u << kButtonCount) - 1;

  // 1. Repique: uma mudança só vale depois de debounceMs com a leitura estável.
  for (uint8_t i = 0; i < kButtonCount; ++i) {
    const ButtonMask bit = 1u << i;
    if ((raw & bit) != (lastRaw_ & bit)) rawChangedMs_[i] = nowMs;
    if ((raw & bit) == (stable_ & bit)) continue;
    if (elapsedMs(nowMs, rawChangedMs_[i]) < timing_.debounceMs) continue;

    const Button button = static_cast<Button>(i);
    if ((raw & bit) != 0) {
      stable_ |= bit;
      longFired_ &= ~bit;
      repeating_ &= ~bit;
      suppressed_ &= ~bit;
      pressedMs_[i] = nowMs;
      emit(button, ButtonAction::Pressed);
    } else {
      stable_ &= ~bit;
      emit(button, ButtonAction::Released);
      if ((longFired_ & bit) == 0 && (suppressed_ & bit) == 0) emit(button, ButtonAction::Click);
    }
  }
  lastRaw_ = raw;

  // 2. Combinações: antes do toque longo e da repetição, para poder suprimi-los.
  updateCombos(nowMs);

  // 3. Toque longo e repetição dos botões segurados.
  for (uint8_t i = 0; i < kButtonCount; ++i) {
    const ButtonMask bit = 1u << i;
    if ((stable_ & bit) == 0 || (suppressed_ & bit) != 0) continue;
    const Button button = static_cast<Button>(i);

    if ((repeatMask_ & bit) != 0) {
      if ((repeating_ & bit) == 0) {
        if (elapsedMs(nowMs, pressedMs_[i]) >= timing_.repeatDelayMs) {
          repeating_ |= bit;
          lastRepeatMs_[i] = nowMs;
          emit(button, ButtonAction::Repeat);
        }
      } else if (elapsedMs(nowMs, lastRepeatMs_[i]) >= timing_.repeatIntervalMs) {
        lastRepeatMs_[i] = nowMs;
        emit(button, ButtonAction::Repeat);
      }
    } else if ((longFired_ & bit) == 0 && elapsedMs(nowMs, pressedMs_[i]) >= timing_.longPressMs) {
      longFired_ |= bit;
      emit(button, ButtonAction::LongPress);
    }
  }
}

void ButtonProcessor::updateCombos(uint32_t nowMs) {
  for (size_t i = 0; i < comboCount_; ++i) {
    ComboState &state = combos_[i];
    const bool held = (stable_ & state.combo.mask) == state.combo.mask;
    if (!held) {
      state.held = false;
      state.fireCount = 0;
      continue;
    }
    if (!state.held) {
      state.held = true;
      state.heldSinceMs = nowMs;
    }

    bool fire = false;
    if (state.fireCount == 0) {
      fire = elapsedMs(nowMs, state.heldSinceMs) >= state.combo.holdMs;
    } else if (state.combo.repeat) {
      // A primeira repetição espera repeatDelayMs; as seguintes, repeatIntervalMs.
      const uint16_t wait = state.fireCount == 1 ? timing_.repeatDelayMs : timing_.repeatIntervalMs;
      fire = elapsedMs(nowMs, state.lastFireMs) >= wait;
    }
    if (!fire) continue;

    // Os botões da combinação deixam de valer sozinhos até serem soltos.
    suppressed_ |= state.combo.mask;
    state.lastFireMs = nowMs;
    if (state.fireCount < UINT16_MAX) ++state.fireCount;
    if (onCombo_ != nullptr) onCombo_(state.combo.id, context_);
  }
}

void ButtonProcessor::releaseAll(uint32_t nowMs) {
  for (uint8_t i = 0; i < kButtonCount; ++i) {
    const ButtonMask bit = 1u << i;
    rawChangedMs_[i] = nowMs;
    if ((stable_ & bit) != 0) emit(static_cast<Button>(i), ButtonAction::Released);
  }
  stable_ = 0;
  lastRaw_ = 0;
  longFired_ = 0;
  repeating_ = 0;
  suppressed_ = 0;
  for (size_t i = 0; i < comboCount_; ++i) {
    combos_[i].held = false;
    combos_[i].fireCount = 0;
  }
}

}  // namespace dbmo::input
