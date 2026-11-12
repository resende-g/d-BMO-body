#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-only
# Licença: GPL-2.0, a mesma do Retro-Go (ver firmware/retro-go/LICENSE).
#
# Compila o Retro-Go para o d-BMO v1 (alvo "dbmo-v1").
#
# O que faz, nesta ordem:
#   1. obtém o Retro-Go no commit fixado (clone em cache, ou a pasta indicada em RETRO_GO_DIR);
#   2. copia firmware/retro-go/targets/dbmo-v1 para components/retro-go/targets/dbmo-v1;
#   3. registra o alvo em components/retro-go/config.h (o Retro-Go escolhe o alvo por uma
#      cadeia de #elif nesse arquivo; sem a entrada, ele compila em silêncio para o ODROID-GO);
#   4. gera dbmo_build_flags.h a partir das variáveis DBMO_RG_*;
#   5. chama o rg_tool.py do Retro-Go com --target=dbmo-v1.
#
# Precisa de um ambiente do ESP-IDF já ativado (idf.py no PATH e IDF_PATH definido). O Retro-Go
# declara suporte às versões 4.4 a 5.3 (BUILDING.md); ver o README desta pasta.
#
# Uso: firmware/retro-go/build.sh [argumentos do rg_tool.py]
# Sem argumentos equivale a "build launcher". Rodar com --help para a lista de variáveis.
#
# Escrito para funcionar também no bash 3.2 (macOS).
set -euo pipefail

RETRO_GO_URL="${RETRO_GO_URL:-https://github.com/ducalex/retro-go.git}"
RETRO_GO_COMMIT=4ced120669750ca7228fd0414211430c1d923166
ALVO=dbmo-v1
# Mesmo cálculo do rg_tool.py: "RG_TARGET_" + nome em maiúsculas, com "_" no lugar do que não
# for letra ou número.
MACRO_ALVO=RG_TARGET_DBMO_V1
ARQUIVOS_DO_ALVO="config.h env.py sdkconfig"

AQUI="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ORIGEM="$AQUI/targets/$ALVO"
CLONE=""
TMP_CLONE=""

info() { printf 'build.sh: %s\n' "$*" >&2; }
aviso() { printf 'build.sh: aviso: %s\n' "$*" >&2; }
erro() {
  printf 'build.sh: erro: %s\n' "$*" >&2
  exit 1
}

limpa_tmp() {
  if [ -n "$TMP_CLONE" ] && [ -d "$TMP_CLONE" ]; then
    rm -rf "$TMP_CLONE"
  fi
}
trap limpa_tmp EXIT

uso() {
  cat <<EOF
Uso: firmware/retro-go/build.sh [argumentos do rg_tool.py]

Sem argumentos: build launcher. Exemplos:
  firmware/retro-go/build.sh                                  compila só o launcher
  firmware/retro-go/build.sh build-img                        compila tudo e gera a imagem .img
  firmware/retro-go/build.sh --port /dev/ttyACM0 install      compila tudo, gera a imagem e grava
  firmware/retro-go/build.sh clean                            apaga os builds (obrigatório depois
                                                              de mudar o sdkconfig do alvo)

Variáveis de ambiente:
  RETRO_GO_DIR                 pasta de um clone do Retro-Go a usar (será alterada: recebe a pasta
                               do alvo e uma entrada em components/retro-go/config.h). Padrão:
                               clone do commit fixado em \${XDG_CACHE_HOME:-~/.cache}/d-bmo/
  DBMO_RG_NETWORKING=1         compila com rede (WiFi e servidor de arquivos). Padrão 0: o build
                               passa --no-networking ao rg_tool.py
  DBMO_RG_PREPARE_ONLY=1       só prepara a árvore (passos 1 a 4) e imprime a pasta dela; não
                               exige o ESP-IDF
  PYTHON                       interpretador para o rg_tool.py (padrão: python3, senão python)

Opções do alvo (todas 0 por padrão; ver targets/dbmo-v1/config.h):
  DBMO_RG_BENCH=1              perfil de bancada: tela + 3 botões diretos, sem MCP23017
  DBMO_RG_ENABLE_BATTERY_ADC=1 leitura da bateria no GPIO 1 (o divisor ainda não existe)
  DBMO_RG_ENABLE_STICK_DPAD=1  analógico esquerdo como D-pad (os analógicos ainda não existem)
  DBMO_RG_ENABLE_POWER_HOLD=1  aciona o power-latch no GPIO 48; exige DBMO_RG_POWER_HOLD_LEVEL
  DBMO_RG_POWER_HOLD_LEVEL     0 ou 1: nível que mantém a alimentação (circuito não decidido)
  DBMO_RG_LCD_MADCTL           registrador 0x36 do ILI9341, de 0x00 a 0xFF (padrão 0x28)
  DBMO_RG_SCREEN_SPEED_HZ      clock SPI da tela em Hz, de 1000000 a 80000000

Retro-Go fixado: $RETRO_GO_COMMIT
EOF
}

