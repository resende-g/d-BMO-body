#!/usr/bin/env python3
"""Lista os fios (jumpers) que faltam numa placa de uma face, a partir da exportação de Gerber
do EasyEDA Pro. Não roteia nada: só lê o cobre que o autoroteador do EasyEDA deixou.

Uso:
  python3 hardware/placa-principal/fios.py hardware/placa-principal/gerber.zip
  python3 hardware/placa-principal/fios.py hardware/placa-energia/gerber.zip hardware/placa-energia/ligacoes.json

Lê `FlyingProbeTesting.json` (pads, com net e posição) e `Gerber_BottomLayer.GBL` (trilhas),
monta as ilhas de cobre ligado e:
  1. falha se uma ilha juntar pads de nets diferentes (curto) ou se houver trilha na camada de cima;
  2. para cada net em mais de uma ilha, escolhe os fios que ligam as ilhas pelo menor
     comprimento total (árvore geradora mínima entre ilhas, fio de pad a pad);
  3. mede a isolação mínima entre cobres de nets diferentes (pads quadrados como quadrados) e
     falha abaixo de ISOLACAO; mede o contorno e o cobre mais perto da borda; lista os trechos
     de trilha mais finos que TRILHA.

Imprime a tabela de fios em Markdown. Só a biblioteca padrão.
"""

from __future__ import annotations

import json
import math
import re
import sys
import zipfile
from pathlib import Path

MIL = 0.0254  # mm
ISOLACAO = 0.5  # mm, regra das placas do Tiago
TRILHA = 0.8  # mm, largura pedida pelo Tiago
HERE = Path(__file__).resolve().parent


def parse_tracks(gbl: str) -> list[tuple[float, float, float, float, float]]:
    """Segmentos (x1, y1, x2, y2, largura) em mm. Só aberturas circulares (trilhas)."""
    apertures = {m[1]: float(m[2]) for m in re.finditer(r"%ADD(\d+)C,([\d.]+)\*%", gbl)}
    if re.search(r"G3[67]\*|G0[23]", gbl):
        raise SystemExit("regiões ou arcos no Gerber: este script só entende trilhas retas")
    width = None
    x = y = 0.0
    segs = []
    for m in re.finditer(r"D(\d+)\*|(?:X(-?\d+))?(?:Y(-?\d+))?D0([123])\*", gbl):
        if m[1] and int(m[1]) >= 10:
            width = apertures.get(m[1])
            continue
        if m[4] is None:
            continue
        nx = int(m[2]) / 1e5 if m[2] else x
        ny = int(m[3]) / 1e5 if m[3] else y
        if m[4] == "1" and width is not None:
            segs.append((x, y, nx, ny, width))
        x, y = nx, ny
    return segs


def seg_dist(a, b) -> float:
    """Menor distância entre dois segmentos de reta."""
    def pt(px, py, x1, y1, x2, y2):
        dx, dy = x2 - x1, y2 - y1
        L = dx * dx + dy * dy
        t = 0.0 if L == 0 else max(0.0, min(1.0, ((px - x1) * dx + (py - y1) * dy) / L))
        return math.hypot(px - x1 - t * dx, py - y1 - t * dy)

    def cross(p, q, r):
        return (q[0] - p[0]) * (r[1] - p[1]) - (q[1] - p[1]) * (r[0] - p[0])

    p1, p2, p3, p4 = (a[0], a[1]), (a[2], a[3]), (b[0], b[1]), (b[2], b[3])
    d1, d2, d3, d4 = cross(p3, p4, p1), cross(p3, p4, p2), cross(p1, p2, p3), cross(p1, p2, p4)
    if d1 * d2 < 0 and d3 * d4 < 0:
        return 0.0
    return min(pt(*p1, *p3, *p4), pt(*p2, *p3, *p4), pt(*p3, *p1, *p2), pt(*p4, *p1, *p2))


def outline(x, y, sx, sy, ang) -> list[tuple[float, float, float, float, float]]:
    """Lados de um pad retangular girado de `ang` graus, como segmentos de largura zero."""
    c, s = math.cos(math.radians(ang)), math.sin(math.radians(ang))
    pts = [(x + dx * c - dy * s, y + dx * s + dy * c)
           for dx, dy in ((-sx / 2, -sy / 2), (sx / 2, -sy / 2), (sx / 2, sy / 2), (-sx / 2, sy / 2))]
    return [(*pts[i], *pts[(i + 1) % 4], 0.0) for i in range(4)]


