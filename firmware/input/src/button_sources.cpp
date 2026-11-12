#include "dbmo/config/pins.h"
#include "dbmo/input/button_source.h"

namespace dbmo::input {

const char *toString(Button button) {
  switch (button) {
    case Button::DpadUp: return "up";
    case Button::DpadDown: return "down";
    case Button::DpadLeft: return "left";
    case Button::DpadRight: return "right";
    case Button::A: return "a";
    case Button::B: return "b";
    case Button::X: return "x";
    case Button::Y: return "y";
    case Button::Select: return "select";
    case Button::Start: return "start";
    case Button::L1: return "l1";
    case Button::R1: return "r1";
    case Button::L2: return "l2";
    case Button::R2: return "r2";
    case Button::StickLeft: return "l3";
    case Button::StickRight: return "r3";
  }
  return "?";
}

Mcp23017ButtonSource::Map Mcp23017ButtonSource::defaultMap() {
  namespace pins = dbmo::config::mcp_buttons;
  Map map;
  for (uint8_t &entry : map.buttonOfPin) entry = kUnmapped;
  auto assign = [&map](uint8_t pin, Button button) {
    map.buttonOfPin[pin] = static_cast<uint8_t>(button);
  };
  assign(pins::kDpadUp, Button::DpadUp);
  assign(pins::kDpadDown, Button::DpadDown);
  assign(pins::kDpadLeft, Button::DpadLeft);
  assign(pins::kDpadRight, Button::DpadRight);
  assign(pins::kActionA, Button::A);
  assign(pins::kActionB, Button::B);
  assign(pins::kActionX, Button::X);
  assign(pins::kActionY, Button::Y);
  assign(pins::kSelect, Button::Select);
  assign(pins::kStart, Button::Start);
  assign(pins::kL1, Button::L1);
  assign(pins::kR1, Button::R1);
  assign(pins::kL2, Button::L2);
  assign(pins::kR2, Button::R2);
  return map;
}

uint16_t Mcp23017ButtonSource::inputMask() const {
  uint16_t mask = 0;
  for (uint8_t pin = 0; pin < 16; ++pin)
    if (map_.buttonOfPin[pin] < kButtonCount) mask = static_cast<uint16_t>(mask | (1u << pin));
  return mask;
}

Status Mcp23017ButtonSource::begin() {
  const uint16_t inputs = inputMask();
  // O driver também recusa, mas aqui o erro fica associado ao mapa de botões.
  if ((inputs & Mcp23017::kOutputOnlyPins) != 0) return Status::InvalidArgument;
  return chip_.begin(inputs, inputs);
}

Result<ButtonMask> Mcp23017ButtonSource::read() {
  const Result<uint16_t> pins = chip_.readPins();
  if (!pins.ok()) return pins.status();
  ButtonMask pressed = 0;
  for (uint8_t pin = 0; pin < 16; ++pin) {
    const uint8_t button = map_.buttonOfPin[pin];
    if (button >= kButtonCount) continue;
    if ((pins.value() & (1u << pin)) == 0) pressed |= 1u << button;  // ativo em baixo
  }
  return pressed;
}

Status GpioButtonSource::begin() {
  for (size_t i = 0; i < count_; ++i)
    DBMO_TRY(gpio_.configure(entries_[i].pin,
                             activeLow_ ? PinMode::InputPullUp : PinMode::InputPullDown));
  return Status::Ok;
}

Result<ButtonMask> GpioButtonSource::read() {
  ButtonMask pressed = 0;
  for (size_t i = 0; i < count_; ++i) {
    const Result<bool> level = gpio_.read(entries_[i].pin);
    if (!level.ok()) return level.status();
    if (level.value() != activeLow_) pressed |= maskOf(entries_[i].button);
  }
  return pressed;
}

}  // namespace dbmo::input