case "${1:-}" in
  -h | --help)
    uso
    exit 0
    ;;
esac

# --- Validação das variáveis -------------------------------------------------------------

valida_01() {
  local nome="$1"
  local valor="${!nome:-0}"
  case "$valor" in
    0 | 1) ;;
    *) erro "$nome deve ser 0 ou 1 (recebido: '$valor')" ;;
  esac
}

for nome in DBMO_RG_NETWORKING DBMO_RG_PREPARE_ONLY DBMO_RG_BENCH DBMO_RG_ENABLE_BATTERY_ADC \
  DBMO_RG_ENABLE_STICK_DPAD DBMO_RG_ENABLE_POWER_HOLD; do
  valida_01 "$nome"
done

if [ -n "${DBMO_RG_POWER_HOLD_LEVEL:-}" ]; then
  valida_01 DBMO_RG_POWER_HOLD_LEVEL
fi
if [ "${DBMO_RG_ENABLE_POWER_HOLD:-0}" = 1 ] && [ -z "${DBMO_RG_POWER_HOLD_LEVEL:-}" ]; then
  erro "DBMO_RG_ENABLE_POWER_HOLD=1 exige DBMO_RG_POWER_HOLD_LEVEL (0 ou 1): o circuito do power-latch não está decidido"
fi
if [ "${DBMO_RG_BENCH:-0}" = 1 ] && [ "${DBMO_RG_ENABLE_STICK_DPAD:-0}" = 1 ]; then
  erro "DBMO_RG_BENCH=1 usa o GPIO 2 como botão; não combina com DBMO_RG_ENABLE_STICK_DPAD=1"
fi
if [ -n "${DBMO_RG_LCD_MADCTL:-}" ]; then
  case "$DBMO_RG_LCD_MADCTL" in
    0[xX][0-9a-fA-F] | 0[xX][0-9a-fA-F][0-9a-fA-F]) ;;
    *) erro "DBMO_RG_LCD_MADCTL deve ser hexadecimal de 0x00 a 0xFF (recebido: '$DBMO_RG_LCD_MADCTL')" ;;
  esac
fi
if [ -n "${DBMO_RG_SCREEN_SPEED_HZ:-}" ]; then
  case "$DBMO_RG_SCREEN_SPEED_HZ" in
    *[!0-9]* | 0*) erro "DBMO_RG_SCREEN_SPEED_HZ deve ser um inteiro em Hz (recebido: '$DBMO_RG_SCREEN_SPEED_HZ')" ;;
  esac
  if [ "${#DBMO_RG_SCREEN_SPEED_HZ}" -gt 8 ] || [ "$DBMO_RG_SCREEN_SPEED_HZ" -lt 1000000 ] ||
    [ "$DBMO_RG_SCREEN_SPEED_HZ" -gt 80000000 ]; then
    erro "DBMO_RG_SCREEN_SPEED_HZ fora da faixa de 1000000 a 80000000 (recebido: '$DBMO_RG_SCREEN_SPEED_HZ')"
  fi
fi

for arquivo in $ARQUIVOS_DO_ALVO; do
  [ -f "$ORIGEM/$arquivo" ] || erro "arquivo do alvo ausente: $ORIGEM/$arquivo"
done

# --- Ambiente do ESP-IDF -----------------------------------------------------------------

PYTHON="${PYTHON:-}"
if [ "${DBMO_RG_PREPARE_ONLY:-0}" != 1 ]; then
  if ! command -v idf.py >/dev/null 2>&1; then
    erro "idf.py não encontrado no PATH. Ative o ESP-IDF antes (por exemplo: . \"\$IDF_PATH/export.sh\"), ou rode dentro da imagem espressif/idf. Versões aceitas pelo Retro-Go: 4.4 a 5.3."
  fi
  if [ -z "${IDF_PATH:-}" ]; then
    erro "IDF_PATH não está definido: o rg_tool.py do Retro-Go exige essa variável. Ative o ESP-IDF antes."
  fi
  if [ -z "$PYTHON" ]; then
    if command -v python3 >/dev/null 2>&1; then
      PYTHON=python3
    elif command -v python >/dev/null 2>&1; then
      PYTHON=python
    else
      erro "python não encontrado no PATH"
    fi
  fi
  # Só um aviso: a faixa vem do BUILDING.md do Retro-Go, e a saída do idf.py pode mudar.
  versao_idf="$(idf.py --version 2>/dev/null || true)"
  if [[ "$versao_idf" =~ v([0-9]+)\.([0-9]+) ]]; then
    maior="${BASH_REMATCH[1]}"
    menor="${BASH_REMATCH[2]}"
    if [ "$maior" -lt 4 ] || { [ "$maior" -eq 4 ] && [ "$menor" -lt 4 ]; } ||
      { [ "$maior" -eq 5 ] && [ "$menor" -gt 3 ]; } || [ "$maior" -gt 5 ]; then
      aviso "ESP-IDF $maior.$menor está fora da faixa que o Retro-Go declara suportar (4.4 a 5.3)"
    fi
  fi
