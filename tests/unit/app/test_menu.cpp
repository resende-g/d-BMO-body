// Lógica do menu, sem tela e sem subsistemas.
#include "dbmo/app/menu.h"
#include "dbmo_test.h"

using dbmo::app::Menu;
using dbmo::app::MenuAction;
using dbmo::app::MenuItem;
using dbmo::app::MenuScreen;
using dbmo::input::Button;
using dbmo::input::ButtonAction;

namespace {

MenuAction click(Menu &menu, Button button, uint32_t nowMs = 0) {
  return menu.handle(button, ButtonAction::Click, nowMs);
}
MenuAction back(Menu &menu, uint32_t nowMs = 0) {
  return menu.handle(Button::A, ButtonAction::LongPress, nowMs);
}

}  // namespace

void setUp() {}
void tearDown() {}

static void test_only_an_ok_click_opens_the_menu() {
  Menu menu;
  TEST_ASSERT_FALSE(menu.isOpen());
  TEST_ASSERT_TRUE(click(menu, Button::DpadUp) == MenuAction::None);
  TEST_ASSERT_TRUE(click(menu, Button::DpadDown) == MenuAction::None);
  TEST_ASSERT_TRUE(menu.handle(Button::A, ButtonAction::Pressed, 0) == MenuAction::None);
  TEST_ASSERT_TRUE(back(menu) == MenuAction::None);  // toque longo no rosto não abre
  TEST_ASSERT_FALSE(menu.isOpen());
  TEST_ASSERT_TRUE(click(menu, Button::A) == MenuAction::Opened);
  TEST_ASSERT_TRUE(menu.screen() == MenuScreen::Main);
  TEST_ASSERT_TRUE(menu.cursor() == MenuItem::Status);
}

static void test_up_and_down_walk_the_items_and_wrap() {
  Menu menu;
  (void)click(menu, Button::A);
  const MenuItem order[] = {MenuItem::Infrared, MenuItem::Radio, MenuItem::Buttons,
                            MenuItem::About,    MenuItem::Face,  MenuItem::Status};
  for (MenuItem expected : order) {
    TEST_ASSERT_TRUE(click(menu, Button::DpadDown) == MenuAction::Redraw);
    TEST_ASSERT_TRUE(menu.cursor() == expected);
  }
  TEST_ASSERT_TRUE(click(menu, Button::DpadUp) == MenuAction::Redraw);
  TEST_ASSERT_TRUE(menu.cursor() == MenuItem::Face);
  // A repetição de um botão segurado também anda; apertar e soltar, não.
  TEST_ASSERT_TRUE(menu.handle(Button::DpadUp, ButtonAction::Repeat, 0) == MenuAction::Redraw);
  TEST_ASSERT_TRUE(menu.cursor() == MenuItem::About);
  TEST_ASSERT_TRUE(menu.handle(Button::DpadUp, ButtonAction::Pressed, 0) == MenuAction::None);
  TEST_ASSERT_TRUE(menu.cursor() == MenuItem::About);
}

static void test_release_after_auto_repeat_does_not_move_again() {
  // O ButtonProcessor manda Click ao soltar mesmo depois de repetições (ver
  // test_button_processor). Segurar BAIXO e soltar não pode passar um item além do que a
  // tela mostrava.
  Menu menu;
  (void)click(menu, Button::A);
  (void)menu.handle(Button::DpadDown, ButtonAction::Pressed, 0);
  (void)menu.handle(Button::DpadDown, ButtonAction::Repeat, 400);
  (void)menu.handle(Button::DpadDown, ButtonAction::Repeat, 520);
  TEST_ASSERT_TRUE(menu.cursor() == MenuItem::Radio);
  (void)menu.handle(Button::DpadDown, ButtonAction::Released, 600);
  TEST_ASSERT_TRUE(menu.handle(Button::DpadDown, ButtonAction::Click, 600) == MenuAction::None);
  TEST_ASSERT_TRUE(menu.cursor() == MenuItem::Radio);
  // O toque curto seguinte volta a andar.
  (void)menu.handle(Button::DpadDown, ButtonAction::Pressed, 900);
  (void)menu.handle(Button::DpadDown, ButtonAction::Released, 950);
  TEST_ASSERT_TRUE(click(menu, Button::DpadDown, 950) == MenuAction::Redraw);
  TEST_ASSERT_TRUE(menu.cursor() == MenuItem::Buttons);
}

