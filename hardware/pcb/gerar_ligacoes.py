#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Gabriel Almeida Resende
# SPDX-License-Identifier: CERN-OHL-P-2.0
"""Gera a lista de ligações (netlist) e a lista de peças da placa principal do d-BMO.

Lê a pinagem do perfil target de firmware/config/include/dbmo/config/pins.h, junta com a pinagem
de cada módulo (tabelas abaixo, cada uma com a fonte) e grava em hardware/pcb/:

  LIGACOES.md     lista de ligações, conector da placa de energia, peças e posições sugeridas
  ligacoes.json   o mesmo, para ferramentas (o esquemático e a placa no EasyEDA Pro)

As posições são uma sugestão que respeita a ordem das zonas de docs/mechanical; as marcadas
SUPOSTO são medidas que ninguém fez ainda. O desenho da placa (esquemático, contorno, trilhas)
é feito no EasyEDA Pro (decisão do Gabriel em 2026-11-25).

Uso:
  python3 hardware/pcb/gerar_ligacoes.py           regrava os arquivos
  python3 hardware/pcb/gerar_ligacoes.py --check   confere sem gravar (sai 1 se divergir; CI)

Sem dependências além da biblioteca padrão.
"""

from __future__ import annotations

import argparse
import json
import sys
from dataclasses import asdict, dataclass, field
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent
sys.path.insert(0, str(ROOT / "tools"))
from check_pinmap import PINS_H, PinMap  # noqa: E402

W, H = 100.0, 176.0  # contorno da placa. SUPOSTO: cabe numa placa virgem de 15 x 20 cm
GRID = 0.508  # as posições sugeridas ficam numa grade de 2,54 / 5 mm

# ---------------------------------------------------------------------------
# DevKitC-1: conectores J1 e J3, guia do usuário v1.1 da Espressif (esp-dev-kits, commit
# 5c06bf1, seção "Header Block"; registro DKC-07, DKC-18 em docs/hardware/sources/ESP32-S3.md).
# ---------------------------------------------------------------------------
J1 = ["3V3", "3V3", "RST", "4", "5", "6", "7", "15", "16", "17", "18", "8", "3", "46", "9",
      "10", "11", "12", "13", "14", "5V", "G"]
J3 = ["G", "TX", "RX", "1", "2", "42", "41", "40", "39", "38", "37", "36", "35", "0", "45",
      "48", "47", "21", "20", "19", "G", "G"]
ROW_GAP = 22.86  # distância entre J1 e J3 (mechanical: header_row_spacing). Conferir na peça


def devkitc_pin(gpio: int) -> str:
    """Nome do pad da DevKitC (J1-n ou J3-n) que leva o GPIO."""
    for header, names in (("J1", J1), ("J3", J3)):
        for i, name in enumerate(names):
            if name == str(gpio) or (name, gpio) in (("TX", 43), ("RX", 44)):
                return f"{header}-{i + 1}"
    raise KeyError(f"GPIO {gpio} não sai na DevKitC")


# ---------------------------------------------------------------------------
# MCP23017 em DIP-28: Table 2-1 do DS20001952D, p. 11 (coluna SOIC/SPDIP/SSOP).
# ---------------------------------------------------------------------------
MCP_PINS = {**{f"GPB{i}": 1 + i for i in range(8)}, "VDD": 9, "VSS": 10, "NC11": 11, "SCL": 12,
            "SDA": 13, "NC14": 14, "A0": 15, "A1": 16, "A2": 17, "RESET": 18, "INTB": 19,
            "INTA": 20, **{f"GPA{i}": 21 + i for i in range(8)}}


# ---------------------------------------------------------------------------
# Geometria
# ---------------------------------------------------------------------------
@dataclass
class Pad:
    ref: str
    name: str  # número ou nome do pino no módulo
    x: float
    y: float
    net: str = ""
    label: str = ""  # nome do sinal no módulo (serigrafia)