fi

# --- 1. Retro-Go no commit fixado --------------------------------------------------------

if [ -n "${RETRO_GO_DIR:-}" ]; then
  [ -d "$RETRO_GO_DIR" ] || erro "RETRO_GO_DIR não é uma pasta: $RETRO_GO_DIR"
  CLONE="$(cd "$RETRO_GO_DIR" && pwd)"
else
  command -v git >/dev/null 2>&1 || erro "git não encontrado no PATH (necessário para baixar o Retro-Go; ou indique um clone em RETRO_GO_DIR)"
  CACHE="${XDG_CACHE_HOME:-$HOME/.cache}/d-bmo"
  CLONE="$CACHE/retro-go-${RETRO_GO_COMMIT:0:12}"
  if [ ! -d "$CLONE" ]; then
    info "baixando o Retro-Go $RETRO_GO_COMMIT em $CLONE"
    mkdir -p "$CACHE"
    TMP_CLONE="$(mktemp -d "$CACHE/retro-go.tmp.XXXXXX")"
    git -C "$TMP_CLONE" init -q
    git -C "$TMP_CLONE" remote add origin "$RETRO_GO_URL"
    git -C "$TMP_CLONE" fetch -q --depth 1 origin "$RETRO_GO_COMMIT"
    git -C "$TMP_CLONE" -c advice.detachedHead=false checkout -q --detach FETCH_HEAD
    mv "$TMP_CLONE" "$CLONE"
    TMP_CLONE=""
  fi
fi

CONFIG_DO_RETRO_GO="$CLONE/components/retro-go/config.h"
if [ ! -f "$CLONE/rg_tool.py" ] || [ ! -f "$CONFIG_DO_RETRO_GO" ]; then
  erro "$CLONE não parece um clone do Retro-Go (faltam rg_tool.py ou components/retro-go/config.h)"
fi

commit_atual=""
if [ -e "$CLONE/.git" ]; then
  commit_atual="$(git -C "$CLONE" rev-parse HEAD 2>/dev/null || true)"
fi
if [ -z "$commit_atual" ]; then
  aviso "não foi possível conferir o commit do Retro-Go em $CLONE"
elif [ "$commit_atual" != "$RETRO_GO_COMMIT" ]; then
  if [ -n "${RETRO_GO_DIR:-}" ]; then
    aviso "RETRO_GO_DIR está em $commit_atual; o alvo foi escrito para $RETRO_GO_COMMIT"
  else
    erro "o cache $CLONE está em $commit_atual, não em $RETRO_GO_COMMIT; apague a pasta e rode de novo"
  fi
fi

# --- 2. Pasta do alvo ---------------------------------------------------------------------

DESTINO="$CLONE/components/retro-go/targets/$ALVO"
mkdir -p "$DESTINO"

sdkconfig_mudou=0
if [ -f "$DESTINO/sdkconfig" ] && ! cmp -s "$ORIGEM/sdkconfig" "$DESTINO/sdkconfig"; then
  sdkconfig_mudou=1
fi
# Só copia o que mudou, para não invalidar o build incremental.
for arquivo in $ARQUIVOS_DO_ALVO; do
  if [ ! -f "$DESTINO/$arquivo" ] || ! cmp -s "$ORIGEM/$arquivo" "$DESTINO/$arquivo"; then
    cp "$ORIGEM/$arquivo" "$DESTINO/$arquivo"
  fi
done
if [ "$sdkconfig_mudou" = 1 ]; then
  aviso "o sdkconfig do alvo mudou: rode 'firmware/retro-go/build.sh clean' antes de compilar, senão o ESP-IDF continua usando a configuração antiga"
fi

# --- 3. Registro do alvo em components/retro-go/config.h ---------------------------------

LINHA_INCLUDE="#include \"targets/$ALVO/config.h\""
if grep -q "defined($MACRO_ALVO)" "$CONFIG_DO_RETRO_GO"; then
  grep -qF "$LINHA_INCLUDE" "$CONFIG_DO_RETRO_GO" ||
    erro "$CONFIG_DO_RETRO_GO cita $MACRO_ALVO mas não inclui targets/$ALVO/config.h; corrija à mão"