static void test_absent_items_are_skipped() {
  Menu menu;
  menu.setAvailable(MenuItem::Radio, false);
  menu.setAvailable(MenuItem::Infrared, false);
  (void)click(menu, Button::A);
  (void)click(menu, Button::DpadDown);
  TEST_ASSERT_TRUE(menu.cursor() == MenuItem::Buttons);
  (void)click(menu, Button::DpadUp);
  TEST_ASSERT_TRUE(menu.cursor() == MenuItem::Status);

  // O item sob o cursor some (rádio caiu): o cursor passa ao seguinte.
  menu.setAvailable(MenuItem::Radio, true);
  (void)click(menu, Button::DpadDown);
  TEST_ASSERT_TRUE(menu.cursor() == MenuItem::Radio);
  menu.setAvailable(MenuItem::Radio, false);
  TEST_ASSERT_TRUE(menu.cursor() == MenuItem::Buttons);
  TEST_ASSERT_FALSE(menu.available(MenuItem::Radio));

  // Estado e Voltar ao rosto não podem sumir.
  menu.setAvailable(MenuItem::Status, false);
  menu.setAvailable(MenuItem::Face, false);
  TEST_ASSERT_TRUE(menu.available(MenuItem::Status));
  TEST_ASSERT_TRUE(menu.available(MenuItem::Face));
}

static void test_ok_enters_each_screen_and_long_press_goes_back() {
  struct Case {
    int steps;
    MenuScreen screen;
  };
  const Case cases[] = {{0, MenuScreen::Status},
                        {1, MenuScreen::Infrared},
                        {2, MenuScreen::Radio},
                        {3, MenuScreen::Buttons},
                        {4, MenuScreen::About}};
  for (const Case &c : cases) {
    Menu menu;
    (void)click(menu, Button::A);
    for (int i = 0; i < c.steps; ++i) (void)click(menu, Button::DpadDown);
    TEST_ASSERT_TRUE(click(menu, Button::A) == MenuAction::Redraw);
    TEST_ASSERT_TRUE(menu.screen() == c.screen);
    TEST_ASSERT_TRUE(back(menu) == MenuAction::Redraw);
    TEST_ASSERT_TRUE(menu.screen() == MenuScreen::Main);
    TEST_ASSERT_EQUAL_INT(c.steps, static_cast<int>(menu.cursor()));  // cursor mantido
  }
}

static void test_back_from_the_main_list_and_the_face_item_close_the_menu() {
  Menu menu;
  (void)click(menu, Button::A);
  TEST_ASSERT_TRUE(back(menu) == MenuAction::Closed);
  TEST_ASSERT_FALSE(menu.isOpen());

  (void)click(menu, Button::A);
  (void)click(menu, Button::DpadUp);  // Voltar ao rosto
  TEST_ASSERT_TRUE(menu.cursor() == MenuItem::Face);
  TEST_ASSERT_TRUE(click(menu, Button::A) == MenuAction::Closed);
  TEST_ASSERT_FALSE(menu.isOpen());

  // Reaberto, começa de novo em Estado.
  (void)click(menu, Button::A);
  TEST_ASSERT_TRUE(menu.cursor() == MenuItem::Status);
}

static void test_b_also_goes_back_on_the_target_profile() {
  Menu menu;
  (void)click(menu, Button::A);
  (void)click(menu, Button::A);  // Estado
  TEST_ASSERT_TRUE(click(menu, Button::B) == MenuAction::Redraw);
  TEST_ASSERT_TRUE(menu.screen() == MenuScreen::Main);
  TEST_ASSERT_TRUE(click(menu, Button::B) == MenuAction::Closed);
  TEST_ASSERT_TRUE(click(menu, Button::B) == MenuAction::None);  // no rosto, B não faz nada
}

static void test_screen_actions_are_only_receive_side() {
  Menu menu;
  (void)click(menu, Button::A);
  (void)click(menu, Button::DpadDown);
  (void)click(menu, Button::A);
  TEST_ASSERT_TRUE(menu.screen() == MenuScreen::Infrared);
  TEST_ASSERT_TRUE(click(menu, Button::A) == MenuAction::LearnIr);
  TEST_ASSERT_TRUE(click(menu, Button::DpadDown) == MenuAction::None);
  (void)back(menu);
  (void)click(menu, Button::DpadDown);
  (void)click(menu, Button::A);
  TEST_ASSERT_TRUE(menu.screen() == MenuScreen::Radio);
  TEST_ASSERT_TRUE(click(menu, Button::A) == MenuAction::ToggleRadio);
  TEST_ASSERT_TRUE(click(menu, Button::DpadUp) == MenuAction::None);
}

