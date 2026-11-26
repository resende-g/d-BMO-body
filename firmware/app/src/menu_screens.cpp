// Menu de tela: executa o que a lógica do menu (menu.h) pede e monta o texto de cada
// tela a partir do estado dos subsistemas. O desenho é da TextScreen, que só repinta as
// linhas que mudaram.
//
// Nenhuma tela envia infravermelho nem transmite por rádio: o infravermelho só aprende e
// o rádio só liga ou desliga a recepção.
#include <cstdio>

#include "dbmo/app/application.h"
#include "dbmo/core/log.h"
#include "dbmo/core/version.h"

namespace dbmo::app {

namespace {

constexpr char kTag[] = "menu";

using Style = display::TextScreen::Style;

// Linhas fixas da grade de 15 linhas.
constexpr size_t kTitleRow = 0;
constexpr size_t kActionHintRow = 13;
constexpr size_t kBackHintRow = 14;

const char *systemLabel(SystemState state) {
  switch (state) {
    case SystemState::Off: return "desligado";
    case SystemState::Booting: return "iniciando";
    case SystemState::Running: return "normal";
    case SystemState::Degraded: return "degradado";
    case SystemState::LowPower: return "economia";
    case SystemState::ShuttingDown: return "desligando";
    case SystemState::Fault: return "falha";
  }
  return "?";
}

const char *subsystemLabel(SubsystemState state) {
  switch (state) {
    case SubsystemState::Uninitialized: return "-";
    case SubsystemState::Ready: return "pronto";
    case SubsystemState::Degraded: return "parcial";
    case SubsystemState::Absent: return "ausente";
    case SubsystemState::Failed: return "falha";
    case SubsystemState::Stopped: return "parado";
  }
  return "?";
}

const char *radioLabel(radio::RadioState state) {
  switch (state) {
    case radio::RadioState::Off: return "desligado";
    case radio::RadioState::Idle: return "em repouso";
    case radio::RadioState::Receiving: return "recebendo";
    case radio::RadioState::Transmitting: return "transmitindo";
    case radio::RadioState::Fault: return "falha";
  }
  return "?";
}

}  // namespace

void Application::updateMenuAvailability() {
  menu_.setAvailable(MenuItem::Infrared, inOperation(infrared_));
  menu_.setAvailable(MenuItem::Radio, inOperation(radio_));
  menu_.setAvailable(MenuItem::Games, hw_.appSwitch != nullptr && hw_.appSwitch->gamesAvailable());
}

void Application::handleMenu(input::Button button, input::ButtonAction action, uint32_t nowMs) {
  switch (menu_.handle(button, action, nowMs)) {
    case MenuAction::None: return;
    case MenuAction::Opened:
      face_.setVisible(false);
      menuText_.invalidate();
      irLearnNote_ = IrLearnNote::None;
      radioToggled_ = false;
      gamesTried_ = false;
      break;
    case MenuAction::Closed: face_.setVisible(true); return;
    case MenuAction::LearnIr:
      irLearnStatus_ = infrared_.startLearning();
      irLearnNote_ = isOk(irLearnStatus_) ? IrLearnNote::Waiting : IrLearnNote::Failed;
      DBMO_LOGI(kTag, "aprender infravermelho: %s", toString(irLearnStatus_));
      break;
    case MenuAction::ToggleRadio: {
      const bool receiving = radio_.state() == radio::RadioState::Receiving;
      radioToggleStatus_ = receiving ? radio_.stop() : radio_.startReceive();
      radioToggled_ = true;
      DBMO_LOGI(kTag, "recepcao do radio %s: %s", receiving ? "desligada" : "ligada",
                toString(radioToggleStatus_));
      break;
    }
    case MenuAction::BootGames:
      // A placa reinicia: a configuração vai para o cartão (ou a memória interna) antes.
      (void)persistence_.saveNow();
      DBMO_LOGI(kTag, "jogos: reiniciando no Retro-Go");
      gamesStatus_ = hw_.appSwitch != nullptr ? hw_.appSwitch->bootGames() : Status::Unsupported;
      gamesTried_ = true;  // só chega aqui se a troca não reiniciou a placa
      DBMO_LOGW(kTag, "jogos: troca falhou: %s", toString(gamesStatus_));
      break;
    case MenuAction::Redraw: break;
  }
  menuDirty_ = true;
}

void Application::updateMenu(uint32_t nowMs) {
  if (menu_.tick(nowMs)) {
    face_.setVisible(true);  // parado tempo demais: volta ao rosto
    return;
  }
  if (!menu_.isOpen()) return;
  if (!menuDirty_ && elapsedMs(nowMs, menuComposedMs_) < kMenuRefreshMs) return;
  menuDirty_ = false;
  menuComposedMs_ = nowMs;
  composeMenu();
  // Erro da tela: as linhas que faltaram continuam diferentes e vão na próxima vez.
  (void)menuText_.render();
}

void Application::composeMenu() {
  menuText_.clear();
  switch (menu_.screen()) {
    case MenuScreen::Main: composeMain(); break;
    case MenuScreen::Status: composeStatus(); break;
    case MenuScreen::Infrared: composeInfrared(); break;
    case MenuScreen::Radio: composeRadio(); break;
    case MenuScreen::Buttons: composeButtons(); break;
    case MenuScreen::Games: composeGames(); break;
    case MenuScreen::About: composeAbout(); break;
    case MenuScreen::Closed: return;
  }
  if (menu_.screen() != MenuScreen::Main)
    menuText_.setLine(kBackHintRow, "OK longo: volta", Style::Hint);
}

void Application::composeMain() {
  menuText_.setLine(kTitleRow, "Menu", Style::Title);
  size_t row = 2;
  for (uint8_t i = 0; i < kMenuItemCount; ++i) {
    const auto item = static_cast<MenuItem>(i);
    if (!menu_.available(item)) continue;
    const bool selected = item == menu_.cursor();
    menuText_.printLine(row++, selected ? Style::Selected : Style::Normal, "%c %s",
                        selected ? '>' : ' ', label(item));
  }
  menuText_.setLine(kActionHintRow, "CIMA/BAIXO: escolher", Style::Hint);
  menuText_.setLine(kBackHintRow, "OK: abrir  OK longo: sair", Style::Hint);
}

void Application::composeStatus() {
  menuText_.setLine(kTitleRow, "Estado", Style::Title);
  menuText_.printLine(1, Style::Normal, "sistema: %s", systemLabel(system_.state()));
  for (size_t i = 0; i < system_.subsystemCount(); ++i) {
    const SubsystemInfo &info = system_.info(i);
    // Parcial e falha mostram o motivo; ausente fica só como "ausente".
    if (info.state == SubsystemState::Degraded || info.state == SubsystemState::Failed) {
      menuText_.printLine(2 + i, Style::Normal, "%-10s %s %s", info.subsystem->name(),
                          subsystemLabel(info.state), toString(info.lastStatus));
    } else {
      menuText_.printLine(2 + i, Style::Normal, "%-10s %s", info.subsystem->name(),
                          subsystemLabel(info.state));
    }
  }
}

void Application::composeInfrared() {
  menuText_.setLine(kTitleRow, "Infravermelho", Style::Title);
  if (!inOperation(infrared_)) {
    menuText_.setLine(2, "infravermelho ausente");
    return;
  }
  menuText_.setLine(2, "Ultimo codigo:");
  infrared::StoredCode code;
  char text[24];
  if (!infrared_.lastReceived(&code)) {
    menuText_.setLine(3, "  nada recebido");
  } else if (code.isRaw) {
    menuText_.printLine(3, Style::Normal, "  bruto, %u duracoes",
                        static_cast<unsigned>(code.raw.count));
  } else if (infrared::formatCode(code, text, sizeof(text)).ok()) {
    menuText_.printLine(3, Style::Normal, "  %s", text);
  }

  menuText_.setLine(5, "Aprender:");
  switch (irLearnNote_) {
    case IrLearnNote::None: menuText_.setLine(6, "  aperte OK para comecar"); break;
    case IrLearnNote::Waiting:
      menuText_.setLine(6, infrared_.learning() ? "  aponte e aperte a tecla" : "  -");
      break;
    case IrLearnNote::Learned:
      if (!infrared_.learned(&code)) {
        menuText_.setLine(6, "  -");
      } else if (code.isRaw) {
        menuText_.printLine(6, Style::Normal, "  bruto, %u duracoes",
                            static_cast<unsigned>(code.raw.count));
      } else if (infrared::formatCode(code, text, sizeof(text)).ok()) {
        menuText_.printLine(6, Style::Normal, "  ok: %s", text);
      }
      break;
    case IrLearnNote::TimedOut: menuText_.setLine(6, "  tempo esgotado"); break;
    case IrLearnNote::Failed:
      menuText_.printLine(6, Style::Normal, "  erro: %s", toString(irLearnStatus_));
      break;
  }

  const bool txEnabled = config_.getBool(infrared::config_key::kTxEnabled).valueOr(false);
  menuText_.setLine(8, txEnabled ? "Envio: so pelo console" : "Envio: desabilitado");
  menuText_.setLine(kActionHintRow, "OK: aprender um codigo", Style::Hint);
}

void Application::composeRadio() {
  menuText_.setLine(kTitleRow, "Radio", Style::Title);
  menuText_.printLine(2, Style::Normal, "estado: %s",
                      inOperation(radio_) ? radioLabel(radio_.state()) : "ausente");
  const uint32_t hz = radio_.radioConfig().frequencyHz;
  menuText_.printLine(3, Style::Normal, "freq: %lu.%03lu MHz",
                      static_cast<unsigned long>(hz / 1000000),
                      static_cast<unsigned long>(hz / 1000 % 1000));
  const radio::RadioCounters &counters = radio_.counters();
  menuText_.printLine(5, Style::Normal, "pacotes  %lu",
                      static_cast<unsigned long>(counters.packets));
  menuText_.printLine(6, Style::Normal, "crc      %lu",
                      static_cast<unsigned long>(counters.crcErrors));
  menuText_.printLine(7, Style::Normal, "negados  %lu",
                      static_cast<unsigned long>(counters.denied));
  if (radioToggled_)
    menuText_.printLine(9, Style::Normal, "recepcao: %s", toString(radioToggleStatus_));
  menuText_.setLine(11, radio_.policy().limits().enabled ? "transmissao: fora do menu"
                                                         : "transmissao: desabilitada");
  menuText_.setLine(kActionHintRow, "OK: liga/desliga recepcao", Style::Hint);
}

void Application::composeButtons() {
  menuText_.setLine(kTitleRow, "Botoes", Style::Title);
  menuText_.setLine(2, "Ultimos botoes:");
  if (menu_.historySize() == 0) menuText_.setLine(3, "  aperte um botao");
  for (size_t i = 0; i < menu_.historySize(); ++i) {
    const Menu::ButtonRecord &record = menu_.history(i);
    menuText_.printLine(3 + i, i == 0 ? Style::Selected : Style::Normal, "  %s: %s",
                        input::toString(record.button), input::toString(record.action));
  }
}

void Application::composeGames() {
  menuText_.setLine(kTitleRow, "Jogos", Style::Title);
  menuText_.setLine(2, "Reinicia no Retro-Go.");
  menuText_.setLine(3, "Reset volta ao d-BMO.");
  if (gamesTried_) menuText_.printLine(5, Style::Selected, "falhou: %s", toString(gamesStatus_));
  menuText_.setLine(kActionHintRow, "OK: abrir o Retro-Go", Style::Hint);
}

void Application::composeAbout() {
  char version[12];
  formatVersion(kFirmwareVersion, version, sizeof(version));
  menuText_.setLine(kTitleRow, "Sobre", Style::Title);
  menuText_.setLine(2, kHardwareName);
  menuText_.printLine(3, Style::Normal, "firmware %s", version);
  menuText_.printLine(4, Style::Normal, "perfil: %s", hw_.profile != nullptr ? hw_.profile : "-");
}

}  // namespace dbmo::app