@dataclass
class Part:
    ref: str
    value: str
    footprint: str
    pads: list[Pad]
    body: tuple[float, float, float, float]  # retângulo do corpo, coordenadas da placa
    low: bool = False  # corpo encostado na placa: jumper não passa por baixo
    note: str = ""
    # Módulo em barra fêmea: posição sem ligação fica sem ilha (o contato é tirado da barra ou a
    # barra é cortada): numa placa de uma face, a trilha passa pelo vão.
    porous: bool = False
    removed: list[str] = field(default_factory=list)


@dataclass
class Board:
    parts: list[Part] = field(default_factory=list)
    keepouts: list[tuple[str, tuple[float, float, float, float], bool]] = field(default_factory=list)

    def part(self, ref: str) -> Part:
        return next(p for p in self.parts if p.ref == ref)

    def pad(self, ref: str, name: str) -> Pad:
        return next(p for p in self.part(ref).pads if p.name == name)


def row(ref, x, y, names, dx=2.54, dy=0.0, labels=None):
    """Fileira de pinos começando em (x, y), passo (dx, dy). names: nome/número de cada pino."""
    labels = labels or [""] * len(names)
    return [Pad(ref, str(n), round(x + i * dx, 4), round(y + i * dy, 4), label=lab)
            for i, (n, lab) in enumerate(zip(names, labels))]


def g(n: float) -> float:
    """n passos de grade, em mm (as origens dos conectores ficam na grade)."""
    return round(n * GRID, 4)


DISPLAY_HEADER_LEFT = False  # SUPOSTO: lado do conector de 14 pinos com o rosto em pé