else
  # A cadeia de alvos termina num "#else" seguido do aviso "No target defined". A entrada do
  # d-BMO entra logo antes desse "#else". Se o arquivo não tiver essa forma, parar.
  # (Sem "grep | grep -q": com pipefail, o primeiro grep pode morrer de SIGPIPE.)
  apos_else="$(grep -m1 -A1 '^#else[[:space:]]*$' "$CONFIG_DO_RETRO_GO" || true)"
  case "$apos_else" in
    *"No target defined"*) ;;
    *) erro "formato inesperado em $CONFIG_DO_RETRO_GO: não achei o '#else' final da cadeia de alvos" ;;
  esac
  awk -v macro="$MACRO_ALVO" -v inc="$LINHA_INCLUDE" '
    !feito && /^#else[[:space:]]*$/ {
      print "#elif defined(" macro ")"
      print inc
      feito = 1
    }
    { print }
  ' "$CONFIG_DO_RETRO_GO" >"$CONFIG_DO_RETRO_GO.dbmo-tmp"
  cat "$CONFIG_DO_RETRO_GO.dbmo-tmp" >"$CONFIG_DO_RETRO_GO"
  rm -f "$CONFIG_DO_RETRO_GO.dbmo-tmp"
  info "alvo $ALVO registrado em $CONFIG_DO_RETRO_GO"
fi
registro="$(grep -A1 "defined($MACRO_ALVO)" "$CONFIG_DO_RETRO_GO" || true)"
case "$registro" in
  *"$LINHA_INCLUDE"*) ;;
  *) erro "o registro do alvo em $CONFIG_DO_RETRO_GO não ficou como esperado" ;;
esac

# --- 4. Opções de build --------------------------------------------------------------------

gera_flags() {
  printf '%s\n' \
    "// SPDX-License-Identifier: GPL-2.0-only" \
    "// GERADO por firmware/retro-go/build.sh a partir das variáveis de ambiente DBMO_RG_*." \
    "// Não editar: é reescrito a cada execução. Os padrões ficam em config.h." \
    "#define DBMO_RG_BENCH ${DBMO_RG_BENCH:-0}" \
    "#define DBMO_RG_ENABLE_BATTERY_ADC ${DBMO_RG_ENABLE_BATTERY_ADC:-0}" \
    "#define DBMO_RG_ENABLE_STICK_DPAD ${DBMO_RG_ENABLE_STICK_DPAD:-0}" \
    "#define DBMO_RG_ENABLE_POWER_HOLD ${DBMO_RG_ENABLE_POWER_HOLD:-0}"
  if [ -n "${DBMO_RG_POWER_HOLD_LEVEL:-}" ]; then
    printf '%s\n' "#define DBMO_RG_POWER_HOLD_LEVEL $DBMO_RG_POWER_HOLD_LEVEL"
  fi
  if [ -n "${DBMO_RG_LCD_MADCTL:-}" ]; then
    printf '%s\n' "#define DBMO_RG_LCD_MADCTL $DBMO_RG_LCD_MADCTL"
  fi
  if [ -n "${DBMO_RG_SCREEN_SPEED_HZ:-}" ]; then
    printf '%s\n' "#define DBMO_RG_SCREEN_SPEED_HZ $DBMO_RG_SCREEN_SPEED_HZ"
  fi
}

FLAGS="$DESTINO/dbmo_build_flags.h"
gera_flags >"$FLAGS.tmp"
if [ -f "$FLAGS" ] && cmp -s "$FLAGS.tmp" "$FLAGS"; then
  rm -f "$FLAGS.tmp"
else
  mv "$FLAGS.tmp" "$FLAGS"
fi

info "Retro-Go em $CLONE (alvo $ALVO, bancada=${DBMO_RG_BENCH:-0}, rede=${DBMO_RG_NETWORKING:-0})"

if [ "${DBMO_RG_PREPARE_ONLY:-0}" = 1 ]; then
  info "DBMO_RG_PREPARE_ONLY=1: árvore preparada, rg_tool.py não foi chamado"
  # Única linha na saída padrão: a pasta do Retro-Go preparado (usada por tests/host_check.sh).
  printf '%s\n' "$CLONE"
  exit 0
fi

# --- 5. rg_tool.py --------------------------------------------------------------------------

# O rg_tool.py usa caminhos relativos à raiz do Retro-Go.
cd "$CLONE"

# Sem isto o rg_tool.py usa "git describe --dirty", que marca a árvore como suja por causa do
# registro do alvo.
if [ -z "${PROJECT_VER:-}" ]; then
  commit_curto="${commit_atual:0:7}"
  export PROJECT_VER="${commit_curto:-desconhecido}-dbmo"
fi

COMANDO=("$PYTHON" rg_tool.py "--target=$ALVO")
if [ "${DBMO_RG_NETWORKING:-0}" != 1 ]; then
  COMANDO+=(--no-networking)
fi
if [ "$#" -eq 0 ]; then
  COMANDO+=(build launcher)
else
  COMANDO+=("$@")
fi

info "executando: ${COMANDO[*]}"
exec "${COMANDO[@]}"
