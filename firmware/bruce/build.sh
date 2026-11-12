#!/usr/bin/env bash
# Licença: AGPL-3.0, a mesma do Bruce (ver firmware/bruce/LICENSE).
# Compila (e, com "upload", grava) o Bruce para o d-BMO v1.
# Uso: firmware/bruce/build.sh [pasta-do-clone-do-bruce] [upload]
# Baixa o Bruce na versão fixada se a pasta ainda não existir e copia a placa do d-BMO para
# boards/bmo dele. Uma pasta que já existe precisa estar exatamente nessa versão.
#
# Variáveis opcionais:
#   BRUCE_ENV  ambiente do bmo.ini a compilar (padrão: bmo-v1; ver README.md)
#   PIO        executável do PlatformIO; sem ela, usa ~/.platformio/penv/bin/pio se existir
#              e, senão, o "pio" do PATH
#
# PlatformIO: a plataforma do Bruce exige Python 3.10 a 3.14, e o platformio do Homebrew
# vem com Python mais novo. Instalar num venv próprio:
#   python3.14 -m venv ~/.platformio/penv && ~/.platformio/penv/bin/pip install platformio
set -euo pipefail

BRUCE_VERSAO=1.16.1
AQUI="$(cd "$(dirname "$0")" && pwd)"
CLONE="${1:-$HOME/.cache/bruce-$BRUCE_VERSAO}"
AMBIENTE="${BRUCE_ENV:-bmo-v1}"
# Mapa de pinos canônico do projeto; o interface.cpp usa dele o mapa de botões do MCP23017.
PINOS="$AQUI/../config/include/dbmo/config/pins.h"

erro() {
  echo "build.sh: $*" >&2
  exit 1
}

if [ -z "${PIO:-}" ]; then
  if [ -x "$HOME/.platformio/penv/bin/pio" ]; then
    PIO="$HOME/.platformio/penv/bin/pio"
  else
    PIO=pio
  fi
fi
PIO_CAMINHO="$(command -v "$PIO")" ||
  erro "PlatformIO não encontrado ('$PIO'). Instale-o ou aponte a variável PIO para o executável."
# Os scripts de build do Bruce chamam "pio" pelo nome (patch da libnet80211).
export PATH="$(dirname "$PIO_CAMINHO"):$PATH"

grep -q "^\[env:$AMBIENTE\]" "$AQUI/bmo.ini" ||
  erro "ambiente '$AMBIENTE' não existe em $AQUI/bmo.ini"
[ -f "$PINOS" ] || erro "mapa de pinos não encontrado: $PINOS"

if [ -d "$CLONE" ]; then
  ESPERADO="$(git -C "$CLONE" rev-parse -q --verify "refs/tags/$BRUCE_VERSAO^{commit}" 2>/dev/null || true)"
  ATUAL="$(git -C "$CLONE" rev-parse -q --verify HEAD 2>/dev/null || true)"
  if [ -z "$ESPERADO" ] || [ "$ESPERADO" != "$ATUAL" ]; then
    erro "$CLONE não está na tag $BRUCE_VERSAO do Bruce (HEAD: ${ATUAL:-não é um repositório git}). Apague a pasta, ou faça nela: git fetch --depth 1 origin tag $BRUCE_VERSAO && git checkout $BRUCE_VERSAO"
  fi
else
  git clone --depth 1 --branch "$BRUCE_VERSAO" https://github.com/pr3y/Bruce.git "$CLONE"
fi

mkdir -p "$CLONE/boards/bmo/dbmo/config"
cp "$AQUI/bmo.ini" "$AQUI/interface.cpp" "$AQUI/disabled_features.cpp" "$CLONE/boards/bmo/"
cp "$PINOS" "$CLONE/boards/bmo/dbmo/config/"

cd "$CLONE"
if [ "${2:-}" = "upload" ]; then
  "$PIO_CAMINHO" run -e "$AMBIENTE" -t upload
else
  "$PIO_CAMINHO" run -e "$AMBIENTE"
fi