def build_board() -> Board:
    """Posição de cada peça. Coordenadas da placa: origem no canto de cima à esquerda, x para a
    direita, y para baixo, vista do lado dos componentes.

    De cima para baixo: faixa do topo (IR, botão de energia, microfone, CC1101 com a SMA para
    fora), DevKitC deitada, tela, controles (select/start, MCP23017, cruz, losango, PN532) e o
    amplificador no canto de baixo, perto do alto-falante (docs/mechanical, zonas ZT, ZF1 a ZF3).
    """
    b = Board()

    # Faixa do topo. Os módulos de IR ficam em balanço para fora da borda de cima.
    b.parts.append(Part("U6", "receptor IR 38 kHz (V1222)", "barra fêmea 1x3",
                        row("U6", g(60), g(8), ["OUT", "VCC", "GND"], labels=["OUT", "VCC", "GND"]),
                        (g(60) - 2, -8.0, g(60) + 7.1, g(8) + 1.3), note='tem (testado)'))
    b.parts.append(Part("U7", "transmissor IR (V1221)", "barra fêmea 1x3",
                        row("U7", g(80), g(8), ["DAT", "VCC", "GND"], labels=["DAT", "VCC", "GND"]),
                        (g(80) - 2, -8.0, g(80) + 7.1, g(8) + 1.3), note='tem (nunca transmitiu)'))
    b.parts.append(switch("SW11", "energia (GPIO 2)", g(108), g(14)))
    # INMP441: duas fileiras de 3 (L/R, WS, SCK | GND, VDD, SD), pela serigrafia; a distância
    # entre as fileiras é SUPOSTA.
    mic_gap = 10.16
    pads = row("U4", g(126), g(6), ["L/R", "WS", "SCK"], labels=["L/R", "WS", "SCK"])
    pads += row("U4", g(126), g(6) + mic_gap, ["GND", "VDD", "SD"], labels=["GND", "VDD", "SD"])
    b.parts.append(Part("U4", "INMP441", "2 x barra fêmea 1x3", pads,
                        (g(126) - 2.5, 1.0, g(126) + 7.6, g(6) + mic_gap + 2.0),
                        note='tem (testado)'))
    # CC1101 (E07-M1101D): barra 2x4, pinos ímpares numa fileira e pares na outra (SUPOSTO), a
    # antena SMA para cima, para fora da borda.
    cx, cy = g(160), g(22)
    cc = []
    for i in range(4):
        cc.append(Pad("U2", str(2 * i + 1), round(cx + i * 2.54, 4), cy))
        cc.append(Pad("U2", str(2 * i + 2), round(cx + i * 2.54, 4), round(cy + 2.54, 4)))
    cc_labels = {"1": "GND", "2": "VCC", "3": "GDO0", "4": "CSN", "5": "SCK", "6": "MOSI",
                 "7": "MISO", "8": "GDO2"}
    for p in cc:
        p.label = cc_labels[p.name]
    b.parts.append(Part("U2", "CC1101 E07-M1101D 433 MHz", "barra fêmea 2x4", cc,
                        (cx - 4.0, -8.0, cx + 11.6, cy + 4.0), note='tem (testado em recepção)'))

    # U1, DevKitC deitada logo abaixo da faixa do topo. Antena na borda esquerda (pino 1 junto
    # dela), USB virado para a direita: o cabo entra por cima da placa, onde só há peças baixas.
    # Vista de cima com a antena para cima, o J1 fica à esquerda (figura de pinagem v1.1:
    # conferir pela serigrafia); girada assim, o J1 vira a fileira de baixo, virada para a tela.
    x1, y3 = g(5), g(42)
    y1 = round(y3 + ROW_GAP, 4)
    pads = row("U1", x1, y1, [f"J1-{i + 1}" for i in range(22)], labels=J1)
    pads += row("U1", x1, y3, [f"J3-{i + 1}" for i in range(22)], labels=J3)
    # Corpo: 25,4 de largura; pino 1 a ~2 mm da ponta da antena e 63 mm de comprimento (SUPOSTO).
    b.parts.append(Part("U1", "ESP32-S3-DevKitC-1 N16R8", "2 x barra fêmea 1x22, 2,54 mm",
                        pads, (x1 - 2.0, y3 - 1.27, x1 - 2.0 + 63.0, y1 + 1.27),
                        porous=True, note='tem (testado)'))
    # Zona da antena (sem cobre): entre as fileiras, da borda até o pino 4.
    b.keepouts.append(("Antena do ESP32-S3", (0.0, y3 + 1.6, x1 + 3 * 2.54 + 1.27, y1 - 1.6), True))

    # DS1, tela MSP3218 na horizontal, centrada. Conector de 14 pinos numa borda curta, de cima
    # para baixo na ordem da serigrafia; SD de 4 pinos na borda oposta (BREADBOARD_MAP, fase F).
    # SUPOSTO: tamanho da placa, lado do conector e posição do SD (medir e conferir com o rosto
    # em pé na bancada).
    disp_w, disp_h, disp_top = 94.0, 55.0, 46.5
    tela = ["VCC", "GND", "CS", "RESET", "DC", "SDI", "SCK", "LED", "SDO", "T_CLK", "T_CS",
            "T_DIN", "T_DO", "T_IRQ"]
    sd = ["SD_CS", "SD_MOSI", "SD_MISO", "SD_SCK"]
    near, far = (g(11), g(186)) if DISPLAY_HEADER_LEFT else (g(186), g(11))
    side = 1 if DISPLAY_HEADER_LEFT else -1
    pads = row("DS1", near, g(114), tela, dx=0, dy=2.54, labels=tela)
    pads += row("DS1", far, g(140), sd, dx=0, dy=2.54, labels=sd)
    b.parts.append(Part("DS1", "MSP3218 (ILI9341 + SD)", "barra fêmea 1x14 + 1x4, 2,54 mm",
                        pads, ((W - disp_w) / 2, disp_top, (W + disp_w) / 2, disp_top + disp_h),
                        porous=True, note='tem (testado, com o SD)'))
    # JP2, perto do conector da tela: 1-2 = luz de fundo sempre acesa (3V3), 2-3 = GPIO 16 (PWM).
    jx = g(24) if DISPLAY_HEADER_LEFT else g(173)
    b.parts.append(Part("JP2", "luz de fundo", "barra macho 1x3 + jumper",
                        row("JP2", jx, g(178), ["1", "2", "3"], labels=["3V3", "LED", "BL"]),
                        (jx - 1.3, g(178) - 1.3, jx + 6.4, g(178) + 1.3), low=True,
                        note='comprar barra macho e jumper'))

    # Controles. Select e start logo abaixo da tela; MCP23017 embaixo deles, girado 180 graus:
    # pino 1 (GPB0) em cima à direita, perto de select e start; GPA0..GPA6 na fileira de baixo,
    # virados para a cruz e o losango.
    b.parts.append(switch("SW9", "select", g(108), g(208)))
    b.parts.append(switch("SW10", "start", g(128), g(208)))
    mx, my = g(132), g(226)  # pino 1
    top = [Pad("U8", str(n), round(mx - (n - 1) * 2.54, 4), my) for n in range(1, 15)]
    bottom = [Pad("U8", str(n), round(mx - (28 - n) * 2.54, 4), round(my + 7.62, 4))
              for n in range(15, 29)]
    names = {v: k for k, v in MCP_PINS.items()}
    for p in top + bottom:
        p.label = names[int(p.name)]
    b.parts.append(Part("U8", "MCP23017-E/SP", "DIP-28 300 mil (soquete)", top + bottom,
                        (mx - 13 * 2.54 - 1.5, my - 1.5, mx + 1.5, my + 7.62 + 1.5), low=True,
                        note='comprar (não possui) + soquete DIP-28'))
    # C1 (100 nF) junto de VDD e VSS do MCP23017; R1 e R2 (4,7 k), pull-ups do I2C.
    b.parts.append(Part("C1", "100 nF", "capacitor cerâmico, passo 5,08 mm",
                        row("C1", g(84), g(216), ["1", "2"], dx=5.08),
                        (g(84) - 1.5, g(216) - 1.5, g(84) + 6.6, g(216) + 1.5), low=True,
                        note='kit "104" (esperado)'))
    for ref, yy in (("R1", g(212)), ("R2", g(217))):
        b.parts.append(Part(ref, "4,7 kΩ", "resistor 1/4 W axial, passo 10,16 mm",
                            row(ref, g(40), yy, ["1", "2"], dx=10.16),
                            (g(40) - 1.0, yy - 1.2, g(40) + 11.2, yy + 1.2), low=True,
                            note="kit 1 % (esperado)"))

    dcx, acx, cy = g(32), g(165), g(262)
    step = g(18)
    for ref, label, x, y in (("SW1", "cima", dcx, cy - step), ("SW2", "baixo", dcx, cy + step),
                             ("SW3", "esquerda", dcx - step, cy), ("SW4", "direita", dcx + step, cy),
                             ("SW5", "A", acx + step, cy), ("SW6", "B", acx, cy + step),
                             ("SW7", "X", acx, cy - step), ("SW8", "Y", acx - step, cy)):
        b.parts.append(switch(ref, label, x, y))
    # PN532 V3 (I2C) entre a cruz e o losango: barra de 4 pinos na borda de cima do módulo.
    # Ordem SUPOSTA: o manual da Elechouse diz que o nome dos pinos de I2C está impresso na
    # frente da placa, mas não dá a ordem. Corpo 42,7 x 40,4 (ELE-08).
    px, py = g(91), g(263)
    pads = row("U3", px, py, ["GND", "VCC", "SDA", "SCL"], labels=["GND", "VCC", "SDA", "SCL"])
    b.parts.append(Part("U3", "PN532 NFC V3 (modo I2C)", "barra fêmea 1x4", pads,
                        (W / 2 - 20.2, py - 1.5, W / 2 + 20.2, py - 1.5 + 42.7),
                        note='tem (barra não soldada)'))
    b.keepouts.append(("Antena do PN532 (sem plano de cobre)",
                       (W / 2 - 18.0, py + 3.0, W / 2 + 18.0, py + 38.5), False))

    # MAX98357A no canto de baixo à esquerda, perto do alto-falante. Ordem pela serigrafia.
    amp = ["LRC", "BCLK", "DIN", "GAIN", "SD", "GND", "VIN"]
    pads = row("U5", g(5), g(296), amp, labels=amp)
    b.parts.append(Part("U5", "MAX98357A (borne do alto-falante na placa dele)", "barra fêmea 1x7",
                        pads, (g(5) - 2.0, g(296) - 1.5, g(5) + 17.3, g(296) + 18.0),
                        porous=True, note='tem (nunca ligado)'))
    # JP1: alimentação do amplificador: 1-2 = 5V do USB, 2-3 = 3V3.
    b.parts.append(Part("JP1", "alimentação do amplificador", "barra macho 1x3 + jumper",
                        row("JP1", g(48), g(300), ["1", "2", "3"], labels=["5V", "VIN", "3V3"]),
                        (g(48) - 1.3, g(300) - 1.3, g(48) + 6.4, g(300) + 1.3), low=True,
                        note='comprar barra macho e jumper'))
    # CN1: conector para a placa de energia futura (bateria, carga, regulador, power-latch).
    # Barra macho 1x8 de 2,54 mm no canto de baixo à direita (posição SUPOSTA: depende de onde a
    # placa de energia ficar na carcaça). Ordem dos pinos em CN1_PINS.
    b.parts.append(Part("CN1", "conector da placa de energia", "barra macho 1x8, 2,54 mm",
                        row("CN1", g(146), g(340), [str(i + 1) for i in range(len(CN1_PINS))],
                            labels=[name for name, _ in CN1_PINS]),
                        (g(146) - 1.3, g(340) - 1.3, g(146) + 19.1, g(340) + 1.3),
                        note="comprar barra macho; o par fêmea fica na placa de energia"))
    return b


