#!/usr/bin/env python3
"""Confere os links relativos dos arquivos Markdown do repositório.

Para cada link ou imagem com destino local: o arquivo precisa existir e, se houver
âncora (#secao) apontando para um Markdown, o título correspondente precisa existir
(regra de âncoras do GitHub). Links http(s) e mailto não são acessados.

Uso: tools/check_links.py [arquivos.md ...]   (sem argumentos: todos os do repositório)
Sem dependências além da biblioteca padrão.
"""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path
from urllib.parse import unquote

ROOT = Path(__file__).resolve().parent.parent
SKIP_DIRS = {"build", ".git", ".pio", "node_modules", "third_party"}

LINK = re.compile(r"!?\[(?:[^\]\[]|\[[^\]]*\])*\]\(\s*<?([^)\s>]+)>?(?:\s+\"[^\"]*\")?\s*\)")
HEADING = re.compile(r"^(#{1,6})\s+(.*?)\s*#*\s*$")
FENCE = re.compile(r"^\s*(```|~~~)")
HTML_ANCHOR = re.compile(r"<a\s+(?:name|id)=\"([^\"]+)\"")


def markdown_files() -> list[Path]:
    try:
        out = subprocess.run(
            ["git", "ls-files", "--cached", "--others", "--exclude-standard", "*.md"],
            cwd=ROOT, check=True, capture_output=True, text=True,
        ).stdout
        files = [ROOT / line for line in out.splitlines() if line]
    except (OSError, subprocess.CalledProcessError):
        files = list(ROOT.rglob("*.md"))
    return sorted(f for f in files if f.exists() and not (set(f.relative_to(ROOT).parts) & SKIP_DIRS))


def slugify(title: str) -> str:
    """Âncora que o GitHub gera para um título."""
    text = re.sub(r"!?\[([^\]]*)\]\([^)]*\)", r"\1", title)  # links: fica só o texto
    text = re.sub(r"<[^>]+>", "", text)                       # HTML embutido
    text = text.replace("`", "").replace("*", "")
    text = text.strip().lower()
    text = re.sub(r"[^\w\- ]", "", text, flags=re.UNICODE)
    return text.replace(" ", "-")


def strip_code(line: str) -> str:
    return re.sub(r"`[^`]*`", "", line)


def anchors_of(path: Path, cache: dict[Path, set[str]]) -> set[str]:
    if path in cache:
        return cache[path]
    anchors: set[str] = set()
    counts: dict[str, int] = {}
    in_fence = False
    for line in path.read_text(encoding="utf-8").splitlines():
        if FENCE.match(line):
            in_fence = not in_fence
            continue
        if in_fence:
            continue
        anchors.update(HTML_ANCHOR.findall(line))
        heading = HEADING.match(line)
        if not heading:
            continue
        slug = slugify(heading.group(2))
        seen = counts.get(slug, 0)
        counts[slug] = seen + 1
        anchors.add(slug if seen == 0 else f"{slug}-{seen}")
    cache[path] = anchors
    return anchors


def check_file(path: Path, cache: dict[Path, set[str]]) -> list[str]:
    errors: list[str] = []
    in_fence = False
    for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
        if FENCE.match(line):
            in_fence = not in_fence
            continue
        if in_fence:
            continue
        for target in LINK.findall(strip_code(line)):
            if re.match(r"^[a-zA-Z][a-zA-Z0-9+.-]*:", target):  # http:, https:, mailto: ...
                continue
            file_part, _, anchor = target.partition("#")
            file_part = unquote(file_part)
            destination = path if not file_part else (path.parent / file_part).resolve()
            where = f"{path.relative_to(ROOT)}:{number}"
            if not destination.exists():
                errors.append(f"{where}: destino inexistente: {target}")
                continue
            if anchor and destination.suffix.lower() == ".md":
                if unquote(anchor).lower() not in anchors_of(destination, cache):
                    errors.append(f"{where}: âncora inexistente: {target}")
    return errors


def main() -> int:
    files = [Path(arg).resolve() for arg in sys.argv[1:]] or markdown_files()
    cache: dict[Path, set[str]] = {}
    errors: list[str] = []
    for path in files:
        errors.extend(check_file(path, cache))
    print(f"{len(files)} arquivos Markdown conferidos, {len(errors)} links quebrados")
    for error in errors:
        print(f"LINK QUEBRADO: {error}", file=sys.stderr)
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
