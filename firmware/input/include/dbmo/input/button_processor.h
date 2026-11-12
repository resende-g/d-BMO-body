// Transforma a leitura bruta dos botões em eventos: sem repique, clique, toque longo,
// repetição e combinações. Lógica pura: o tempo entra por parâmetro.
#pragma once

#include <cstddef>
#include <cstdint>

#include "dbmo/core/status.h"
#include "dbmo/input/types.h"

namespace dbmo::input {

struct ButtonTiming {
  uint16_t debounceMs = 20;  // tempo que a leitura precisa ficar estável
  uint16_t longPressMs = 600;
  uint16_t repeatDelayMs = 400;  // espera antes da primeira repetição
  uint16_t repeatIntervalMs = 120;
};

struct ButtonEvent {
  Button button;
  ButtonAction action;
};

// Combinação: dispara quando todos os botões de mask estão segurados há holdMs.
struct Combo {
  uint8_t id;
  ButtonMask mask;
  uint16_t holdMs;
  bool repeat;  // true: volta a disparar no ritmo de repetição enquanto segurada
};

class ButtonProcessor {
 public:
  static constexpr size_t kMaxCombos = 8;

  using EventSink = void (*)(const ButtonEvent &event, void *context);
  using ComboSink = void (*)(uint8_t comboId, void *context);

  void setTiming(const ButtonTiming &timing) { timing_ = timing; }
  const ButtonTiming &timing() const { return timing_; }
  // Botões com repetição automática (não geram LongPress). Padrão: o D-pad.
  void setRepeatMask(ButtonMask mask) { repeatMask_ = mask; }
  void setSinks(EventSink onButton, ComboSink onCombo, void *context);

  // InvalidArgument para máscara com menos de dois botões; AlreadyInitialized para id
  // repetido; NoSpace quando a tabela enche.
  Status addCombo(const Combo &combo);

  // Entrega a leitura bruta deste instante (um bit por Button).
  void update(ButtonMask raw, uint32_t nowMs);

  // Solta tudo o que está apertado, com os eventos Released (sem Click). Usado quando a
  // origem dos botões falha, para não ficar tecla presa.
  void releaseAll(uint32_t nowMs);

  // Estado atual, já sem repique.
  ButtonMask pressed() const { return stable_; }
  bool isPressed(Button button) const { return (stable_ & maskOf(button)) != 0; }

 private:
  struct ComboState {
    Combo combo;
    bool held;
    uint16_t fireCount;  // quantas vezes disparou neste aperto
    uint32_t heldSinceMs;
    uint32_t lastFireMs;
  };

  void emit(Button button, ButtonAction action);
  void updateCombos(uint32_t nowMs);

  ButtonTiming timing_;
  ButtonMask repeatMask_ = kDpadMask;
  EventSink onButton_ = nullptr;
  ComboSink onCombo_ = nullptr;
  void *context_ = nullptr;

  ButtonMask lastRaw_ = 0;
  ButtonMask stable_ = 0;
  ButtonMask longFired_ = 0;
  ButtonMask repeating_ = 0;
  ButtonMask suppressed_ = 0;  // participou de uma combinação neste aperto
  uint32_t rawChangedMs_[kButtonCount] = {};
  uint32_t pressedMs_[kButtonCount] = {};
  uint32_t lastRepeatMs_[kButtonCount] = {};

  ComboState combos_[kMaxCombos] = {};
  size_t comboCount_ = 0;
};

}  // namespace dbmo::input