# Pinos do CN1, em ordem: (nome, ligação). GND nas duas pontas: um conector invertido não põe
# alimentação no lugar do GND.
CN1_PINS = [("GND", "GND"), ("+5V", "+5V"), ("+3V3", "+3V3"), ("VBAT_SENSE", "VBAT_SENSE"),
            ("POWER_HOLD", "POWER_HOLD"), ("POWER_BUTTON", "POWER_BUTTON"),
            ("CHARGER_STATUS", "CHARGER_STATUS"), ("GND", "GND")]


def switch(ref: str, label: str, x: float, y: float) -> Part:
    """Botão táctil 6 x 6 mm PTH (pernas a 6,5 x 4,5 mm). Só as pernas em diagonal são ligadas:
    estão sempre em lados opostos da chave, qualquer que seja a posição do botão."""
    pads = [Pad(ref, "1", round(x - 3.25, 4), round(y - 2.25, 4)),
            Pad(ref, "2", round(x + 3.25, 4), round(y - 2.25, 4)),
            Pad(ref, "3", round(x - 3.25, 4), round(y + 2.25, 4)),
            Pad(ref, "4", round(x + 3.25, 4), round(y + 2.25, 4))]
    return Part(ref, label, "botão táctil 6x6 mm PTH", pads, (x - 3.2, y - 3.2, x + 3.2, y + 3.2),
                low=True, note="comprar")


