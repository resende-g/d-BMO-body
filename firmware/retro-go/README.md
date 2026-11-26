<!-- SPDX-License-Identifier: GPL-2.0-only -->

# Retro-Go para o d-BMO v1 (alvo `dbmo-v1`)

Alvo de build do [Retro-Go](https://github.com/ducalex/retro-go) para o d-BMO v1
(ESP32-S3-WROOM-1-N16R8, tela ILI9341 320x240, SD em SPI, MAX98357A, 10 botões no MCP23017).

## Estado

- **A imagem completa compila no perfil de bancada** (`DBMO_RG_BENCH=1`): launcher e os quatro
  emuladores numa `.img` de 6,5 MB, compilada em 24/11/2026 no contêiner `espressif/idf:v4.4.8`
  (ver [Imagem de bancada](#imagem-de-bancada)). A tarefa `retro-go` do fluxo `CI` passa a gerar
  essa imagem e a publicá-la como artefato, e também compila o launcher no perfil padrão
  (MCP23017), como fazia desde 12/11/2026. O que mais foi conferido está em
  [Declaração de verificação](#declaração-de-verificação).
- **Rodou na placa em 25/11/2026**, no perfil de bancada (tela, 3 botões e o SD da tela): o
  launcher abriu, montou o cartão e iniciou um jogo de Game Boy Color, que rodou por mais de
  2 minutos sem travar (frameskip de 1 a 3, com o SPI da tela a 10 MHz). Sem som (o
  MAX98357A não está ligado) e sem Start na bancada. Registro em
  [`docs/testing/TESTING.md`](../../docs/testing/TESTING.md), entrada de 25/11/2026.
- A pinagem é a do perfil `target` de
  [`firmware/config/include/dbmo/config/pins.h`](../config/include/dbmo/config/pins.h), que é uma
  **proposta**. Na bancada só existem hoje a tela e três botões diretos. MCP23017, MAX98357A,
  divisor da bateria e power-latch ainda não estão ligados.
- O Retro-Go fica fixado no commit `4ced120669750ca7228fd0414211430c1d923166` (2026-01-19).

## Arquivos

| Arquivo | O que é |
|---|---|
| `targets/dbmo-v1/config.h` | descrição do hardware para o Retro-Go (pinos, drivers, mapa de botões) |
| `targets/dbmo-v1/env.py` | variáveis lidas pelo `rg_tool.py` (chip `esp32s3`, sem formato `.fw`) |
| `targets/dbmo-v1/sdkconfig` | padrões do ESP-IDF (flash de 16 MB, PSRAM octal, console no USB) |
| `build.sh` | baixa o Retro-Go no commit fixado, instala e registra o alvo e chama o `rg_tool.py` |
| `tests/host_check.sh` | conferência de sintaxe e de registro do alvo no computador, sem ESP-IDF |
| `LICENSE` | texto da GPL-2.0, cópia do `COPYING` do Retro-Go |

## Origem e licença

Os três arquivos do alvo derivam de alvos do próprio Retro-Go:

- `config.h`: estrutura, sequência de init do ILI9341 e DAC I2S de `targets/esp32-s3-devkit`;
  compartilhamento de SPI entre tela e SD como em `targets/fri3d-2024`, que usa o mesmo módulo
  N16R8 (`targets/fri3d-2024/docs/README.md:10`).
- `env.py` e `sdkconfig`: de `targets/esp32-s3-devkit`, com as diferenças listadas no cabeçalho
  do `sdkconfig`.

O Retro-Go é GPL-2.0 (`README.md:193-194` e `COPYING` do upstream). Como ele não declara
"ou posterior", os arquivos desta pasta usam `SPDX-License-Identifier: GPL-2.0-only`. O resto do
repositório é Apache-2.0; por isso o alvo fica nesta pasta separada (README da raiz, seção
Licença). O `LICENSE` daqui é cópia byte a byte do `COPYING` do upstream.

A imagem padrão do Retro-Go inclui o emulador fMSX, que tem licença própria, não comercial
(`README.md:195` do upstream). Para uma imagem só com código GPL, indicar os apps sem `fmsx`.

## Mapa de hardware

Legenda do status:

- **CONFIRMADO**: fiação já validada na bancada segundo o `pins.h` (`BenchValidated`), ou função
  fixa do chip. A validação foi com o `teste_tela` e o Bruce, não com o Retro-Go.
- **PROPOSTO**: está no `pins.h` como proposta; o hardware ainda não foi ligado.
- **PENDENTE DE VALIDAÇÃO FÍSICA**: parâmetro que só se decide com o aparelho montado.

Todos os itens, inclusive os confirmados, ainda precisam ser validados com o Retro-Go.

| Sinal | GPIO | Macro em `config.h` | `pins.h` | Status |
|---|---:|---|---|---|
| SPI MOSI (tela e SD) | 11 | `RG_GPIO_LCD_MOSI`, `RG_GPIO_SDSPI_MOSI` | `target::kSpiMosi` | CONFIRMADO |
| SPI SCK (tela e SD) | 12 | `RG_GPIO_LCD_CLK`, `RG_GPIO_SDSPI_CLK` | `target::kSpiSck` | CONFIRMADO |
| SPI MISO (tela e SD) | 13 | `RG_GPIO_LCD_MISO`, `RG_GPIO_SDSPI_MISO` | `target::kSpiMiso` | CONFIRMADO |
| CS da tela | 10 | `RG_GPIO_LCD_CS` | `target::kDisplayCs` | CONFIRMADO |
| DC da tela | 17 | `RG_GPIO_LCD_DC` | `target::kDisplayDc` | CONFIRMADO |
| Reset da tela | 18 | `RG_GPIO_LCD_RST` | `target::kDisplayReset` | CONFIRMADO |
| Backlight (PWM) | 16 | `RG_GPIO_LCD_BCKL` | `target::kDisplayBacklight` | PROPOSTO |
| CS do SD | 21 | `RG_GPIO_SDSPI_CS` | `target::kSdCs` | PROPOSTO |
| CS do CC1101 (só mantido em alto) | 14 | `DBMO_GPIO_CC1101_CS` | `target::kCc1101Cs` | PROPOSTO |
| CS do cartucho (só mantido em alto) | 38 | `DBMO_GPIO_CARTRIDGE_CS` | `target::kCartridgeCs` | PROPOSTO |
| I2S BCLK | 40 | `RG_GPIO_SND_I2S_BCK` | `target::kI2sBclk` | PROPOSTO |
| I2S LRCLK | 41 | `RG_GPIO_SND_I2S_WS` | `target::kI2sLrclk` | PROPOSTO |
| I2S DATA (DIN do MAX98357A) | 42 | `RG_GPIO_SND_I2S_DATA` | `target::kI2sSpeakerData` | PROPOSTO |
| Shutdown do amplificador | 3 | `RG_GPIO_SND_AMP_ENABLE` | `target::kAmpShutdown` | PROPOSTO |
| I2C SDA | 8 | `RG_GPIO_I2C_SDA` | `target::kI2cSda` | PROPOSTO |
| I2C SCL | 9 | `RG_GPIO_I2C_SCL` | `target::kI2cScl` | PROPOSTO |
| MCP23017 dos botões | I2C 0x20 | `RG_I2C_GPIO_DRIVER 3`, `RG_I2C_GPIO_ADDR` | `i2c::kMcp23017Buttons` | PROPOSTO |
| Bateria (ADC1 canal 0, divisor 100k/100k) | 1 | `RG_BATTERY_*`, atrás de `DBMO_RG_ENABLE_BATTERY_ADC` | `target::kVbatSense` | PROPOSTO, desligado |
| Power-latch | 48 | `DBMO_GPIO_POWER_HOLD`, atrás de `DBMO_RG_ENABLE_POWER_HOLD` | `target::kPowerHold` | PROPOSTO, desligado |
| USB D- / D+ (console e gravação) | 19 / 20 | `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG` no `sdkconfig` | `target::kUsbDm`, `kUsbDp` | CONFIRMADO (função fixa do chip) |
| Orientação da tela (MADCTL `0x28`, com `0xB6` = `0x0A, 0x82`) | - | `DBMO_RG_LCD_MADCTL`, `RG_SCREEN_INIT` | - | VALIDADO EM BANCADA em 25/11/2026 (com `0xB6` = `0x0A, 0xA2` a imagem saiu de cabeça para baixo e espelhada) |
| Clock SPI da tela (40 MHz) | - | `DBMO_RG_SCREEN_SPEED_HZ` | `spi::kDisplayHz` é 10 MHz | PENDENTE DE VALIDAÇÃO FÍSICA |
| Clock do SD (20 MHz) | - | `RG_STORAGE_SDSPI_SPEED` | - | PENDENTE DE VALIDAÇÃO FÍSICA |
| Curva de carga da bateria | - | `RG_BATTERY_CALC_PERCENT` | - | PENDENTE DE VALIDAÇÃO FÍSICA |

Pinos do perfil `target` que o Retro-Go não usa nem configura: botão de energia (2), microfone I2S (7), GDO0 do CC1101 (15), IR RX (39), IR TX (47) e o UART0 do
cartucho (43, 44; ver a nota sobre o console em [sdkconfig](#sdkconfig-psram-octal-flash-e-console)).

Cada linha do `config.h` que fixa um pino termina com o comentário
`pins.h <namespace>::<constante>`, para a conferência automática contra o mapa canônico.

## Botões

O Retro-Go tem 14 teclas (`rg_input.h:6-26`, `RG_KEY_COUNT = 14`). O d-BMO tem 10 botões e, desde
2026-11-25, nenhum analógico nem gatilho de ombro. No commit fixado nenhum emulador lê `RG_KEY_X`,
`RG_KEY_Y`, `RG_KEY_L` ou `RG_KEY_R` (o Mega Drive usa A, B e SELECT, `gwenesis/main/main.c:268`; o
SNES troca os ombros por MENU+B e MENU+A, `main_snes.c:22-23`). Por isso os botões X e Y viram
MENU e OPTION.

| Tecla do Retro-Go | Bit do MCP23017 | Pino | Botão do d-BMO (`pins.h mcp_buttons`) |
|---|---:|---|---|
| `RG_KEY_UP` | 0 | GPA0 | D-pad cima (`kDpadUp`) |
| `RG_KEY_DOWN` | 1 | GPA1 | D-pad baixo (`kDpadDown`) |
| `RG_KEY_LEFT` | 2 | GPA2 | D-pad esquerda (`kDpadLeft`) |
| `RG_KEY_RIGHT` | 3 | GPA3 | D-pad direita (`kDpadRight`) |
| `RG_KEY_A` | 4 | GPA4 | A (`kActionA`) |
| `RG_KEY_B` | 5 | GPA5 | B (`kActionB`) |
| `RG_KEY_MENU` | 6 | GPA6 | X (`kActionX`) |
| sem botão | 7 | GPA7 | somente saída no MCP23017 (datasheet rev. D) |
| `RG_KEY_OPTION` | 8 | GPB0 | Y (`kActionY`) |
| `RG_KEY_SELECT` | 9 | GPB1 | Select (`kSelect`) |
| `RG_KEY_START` | 10 | GPB2 | Start (`kStart`) |
| sem botão | 11 a 14 | GPB3 a GPB6 | livres |
| sem botão | 15 | GPB7 | somente saída no MCP23017 (datasheet rev. D) |

Segurar MENU (o botão X) no boot abre o menu de recuperação (`RG_RECOVERY_BTN`).

Botão ligado ao GND, pull-up interno do MCP23017 (`.pullup = 1`), ativo em nível baixo
(`.level = 0`). Nos emuladores, MENU abre o menu do jogo e OPTION abre as opções (por exemplo
`retro-core/main/main_nes.c:266-272`); volume e brilho ficam nas opções (`rg_gui.c:2015-2024`).
Segurar MENU ao ligar abre o menu de recuperação (`RG_RECOVERY_BTN`; `rg_system.c:465`).

Perfil de bancada (`DBMO_RG_BENCH=1`), com os três botões diretos do `pins.h bench`:

| Tecla | Como |
|---|---|
| `RG_KEY_UP` | botão do GPIO 39 |
| `RG_KEY_A` | botão OK do GPIO 2 |
| `RG_KEY_DOWN` | botão do GPIO 47 |
| `RG_KEY_B` | cima + baixo (igual ao Bruce de bancada) |
| `RG_KEY_RIGHT` | OK + cima |
| `RG_KEY_MENU` | OK + baixo |
| `RG_KEY_OPTION` | os três juntos |

As combinações usam o mapa virtual do Retro-Go, que compara o estado inteiro
(`rg_input.c:202-208`). O primeiro botão apertado ainda é lido sozinho antes de a combinação se
formar. É um recurso para testar a tela, não um mapa definitivo.

## Barramento SPI compartilhado (tela + SD)

O upstream **consegue** compartilhar um host SPI entre a tela e o SD. Não é uma pendência. Como
ele faz, no commit fixado:

1. `rg_system.c:362-371` (`platform_init`): põe em nível alto os CS da tela e do SD antes de
   qualquer acesso ao barramento. Em seguida roda `RG_CUSTOM_PLATFORM_INIT()` (`:384-387`).
2. `rg_system.c:461`: `rg_storage_init()` vem **antes** da tela. É ele que inicializa o
   barramento, com os pinos `RG_GPIO_SDSPI_*` (`rg_storage.c:75-83`), e monta o cartão
   (`rg_storage.c:110-116`). Se o barramento já existir, só avisa (`rg_storage.c:84-85`).
3. `rg_system.c:476`: `rg_display_init()` vem depois. O driver chama `spi_bus_initialize` de novo e
   aceita `ESP_ERR_INVALID_STATE`, que é a resposta do ESP-IDF para barramento já inicializado
   (`drivers/display/ili9341.h:129-130`); depois só acrescenta o dispositivo da tela (`:132-133`).
4. A exclusão mútua entre os dois dispositivos não está no Retro-Go: é a trava de barramento do
   driver SPI do ESP-IDF. O driver do SD adquire o barramento a cada comando
   (ESP-IDF v4.4.8, `components/driver/sdspi_host.c:447`); a tela usa transações enfileiradas
   (`ili9341.h:66`).

Essa ordem é a que o ESP-IDF pede para SD em barramento compartilhado: CS dos outros dispositivos
em alto, cartão montado primeiro, os demais depois (ESP-IDF v5.3.6,
`docs/en/api-reference/peripherals/sdspi_share.rst:56-76`). Dois alvos do upstream em ESP32-S3
usam `SPI2_HOST` para os dois: `fri3d-2024` (`config.h:14,25,94-105`) e `t-deck-plus`
(`config.h:6,37,100-111`).

O que o d-BMO acrescenta: o barramento também tem o CC1101 (CS 14) e o cartucho (CS 38), que o
Retro-Go não conhece. `RG_CUSTOM_PLATFORM_INIT()` mantém esses dois CS em nível alto, do mesmo
jeito que o alvo `t-deck-plus` faz com o rádio dele (`t-deck-plus/config.h:23-33`).
`RG_STORAGE_SDSPI_HOLD_CS` não é definido: ele só serve para barramento não compartilhado
(`rg_storage.c:96-102`).

## Opções de build

Todas ficam desligadas por padrão. O `build.sh` lê as variáveis de ambiente abaixo e gera
`dbmo_build_flags.h` na pasta do alvo dentro do clone do Retro-Go; o `config.h` inclui esse
arquivo se ele existir.

| Variável | Efeito |
|---|---|
| `DBMO_RG_BENCH=1` | perfil de bancada: 3 botões diretos, sem I2C nem MCP23017, tela a 10 MHz, partição FAT interna `vfs` como reserva do SD |
| `DBMO_RG_ENABLE_BATTERY_ADC=1` | leitura da bateria no GPIO 1. Desligada porque o divisor não existe: com o pino solto o Retro-Go mostraria um nível falso |
| `DBMO_RG_ENABLE_POWER_HOLD=1` | aciona o GPIO 48 no começo do boot. Exige `DBMO_RG_POWER_HOLD_LEVEL` (0 ou 1): o circuito do power-latch não está decidido e não há valor padrão |
| `DBMO_RG_LCD_MADCTL=0x..` | registrador de orientação e ordem de cor do ILI9341 (padrão `0x28`) |
| `DBMO_RG_SCREEN_SPEED_HZ=n` | clock SPI da tela em Hz |
| `DBMO_RG_NETWORKING=1` | compila com rede. O padrão passa `--no-networking` ao `rg_tool.py` |

Sobre a rede: no Retro-Go ela liga o WiFi e, por padrão, um servidor de arquivos HTTP no launcher
(`launcher/main/main.c:208-212`). O d-BMO não depende de nuvem nem de rede, então o padrão daqui
é compilar sem ela. Mesmo com `DBMO_RG_NETWORKING=1`, a verificação de atualização no GitHub fica
desligada (`RG_UPDATER_ENABLE 0`; ver `components/retro-go/config.h:87-97` do upstream), e o
`sdkconfig` não traz as opções `ESP_TLS_INSECURE` e `ESP_TLS_SKIP_SERVER_CERT_VERIFY` do arquivo
de origem.

## Como compilar

### Versão do ESP-IDF

O Retro-Go declara suporte às versões 4.4 a 5.3 (`BUILDING.md:10`), e o autor usa a 4.4.8
(`BUILDING.md:12`). A imagem do CI dele é `espressif/idf:release-v4.4` (`Dockerfile:1`), mas esse
CI só compila alvos ESP32 (`Dockerfile:17-18`), nenhum ESP32-S3.

Imagem recomendada para o CI do d-BMO: **`espressif/idf:v4.4.8`**.

- É a versão que o `BUILDING.md` cita como a usada pelo autor, numa tag imutável (a
  `release-v4.4` do `Dockerfile` do upstream é um ramo).
- A tag existe no Docker Hub, para amd64 e arm64 (API do Docker Hub consultada em 2026-11-12;
  resumo do índice `sha256:223a6bb14aaa0ca196a4a8d9fb24c5d71b85335e7617da062b5d12717708be9c`).
- O `rg_storage.c:96-97` registra que, no ESP-IDF 5.0 ou mais novo, o SD fica lento a menos que
  o CS seja mantido ativo, o que só vale para barramento não compartilhado. O d-BMO compartilha
  o barramento, então a linha 4.4 evita esse caso.
- Dos 110 nomes de opção citados no `sdkconfig`, 106 existem nos Kconfig da v4.4.8 (lista em
  [sdkconfig](#sdkconfig-psram-octal-flash-e-console)).

Alternativa, se a 4.4.8 falhar para o ESP32-S3: `espressif/idf:v5.3.6`, a última 5.3, ainda na
faixa declarada. Os nomes de opção também foram conferidos contra ela. O SD tende a ficar lento
pelo motivo acima.

Os patches de `tools/patches` do upstream não são aplicados: o `sdcard-fix` é obrigatório só
para o ODROID-GO e o `panic-hook` é opcional (`BUILDING.md:14-17`).

### No computador

Com o ESP-IDF já ativado no terminal (`. "$IDF_PATH/export.sh"`):

```bash
firmware/retro-go/build.sh                 # só o launcher (padrão: build launcher)
firmware/retro-go/build.sh build-img       # todos os apps e a imagem .img
firmware/retro-go/build.sh clean           # obrigatório depois de mudar o sdkconfig do alvo
firmware/retro-go/build.sh --help          # variáveis aceitas
```

O script falha com mensagem clara se o `idf.py` não estiver no `PATH`. Os argumentos vão para o
`rg_tool.py`; as opções (`--port`, `--fatsize`) têm que vir **antes** do comando, porque o
`rg_tool.py` declara o comando e a lista de apps como posicionais (`rg_tool.py:163-169`).

O Retro-Go é baixado em `${XDG_CACHE_HOME:-~/.cache}/d-bmo/retro-go-4ced12066975`. Para usar
outro clone, `RETRO_GO_DIR=/caminho`. Nos dois casos o script altera a árvore do Retro-Go: copia
a pasta do alvo e acrescenta duas linhas em `components/retro-go/config.h`. Se o clone já foi
usado para outro alvo, rodar `clean` antes: o `sdkconfig` gerado em cada app é reaproveitado.

### Docker e CI

A imagem da Espressif define `IDF_PATH=/opt/esp/idf` e ativa o ESP-IDF no `ENTRYPOINT`
(ESP-IDF v4.4.8, `tools/docker/Dockerfile:56,86` e `tools/docker/entrypoint.sh:4`). Com
`docker run` basta chamar o script. Este é o comando que gerou a imagem de bancada em 24/11/2026,
num Mac com Apple Silicon (a tag tem variante arm64):

```bash
docker run --rm -v "$PWD":/project -w /project -e XDG_CACHE_HOME=/project/build/cache \
  -e DBMO_RG_BENCH=1 \
  -e GIT_CONFIG_COUNT=1 -e GIT_CONFIG_KEY_0=safe.directory -e GIT_CONFIG_VALUE_0='*' \
  espressif/idf:v4.4.8 firmware/retro-go/build.sh --fatsize 0x200000 build-img
```

As três variáveis `GIT_CONFIG_*` liberam o git do contêiner para ler o clone do Retro-Go, que
pertence a outro usuário (sem elas a versão sai `desconhecido-dbmo`). Não use
`git config --global` dentro do contêiner a partir de um worktree do git: o `.git` do worktree
aponta para uma pasta do computador que não está montada, e o git para com `fatal: not a git
repository`.

Num job do GitHub Actions com `container: espressif/idf:v4.4.8`, o `ENTRYPOINT` não roda nos
passos; ativar o ESP-IDF no próprio passo. É o que a tarefa `retro-go` do fluxo `CI` faz:

```bash
git config --global --add safe.directory '*'
. "$IDF_PATH/export.sh"
DBMO_RG_BENCH=1 XDG_CACHE_HOME="$PWD/build/cache" firmware/retro-go/build.sh --fatsize 0x200000 build-img
```

A imagem sai em `build/cache/d-bmo/retro-go-4ced12066975/retro-go_<versão>_dbmo-v1.img`
(`rg_tool.py:224`; a versão padrão é `4ced120-dbmo`). Na CI ela é publicada como o artefato
`dbmo-retro-go-bancada` (guardado por 14 dias), na página da execução do fluxo `CI`. Depois a
mesma tarefa compila o launcher no perfil padrão, para manter conferido o caminho do MCP23017.

### Imagem de bancada

Compilada em 24/11/2026 com o comando de `docker run` acima: ESP-IDF 4.4.8, alvo `dbmo-v1`,
`DBMO_RG_BENCH=1`, sem rede, com a partição `vfs` de 2 MB. São os cinco apps padrão do
`rg_tool.py` (`rg_tool.py:17`):

| Subtipo | App | Sistemas | Binário | Partição na imagem |
|---|---|---|---:|---:|
| `ota_0` | `launcher` | menu, explorador de arquivos | 406.688 B | 1.024 KiB |
| `ota_1` | `retro-core` | NES, Game Boy, Game Boy Color, SNES, Master System, Game Gear, ColecoVision, PC Engine, Lynx, Game & Watch | 945.584 B | 1.024 KiB |
| `ota_2` | `prboom-go` | DOOM | 786.496 B | 832 KiB |
| `ota_3` | `gwenesis` | Mega Drive | 990.432 B | 1.024 KiB |
| `ota_4` | `fmsx` | MSX | 592.080 B | 640 KiB |
| dados FAT | `vfs` | reserva do SD no perfil de bancada | vazia | 2.048 KiB |

Os sistemas de cada app vêm de `launcher/main/applications.c:692-708`.

- **Tamanho:** a imagem tem 6.816.000 bytes (6,5 MB de partições mais 256 bytes de rodapé do
  `mkfw.py`, `tools/mkfw.py:90-102`). A flash do N16R8 tem 16 MB (16.777.216 bytes): a imagem
  ocupa 41 % dela e sobram cerca de 9,5 MB livres depois da última partição.
- **Avisos do empacotamento:** o `prboom-go` passou 64 bytes e o `fmsx` 2.256 bytes do tamanho
  declarado no `rg_tool.py` (`:20-27`). O `mkfw.py` aumenta a partição até o próximo múltiplo
  de 64 KiB e monta a tabela já com o tamanho novo (`tools/mkfw.py:25-36`), por isso as duas
  ficaram com 832 KiB e 640 KiB. É o comportamento previsto do upstream, não um erro.
- **Avisos do compilador:** um só, repetido uma vez por app: `unused variable 'raw_value'` em
  `components/retro-go/rg_input.c:71`, porque o perfil de bancada não tem driver de bateria
  (`RG_BATTERY_DRIVER 0`). Nenhum erro.
- **Não é reprodutível byte a byte:** o rodapé guarda a hora do empacotamento
  (`tools/mkfw.py:99`), então a imagem da CI tem outro SHA-256.
- A imagem inclui o fMSX, que tem licença própria, não comercial (ver
  [Origem e licença](#origem-e-licença)). O artefato da CI é privado ao repositório.

### Conferência sem ESP-IDF

```bash
firmware/retro-go/tests/host_check.sh
```

Prepara a árvore do Retro-Go (clone, cópia e registro do alvo) e passa o `config.h` pelo
compilador do computador com `-fsyntax-only`, em cada combinação de opções, contra o `rg_input.h`
do Retro-Go e substitutos dos símbolos do ESP-IDF. Confere sintaxe e estrutura. Não compila o
firmware.

## Como gravar

A gravação é pela imagem `.img` inteira, a partir do endereço 0, na porta USB nativa (USB
Serial/JTAG, GPIO 19/20). **Isso apaga o firmware anterior** (o Bruce, por exemplo). Nada disto
foi executado: o Retro-Go nunca foi gravado na placa.

**Imagem de bancada da CI.** Baixar o artefato `dbmo-retro-go-bancada` da execução do fluxo
`CI` (vem num `.zip`), descompactar e gravar. No Mac, o `esptool` que o PlatformIO já instalou
serve (v5.3.0, que usa `write-flash` e `--flash-size`):

```bash
~/.platformio/penv/bin/python ~/.platformio/packages/tool-esptoolpy/esptool.py --chip esp32s3 --port /dev/cu.usbmodem<N> write-flash --flash-size detect 0x0 retro-go_4ced120-dbmo_dbmo-v1.img
```

É o comando do upstream (`esptool.py write_flash --flash_size detect 0x0 <imagem>`,
`BUILDING.md:61`), com os nomes novos das opções. Com o ESP-IDF ativado, `esptool.py` e os
nomes antigos também servem.

**Compilando e gravando de uma vez**, com o ESP-IDF ativado:

```bash
DBMO_RG_BENCH=1 firmware/retro-go/build.sh --port /dev/ttyACM0 --fatsize 0x200000 install
```

O `install` faz o mesmo `write_flash` em 0 (`rg_tool.py:144-150,222-231`). Depois da primeira
gravação dá para regravar um app só: `build.sh --port <porta> flash launcher`, e acompanhar o log
com `build.sh --port <porta> monitor launcher` (`BUILDING.md:69-74`).

O nome da porta depende do sistema (`/dev/cu.usbmodem<N>` no macOS, `/dev/ttyACM0` no Linux). Se
a placa não entrar em modo de gravação sozinha, usar o procedimento da própria placa (na
DevKitC, segurar BOOT e apertar RESET). Essas duas informações não foram verificadas.

`--fatsize` cria a partição FAT interna `vfs` (`rg_tool.py:93-94`), que o perfil de bancada usa
quando o SD não monta (`rg_storage.c:168-183`). Sem rede não há como copiar ROMs para ela: pelo
código, ela só dá ao launcher onde guardar a configuração quando não há cartão. O valor tem que ser numérico: o
`mkfw.py` o converte com `int()` (`tools/mkfw.py:19`), apesar de a ajuda do `rg_tool.py` citar
`500K` e `5M`.

### ROMs no cartão SD

O cartão tem que estar em FAT32 (o de teste é o de 2 GB com o volume `DBMO`). O Retro-Go procura
as ROMs em `/roms/<sistema>` (`rg_storage.h:14`; `launcher/main/applications.c:680`):

| Sistema | Pasta no cartão | Extensões aceitas (`applications.c:692-695`) |
|---|---|---|
| NES | `/roms/nes` | `.nes`, `.fc`, `.fds`, `.nsf`, `.zip` |
| Game Boy | `/roms/gb` | `.gb`, `.gbc`, `.zip` |
| Game Boy Color | `/roms/gbc` | `.gbc`, `.gb`, `.zip` |

Os demais sistemas seguem a mesma regra, com o nome curto de cada um (`sms`, `gg`, `md`, `pce`,
`doom`, `msx` e outros; `applications.c:692-708`). O `.7z` não é aceito: descompactar antes, ou
recompactar em `.zip`. Os saves ficam em `/retro-go/saves/<sistema>` e a configuração em
`/retro-go/config` (`rg_storage.h:8-15`). O launcher ignora arquivos que começam com ponto
(`applications.c:33-35`), então os `._*` que o macOS cria no FAT32 não aparecem como jogos.

O cartão de teste funciona na bancada desde 25/11/2026 (em 24/11 faltavam fios do SPI): o
Retro-Go o montou em `/sd` e criou sozinho as pastas `/roms/<sistema>`, vazias. O `.7z` que o
Gabriel tinha foi descompactado no Mac e o `.gbc` copiado para `/roms/gbc`.

## sdkconfig: PSRAM octal, flash e console

O mecanismo foi copiado dos alvos ESP32-S3 do upstream, que já usam PSRAM octal
(`esp32-s3-devkit/sdkconfig:86`, `fri3d-2024/sdkconfig:86`).

| Opção | ESP-IDF v4.4.8 | ESP-IDF v5.3.6 |
|---|---|---|
| `CONFIG_SPIRAM_MODE_OCT=y` | `components/esp32s3/Kconfig:236` | `components/esp_psram/esp32s3/Kconfig.spiram:18` |
| `CONFIG_ESP32S3_SPIRAM_SUPPORT=y` | `components/esp32s3/Kconfig:218` | renomeada para `CONFIG_SPIRAM` (`components/esp_hw_support/sdkconfig.rename.esp32s3:11`) |
| `CONFIG_SPIRAM=y` | `components/esp_hw_support/Kconfig.spiram.common:6` | `components/esp_psram/esp32s3/Kconfig.spiram:1` |
| `CONFIG_SPIRAM_TYPE_AUTO=y` | `components/esp32s3/Kconfig:244` | `components/esp_psram/esp32s3/Kconfig.spiram:26` |
| `CONFIG_SPIRAM_SPEED_80M=y` | `components/esp32s3/Kconfig:311` | `components/esp_psram/esp32s3/Kconfig.spiram:114` |
| `CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y` | `components/esptool_py/Kconfig.projbuild:178` | `components/esptool_py/Kconfig.projbuild:199` |
| `CONFIG_ESPTOOLPY_FLASHMODE_QIO=y` | `components/esptool_py/Kconfig.projbuild:77` | `components/esptool_py/Kconfig.projbuild:47` |
| `CONFIG_ESPTOOLPY_FLASHFREQ_80M=y` | `components/esptool_py/Kconfig.projbuild:138` | `components/esptool_py/Kconfig.projbuild:114` |
| `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y` | `components/esp_system/Kconfig:228` | `components/esp_system/Kconfig:237` |
| `CONFIG_ESP_CONSOLE_SECONDARY_NONE=y` | `components/esp_system/Kconfig:248` | `components/esp_system/Kconfig:256` |

Na documentação da Espressif, a tabela "F4R8 Hardware" (flash quad com PSRAM octal) lista flash a
80 MHz junto com PSRAM a 80 MHz (ESP-IDF v5.3.6,
`docs/en/api-guides/flash_psram_config.rst:119-133`). O N16R8 não aparece ali pelo nome; é a mesma
combinação de tipos de memória. `CONFIG_ESPTOOLPY_OCT_FLASH` fica desligado: a flash do N16R8 é
quad.

**Console.** Nenhum alvo do upstream define `CONFIG_ESP_CONSOLE_*`; todos ficam com o padrão do
ESP-IDF, que é o UART0 (ESP-IDF v4.4.8, `components/esp_system/Kconfig:204-206`). No ESP32-S3 o
UART0 são os GPIO 43 e 44 (`components/soc/esp32s3/include/soc/uart_pins.h:23-24`), reservados ao
cartucho no d-BMO. Por isso este alvo escolhe o USB Serial/JTAG como console principal. O texto
de ajuda do mesmo Kconfig registra que a saída inicial do bootloader da ROM existe mesmo sem
console em UART e só é suprimida por strapping ou eFuse (`components/esp_system/Kconfig:213-215`).
É de se esperar, então, tráfego no GPIO 43 durante o reset; isso não foi medido.

Nomes citados no `sdkconfig` que não existem em uma das versões (todos herdados do arquivo de
origem, que serve às duas linhas do ESP-IDF):

- ausentes na v4.4.8 (4 de 110): `ADC_SUPPRESS_DEPRECATE_WARN`, `I2S_SUPPRESS_DEPRECATE_WARN`,
  `ESP_DEBUG_OCDAWARE` e `ESP_WIFI_NVS_ENABLED`. Para as duas últimas o `sdkconfig` traz também o
  nome da v4.4 (`ESP32S3_DEBUG_OCDAWARE`, `components/esp32s3/Kconfig:385`;
  `ESP32_WIFI_NVS_ENABLED`, `components/esp_wifi/Kconfig:212`).
- ausentes na v5.3.6 (7 de 110): `COMPILER_OPTIMIZATION_DEFAULT`, `ESPTOOLPY_FLASHSIZE_DETECT`,
  `FATFS_API_ENCODING_UTF_16`, `FREERTOS_ASSERT_DISABLE`, `FREERTOS_ASSERT_FAIL_ABORT`,
  `FREERTOS_ASSERT_FAIL_PRINT_CONTINUE` e `SPIRAM_SIZE`. Outros 8 são nomes antigos aceitos por
  renomeação (`sdkconfig.rename*`).

A conferência foi por leitura dos arquivos Kconfig. A ferramenta de configuração do ESP-IDF não
foi executada: não se sabe, por teste, como ela trata os nomes ausentes. Os alvos do upstream
dependem de que ela os ignore.

## Limitações conhecidas

1. **Sem MCP23017 no barramento, o perfil padrão fica sem entrada.** Cada leitura que falha
   registra um erro (`rg_i2c.c:85`), e a tarefa de entrada lê as duas portas a cada 10 ms
   (`rg_input.c:139,261`). Até o MCP23017 chegar, usar `DBMO_RG_BENCH=1`.
2. **O caminho do MCP23017 nunca é usado por um alvo do upstream.** O driver existe
   (`rg_i2c.c:155-163`), mas o único alvo com extensor por esse caminho usa o PCF9539
   (`byteboi-rev1/config.h:13-14`), e nenhum usa `.pullup = 1` num mapa I2C.
3. **Sem teclas X, Y, L e R.** Os botões X e Y fazem MENU e OPTION, então o Retro-Go não recebe
   `RG_KEY_X` nem `RG_KEY_Y`. Nenhum emulador do commit fixado os lê; se uma versão futura do
   Retro-Go passar a ler, o mapa precisa mudar.
4. **Sem analógicos (desde 2026-11-25).** O Retro-Go não tem API de eixo analógico: a leitura é
   uma máscara de teclas (`rg_input.h:96`). Por isso os analógicos saíram do v1.
5. **I2C a 400 kHz.** A velocidade é fixa no código (`rg_i2c.c:30`), diferente dos 100 kHz de
   `pins.h i2c::kBusHz`. O Retro-Go também liga os pull-ups internos do ESP32-S3
   (`rg_i2c.c:28-29`). Mudar exige alterar o `rg_i2c.c`.
6. **Clock da tela.** 40 MHz vem dos alvos de origem. No d-BMO só 10 MHz foi validado, em
   jumpers. O barramento também carrega o CC1101 e o cartucho.
7. **SD lento no ESP-IDF 5.x**, pelo motivo descrito em [Versão do ESP-IDF](#versão-do-esp-idf).
8. **Áudio.** O Retro-Go liga o amplificador pondo o GPIO 3 em nível alto
   (`drivers/audio/i2s.c:15-21,94-98,190-198`). Pelo datasheet do MAX98357A, o nível no pino
   SD_MODE também escolhe o canal: acionado direto pelo GPIO, tende a sair só o canal esquerdo.
   Isso não foi verificado nesta sessão e depende de como o módulo for ligado. O volume é digital
   (`drivers/audio/i2s.c:132-140`).
9. **Bateria.** A faixa de 3,5 V a 4,2 V é a aproximação linear do alvo `esp32-s3-devkit`
   (`config.h:65-69`), não uma curva medida. No ESP32-S3 a calibração do ADC depende de um eFuse
   gravado (ESP-IDF v4.4.8, `components/esp_adc_cal/esp32s3/esp_adc_cal.c:139-140`).
10. **Backlight.** PWM de 5 kHz no GPIO 16 (`drivers/display/ili9341.h:197-216`). A corrente do
    pino LED do módulo da tela não foi medida; pode ser preciso um transistor.
11. **Energia.** O Retro-Go não conhece power-latch nem botão de energia. O desligamento dele
    apenas para o sistema (`rg_system.c:873-884`), e o modo de espera entra em deep sleep
    (`rg_system.c:886-896`) sem que o Retro-Go configure uma fonte de despertar em parte alguma.
12. **Overclock.** A opção de overclock do Retro-Go altera o PLL também no ESP32-S3
    (`rg_system.c:1128-1163`). O efeito sobre o console USB não foi verificado.
13. **Sem LED de status.** O d-BMO não tem; `RG_GPIO_LED` fica indefinido.

## Pendências bloqueantes

1. **Rodar na placa: feito em 25/11/2026** no perfil de bancada (ver [Estado](#estado)). Falta
   o perfil padrão, que depende do MCP23017.
2. **MCP23017 ausente.** Sem ele o perfil padrão não tem botões. Bloqueia o uso, não o build.
3. **SD no barramento compartilhado: resolvido em 25/11/2026.** Em 24/11 o cartão não respondeu
   porque faltavam os fios dos GPIO 10, 11 e 12; com eles, o firmware próprio e o Retro-Go
   montaram o cartão.
4. **Power-latch não decidido.** Com o latch montado e sem `DBMO_RG_ENABLE_POWER_HOLD`, nada
   segura a alimentação depois que o botão é solto. O desligamento por software exigiria alterar
   o Retro-Go (por exemplo com `#ifdef RG_TARGET_DBMO_V1`, mecanismo descrito em
   `PORTING.md:119-131`).
5. **Divisor da bateria ausente e carregador não decidido.** Sem eles não há nível de bateria
   nem aviso de carga baixa.
6. **Orientação e clock da tela** só se confirmam com o Retro-Go rodando na tela.
7. **UART0 do cartucho.** A saída da ROM esperada no GPIO 43 durante o reset (ver a nota sobre
   o console) precisa ser aceita pelo projeto do cartucho, ou suprimida por eFuse ou strapping.

## Declaração de verificação

Nada foi executado no hardware. A compilação com o ESP-IDF está em
[Executado com o ESP-IDF](#executado-com-o-esp-idf); o resto foi conferido por leitura do código,
com arquivo e linha.

**Retro-Go, commit `4ced120669750ca7228fd0414211430c1d923166`:**

- Seleção do alvo: `PORTING.md:24-36,123`; cadeia de `#elif` em `components/retro-go/config.h:1-39`,
  que cai no ODROID-GO com um aviso (`:35-38`); `rg_tool.py:41-43` (alvos pela existência de
  `targets/*/config.h`), `:119` (nome da macro: `RG_TARGET_DBMO_V1`), `:171`, `:187-197` (`env.py`
  e `SDKCONFIG_DEFAULTS`); `base.cmake:6` e `components/retro-go/CMakeLists.txt:49` (a macro vira
  `-D`).
- SPI compartilhado: `rg_system.c:352-388,439,456-462,476,858-871`; `rg_storage.c:66-117`;
  `drivers/display/ili9341.h:92-146`; alvos `fri3d-2024` e `t-deck-plus`.
- Tela: `rg_display.c:39-40`; `drivers/display/ili9341.h:15-21,195-248`;
  `esp32-s3-devkit/config.h:17-44`.
- Entrada: `rg_input.h:6-56,96`; `rg_input.c:23-40,106-213,268-365`; `rg_i2c.c:6-49,129-312`;
  sintaxe do mapa I2C em `byteboi-rev1/config.h:55-63`.
- Áudio: `rg_audio.c:16-31`; `drivers/audio/i2s.c:4-100,190-198`.
- Bateria: `rg_input.c:18-21,69-104,338-355`; `components/retro-go/config.h:107-113`;
  `rg_gui.c:557-565` (o ícone só aparece se houver leitura).
- Rede e atualizador: `rg_tool.py:122,173-175`; `base.cmake:22-26`;
  `launcher/main/main.c:147-161,208-212,435-437`; `launcher/main/updater.c:164-166`.
- Build e gravação: `BUILDING.md:10-17,56,61`; `Dockerfile:1,17-18`; `rg_tool.py:72-96,113-150`;
  `tools/mkfw.py:12-46,62-103`.
- Licença: `README.md:193-196`; `COPYING` (SHA-256
  `8177f97513213526df2cf6184d8ff986c675afb514d4e68a404010521b880643`, igual ao `LICENSE` daqui).

**ESP-IDF v4.4.8 (commit `e499576efdb086551abe309a72899302f82077b7`) e v5.3.6 (commit
`79e3454c68248bd7d881721c9b2e94561378a8ce`), só leitura:**

- Cada nome de opção do `sdkconfig` foi procurado nos arquivos Kconfig e `sdkconfig.rename*` das
  duas versões (tabela e listas acima).
- `spi_bus_initialize` devolve `ESP_ERR_INVALID_STATE` para barramento já inicializado
  (v4.4.8 `components/driver/spi_common.c:756`; v5.3.6
  `components/esp_driver_spi/src/gpspi/spi_common.c:821`).
- Pinos do ESP32-S3 (v4.4.8, `components/soc/esp32s3/include/soc/`): ADC1 canal n é o GPIO n+1
  (`adc_channel.h:18-34`); pinos nativos do SPI2 são CS 10, MOSI 11, CLK 12 e MISO 13
  (`spi_pins.h:30-33`); UART0 em 43 e 44 (`uart_pins.h:23-24`); USB em 19 e 20
  (`io_mux_reg.h:166-167`).
- Funções de GPIO usadas em `RG_CUSTOM_PLATFORM_INIT()` (v4.4.8,
  `components/driver/include/driver/gpio.h:84,140,169,184`).

**Bruce 1.16.1 (para a orientação da tela):** a rotação 1 do TFT_eSPI envia `MV | BGR`, que é
`0x28` (`lib/TFT_eSPI/TFT_Drivers/ILI9341_Rotation.h:17-25`, `ILI9341_Defines.h:57-72`); o
`firmware/bruce/bmo.ini` usa `ROTATION=1`.

**Executado no computador, sem ESP-IDF:**

- `bash -n` em `build.sh` e em `tests/host_check.sh`.
- `build.sh` com `DBMO_RG_PREPARE_ONLY=1`: clone do commit fixado a partir do GitHub, cópia do
  alvo e registro em `components/retro-go/config.h` (duas linhas antes do `#else` final); segunda
  execução sem alterar nenhum arquivo.
- `build.sh` completo contra substitutos do `idf.py` e do `rg_tool.py`, para conferir os
  argumentos repassados e as mensagens de erro (sem `idf.py`, sem `IDF_PATH`, opções inválidas).
  O `rg_tool.py` de verdade não foi executado.
- `tests/host_check.sh` com gcc 13 e clang: `-fsyntax-only` do `config.h` em todas as
  combinações de opções. Os símbolos do ESP-IDF são substitutos; isso não prova que o firmware
  compila.
- Cada pino do `config.h` comparado com o `pins.h` por um script: 40 linhas, nenhuma divergência.

### Executado com o ESP-IDF

- 12/11/2026, CI: launcher no perfil padrão, ESP-IDF 4.4.8 (`docs/testing/TESTING.md`).
- 24/11/2026, num Mac com Apple Silicon, Docker Desktop e `espressif/idf:v4.4.8` (arm64): o
  `docker run` de [Docker e CI](#docker-e-ci), com `DBMO_RG_BENCH=1` e `--fatsize 0x200000`.
  Os cinco apps compilaram e o `mkfw.py` gerou `retro-go_4ced120-dbmo_dbmo-v1.img` com
  6.816.000 bytes; avisos e tamanhos em [Imagem de bancada](#imagem-de-bancada). Com isso a
  ferramenta de configuração do ESP-IDF rodou com o `sdkconfig` do alvo, sem erro.
- A tarefa `retro-go` da CI foi alterada para gerar e publicar essa imagem; ainda não rodou na
  CI com a alteração.

**Não feito:** qualquer teste em hardware; build da imagem completa no perfil padrão (MCP23017),
do qual só o launcher compila na CI.
