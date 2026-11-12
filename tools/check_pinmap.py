#!/usr/bin/env python3
"""Confere o mapa de pinos canônico contra tudo o que o repete.

Fonte de verdade: firmware/config/include/dbmo/config/pins.h.

Confere:
  1. firmware/bruce/bmo.ini      -> cada -D de pino igual ao perfil bench/target;
  2. firmware/retro-go/.../config.h -> cada RG_GPIO_* igual ao perfil target;
  3. blocos gerados nos Markdown (entre os marcadores BEGIN/END pinmap) -> atualizados.

Uso:
  tools/check_pinmap.py           confere e sai com 1 se houver divergência
  tools/check_pinmap.py --write   regrava os blocos gerados nos Markdown

Sem dependências além da biblioteca padrão.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PINS_H = ROOT / "firmware/config/include/dbmo/config/pins.h"
BMO_INI = ROOT / "firmware/bruce/bmo.ini"
RETRO_GO_CONFIG = ROOT / "firmware/retro-go/targets/dbmo-v1/config.h"

# Arquivos Markdown que carregam um bloco gerado, e o perfil de cada bloco.
GENERATED_BLOCKS = [
    (ROOT / "docs/hardware/GPIO_MAP.md", "target"),
    (ROOT / "docs/hardware/GPIO_MAP.md", "bench"),
    (ROOT / "README.md", "target"),
]

# Flag do Bruce -> (perfil, constante de pins.h).
BRUCE_FLAGS = {
    "TFT_CS": ("target", "kDisplayCs"),
    "TFT_DC": ("target", "kDisplayDc"),
    "TFT_RST": ("target", "kDisplayReset"),
    "TFT_MOSI": ("target", "kSpiMosi"),
    "TFT_SCLK": ("target", "kSpiSck"),
    "TFT_MISO": ("target", "kSpiMiso"),
    "TFT_BL": ("target", "kDisplayBacklight"),
    "SPI_SCK_PIN": ("target", "kSpiSck"),
    "SPI_MOSI_PIN": ("target", "kSpiMosi"),
    "SPI_MISO_PIN": ("target", "kSpiMiso"),
    "SPI_SS_PIN": ("target", "kCc1101Cs"),
    "SDCARD_CS": ("target", "kSdCs"),
    "SDCARD_SCK": ("target", "kSpiSck"),
    "SDCARD_MISO": ("target", "kSpiMiso"),
    "SDCARD_MOSI": ("target", "kSpiMosi"),
    "CC1101_SS_PIN": ("target", "kCc1101Cs"),
    "CC1101_GDO0_PIN": ("target", "kCc1101Gdo0"),
    "CC1101_MOSI_PIN": ("target", "kSpiMosi"),
    "CC1101_SCK_PIN": ("target", "kSpiSck"),
    "CC1101_MISO_PIN": ("target", "kSpiMiso"),
    "GROVE_SDA": ("target", "kI2cSda"),
    "GROVE_SCL": ("target", "kI2cScl"),
    "UP_BTN": ("bench", "kButtonUp"),
    "SEL_BTN": ("bench", "kButtonOk"),
    "DW_BTN": ("bench", "kButtonDown"),
    "DEEPSLEEP_WAKEUP_PIN": ("bench", "kButtonOk"),
    "TXLED": ("bench", "kIrTx"),
    "RXLED": ("bench", "kIrRx"),
}

# Macro do alvo do Retro-Go -> constante do perfil target.
RETRO_GO_MACROS = {
    "RG_GPIO_LCD_MISO": "kSpiMiso",
    "RG_GPIO_LCD_MOSI": "kSpiMosi",
    "RG_GPIO_LCD_CLK": "kSpiSck",
    "RG_GPIO_LCD_CS": "kDisplayCs",
    "RG_GPIO_LCD_DC": "kDisplayDc",
    "RG_GPIO_LCD_BCKL": "kDisplayBacklight",
    "RG_GPIO_LCD_RST": "kDisplayReset",
    "RG_GPIO_SDSPI_MISO": "kSpiMiso",
    "RG_GPIO_SDSPI_MOSI": "kSpiMosi",
    "RG_GPIO_SDSPI_CLK": "kSpiSck",
    "RG_GPIO_SDSPI_CS": "kSdCs",
    "RG_GPIO_SND_I2S_BCK": "kI2sBclk",
    "RG_GPIO_SND_I2S_WS": "kI2sLrclk",
    "RG_GPIO_SND_I2S_DATA": "kI2sSpeakerData",
    "RG_GPIO_SND_AMP_ENABLE": "kAmpShutdown",
    "RG_GPIO_I2C_SDA": "kI2cSda",
    "RG_GPIO_I2C_SCL": "kI2cScl",
}

STATUS_LABEL = {
    "ChipFixed": "CONFIRMADO (fabricante)",
    "BenchValidated": "VALIDADO EM BANCADA",
    "Proposed": "PROPOSTO",
    "ProposedAtRisk": "PROPOSTO, COM RISCO ABERTO",
}


class PinMap:
    """Constantes e tabelas lidas de pins.h, por perfil."""

    def __init__(self, text: str):
        self.constants: dict[str, dict[str, int]] = {}
        self.tables: dict[str, list[tuple[str, int, str]]] = {}
        self._parse(text)

    def _parse(self, text: str) -> None:
        namespace = None
        pending: dict[str, list[tuple[str, str, str]]] = {}
        in_table = False
        for line in text.splitlines():
            opened = re.match(r"\s*namespace (\w+) \{", line)
            if opened:
                namespace = opened.group(1)
                self.constants.setdefault(namespace, {})
                continue
            if re.match(r"\s*\}\s*//\s*namespace", line):
                namespace = None
                in_table = False
                continue
            if namespace is None:
                continue

            constant = re.match(r"\s*inline constexpr u?int\d+_t (k\w+) = (0x[0-9A-Fa-f]+|\d+);", line)
            if constant:
                self.constants[namespace][constant.group(1)] = int(constant.group(2), 0)
                continue

            if "PinAssignment kPins[]" in line:
                in_table = True
                pending[namespace] = []
                continue
            if in_table:
                if line.strip().startswith("};"):
                    in_table = False
                    continue
                row = re.match(r'\s*\{"(\w+)", ([\w:]+), PinStatus::(\w+)\},', line)
                if row:
                    pending[namespace].append((row.group(1), row.group(2), row.group(3)))

        for profile, rows in pending.items():
            resolved = []
            for signal, reference, status in rows:
                if "::" in reference:
                    owner, name = reference.split("::")
                else:
                    owner, name = profile, reference
                resolved.append((signal, self.constants[owner][name], status))
            self.tables[profile] = resolved

    def gpio(self, profile: str, name: str) -> int:
        return self.constants[profile][name]


def check_bruce(pins: PinMap, errors: list[str]) -> int:
    if not BMO_INI.exists():
        errors.append(f"{BMO_INI.relative_to(ROOT)}: arquivo ausente")
        return 0
    text = BMO_INI.read_text(encoding="utf-8")
    # Só o ambiente base: ambientes derivados herdam dele.
    defines = dict(re.findall(r"^\s*-D\s*(\w+)=(-?\d+)\s*(?:;.*)?$", text, flags=re.M))
    checked = 0
    for flag, (profile, name) in BRUCE_FLAGS.items():
        if flag not in defines:
            continue
        checked += 1
        expected = pins.gpio(profile, name)
        if int(defines[flag]) != expected:
            errors.append(
                f"bmo.ini: -D{flag}={defines[flag]}, mas pins.h {profile}::{name} = {expected}"
            )
    return checked


def check_retro_go(pins: PinMap, errors: list[str]) -> int:
    if not RETRO_GO_CONFIG.exists():
        return 0
    text = RETRO_GO_CONFIG.read_text(encoding="utf-8")
    defines = dict(re.findall(r"^\s*#define\s+(\w+)\s+(\w+)", text, flags=re.M))

    def resolve(macro: str) -> str | None:
        # Segue apelidos (uma macro definida como outra) até chegar a um número.
        value = defines.get(macro)
        for _ in range(8):
            if value is None or value not in defines:
                break
            value = defines[value]
        return value.removeprefix("GPIO_NUM_") if value else None

    checked = 0
    for macro, name in RETRO_GO_MACROS.items():
        value = resolve(macro)
        if value is None or value == "NC":
            continue
        checked += 1
        expected = pins.gpio("target", name)
        if not value.isdigit() or int(value) != expected:
            errors.append(
                f"retro-go config.h: {macro} = {value}, mas pins.h target::{name} = {expected}"
            )
    return checked


def render_block(pins: PinMap, profile: str) -> str:
    rows = sorted(pins.tables[profile], key=lambda row: row[1])
    lines = [
        f"<!-- BEGIN pinmap:{profile} (gerado por tools/check_pinmap.py --write; não editar) -->",
        "",
        "| GPIO | Sinal | Classificação |",
        "|---:|---|---|",
    ]
    for signal, gpio, status in rows:
        lines.append(f"| {gpio} | `{signal}` | {STATUS_LABEL[status]} |")
    lines += ["", f"<!-- END pinmap:{profile} -->"]
    return "\n".join(lines)


def block_pattern(profile: str) -> re.Pattern[str]:
    return re.compile(
        rf"<!-- BEGIN pinmap:{profile} .*?-->.*?<!-- END pinmap:{profile} -->", flags=re.S
    )


def check_blocks(pins: PinMap, write: bool, errors: list[str]) -> int:
    checked = 0
    for path, profile in GENERATED_BLOCKS:
        if not path.exists():
            continue
        text = path.read_text(encoding="utf-8")
        pattern = block_pattern(profile)
        match = pattern.search(text)
        if not match:
            continue
        checked += 1
        expected = render_block(pins, profile)
        if match.group(0) == expected:
            continue
        if write:
            path.write_text(pattern.sub(lambda _: expected, text), encoding="utf-8")
            print(f"atualizado: {path.relative_to(ROOT)} (pinmap:{profile})")
        else:
            errors.append(
                f"{path.relative_to(ROOT)}: bloco pinmap:{profile} desatualizado "
                "(rode tools/check_pinmap.py --write)"
            )
    return checked


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--write", action="store_true", help="regrava os blocos gerados")
    args = parser.parse_args()

    pins = PinMap(PINS_H.read_text(encoding="utf-8"))
    for profile in ("target", "bench"):
        if not pins.tables.get(profile):
            print(f"erro: tabela kPins do perfil {profile} não encontrada em pins.h", file=sys.stderr)
            return 2

    errors: list[str] = []
    bruce = check_bruce(pins, errors)
    retro = check_retro_go(pins, errors)
    blocks = check_blocks(pins, args.write, errors)

    print(
        f"pins.h: {len(pins.tables['target'])} sinais no perfil target, "
        f"{len(pins.tables['bench'])} no perfil bench"
    )
    print(f"conferidos: {bruce} flags do Bruce, {retro} macros do Retro-Go, {blocks} blocos Markdown")
    for error in errors:
        print(f"DIVERGÊNCIA: {error}", file=sys.stderr)
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