# ---------------------------------------------------------------------------
# Ligações
# ---------------------------------------------------------------------------
def build_nets(b: Board, pins: PinMap) -> dict[str, list[tuple[str, str]]]:
    """Ligação -> [(ref, pino)]. O pino da DevKitC sai do GPIO de pins.h (perfil target)."""
    gpio = {signal: n for signal, n, _ in pins.tables["target"]}
    nets: dict[str, list[tuple[str, str]]] = {}

    def to_gpio(signal: str, *ends: tuple[str, str]) -> None:
        nets[signal] = [("U1", devkitc_pin(gpio[signal])), *ends]

    to_gpio("SPI_MOSI", ("DS1", "SDI"), ("DS1", "SD_MOSI"), ("U2", "6"))
    to_gpio("SPI_SCK", ("DS1", "SCK"), ("DS1", "SD_SCK"), ("U2", "5"))
    to_gpio("SPI_MISO", ("DS1", "SDO"), ("DS1", "SD_MISO"), ("U2", "7"))
    to_gpio("DISPLAY_CS", ("DS1", "CS"))
    to_gpio("DISPLAY_DC", ("DS1", "DC"))
    to_gpio("DISPLAY_RESET", ("DS1", "RESET"))
    to_gpio("DISPLAY_BACKLIGHT", ("JP2", "3"))
    to_gpio("SD_CS", ("DS1", "SD_CS"))
    to_gpio("CC1101_CS", ("U2", "4"))
    to_gpio("CC1101_GDO0", ("U2", "3"))
    to_gpio("I2C_SDA", ("U8", str(MCP_PINS["SDA"])), ("U3", "SDA"), ("R1", "1"))
    to_gpio("I2C_SCL", ("U8", str(MCP_PINS["SCL"])), ("U3", "SCL"), ("R2", "1"))
    to_gpio("I2S_BCLK", ("U4", "SCK"), ("U5", "BCLK"))
    to_gpio("I2S_LRCLK", ("U4", "WS"), ("U5", "LRC"))
    to_gpio("I2S_MIC_DATA", ("U4", "SD"))
    to_gpio("I2S_SPEAKER_DATA", ("U5", "DIN"))
    to_gpio("AMP_SHUTDOWN", ("U5", "SD"))
    to_gpio("IR_RX", ("U6", "OUT"))
    to_gpio("IR_TX", ("U7", "DAT"))
    legs = {}  # botão -> perna do sinal (a do GND é a diagonal oposta)

    def leg(ref: str, toward: Pad) -> str:
        """A perna do botão mais perto de quem lê o botão: encurta a trilha."""
        pads = b.part(ref).pads
        best = min(pads, key=lambda p: abs(p.x - toward.x) + abs(p.y - toward.y))
        legs[ref] = best.name
        return best.name

    to_gpio("POWER_BUTTON", ("SW11", leg("SW11", b.pad("U1", devkitc_pin(gpio["POWER_BUTTON"])))),
            ("CN1", "6"))
    # Sinais que só vão ao conector da placa de energia.
    to_gpio("VBAT_SENSE", ("CN1", "4"))
    to_gpio("POWER_HOLD", ("CN1", "5"))
    to_gpio("CHARGER_STATUS", ("CN1", "7"))
    nets["LCD_LED"] = [("DS1", "LED"), ("JP2", "2")]
    nets["AMP_VIN"] = [("U5", "VIN"), ("JP1", "2")]

    # Botões no MCP23017: bit 0-7 = GPA0-7, 8-15 = GPB0-7 (pins.h, namespace mcp_buttons).
    bits = pins.constants["mcp_buttons"]
    for ref, const in (("SW1", "kDpadUp"), ("SW2", "kDpadDown"), ("SW3", "kDpadLeft"),
                       ("SW4", "kDpadRight"), ("SW5", "kActionA"), ("SW6", "kActionB"),
                       ("SW7", "kActionX"), ("SW8", "kActionY"), ("SW9", "kSelect"),
                       ("SW10", "kStart")):
        bit = bits[const]
        port = f"GPA{bit}" if bit < 8 else f"GPB{bit - 8}"
        mcp = b.pad("U8", str(MCP_PINS[port]))
        nets[f"BTN_{const[1:].upper()}"] = [(ref, leg(ref, mcp)), ("U8", mcp.name)]

    # Endereço do MCP23017: 0 1 0 0 A2 A1 A0 (MCP-11).
    addr = pins.constants["i2c"]["kMcp23017Buttons"] - 0x20
    gnd = [("U1", "J1-22"), ("U1", "J3-22"), ("DS1", "GND"), ("U2", "1"), ("U3", "GND"),
           ("U4", "GND"), ("U4", "L/R"), ("U5", "GND"), ("U6", "GND"), ("U7", "GND"),
           ("U8", str(MCP_PINS["VSS"])), ("C1", "2")]
    v33 = [("U1", "J1-1"), ("DS1", "VCC"), ("U2", "2"), ("U3", "VCC"), ("U4", "VDD"),
           ("U6", "VCC"), ("U7", "VCC"), ("U8", str(MCP_PINS["VDD"])),
           ("U8", str(MCP_PINS["RESET"])), ("C1", "1"), ("R1", "2"), ("R2", "2"), ("JP2", "1"),
           ("JP1", "3")]
    for i in range(3):
        (v33 if addr >> i & 1 else gnd).append(("U8", str(MCP_PINS[f"A{i}"])))
    diagonal = {"1": "4", "4": "1", "2": "3", "3": "2"}
    gnd += [(ref, diagonal[name]) for ref, name in legs.items()]
    gnd += [("CN1", "1"), ("CN1", "8")]
    v33.append(("CN1", "3"))
    nets["GND"] = gnd
    nets["+3V3"] = v33
    nets["+5V"] = [("U1", "J1-21"), ("JP1", "1"), ("CN1", "2")]

    for net, ends in nets.items():
        for ref, name in ends:
            pad = b.pad(ref, name)
            if pad.net:
                raise ValueError(f"{ref}.{name} em duas ligações: {pad.net} e {net}")
            pad.net = net
    for part in b.parts:
        if part.porous:
            part.removed = [p.name for p in part.pads if not p.net]
            part.pads = [p for p in part.pads if p.net]
    return nets


