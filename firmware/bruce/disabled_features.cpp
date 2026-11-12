// Licença: AGPL-3.0, a mesma do Bruce (ver firmware/bruce/LICENSE).
// O d-BMO não inclui bloqueadores de sinal. O bmo.ini tira do build os três arquivos do Bruce
// que os implementam (modules/rf/rf_jammer.cpp, modules/ir/ir_jammer.cpp e
// modules/NRF24/nrf_jammer.cpp); os menus do Bruce continuam chamando estes três pontos de
// entrada, que aqui só mostram um aviso. Motivo e alcance: firmware/bruce/README.md.
#include "core/display.h"
#include "modules/rf/rf_jammer.h"

static void showDisabled() { displayWarning("Disabled on d-BMO", true); }

// Item "Jammer" do menu RF (src/core/menu_items/RFMenu.cpp).
RFJammer::RFJammer(bool) { showDisabled(); }
RFJammer::~RFJammer() {}

// Item "IR Jammer" do menu IR (src/core/menu_items/IRMenu.cpp).
void startIrJammer() { showDisabled(); }

// Item "NRF Jammer" do menu NRF24 (src/core/menu_items/NRF24.cpp).
void nrf_jammer() { showDisabled(); }
