#!/usr/bin/env python3
"""Confere a netlist exportada do EasyEDA Pro contra ligacoes.json e contra pins.h.

Uso:
  python3 hardware/placa-principal/conferir_netlist.py placa-principal.enet
  python3 hardware/placa-principal/conferir_netlist.py placa-energia.enet hardware/placa-energia/ligacoes.json

Confere:
  1. cada pino listado em ligacoes.json está na net esperada, e nenhum outro pino tem net;
  2. só na placa-base (o ligacoes.json desta pasta): cada sinal de pins.h (perfil target) chega
     ao pino da DevKitC (U1) com o GPIO certo, pelo nome do pino no símbolo da LCSC ("GPIO11" etc.);
  3. na placa que tem o MCP23017 (U2, a de botões da esquerda): cada pino tem o nome que o
     datasheet dá ao número;
  4. o botão de cada pino do U2 cai no bit que mcp_buttons de pins.h dá a ele.
As conferências 2 e 3 dependem do nome dos pinos, que só a netlist do esquemático traz; a do
PCB passa só pelas 1 e 4.

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
MCP_NAMES = {"9": "VDD", "10": "VSS", "12": "SC", "13": "SDA", "15": "A0", "16": "A1", "17": "A2",
             "18": "RESET"} | {str(1 + k): f"GPB{k}" for k in range(8)} | {str(21 + k): f"GPA{k}" for k in range(8)}

# Botão -> constante de pins.h (mcp_buttons): bit 0-7 = GPA0-7 (pinos 21-28), 8-15 = GPB0-7 (pinos 1-8).
BTN_TO_CONST = {"BTN_UP": "kDpadUp", "BTN_DOWN": "kDpadDown", "BTN_LEFT": "kDpadLeft",
                "BTN_RIGHT": "kDpadRight", "BTN_A": "kActionA", "BTN_B": "kActionB", "BTN_X": "kActionX",
                "BTN_Y": "kActionY", "BTN_SELECT": "kSelect", "BTN_START": "kStart"}


def target_pins() -> dict[str, int]:
    text = PINS_H.read_text()
    block = text[text.index("namespace target {"):text.index("}  // namespace target")]
    return {m[1]: int(m[2]) for m in re.finditer(r"constexpr int8_t (k\w+) = (\d+);", block)}


def mcp_bits() -> dict[str, int]:
    text = PINS_H.read_text()
    block = text[text.index("namespace mcp_buttons {"):text.index("}  // namespace mcp_buttons")]
    return {m[1]: int(m[2]) for m in re.finditer(r"constexpr uint8_t (k\w+) = (\d+);", block)}


def main(enet: str, ligacoes: str | None = None) -> int:
    path = Path(ligacoes) if ligacoes else HERE / "ligacoes.json"
    principal = path.resolve() == (HERE / "ligacoes.json").resolve()
    expected = json.loads(path.read_text())
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

    # A netlist exportada do PCB não traz o nome dos pinos: nela só as nets são conferidas.
    named = any(name for pins in actual.values() for name, _ in pins.values())
    if not named:
        print("netlist sem nomes de pino (exportada do PCB): conferidas só as nets")
    gpio = target_pins()
    u1 = actual.get("U1", {}) if named and principal else {}
    for net, const in NET_TO_PIN.items() if named and principal else ():
        hits = [name for name, n in u1.values() if n == net]
        want = f"GPIO{gpio[const]}"
        if not any(re.search(rf"\b{want}\b", h) for h in hits):
            errors.append(f"{net}: pins.h {const} = {want}, mas U1 liga {hits or 'nada'}")

    for num in expected.get("U2", {}) if named else ():
        name, want = actual.get("U2", {}).get(num, ("?", ""))[0], MCP_NAMES.get(num, "?")
        if want not in name.upper():
            errors.append(f"U2.{num}: nome {name}, esperado {want}")

    # O mapa dos botões no MCP23017 (ligacoes.json) tem de bater com mcp_buttons de pins.h.
    bits = mcp_bits()
    for num, net in expected.get("U2", {}).items():
        if net in BTN_TO_CONST:
            n = int(num)
            bit = n - 21 if n >= 21 else n - 1 + 8
            if bits.get(BTN_TO_CONST[net]) != bit:
                errors.append(f"{net} em U2.{num} (bit {bit}), mas pins.h "
                              f"{BTN_TO_CONST[net]} = {bits.get(BTN_TO_CONST[net])}")

    for e in errors:
        print(e)
    total = sum(len(p) for p in expected.values())
    print(f"{total} pinos esperados, {len(errors)} divergências")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main(*sys.argv[1:3]))
