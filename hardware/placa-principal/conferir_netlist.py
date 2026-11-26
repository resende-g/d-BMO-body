#!/usr/bin/env python3
"""Confere a netlist exportada do EasyEDA Pro contra ligacoes.json e contra pins.h.

Uso:
  python3 hardware/placa-principal/conferir_netlist.py placa-principal.enet

Confere:
  1. cada pino listado em ligacoes.json está na net esperada, e nenhum outro pino tem net;
  2. cada sinal de pins.h (perfil target) chega ao pino da DevKitC (U1) com o GPIO certo,
     pelo nome do pino no símbolo da LCSC ("GPIO11" etc.);
  3. os pinos do MCP23017 (U2) têm o nome que o datasheet dá ao número.

Sai com 1 se houver divergência. Só a biblioteca padrão.
"""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
PINS_H = HERE.parent.parent / "firmware/config/include/dbmo/config/pins.h"

# Net da placa -> constante do perfil target em pins.h.
NET_TO_PIN = {
    "SPI_MOSI": "kSpiMosi", "SPI_SCK": "kSpiSck", "SPI_MISO": "kSpiMiso",
    "DISPLAY_CS": "kDisplayCs", "DISPLAY_DC": "kDisplayDc", "DISPLAY_RESET": "kDisplayReset",
    "DISPLAY_BACKLIGHT": "kDisplayBacklight", "SD_CS": "kSdCs", "CC1101_CS": "kCc1101Cs",
    "CC1101_GDO0": "kCc1101Gdo0", "I2C_SDA": "kI2cSda", "I2C_SCL": "kI2cScl",
    "I2S_BCLK": "kI2sBclk", "I2S_LRCLK": "kI2sLrclk", "I2S_MIC_DATA": "kI2sMicData",
    "I2S_SPK_DATA": "kI2sSpeakerData", "AMP_SHUTDOWN": "kAmpShutdown", "IR_RX": "kIrRx",
    "IR_TX": "kIrTx", "POWER_BTN": "kPowerButton", "POWER_HOLD": "kPowerHold",
    "VBAT_SENSE": "kVbatSense", "CHG_STAT": "kChargerStatus",
}

# MCP23017 em DIP-28 (Microchip DS20001952D, Table 2-1): número -> nome. O símbolo da LCSC chama
# o SCL de "SCK"; por isso o 12 só exige "SC".
MCP_NAMES = {"1": "GPB0", "2": "GPB1", "3": "GPB2", "9": "VDD", "10": "VSS", "12": "SC",
             "13": "SDA", "15": "A0", "16": "A1", "17": "A2", "18": "RESET", "21": "GPA0",
             "22": "GPA1", "23": "GPA2", "24": "GPA3", "25": "GPA4", "26": "GPA5", "27": "GPA6"}


def target_pins() -> dict[str, int]:
    text = PINS_H.read_text()
    block = text[text.index("namespace target {"):text.index("}  // namespace target")]
    return {m[1]: int(m[2]) for m in re.finditer(r"constexpr int8_t (k\w+) = (\d+);", block)}


def main(enet: str) -> int:
    expected = json.loads((HERE / "ligacoes.json").read_text())
    netlist = json.loads(Path(enet).read_text())
    actual: dict[str, dict[str, tuple[str, str]]] = {}
    for comp in netlist["components"].values():
        ref = comp["props"]["Designator"]
        actual[ref] = {n: (p["name"], p["net"]) for n, p in comp["pinInfoMap"].items()}

    errors = []
    for ref, pins in expected.items():
        if ref not in actual:
            errors.append(f"{ref}: ausente na netlist")
            continue
        for num, (name, net) in actual[ref].items():
            want = pins.get(num, "")
            if net != want:
                errors.append(f"{ref}.{num} ({name}): net {net or '-'}, esperado {want or '-'}")
    for ref in actual.keys() - expected.keys():
        errors.append(f"{ref}: na netlist mas não em ligacoes.json")

    gpio = target_pins()
    u1 = actual.get("U1", {})
    for net, const in NET_TO_PIN.items():
        hits = [name for name, n in u1.values() if n == net]
        want = f"GPIO{gpio[const]}"
        if not any(re.search(rf"\b{want}\b", h) for h in hits):
            errors.append(f"{net}: pins.h {const} = {want}, mas U1 liga {hits or 'nada'}")

    for num, want in MCP_NAMES.items():
        name = actual.get("U2", {}).get(num, ("?", ""))[0]
        if want not in name.upper():
            errors.append(f"U2.{num}: nome {name}, esperado {want}")

    for e in errors:
        print(e)
    total = sum(len(p) for p in expected.values())
    print(f"{total} pinos esperados, {len(errors)} divergências")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1]))
