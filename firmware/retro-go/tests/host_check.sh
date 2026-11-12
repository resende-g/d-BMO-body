#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-only
# Licença: GPL-2.0, a mesma do Retro-Go (ver firmware/retro-go/LICENSE).
#
# Conferência do alvo dbmo-v1 que roda no computador, SEM o ESP-IDF:
#   1. prepara a árvore do Retro-Go com o build.sh (DBMO_RG_PREPARE_ONLY=1), o que exercita o
#      clone no commit fixado, a cópia do alvo e o registro em components/retro-go/config.h;
#   2. passa o config.h do alvo pelo compilador do computador com -fsyntax-only, contra o
#      rg_input.h do Retro-Go e substitutos dos símbolos do ESP-IDF (idf_stubs.h), em cada
#      combinação de opções.
# Isto confere sintaxe e estrutura. NÃO compila o firmware e não substitui o build com o ESP-IDF.
#
# Uso: firmware/retro-go/tests/host_check.sh        (CC escolhe o compilador; padrão: cc)
# Precisa de git e de rede na primeira vez (ou de RETRO_GO_DIR apontando para um clone).
set -euo pipefail

AQUI="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RAIZ="$(cd "$AQUI/.." && pwd)"
CC="${CC:-cc}"
command -v "$CC" >/dev/null 2>&1 || {
  echo "host_check.sh: erro: compilador '$CC' não encontrado" >&2
  exit 1
}

# As opções do ambiente de quem chama não entram no teste.
unset DBMO_RG_BENCH DBMO_RG_ENABLE_BATTERY_ADC DBMO_RG_ENABLE_STICK_DPAD DBMO_RG_ENABLE_POWER_HOLD \
  DBMO_RG_POWER_HOLD_LEVEL DBMO_RG_LCD_MADCTL DBMO_RG_SCREEN_SPEED_HZ

prepara() { # [VAR=valor ...] -> imprime a pasta do Retro-Go preparado
  env DBMO_RG_PREPARE_ONLY=1 "$@" "$RAIZ/build.sh" 2>/dev/null
}

CLONE="$(prepara)" || {
  echo "host_check.sh: erro: o build.sh não conseguiu preparar a árvore do Retro-Go" >&2
  exit 1
}
RG="$CLONE/components/retro-go"
ALVO="$RAIZ/targets/dbmo-v1/config.h"
LOG="$(mktemp)"
trap 'rm -f "$LOG"' EXIT

falhas=0
compila() { # argumentos extras do compilador
  "$CC" -std=gnu99 -Wall -Wextra -Wundef -Werror -fsyntax-only -I "$AQUI" -iquote "$RG" "$@" \
    "$AQUI/check_config.c" >"$LOG" 2>&1
}
deve_passar() { # descrição, argumentos
  local descricao="$1"
  shift
  if compila "$@"; then
    echo "ok     $descricao"
  else
    echo "FALHA  $descricao"
    sed 's/^/         /' "$LOG" | head -n 12
    falhas=$((falhas + 1))
  fi
}
deve_falhar() { # descrição, texto esperado no erro, argumentos
  local descricao="$1" esperado="$2"
  shift 2
  if compila "$@"; then
    echo "FALHA  $descricao (deveria ser rejeitado)"
    falhas=$((falhas + 1))
  elif grep -q "$esperado" "$LOG"; then
    echo "ok     $descricao (rejeitado, como esperado)"
  else
    echo "FALHA  $descricao (rejeitado por outro motivo)"
    sed 's/^/         /' "$LOG" | head -n 12
    falhas=$((falhas + 1))
  fi
}

DIRETO="-DDBMO_CONFIG_H=\"$ALVO\""

echo "Retro-Go preparado em: $CLONE"
echo "-- cadeia de alvos do Retro-Go (registro feito pelo build.sh):"
deve_passar "cadeia, opções padrão" -DCHECK_CHAIN -DRG_TARGET_DBMO_V1=1
deve_falhar "cadeia sem a macro do alvo" "No target defined" -DCHECK_CHAIN
prepara DBMO_RG_BENCH=1 >/dev/null
deve_passar "cadeia, DBMO_RG_BENCH=1 lido de dbmo_build_flags.h" -DCHECK_CHAIN -DRG_TARGET_DBMO_V1=1 -DEXPECT_BENCH
prepara >/dev/null # volta às opções padrão

echo "-- config.h do alvo, uma combinação de opções por vez:"
deve_passar "padrão (perfil target)" "$DIRETO"
deve_passar "DBMO_RG_BENCH=1" "$DIRETO" -DDBMO_RG_BENCH=1
deve_passar "DBMO_RG_ENABLE_BATTERY_ADC=1" "$DIRETO" -DDBMO_RG_ENABLE_BATTERY_ADC=1
deve_passar "DBMO_RG_ENABLE_STICK_DPAD=1" "$DIRETO" -DDBMO_RG_ENABLE_STICK_DPAD=1
deve_passar "DBMO_RG_ENABLE_POWER_HOLD=1, nível 1" "$DIRETO" -DDBMO_RG_ENABLE_POWER_HOLD=1 -DDBMO_RG_POWER_HOLD_LEVEL=1
deve_passar "bateria + analógico + power-hold nível 0" "$DIRETO" -DDBMO_RG_ENABLE_BATTERY_ADC=1 \
  -DDBMO_RG_ENABLE_STICK_DPAD=1 -DDBMO_RG_ENABLE_POWER_HOLD=1 -DDBMO_RG_POWER_HOLD_LEVEL=0
deve_passar "bancada + bateria" "$DIRETO" -DDBMO_RG_BENCH=1 -DDBMO_RG_ENABLE_BATTERY_ADC=1
deve_passar "MADCTL e clock SPI informados" "$DIRETO" -DDBMO_RG_LCD_MADCTL=0x68 -DDBMO_RG_SCREEN_SPEED_HZ=20000000
deve_falhar "power-hold sem nível" "DBMO_RG_POWER_HOLD_LEVEL" "$DIRETO" -DDBMO_RG_ENABLE_POWER_HOLD=1
deve_falhar "bancada + analógico" "DBMO_RG_ENABLE_STICK_DPAD" "$DIRETO" -DDBMO_RG_BENCH=1 -DDBMO_RG_ENABLE_STICK_DPAD=1

if [ "$falhas" -ne 0 ]; then
  echo "host_check.sh: $falhas conferência(s) falharam" >&2
  exit 1
fi
echo "host_check.sh: todas as conferências passaram (sintaxe no computador; o firmware não foi compilado)"
