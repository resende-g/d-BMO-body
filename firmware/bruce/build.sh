#!/usr/bin/env bash
# Licença: AGPL-3.0, a mesma do Bruce (ver firmware/bruce/LICENSE).
# Compila (e, com "upload", grava) o Bruce para o BMO v1.
# Uso: firmware/bruce/build.sh [pasta-do-clone-do-bruce] [upload]
# Baixa o Bruce na versão fixada se a pasta ainda não existir, confere que o clone está no
# commit esperado e copia a placa do BMO para boards/bmo dele.
#
# PlatformIO: a plataforma do Bruce exige Python 3.10 a 3.14, e o platformio do Homebrew
# vem com Python mais novo. Instalar num venv próprio:
#   python3.14 -m venv ~/.platformio/penv && ~/.platformio/penv/bin/pip install platformio
set -euo pipefail

BRUCE_VERSAO=1.16.1
# Commit da tag 1.16.1. Tag pode ser movida; o commit, não.
BRUCE_COMMIT=ba519c936c87b89c9c667c15b932d7faca5360d5
AQUI="$(cd "$(dirname "$0")" && pwd)"
CLONE="${1:-$HOME/.cache/bruce-$BRUCE_VERSAO}"
PIO="${PIO:-$HOME/.platformio/penv/bin/pio}"
# Os scripts de build do Bruce chamam "pio" pelo nome (patch da libnet80211).
PIO_DIR="$(dirname "$PIO")"
export PATH="$PIO_DIR:$PATH"

if [ ! -d "$CLONE" ]; then
  git clone --depth 1 --branch "$BRUCE_VERSAO" https://github.com/pr3y/Bruce.git "$CLONE"
fi

# Pega clone interrompido, pasta de outra versão e tag movida.
ATUAL="$(git -C "$CLONE" rev-parse HEAD 2>/dev/null || true)"
if [ "$ATUAL" != "$BRUCE_COMMIT" ]; then
  echo "erro: $CLONE está em '${ATUAL:-sem git}', esperado $BRUCE_COMMIT (Bruce $BRUCE_VERSAO)." >&2
  echo "Apague a pasta para baixar de novo, ou passe outra como primeiro argumento." >&2
  exit 1
fi
if [ ! -x "$PIO" ]; then
  echo "erro: PlatformIO não encontrado em $PIO (ver o cabeçalho deste script ou definir PIO=)." >&2
  exit 1
fi

mkdir -p "$CLONE/boards/bmo"
cp "$AQUI/bmo.ini" "$AQUI/interface.cpp" "$CLONE/boards/bmo/"

cd "$CLONE"
if [ "${2:-}" = "upload" ]; then
  "$PIO" run -e bmo-v1 -t upload
else
  "$PIO" run -e bmo-v1
fi
