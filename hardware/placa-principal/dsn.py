#!/usr/bin/env python3
"""Gera um arquivo Specctra DSN de uma face para o Freerouting, a partir do Gerber exportado
pelo EasyEDA Pro (posição, forma e net de cada pad vêm de FlyingProbeTesting.json; o contorno,
de Gerber_BoardOutlineLayer.GKO).

Uso:
  python3 hardware/placa-principal/dsn.py gerber.zip ligacoes.json saida.dsn \
      [--largura 800] [--isolacao 500] [--borda 1000] [--keepout x1,y1,x2,y2 ...]

- Tudo em µm. Uma camada só (B.Cu, a de baixo): sem vias, o que o Freerouting não liga vira fio.
- Pads sem ligação entram como obstáculo (sem net), menos os que saem da pegada
  (`REMOVIDOS`: pinos livres da DevKitC, J3 10-14, J5-8, J9-4; README, "Pads tirados das
  pegadas"). Na placa de energia nenhum pad sai.
- A borda vale `--borda` de cobre até o contorno: o contorno do DSN encolhe borda - isolação.
- `--keepout` em mm, retângulo sem cobre (antena da DevKitC).
Depois: java -jar freerouting.jar -de saida.dsn -do saida.ses, e o .ses volta pelo MCP
(easyeda_prepare_pcb_external_import, formato specctra_ses, camada B.Cu -> 2).
Só a biblioteca padrão.
"""

from __future__ import annotations

import argparse
import json
import re
import zipfile
from pathlib import Path

MIL_UM = 25.4  # µm por mil
# Peças cujos pads sem ligação saem da pegada; nas outras eles ficam e viram obstáculo.
REMOVIDOS = {"U1", "J3", "J5", "J9"}


def removido(ref: str, pin: str, lig: dict) -> bool:
    """Pad sem ligação que sai da pegada (só na principal; no ligacoes da energia, U1 é o TP4056)."""
    return lig.get(ref, {}).get(pin) is None and "Q1" not in lig and ref in REMOVIDOS


def pads_do_gerber(gerber: str, lig: dict) -> tuple[list, tuple]:
    """Pads ligados [(ref, pino, x, y, forma, sx, sy)] em µm e o contorno (x1, y1, x2, y2) em µm."""
    z = zipfile.ZipFile(gerber)
    gko = z.read("Gerber_BoardOutlineLayer.GKO").decode()
    xs = [int(v) / 100 for v in re.findall(r"X(-?\d+)", gko)] or [0.0]  # mm com 5 decimais -> µm
    ys = [int(v) / 100 for v in re.findall(r"Y(-?\d+)", gko)] or [0.0]
    fp = json.loads(z.read("FlyingProbeTesting.json"))
    f = fp["pins"]["fields"]
    pads, seen = [], set()
    for r in fp["pins"]["rows"]:
        name = r[f.index("PIN_NAME")]
        if name.startswith("PAD"):
            continue
        ref, pin = name.rsplit("_", 1)
        if removido(ref, pin, lig) or (ref, pin) in seen:
            continue  # pad que sai da pegada, ou repetido (PTH aparece uma vez por face)
        seen.add((ref, pin))
        sx, sy = r[f.index("PAD_SIZEX")] * MIL_UM, r[f.index("PAD_SIZEY")] * MIL_UM
        if round(r[f.index("PAD_ANGLE")] or 0) % 180 == 90:
            sx, sy = sy, sx
        pads.append((ref, pin, r[f.index("PIN_X")] * MIL_UM, r[f.index("PIN_Y")] * MIL_UM,
                     r[f.index("PAD_SHAPE")], sx, sy))
    return pads, (min(xs), min(ys), max(xs), max(ys))


