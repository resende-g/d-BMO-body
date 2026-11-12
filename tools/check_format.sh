#!/usr/bin/env bash
# Confere (ou aplica, com --fix) a formatação do código próprio com clang-format.
# Fica de fora o que segue o estilo de outro projeto: firmware/bruce, firmware/retro-go,
# os sketches do Arduino IDE (inclusive a tabela de rostos, desenhada à mão em texto) e o
# código de terceiros em tests/third_party.
set -euo pipefail

cd "$(dirname "$0")/.."

FORMAT="${CLANG_FORMAT:-}"
if [ -z "$FORMAT" ]; then
  if command -v clang-format-18 >/dev/null 2>&1; then
    FORMAT=clang-format-18
  else
    FORMAT=clang-format
  fi
fi

mapfile -t FILES < <(
  git ls-files --cached --others --exclude-standard -- \
    'firmware/*.h' 'firmware/*.cpp' 'tests/*.h' 'tests/*.cpp' |
    grep -v -E '^(firmware/(bruce|retro-go|teste_tela|teste_cc1101)/|firmware/rosto/(rosto_faces\.h|teste/)|tests/third_party/)'
)

if [ "${1:-}" = "--fix" ]; then
  "$FORMAT" -i "${FILES[@]}"
  echo "${#FILES[@]} arquivos formatados com $("$FORMAT" --version)"
else
  "$FORMAT" --dry-run --Werror "${FILES[@]}"
  echo "${#FILES[@]} arquivos conferidos com $("$FORMAT" --version)"
fi