static void test_buttons_screen_keeps_the_latest_presses() {
  Menu menu;
  (void)click(menu, Button::A);
  for (int i = 0; i < 3; ++i) (void)click(menu, Button::DpadDown);
  (void)click(menu, Button::A);
  TEST_ASSERT_TRUE(menu.screen() == MenuScreen::Buttons);
  TEST_ASSERT_EQUAL_UINT(0, menu.historySize());  // o OK que entrou não conta

  TEST_ASSERT_TRUE(menu.handle(Button::DpadUp, ButtonAction::Pressed, 0) == MenuAction::Redraw);
  (void)menu.handle(Button::DpadUp, ButtonAction::Repeat, 0);
  (void)menu.handle(Button::DpadUp, ButtonAction::Repeat, 0);  // repetição em seguida: uma linha
  (void)menu.handle(Button::DpadUp, ButtonAction::Released, 0);
  TEST_ASSERT_EQUAL_UINT(3, menu.historySize());
  TEST_ASSERT_TRUE(menu.history(0).action == ButtonAction::Released);
  TEST_ASSERT_TRUE(menu.history(1).action == ButtonAction::Repeat);
  TEST_ASSERT_TRUE(menu.history(2).action == ButtonAction::Pressed);

  // CIMA, BAIXO e OK curto não navegam aqui; a lista guarda só os últimos.
  (void)click(menu, Button::A);
  (void)click(menu, Button::DpadDown);
  TEST_ASSERT_TRUE(menu.screen() == MenuScreen::Buttons);
  TEST_ASSERT_EQUAL_UINT(Menu::kHistorySize, menu.historySize());
  TEST_ASSERT_TRUE(menu.history(0).button == Button::DpadDown);
  TEST_ASSERT_TRUE(menu.history(1).button == Button::A);

  TEST_ASSERT_TRUE(back(menu) == MenuAction::Redraw);
  TEST_ASSERT_TRUE(menu.screen() == MenuScreen::Main);
}

static void test_idle_menu_returns_to_the_face() {
  Menu menu;
  TEST_ASSERT_FALSE(menu.tick(100000));  // fechado: nada a fazer
  (void)click(menu, Button::A, 1000);
  TEST_ASSERT_FALSE(menu.tick(1000 + Menu::kIdleTimeoutMs - 1));
  (void)click(menu, Button::DpadDown, 20000);  // qualquer botão reinicia a contagem
  TEST_ASSERT_FALSE(menu.tick(1000 + Menu::kIdleTimeoutMs));
  TEST_ASSERT_TRUE(menu.tick(20000 + Menu::kIdleTimeoutMs));
  TEST_ASSERT_FALSE(menu.isOpen());
  TEST_ASSERT_FALSE(menu.tick(20000 + Menu::kIdleTimeoutMs + 1));

  // Relógio que dá a volta (uint32_t) não fecha antes da hora.
  (void)click(menu, Button::A, 0xFFFFF000u);
  TEST_ASSERT_FALSE(menu.tick(0x00000010u));
  TEST_ASSERT_TRUE(menu.tick(0xFFFFF000u + Menu::kIdleTimeoutMs));
}

static void test_labels() {
  TEST_ASSERT_EQUAL_STRING("Estado", dbmo::app::label(MenuItem::Status));
  TEST_ASSERT_EQUAL_STRING("Voltar ao rosto", dbmo::app::label(MenuItem::Face));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_only_an_ok_click_opens_the_menu);
  RUN_TEST(test_up_and_down_walk_the_items_and_wrap);
  RUN_TEST(test_release_after_auto_repeat_does_not_move_again);
  RUN_TEST(test_absent_items_are_skipped);
  RUN_TEST(test_ok_enters_each_screen_and_long_press_goes_back);
  RUN_TEST(test_back_from_the_main_list_and_the_face_item_close_the_menu);
  RUN_TEST(test_b_also_goes_back_on_the_target_profile);
  RUN_TEST(test_screen_actions_are_only_receive_side);
  RUN_TEST(test_buttons_screen_keeps_the_latest_presses);
  RUN_TEST(test_idle_menu_returns_to_the_face);
  RUN_TEST(test_labels);
  return UNITY_END();
}