def escrever_dsn(pads: list, lig: dict, saida: str, contorno: tuple, largura: int = 800,
                 isolacao: int = 500, borda: int = 1000, keepouts: list = ()) -> None:
    m = borda - isolacao
    bx1, by1, bx2, by2 = contorno[0] + m, contorno[1] + m, contorno[2] - m, contorno[3] - m
    comps: dict[str, list] = {}
    stacks: dict[str, str] = {}
    nets: dict[str, list[str]] = {}
    for ref, pin, x, y, shape, sx, sy in pads:
        if shape == "R":
            key = f"R{round(sx)}x{round(sy)}"
            stacks[key] = f"(rect B.Cu {-sx / 2:.0f} {-sy / 2:.0f} {sx / 2:.0f} {sy / 2:.0f})"
        elif abs(sx - sy) < 1:
            key = f"C{round(sx)}"
            stacks[key] = f"(circle B.Cu {sx:.0f})"
        else:  # oval: traço com a largura menor
            w, L = min(sx, sy), abs(sx - sy) / 2
            key = f"O{round(sx)}x{round(sy)}"
            seg = f"{-L:.0f} 0 {L:.0f} 0" if sx > sy else f"0 {-L:.0f} 0 {L:.0f}"
            stacks[key] = f"(path B.Cu {w:.0f} {seg})"
        comps.setdefault(ref, []).append((pin, key, x, y))
        if lig.get(ref, {}).get(pin) is not None:  # sem net: obstáculo
            nets.setdefault(lig[ref][pin], []).append(f"{ref}-{pin}")
    out = [f'(pcb "{Path(saida).name}"',
           '  (parser (string_quote ") (space_in_quoted_tokens on) (host_cad "dbmo") (host_version "1"))',
           "  (resolution um 1)", "  (unit um)",
           "  (structure", "    (layer B.Cu (type signal) (property (index 0)))",
           f"    (boundary (rect pcb {bx1:.0f} {by1:.0f} {bx2:.0f} {by2:.0f}))"]
    for x1, y1, x2, y2 in keepouts:
        out.append(f'    (keepout "" (rect B.Cu {x1 * 1000:.0f} {y1 * 1000:.0f} {x2 * 1000:.0f} {y2 * 1000:.0f}))')
    out += ['    (via "VIA")', f"    (rule (width {largura}) (clearance {isolacao}))", "  )",
            "  (placement"]
    out += [f'    (component "IMG_{ref}" (place "{ref}" 0 0 front 0))' for ref in comps]
    out += ["  )", "  (library"]
    for ref, pins in comps.items():
        out.append(f'    (image "IMG_{ref}"')
        out += [f'      (pin "{key}" "{pin}" {x:.0f} {y:.0f})' for pin, key, x, y in pins]
        out.append("    )")
    out += [f'    (padstack "{key}" (shape {shape}) (attach off))' for key, shape in stacks.items()]
    out.append('    (padstack "VIA" (shape (circle B.Cu 2000)) (attach off))')
    out += ["  )", "  (network"]
    multi = [n for n, p in nets.items() if len(p) > 1]
    out += [f'    (net "{n}" (pins {" ".join(nets[n])}))' for n in multi]
    out.append(f'    (class "padrao" {" ".join(chr(34) + n + chr(34) for n in multi)}'
               f' (circuit (use_via "VIA")) (rule (width {largura}) (clearance {isolacao})))')
    out += ["  )", "  (wiring)", ")"]
    Path(saida).write_text("\n".join(out) + "\n")


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("gerber"); ap.add_argument("ligacoes"); ap.add_argument("saida")
    ap.add_argument("--largura", type=int, default=800)
    ap.add_argument("--isolacao", type=int, default=500)
    ap.add_argument("--borda", type=int, default=1000)
    ap.add_argument("--keepout", action="append", default=[])
    a = ap.parse_args()
    lig = json.loads(Path(a.ligacoes).read_text())
    pads, contorno = pads_do_gerber(a.gerber, lig)
    keep = [tuple(float(v) for v in k.split(",")) for k in a.keepout]
    escrever_dsn(pads, lig, a.saida, contorno, a.largura, a.isolacao, a.borda, keep)
    print(f"{len({p[0] for p in pads})} peças, {len(pads)} pads; contorno "
          f"{contorno[0] / 1000:.1f},{contorno[1] / 1000:.1f} a {contorno[2] / 1000:.1f},{contorno[3] / 1000:.1f} mm")


if __name__ == "__main__":
    main()