def main(path: str, ligacoes: str | None = None) -> int:
    if zipfile.is_zipfile(path):
        z = zipfile.ZipFile(path)
        read = lambda name: z.read(name).decode()  # noqa: E731
    else:
        read = lambda name: (Path(path) / name).read_text()  # noqa: E731
    if "D01*" in read("Gerber_TopLayer.GTL"):
        raise SystemExit("há trilha na camada de cima: a placa não é de uma face")
    fp = json.loads(read("FlyingProbeTesting.json"))
    f = fp["pins"]["fields"]
    # O arquivo repete cada pad como "REF_PINO" e como "PADn_PINO", e troca alguns nomes de net
    # por "NET_n". O nome e a net de verdade vêm de REF_PINO e de ligacoes.json.
    expected = json.loads((Path(ligacoes) if ligacoes else HERE / "ligacoes.json").read_text())
    rows = [r for r in fp["pins"]["rows"]
            if r[f.index("NET_NAME")] and not r[f.index("PIN_NAME")].startswith("PAD")]
    # Pad sem ligação recebe uma "NET_n" só dele: não entra na conta.
    uses: dict[str, set] = {}
    for r in rows:
        uses.setdefault(r[f.index("NET_NAME")], set()).add(r[f.index("PIN_NAME")])
    pads = {}
    square = {}
    for r in rows:
        name, net = r[f.index("PIN_NAME")], r[f.index("NET_NAME")]
        ref, pin = name.rsplit("_", 1)
        real = expected.get(ref, {}).get(pin)
        if real is None:
            if len(uses[net]) == 1:
                continue
            raise SystemExit(f"{ref}.{pin} tem a net {net} na placa e nenhuma em ligacoes.json")
        x, y = r[f.index("PIN_X")] * MIL, r[f.index("PIN_Y")] * MIL
        sx, sy = r[f.index("PAD_SIZEX")] * MIL, r[f.index("PAD_SIZEY")] * MIL
        pads[f"{ref}.{pin}"] = (x, y, min(sx, sy) / 2, real)
        if r[f.index("PAD_SHAPE")] != "C":
            square[f"{ref}.{pin}"] = outline(x, y, sx, sy, r[f.index("PAD_ANGLE")])
    if len(pads) != sum(len(v) for v in expected.values()):
        raise SystemExit(f"{len(pads)} pads com net na placa; ligacoes.json tem "
                         f"{sum(len(v) for v in expected.values())}")
    segs = parse_tracks(read("Gerber_BottomLayer.GBL"))

    # Nós: pads (como segmento de comprimento zero) e trilhas; ligados quando o cobre se toca.
    nodes = [(x, y, x, y, 2 * rad) for x, y, rad, _ in pads.values()] + segs
    parent = list(range(len(nodes)))

    def find(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i

    for i in range(len(nodes)):
        for j in range(i + 1, len(nodes)):
            if seg_dist(nodes[i], nodes[j]) <= (nodes[i][4] + nodes[j][4]) / 2 - 1e-3:
                parent[find(i)] = find(j)

    names = list(pads)
    islands: dict[int, list[str]] = {}
    for k, name in enumerate(names):
        islands.setdefault(find(k), []).append(name)
    shorts = [sorted({pads[n][3] for n in members}) for members in islands.values()
              if len({pads[n][3] for n in members}) > 1]
    if shorts:
        for s in shorts:
            print("CURTO entre", ", ".join(s))
        return 1

    by_net: dict[str, list[list[str]]] = {}
    for members in islands.values():
        by_net.setdefault(pads[members[0]][3], []).append(members)

    wires = []
    for net, groups in sorted(by_net.items()):
        joined = [groups[0]]
        rest = groups[1:]
        while rest:  # Prim entre ilhas: o fio mais curto que liga uma ilha nova
            best = min(((math.dist(pads[a][:2], pads[b][:2]), a, b, g) for g in rest
                        for a in (p for j in joined for p in j) for b in g))
            wires.append((net, best[1], best[2], best[0]))
            joined.append(best[3])
            rest.remove(best[3])

    print("| Fio | Net | De | Até | Comprimento |")
    print("|---|---|---|---|---:|")
    for i, (net, a, b, length) in enumerate(wires, 1):
        print(f"| W{i} | `{net}` | {a} | {b} | {length:.1f} mm |")
    total = sum(w[3] for w in wires)
    print(f"\n{len(wires)} fios, {total:.0f} mm de fio no total; {len(segs)} trilhas; sem curtos.")

    # Isolação: menor distância de borda a borda entre cobres de ilhas com nets diferentes.
    net_of = {find(k): pads[n][3] for k, n in enumerate(names)}
    shapes = [square.get(n, [nodes[k]]) for k, n in enumerate(names)] + [[s] for s in segs]
    box = [(min(min(s[0], s[2]) - s[4] / 2 for s in sh), min(min(s[1], s[3]) - s[4] / 2 for s in sh),
            max(max(s[0], s[2]) + s[4] / 2 for s in sh), max(max(s[1], s[3]) + s[4] / 2 for s in sh))
           for sh in shapes]
    label = lambda i: names[i] if i < len(names) else f"trilha `{net_of.get(find(i))}`"  # noqa: E731
    best, low = (math.inf, 0, 0), []
    for i in range(len(nodes)):
        for j in range(i + 1, len(nodes)):
            if find(i) == find(j) or find(i) not in net_of or find(j) not in net_of:
                continue
            a, b = box[i], box[j]
            if max(a[0] - b[2], b[0] - a[2], a[1] - b[3], b[1] - a[3]) > ISOLACAO:
                continue
            d = min(seg_dist(p, q) - p[4] / 2 - q[4] / 2 for p in shapes[i] for q in shapes[j])
            best = min(best, (d, i, j))
            if d < ISOLACAO - 0.005:
                low.append(f"{label(i)} × {label(j)}: {d:.2f} mm")
    if best[0] < math.inf:
        print(f"Isolação mínima: {best[0]:.2f} mm ({label(best[1])} × {label(best[2])}).")
    for line in low:
        print(f"ISOLAÇÃO abaixo de {ISOLACAO} mm: {line}")

    # Borda: cobre até o retângulo que envolve o contorno da placa.
    x = y = 0.0
    corners = []
    for m in re.finditer(r"(?:X(-?\d+))?(?:Y(-?\d+))?D0[12]\*", read("Gerber_BoardOutlineLayer.GKO")):
        x, y = (int(m[1]) / 1e5 if m[1] else x), (int(m[2]) / 1e5 if m[2] else y)
        corners.append((x, y))
    x0, y0 = min(c[0] for c in corners), min(c[1] for c in corners)
    x1, y1 = max(c[0] for c in corners), max(c[1] for c in corners)
    edge = min((min(min(s[0], s[2]) - x0, x1 - max(s[0], s[2]), min(s[1], s[3]) - y0, y1 - max(s[1], s[3]))
                - s[4] / 2, k) for k, sh in enumerate(shapes) for s in sh)
    print(f"Contorno: {x1 - x0:.2f} × {y1 - y0:.2f} mm. Cobre mais perto da borda: {edge[0]:.2f} mm "
          f"({label(edge[1])}).")

    thin: dict[tuple, list] = {}
    for k, s in enumerate(segs, len(names)):
        if s[4] < TRILHA - 1e-6:
            near = min(range(len(names)), key=lambda p: seg_dist(nodes[p], s))
            t = thin.setdefault((net_of.get(find(k)), s[4]), [0.0, set()])
            t[0] += math.hypot(s[2] - s[0], s[3] - s[1])
            t[1].add(names[near])
    if thin:
        print(f"\nTrilhas mais finas que {TRILHA} mm:\n\n| Net | Largura | Comprimento | Perto de |\n|---|---:|---:|---|")
        for (net, w), (length, near) in sorted(thin.items(), key=lambda t: (t[0][0] or "", t[0][1])):
            print(f"| `{net}` | {w:.2f} mm | {length:.1f} mm | {', '.join(sorted(near))} |")
    return 1 if low else 0


if __name__ == "__main__":
    sys.exit(main(*sys.argv[1:3]))