def markdown(b: Board, pins: PinMap, nets) -> str:
    gpio = {signal: n for signal, n, _ in pins.tables["target"]}

    def point(ref, name):
        pad = b.pad(ref, name)
        return f"{ref}.{name}" + (f" = {pad.label}" if pad.label and pad.label != name else "")

    lines = ["# Placa principal: ligações e peças", "",
             "Gerado por `hardware/pcb/gerar_ligacoes.py` a partir de `pins.h` (perfil `target`). "
             "Não editar à mão: mudar o gerador e rodar de novo. Contexto, regras e pendências em "
             "[`README.md`](README.md).", "",
             "## Ligações", "", "| Ligação | GPIO | Pontos (ref.pino = nome na serigrafia) |",
             "|---|---:|---|"]
    for net, ends in nets.items():
        lines.append(f"| `{net}` | {gpio.get(net, '')} | {', '.join(point(*e) for e in ends)} |")
    free = [f"{p.ref}.{p.name}" + (f" ({p.label})" if p.label and p.label != p.name else "")
            for part in b.parts for p in part.pads if not p.net]
    removed = [f"{part.ref}: " + ", ".join(part.removed) for part in b.parts if part.removed]
    lines += ["", f"Pinos sem ligação ({len(free)}): " + ", ".join(free) + ".", "",
              "Posições de barra fêmea **sem ligação, que não precisam de ilha** (tirar o contato "
              "da barra ou cortá-la em pedaços; numa placa de uma face, a trilha passa pelo vão): "
              + "; ".join(removed) + ".", "",
              "## Conector da placa de energia (CN1)", "",
              "| Pino | Sinal | GPIO | Sem a placa de energia (só o USB da DevKitC) |",
              "|---:|---|---:|---|"]
    alone = {"GND": "GND", "+5V": "5 V do USB, saindo da DevKitC (J1-21)",
             "+3V3": "3,3 V do regulador da DevKitC (J1-1)",
             "VBAT_SENSE": "solto: a leitura da bateria não vale nada",
             "POWER_HOLD": "saída sem carga", "POWER_BUTTON": "o botão SW11 continua no GPIO 2",
             "CHARGER_STATUS": "solto: com o pull-up interno, lê nível alto (sem carga)"}
    for i, (name, net) in enumerate(CN1_PINS, 1):
        lines.append(f"| {i} | `{name}` | {gpio.get(net, '')} | {alone[name]} |")
    lines += ["", "## Peças", "", "| Ref | Peça | Montagem na placa | Observação |",
              "|---|---|---|---|"]
    for part in b.parts:
        lines.append(f"| {part.ref} | {part.value} | {part.footprint} | {part.note} |")
    female = sum(len(p.pads) for p in b.parts if "barra fêmea" in p.footprint)
    male = sum(len(p.pads) for p in b.parts if "barra macho" in p.footprint)
    lines += ["", f"Barra fêmea 2,54 mm: {female} posições com contato; barra macho: {male} "
              "posições e 2 jumpers de encaixe.", "",
              f"## Posições sugeridas (placa de {W:g} x {H:g} mm, SUPOSTO)", "",
              "Pino 1 de cada peça e o retângulo do corpo, em mm a partir do canto de cima à "
              "esquerda, vista do lado dos componentes. Ordem das zonas: faixa do topo (IR, botão "
              "de energia, microfone, CC1101 com a SMA para fora), DevKitC deitada com a antena na "
              "borda esquerda, tela, controles e PN532, MAX98357A no canto de baixo à esquerda, "
              "CN1 no canto de baixo à direita.", "",
              "| Ref | Pino 1 (x, y) | Corpo (x0, y0, x1, y1) |", "|---|---|---|"]
    for part in b.parts:
        p = part.pads[0]
        lines.append(f"| {part.ref} | {p.x:.2f}, {p.y:.2f} | "
                     + ", ".join(f"{v:.1f}" for v in part.body) + " |")
    return "\n".join(lines) + "\n"


