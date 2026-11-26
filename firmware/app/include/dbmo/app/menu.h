// Lógica do menu de tela: em que tela se está, o cursor e o que cada botão faz. Não
// desenha e não fala com os subsistemas: devolve uma MenuAction e a aplicação executa
// (firmware/app/src/menu_screens.cpp monta o texto de cada tela).
//
// Botões: OK (A) abre o menu a partir do rosto e escolhe; CIMA e BAIXO mudam o item; toque
// longo no OK volta (no perfil target, B também volta). Na bancada só existem os três.
#pragma once

#include <cstddef>
#include <cstdint>

#include "dbmo/input/types.h"

namespace dbmo::app {

// Itens do menu principal, na ordem da tela.
enum class MenuItem : uint8_t { Status, Infrared, Radio, Buttons, Games, About, Face };
inline constexpr uint8_t kMenuItemCount = 7;

const char *label(MenuItem item);

enum class MenuScreen : uint8_t { Closed, Main, Status, Infrared, Radio, Buttons, Games, About };

// O que a aplicação precisa fazer depois de um botão.
enum class MenuAction : uint8_t {
  None,         // o botão não mudou nada no menu
  Redraw,       // o menu mudou: redesenhar
  Opened,       // saiu do rosto para o menu: esconder o rosto
  Closed,       // voltou ao rosto
  LearnIr,      // tela de infravermelho: aprender um código (sem envio)
  ToggleRadio,  // tela de rádio: ligar ou desligar a recepção (sem transmissão)
  BootGames,    // tela de jogos: reiniciar no Retro-Go
};

class Menu {
 public:
  // Sem nenhum botão por este tempo, o menu volta ao rosto.
  static constexpr uint32_t kIdleTimeoutMs = 30000;
  static constexpr size_t kHistorySize = 4;

  struct ButtonRecord {
    input::Button button;
    input::ButtonAction action;
  };

  bool isOpen() const { return screen_ != MenuScreen::Closed; }
  MenuScreen screen() const { return screen_; }
  MenuItem cursor() const { return cursor_; }

  // Infravermelho e Rádio só aparecem com o subsistema em operação, e Jogos só com o Retro-Go
  // gravado na mesma flash. Se o item sob o cursor some, o cursor passa ao seguinte disponível.
  void setAvailable(MenuItem item, bool available);
  bool available(MenuItem item) const;

  // Um evento de botão (a repetição automática conta como um novo toque em CIMA e BAIXO).
  MenuAction handle(input::Button button, input::ButtonAction action, uint32_t nowMs);
  // Fecha o menu depois de kIdleTimeoutMs sem botão. true se fechou agora.
  bool tick(uint32_t nowMs);
  void close() { screen_ = MenuScreen::Closed; }

  // Últimos botões vistos na tela Botões; 0 é o mais recente.
  size_t historySize() const { return historyCount_; }
  const ButtonRecord &history(size_t index) const { return history_[index]; }

 private:
  void move(int step);
  MenuAction enter();
  void record(input::Button button, input::ButtonAction action);

  MenuScreen screen_ = MenuScreen::Closed;
  MenuItem cursor_ = MenuItem::Status;
  bool available_[kMenuItemCount] = {true, true, true, true, false, true, true};  // Jogos não
  uint32_t lastInputMs_ = 0;
  // Botões que repetiram desde que foram apertados: o clique ao soltar não anda de novo.
  input::ButtonMask repeated_ = 0;
  ButtonRecord history_[kHistorySize] = {};
  size_t historyCount_ = 0;
};

}  // namespace dbmo::app
