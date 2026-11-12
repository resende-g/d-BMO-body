# SPDX-License-Identifier: GPL-2.0-only
# Licença: GPL-2.0, a mesma do Retro-Go (ver firmware/retro-go/LICENSE).
# Derivado de components/retro-go/targets/esp32-s3-devkit/env.py do Retro-Go
# (ducalex/retro-go, commit 4ced120669750ca7228fd0414211430c1d923166).
#
# O rg_tool.py executa este arquivo depois de ler os argumentos; as variáveis abaixo
# substituem as dele.

# Chip da Espressif no d-BMO v1 (ESP32-S3-WROOM-1-N16R8)
IDF_TARGET = "esp32s3"
# Formato .fw (ODROID-GO/esplay): o d-BMO não usa; a gravação é pela imagem .img
FW_FORMAT = "none"
# Apps compilados quando nenhum é indicado (comentado = todos)
# DEFAULT_APPS = "launcher retro-core"