def generate() -> dict[str, str]:
    pins = PinMap(PINS_H.read_text(encoding="utf-8"))
    b = build_board()
    nets = build_nets(b, pins)
    for i, (_, net) in enumerate(CN1_PINS, 1):
        assert b.pad("CN1", str(i)).net == net, f"CN1.{i} deveria ser {net}"
    data = {"fonte": "hardware/pcb/gerar_ligacoes.py (pins.h, perfil target)",
            "placa_mm": [W, H],
            "ligacoes": nets,
            "pecas": [{"ref": p.ref, "valor": p.value, "montagem": p.footprint, "nota": p.note,
                       "corpo_mm": list(p.body),
                       "pinos": [{k: v for k, v in asdict(pad).items() if k != "ref"}
                                 for pad in p.pads],
                       "pinos_sem_ilha": p.removed} for p in b.parts]}
    return {"LIGACOES.md": markdown(b, pins, nets),
            "ligacoes.json": json.dumps(data, indent=1, ensure_ascii=False) + "\n"}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true", help="confere sem gravar")
    args = parser.parse_args()
    files = generate()
    stale = [name for name, text in files.items()
             if not (HERE / name).exists() or (HERE / name).read_text(encoding="utf-8") != text]
    for name in stale:
        if args.check:
            print(f"erro: hardware/pcb/{name} desatualizado (rode gerar_ligacoes.py)",
                  file=sys.stderr)
        else:
            (HERE / name).write_text(files[name], encoding="utf-8")
            print(f"gravado: hardware/pcb/{name}")
    return 1 if args.check and stale else 0


if __name__ == "__main__":
    sys.exit(main())
