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
};
inline constexpr uint8_t kButtonCount = 10;

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

const char *toString(Button button);
const char *toString(ButtonAction action);

}  // namespace dbmo::input
