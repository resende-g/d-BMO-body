#!/usr/bin/env python3
"""Verificações de integridade do repositório que não dependem de compilar nada.

  1. Estrutura: cada subsistema tem CMakeLists.txt, library.json, README.md e testes.
  2. Portabilidade: o código portável (tudo em firmware/ menos platform/, app/, bruce/,
     retro-go/ e os sketches) não usa Arduino, alocação dinâmica, exceções nem RTTI.
  3. Privacidade: o firmware próprio não inclui pilha de rede (sem telemetria por
     construção) e o repositório não versiona arquivos de segredo.
  4. Scripts de shell passam em "bash -n".

Uso: tools/check_repo.py
Sem dependências além da biblioteca padrão.
"""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
FIRMWARE = ROOT / "firmware"

SUBSYSTEMS = [
    "core", "config", "display", "input", "audio", "radio", "nfc",
    "infrared", "storage", "power", "cartridges", "rosto",
]
# Subsistemas só de cabeçalho ou sem testes próprios em tests/unit/<nome>.
NO_README_REQUIRED = {"core", "config"}

# Pastas de firmware que podem depender do Arduino/ESP-IDF ou que são de terceiros.
NON_PORTABLE_DIRS = {"platform", "app", "bruce", "retro-go", "teste_tela", "teste_cc1101"}

FORBIDDEN_IN_PORTABLE = [
    (re.compile(r"#\s*include\s*[<\"](Arduino|Wire|SPI|WiFi|FS|SD)\.h[>\"]"), "cabeçalho do Arduino"),
    (re.compile(r"#\s*include\s*[<\"](esp_|driver/|freertos/)"), "cabeçalho do ESP-IDF"),
    (re.compile(r"#\s*include\s*<(string|vector|functional|iostream|memory|map|list|sstream)>"),
     "biblioteca padrão com alocação dinâmica"),
    (re.compile(r"\bstd::(string|vector|function|map|list|unique_ptr|shared_ptr)\b"),
     "tipo com alocação dinâmica"),
    (re.compile(r"(?<![\w.>:])(malloc|calloc|realloc)\s*\("), "alocação dinâmica"),
    # free(ponteiro): com argumento, para não acusar métodos chamados free().
    (re.compile(r"(?<![\w.>:])free\s*\(\s*[^)\s]"), "alocação dinâmica (free)"),
    (re.compile(r"(?<![\w.])new\s+[A-Za-z_(]"), "alocação dinâmica (new)"),
    (re.compile(r"\b(throw|try|catch)\b\s*[({]"), "exceções"),
    (re.compile(r"\b(dynamic_cast|typeid)\b"), "RTTI"),
]

NETWORK_INCLUDES = re.compile(
    r"#\s*include\s*[<\"](WiFi|WiFiClient|WiFiClientSecure|HTTPClient|ESPmDNS|PubSubClient|"
    r"esp_http_client|esp_wifi|esp_now|BLEDevice|NimBLEDevice|Ethernet|AsyncUDP)\.h[>\"]"
)

SECRET_FILE_NAMES = {".env", "secrets.h", "secrets.py", "credentials.h", "id_rsa", "id_ed25519"}
SECRET_CONTENT = re.compile(r"-----BEGIN [A-Z ]*PRIVATE KEY-----")


def tracked_files() -> list[Path]:
    out = subprocess.run(
        ["git", "ls-files", "--cached", "--others", "--exclude-standard"],
        cwd=ROOT, check=True, capture_output=True, text=True,
    ).stdout
    return [ROOT / line for line in out.splitlines() if line and (ROOT / line).is_file()]


def strip_comments_and_strings(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", lambda m: "\n" * m.group(0).count("\n"), text, flags=re.S)
    text = re.sub(r"//[^\n]*", "", text)
    return re.sub(r'"(?:\\.|[^"\\\n])*"', '""', text)


def check_structure(errors: list[str]) -> None:
    for name in SUBSYSTEMS:
        base = FIRMWARE / name
        if not base.is_dir():
            errors.append(f"estrutura: firmware/{name}/ não existe")
            continue
        required = ["CMakeLists.txt", "library.json"]
        if name not in NO_README_REQUIRED:
            required.append("README.md")
        for filename in required:
            if not (base / filename).is_file():
                errors.append(f"estrutura: firmware/{name}/{filename} ausente")
        if not (ROOT / "tests/unit" / name / "CMakeLists.txt").is_file():
            errors.append(f"estrutura: tests/unit/{name}/CMakeLists.txt ausente")


def is_portable_source(path: Path) -> bool:
    try:
        relative = path.relative_to(FIRMWARE)
    except ValueError:
        return False
    if relative.parts[0] in NON_PORTABLE_DIRS:
        return False
    if path.suffix not in {".h", ".hpp", ".cpp", ".c"}:
        return False
    # Ferramentas de computador e exemplos podem usar a biblioteca padrão completa.
    return not ({"tools", "examples", "teste"} & set(relative.parts))


def check_portability(files: list[Path], errors: list[str]) -> int:
    checked = 0
    for path in files:
        if not is_portable_source(path):
            continue
        checked += 1
        code = strip_comments_and_strings(path.read_text(encoding="utf-8"))
        for number, line in enumerate(code.splitlines(), start=1):
            for pattern, what in FORBIDDEN_IN_PORTABLE:
                if pattern.search(line):
                    errors.append(f"portabilidade: {path.relative_to(ROOT)}:{number}: {what}")
    return checked


def check_privacy(files: list[Path], errors: list[str]) -> None:
    for path in files:
        relative = path.relative_to(ROOT)
        if path.name in SECRET_FILE_NAMES:
            errors.append(f"segredo: arquivo {relative} não deve ser versionado")
        if path.suffix in {".h", ".hpp", ".cpp", ".c", ".ino", ".py", ".sh", ".ini", ".md", ".json",
                           ".yml", ".yaml", ".txt", ".cfg"}:
            try:
                text = path.read_text(encoding="utf-8")
            except UnicodeDecodeError:
                continue
            if SECRET_CONTENT.search(text) and "tools/check_repo.py" not in str(relative):
                errors.append(f"segredo: chave privada em {relative}")
            own_firmware = (
                relative.parts[:1] == ("firmware",)
                and len(relative.parts) > 1
                and relative.parts[1] not in {"bruce", "retro-go"}
            )
            if own_firmware and path.suffix in {".h", ".hpp", ".cpp", ".c", ".ino"}:
                for number, line in enumerate(text.splitlines(), start=1):
                    if NETWORK_INCLUDES.search(line):
                        errors.append(f"rede: {relative}:{number}: o firmware próprio não usa rede")


def check_shell(files: list[Path], errors: list[str]) -> int:
    checked = 0
    for path in files:
        if path.suffix != ".sh":
            continue
        checked += 1
        result = subprocess.run(["bash", "-n", str(path)], capture_output=True, text=True)
        if result.returncode != 0:
            errors.append(f"shell: {path.relative_to(ROOT)}: {result.stderr.strip()}")
    return checked


def main() -> int:
    files = tracked_files()
    errors: list[str] = []
    check_structure(errors)
    portable = check_portability(files, errors)
    check_privacy(files, errors)
    scripts = check_shell(files, errors)
    print(
        f"{len(SUBSYSTEMS)} subsistemas, {portable} fontes portáveis e {scripts} scripts de shell "
        f"conferidos; {len(errors)} problemas"
    )
    for error in errors:
        print(f"PROBLEMA: {error}", file=sys.stderr)
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
