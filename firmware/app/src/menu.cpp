#include "dbmo/app/menu.h"

#include "dbmo/core/hal/clock.h"

namespace dbmo::app {

using input::Button;
using input::ButtonAction;

const char *label(MenuItem item) {
  switch (item) {
    case MenuItem::Status: return "Estado";
    case MenuItem::Infrared: return "Infravermelho";
    case MenuItem::Radio: return "Radio";
    case MenuItem::Buttons: return "Botoes";
    case MenuItem::About: return "Sobre";
    case MenuItem::Face: return "Voltar ao rosto";
  }
  return "?";
}

void Menu::setAvailable(MenuItem item, bool available) {
  // Estado e Voltar ao rosto existem sempre: é o que garante que move() para.
  if (item == MenuItem::Status || item == MenuItem::Face) return;
  available_[static_cast<uint8_t>(item)] = available;
  if (!available && cursor_ == item) move(1);
}

bool Menu::available(MenuItem item) const { return available_[static_cast<uint8_t>(item)]; }

void Menu::move(int step) {
  int index = static_cast<int>(cursor_);
  do {
    index = (index + step + kMenuItemCount) % kMenuItemCount;
  } while (!available_[index]);
  cursor_ = static_cast<MenuItem>(index);
}

MenuAction Menu::enter() {
  switch (cursor_) {
    case MenuItem::Status: screen_ = MenuScreen::Status; break;
    case MenuItem::Infrared: screen_ = MenuScreen::Infrared; break;
    case MenuItem::Radio: screen_ = MenuScreen::Radio; break;
    case MenuItem::Buttons:
      screen_ = MenuScreen::Buttons;
      historyCount_ = 0;
      break;
    case MenuItem::About: screen_ = MenuScreen::About; break;
    case MenuItem::Face: screen_ = MenuScreen::Closed; return MenuAction::Closed;
  }
  return MenuAction::Redraw;
}

void Menu::record(Button button, ButtonAction action) {
  // Botão segurado: uma linha de repetição só, não a lista inteira.
  if (historyCount_ > 0 && action == ButtonAction::Repeat && history_[0].button == button &&
      history_[0].action == action)
    return;
  for (size_t i = kHistorySize - 1; i > 0; --i) history_[i] = history_[i - 1];
  history_[0] = ButtonRecord{button, action};
  if (historyCount_ < kHistorySize) ++historyCount_;
}

MenuAction Menu::handle(Button button, ButtonAction action, uint32_t nowMs) {
  if (!isOpen()) {
    if (button != Button::A || action != ButtonAction::Click) return MenuAction::None;
    screen_ = MenuScreen::Main;
    cursor_ = MenuItem::Status;
    lastInputMs_ = nowMs;
    return MenuAction::Opened;
  }
  lastInputMs_ = nowMs;

  if (screen_ == MenuScreen::Buttons) record(button, action);

  const bool back = (button == Button::A && action == ButtonAction::LongPress) ||
                    (button == Button::B && action == ButtonAction::Click);
  if (back) {
    if (screen_ == MenuScreen::Main) {
      screen_ = MenuScreen::Closed;
      return MenuAction::Closed;
    }
    screen_ = MenuScreen::Main;  // o cursor continua no item de onde se saiu
    return MenuAction::Redraw;
  }

  const bool step = action == ButtonAction::Click || action == ButtonAction::Repeat;
  const bool ok = button == Button::A && action == ButtonAction::Click;
  switch (screen_) {
    case MenuScreen::Main:
      if (step && button == Button::DpadUp) {
        move(-1);
        return MenuAction::Redraw;
      }
      if (step && button == Button::DpadDown) {
        move(1);
        return MenuAction::Redraw;
      }
      if (ok) return enter();
      return MenuAction::None;
    case MenuScreen::Infrared: return ok ? MenuAction::LearnIr : MenuAction::None;
    case MenuScreen::Radio: return ok ? MenuAction::ToggleRadio : MenuAction::None;
    case MenuScreen::Buttons: return MenuAction::Redraw;
    case MenuScreen::Status:
    case MenuScreen::About:
    case MenuScreen::Closed: break;
  }
  return MenuAction::None;
}

bool Menu::tick(uint32_t nowMs) {
  if (!isOpen() || elapsedMs(nowMs, lastInputMs_) < kIdleTimeoutMs) return false;
  screen_ = MenuScreen::Closed;
  return true;
}

}  // namespace dbmo::app
