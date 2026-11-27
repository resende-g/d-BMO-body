#!/usr/bin/env python3
"""Roteia uma placa de uma face com o Freerouting a partir do Gerber exportado pelo EasyEDA Pro.

Uso:
  python3 hardware/placa-principal/rotear.py gerber.zip ligacoes.json saida \
      --jar freerouting-2.5.0.jar [--java java] [--tentativas 8] [--passes 60] \
      [--largura 800] [--isolacao 500] [--keepout x1,y1,x2,y2 ...]

O Freerouting é determinístico, mas o resultado muda com a ordem das peças no DSN. Cada
tentativa embaralha essa ordem; fica a que deixa menos conexões sem rota. Grava `saida.ses`
(sem a seção de posicionamento, para a importação não mover peças) e `saida.dsn`.
O SES volta ao EasyEDA pelo MCP: easyeda_prepare_pcb_external_import, formato specctra_ses,
layerMapping {"B.Cu": 2}. Precisa de Java 25 ou mais novo.
"""

from __future__ import annotations

import argparse
import json
import random
import re
import subprocess
import tempfile
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

import dsn


def uma(i: int, pads: list, lig: dict, contorno: tuple, a, tmp: Path) -> tuple[int, int, int]:
    p = list(pads)
    random.Random(i).shuffle(p)
    d, s, log = tmp / f"t{i}.dsn", tmp / f"t{i}.ses", tmp / f"t{i}.log"
    keep = [tuple(float(v) for v in k.split(",")) for k in a.keepout]
    dsn.escrever_dsn(p, lig, str(d), contorno, a.largura, a.isolacao, a.borda, keep)
    try:  # o Freerouting às vezes trava numa passagem
        with open(log, "w") as fh:
            subprocess.run([a.java, "-jar", a.jar, "-de", str(d), "-do", str(s), "-mp", str(a.passes),
                            "--gui.enabled=false"], stdout=fh, stderr=subprocess.STDOUT,
                           timeout=240 + 8 * a.passes)
    except subprocess.TimeoutExpired:
        return 999, 0, i
    m = re.findall(r"\((\d+) unrouted and (\d+) violations?\)", log.read_text())
    return (int(m[-1][0]), int(m[-1][1]), i) if m and s.exists() else (999, 0, i)


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("gerber"); ap.add_argument("ligacoes"); ap.add_argument("saida")
    ap.add_argument("--jar", required=True)
    ap.add_argument("--java", default="/opt/homebrew/opt/openjdk/bin/java")
    ap.add_argument("--tentativas", type=int, default=8)
    ap.add_argument("--passes", type=int, default=60)
    ap.add_argument("--largura", type=int, default=800)
    ap.add_argument("--isolacao", type=int, default=500)
    ap.add_argument("--borda", type=int, default=1000)
    ap.add_argument("--keepout", action="append", default=[])
    ap.add_argument("--contorno", help="x1,y1,x2,y2 em mm, quando o Gerber ainda não tem contorno")
    a = ap.parse_args()
    lig = json.loads(Path(a.ligacoes).read_text())
    pads, contorno = dsn.pads_do_gerber(a.gerber, lig)
    if a.contorno:
        contorno = tuple(float(v) * 1000 for v in a.contorno.split(","))
    tmp = Path(tempfile.mkdtemp(prefix="rotear-"))
    with ThreadPoolExecutor(4) as ex:
        res = list(ex.map(lambda i: uma(i, pads, lig, contorno, a, tmp), range(a.tentativas)))
    for n, v, i in sorted(res):
        print(f"tentativa {i}: {n} sem rota, {v} violações")
    n, v, i = min(res)
    if n == 999:
        raise SystemExit("nenhuma tentativa terminou")
    ses = (tmp / f"t{i}.ses").read_text()
    ses = re.sub(r"\(placement.*?\n  \)\n", "", ses, count=1, flags=re.S)  # não mover peças
    Path(a.saida + ".ses").write_text(ses)
    Path(a.saida + ".dsn").write_text((tmp / f"t{i}.dsn").read_text())
    print(f"melhor: tentativa {i}, {n} sem rota, {v} violações -> {a.saida}.ses")


if __name__ == "__main__":
    main()
