// Modelo lógico dos controles do d-BMO, independente de como eles estão ligados.
#pragma once

#include <cstdint>

namespace dbmo::input {

enum class Button : uint8_t {
  DpadUp = 0,
  DpadDown,
  DpadLeft,
  DpadRight,
  A,
  B,
  X,
  Y,
  Select,
  Start,
  L1,
  R1,
  L2,
  R2,
  StickLeft,   // clique do analógico esquerdo
  StickRight,  // clique do analógico direito
};
inline constexpr uint8_t kButtonCount = 16;

// Um bit por Button.
using ButtonMask = uint32_t;
constexpr ButtonMask maskOf(Button button) { return 1u << static_cast<uint8_t>(button); }
inline constexpr ButtonMask kDpadMask = maskOf(Button::DpadUp) | maskOf(Button::DpadDown) |
                                        maskOf(Button::DpadLeft) | maskOf(Button::DpadRight);

enum class ButtonAction : uint8_t {
  Pressed,    // apertou (já sem repique)
  Released,   // soltou
  Click,      // apertou e soltou antes do tempo de toque longo
  LongPress,  // está segurado há mais que o tempo de toque longo (uma vez)
  Repeat,     // repetição automática enquanto segurado
};

enum class Stick : uint8_t { Left = 0, Right = 1 };

// Posição de um analógico, de -1000 a +1000 em cada eixo, já com zona morta.
struct StickPosition {
  int16_t x = 0;
  int16_t y = 0;
};

const char *toString(Button button);

}  // namespace dbmo::input
