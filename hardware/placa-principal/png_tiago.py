#!/usr/bin/env python3
"""Imagem da camada de cobre para o Tiago gravar a laser, a partir do Gerber do EasyEDA Pro.

Uso:
  python3 hardware/placa-principal/png_tiago.py gerber.zip saida [--dpi 600]

Gera `saida-vista-de-cima.svg/.png` (como o EasyEDA mostra, olhando pelo lado das peças) e
`saida-espelhada.svg/.png` (lado do cobre visto de frente: é a que vai para a gravação, porque o
cobre desta placa está na camada de baixo). Preto = cobre, branco = sem cobre, furos em branco no
centro dos pads, contorno da placa em linha fina. Escala 1:1 (o PNG sai no DPI pedido).
O Tiago engrossa trilhas e pads depois no Paint. Precisa do Google Chrome para o PNG.
Só a biblioteca padrão.
"""

from __future__ import annotations

import re
import subprocess
import sys
import zipfile
from pathlib import Path

CHROME = "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome"


def gerber_svg(g: str) -> list[str]:
    """Trilhas (D01), pads (D03) e regiões (G36/G37) de um Gerber em mm, como elementos SVG."""
    ap: dict[str, tuple] = {}
    for m in re.finditer(r"%ADD(\d+)([CRO]),([\d.]+)(?:X([\d.]+))?\*%", g):
        ap[m[1]] = (m[2], float(m[3]), float(m[4] or m[3]))
    out, cur, x, y, reg, pts = [], None, 0.0, 0.0, False, []
    for m in re.finditer(r"G36\*|G37\*|D(\d+)\*|(?:X(-?\d+))?(?:Y(-?\d+))?D0([123])\*", g):
        t = m[0]
        if t == "G36*": reg, pts = True, []; continue
        if t == "G37*":
            if pts: out.append(f'<polygon points="{" ".join(f"{a:.3f},{-b:.3f}" for a, b in pts)}"/>')
            reg = False; continue
        if m[1] and int(m[1]) >= 10: cur = ap.get(m[1]); continue
        if m[4] is None: continue
        nx = int(m[2]) / 1e5 if m[2] else x
        ny = int(m[3]) / 1e5 if m[3] else y
        op = m[4]
        if reg:
            if op == "2": pts = [(nx, ny)]
            else: pts.append((nx, ny))
        elif op == "1" and cur:
            out.append(f'<line x1="{x:.3f}" y1="{-y:.3f}" x2="{nx:.3f}" y2="{-ny:.3f}" stroke="#000" stroke-width="{cur[1]:.3f}" stroke-linecap="round"/>')
        elif op == "3" and cur:
            f, a, b = cur
            if f == "C":
                out.append(f'<circle cx="{nx:.3f}" cy="{-ny:.3f}" r="{a / 2:.3f}"/>')
            else:
                rx = min(a, b) / 2 if f == "O" else 0
                out.append(f'<rect x="{nx - a / 2:.3f}" y="{-ny - b / 2:.3f}" width="{a:.3f}" height="{b:.3f}" rx="{rx:.3f}"/>')
        x, y = nx, ny
    return out


def furos(drl: str) -> list[tuple[float, float, float]]:
    """Furos do Excellon (mm): (x, y, diâmetro)."""
    tools = {m[1]: float(m[2]) for m in re.finditer(r"^T(\d+)C([\d.]+)", drl, re.M)}
    out, d = [], 0.0
    for line in drl.splitlines():
        m = re.match(r"^T(\d+)$", line.strip())
        if m: d = tools.get(m[1], d); continue
        m = re.match(r"^X(-?[\d.]+)Y(-?[\d.]+)", line.strip())
        if m: out.append((float(m[1]), float(m[2]), d))
    return out


def main(zipf: str, saida: str, dpi: int = 600) -> None:
    z = zipfile.ZipFile(zipf)
    names = z.namelist()
    cobre = gerber_svg(z.read("Gerber_BottomLayer.GBL").decode())
    borda = gerber_svg(z.read("Gerber_BoardOutlineLayer.GKO").decode())
    drl = next((n for n in names if n.upper().endswith(".DRL") and "PTH" in n.upper()), None)
    holes = furos(z.read(drl).decode()) if drl else []
    xs = [float(v) for e in borda for v in re.findall(r'x\d?="(-?[\d.]+)"', e)]
    ys = [float(v) for e in borda for v in re.findall(r'y\d?="(-?[\d.]+)"', e)]
    x0, x1, y0, y1 = min(xs) - 1, max(xs) + 1, min(ys) - 1, max(ys) + 1
    w, h = x1 - x0, y1 - y0
    for nome, espelho in (("vista-de-cima", False), ("espelhada", True)):
        tr = f"translate({x0 + x1} 0) scale(-1 1)" if espelho else ""
        svg = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}mm" height="{h}mm" viewBox="{x0} {y0} {w} {h}">',
               f'<rect x="{x0}" y="{y0}" width="{w}" height="{h}" fill="#fff"/>',
               f'<g transform="{tr}">',
               '<g fill="#000">', *cobre, "</g>",
               '<g fill="#fff">', *[f'<circle cx="{a:.3f}" cy="{-b:.3f}" r="{max(d / 2 - 0.15, 0.2):.3f}"/>' for a, b, d in holes], "</g>",
               '<g fill="none">', *[re.sub(r'stroke="#000" stroke-width="[\d.]+"', 'stroke="#888" stroke-width="0.1"', e) for e in borda], "</g>",
               "</g></svg>"]
        f = Path(f"{saida}-{nome}.svg")
        f.write_text("\n".join(svg))
        px_w, px_h = round(w / 25.4 * dpi), round(h / 25.4 * dpi)
        html = f.with_suffix(".html")
        html.write_text(f'<html><body style="margin:0"><img src="{f.name}" style="width:{px_w}px;height:{px_h}px;display:block"></body></html>')
        subprocess.run([CHROME, "--headless=new", "--disable-gpu", "--hide-scrollbars", "--force-device-scale-factor=1",
                        f"--screenshot={f.with_suffix('.png')}", f"--window-size={px_w},{px_h}", f"file://{html.resolve()}"],
                       capture_output=True, timeout=300)
        html.unlink()
        print(f.with_suffix(".png"), f"{w:.1f} × {h:.1f} mm a {dpi} dpi")


if __name__ == "__main__":
    a = sys.argv[1:]
    dpi = int(a.pop(a.index("--dpi") + 1)) if "--dpi" in a else 600
    a = [v for v in a if v != "--dpi"]
    main(a[0], a[1], dpi)
