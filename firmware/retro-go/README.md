<!-- SPDX-License-Identifier: GPL-2.0-only -->

# Retro-Go para o d-BMO v1 (alvo `dbmo-v1`)

Alvo de build do [Retro-Go](https://github.com/ducalex/retro-go) para o d-BMO v1
(ESP32-S3-WROOM-1-N16R8, tela ILI9341 320x240, SD em SPI, MAX98357A, 14 botões no MCP23017).

## Estado

- **O launcher compila na CI** desde 12/11/2026 (tarefa `retro-go` do fluxo `CI`: ESP-IDF
  4.4.8 no contêiner `espressif/idf:v4.4.8`, alvo `dbmo-v1`, `launcher.bin` gerado). Os
  emuladores e a imagem completa (`build-img`) **ainda não foram compilados**. O que mais foi
  conferido está em [Declaração de verificação](#declaração-de-verificação).
- **Nunca rodou no hardware.** Nada aqui afirma que o Retro-Go funciona no d-BMO.
- A pinagem é a do perfil `target` de
  [`firmware/config/include/dbmo/config/pins.h`](../config/include/dbmo/config/pins.h), que é uma
  **proposta**. Na bancada só existem hoje a tela e três botões diretos. MCP23017, MAX98357A,
  analógicos, divisor da bateria e power-latch ainda não estão ligados.
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
| Analógico esquerdo X (ADC1 canal 1) | 2 | `RG_GAMEPAD_ADC_MAP`, atrás de `DBMO_RG_ENABLE_STICK_DPAD` | `target::kStickLeftX` | PROPOSTO, desligado |
| Analógico esquerdo Y (ADC1 canal 3) | 4 | idem | `target::kStickLeftY` | PROPOSTO, desligado |
| Power-latch | 48 | `DBMO_GPIO_POWER_HOLD`, atrás de `DBMO_RG_ENABLE_POWER_HOLD` | `target::kPowerHold` | PROPOSTO, desligado |
| USB D- / D+ (console e gravação) | 19 / 20 | `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG` no `sdkconfig` | `target::kUsbDm`, `kUsbDp` | CONFIRMADO (função fixa do chip) |
| Orientação da tela (MADCTL `0x28`) | - | `DBMO_RG_LCD_MADCTL` | - | PENDENTE DE VALIDAÇÃO FÍSICA |
| Clock SPI da tela (40 MHz) | - | `DBMO_RG_SCREEN_SPEED_HZ` | `spi::kDisplayHz` é 10 MHz | PENDENTE DE VALIDAÇÃO FÍSICA |
| Clock do SD (20 MHz) | - | `RG_STORAGE_SDSPI_SPEED` | - | PENDENTE DE VALIDAÇÃO FÍSICA |
| Curva de carga da bateria | - | `RG_BATTERY_CALC_PERCENT` | - | PENDENTE DE VALIDAÇÃO FÍSICA |
| Sentido dos eixos do analógico | - | `RG_GAMEPAD_ADC_MAP` | - | PENDENTE DE VALIDAÇÃO FÍSICA |

Pinos do perfil `target` que o Retro-Go não usa nem configura: botão de energia (0), analógico
direito (5, 6), microfone I2S (7), GDO0 do CC1101 (15), IR RX (39), IR TX (47) e o UART0 do
cartucho (43, 44; ver a nota sobre o console em [sdkconfig](#sdkconfig-psram-octal-flash-e-console)).

Cada linha do `config.h` que fixa um pino termina com o comentário
`pins.h <namespace>::<constante>`, para a conferência automática contra o mapa canônico.

## Botões

O Retro-Go tem 14 teclas (`rg_input.h:6-26`, `RG_KEY_COUNT = 14`). O d-BMO tem 16 botões. L2 e R2
viram MENU e OPTION; os cliques dos analógicos ficam sem função.

| Tecla do Retro-Go | Bit do MCP23017 | Pino | Botão do d-BMO (`pins.h mcp_buttons`) |
|---|---:|---|---|
| `RG_KEY_UP` | 0 | GPA0 | D-pad cima (`kDpadUp`) |
| `RG_KEY_DOWN` | 1 | GPA1 | D-pad baixo (`kDpadDown`) |
| `RG_KEY_LEFT` | 2 | GPA2 | D-pad esquerda (`kDpadLeft`) |
| `RG_KEY_RIGHT` | 3 | GPA3 | D-pad direita (`kDpadRight`) |
| `RG_KEY_A` | 4 | GPA4 | A (`kActionA`) |
| `RG_KEY_B` | 5 | GPA5 | B (`kActionB`) |
| `RG_KEY_X` | 6 | GPA6 | X (`kActionX`) |
| sem botão | 7 | GPA7 | somente saída no MCP23017 (datasheet rev. D) |
| `RG_KEY_Y` | 8 | GPB0 | Y (`kActionY`) |
| `RG_KEY_SELECT` | 9 | GPB1 | Select (`kSelect`) |
| `RG_KEY_START` | 10 | GPB2 | Start (`kStart`) |
| `RG_KEY_L` | 11 | GPB3 | L1 (`kL1`) |
| `RG_KEY_R` | 12 | GPB4 | R1 (`kR1`) |
| `RG_KEY_MENU` | 13 | GPB5 | L2 (`kL2`) |
| `RG_KEY_OPTION` | 14 | GPB6 | R2 (`kR2`) |
| sem botão | 15 | GPB7 | somente saída no MCP23017 (datasheet rev. D) |

Os cliques dos analógicos não cabem nas 14 entradas de um MCP23017; a proposta, pendente de
decisão, é lê-los no segundo MCP23017 (`pins.h mcp_aux`). O Retro-Go não tem tecla para eles.

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
| `DBMO_RG_ENABLE_STICK_DPAD=1` | analógico esquerdo como D-pad. Desligada porque os analógicos não estão ligados: pino solto gera toque fantasma |
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
`docker run` basta chamar o script:

```bash
docker run --rm -v "$PWD":/project -w /project -e XDG_CACHE_HOME=/project/build/cache \
  espressif/idf:v4.4.8 firmware/retro-go/build.sh build-img
```

Num job do GitHub Actions com `container: espressif/idf:v4.4.8`, o `ENTRYPOINT` não roda nos
passos; ativar o ESP-IDF no próprio passo:

```bash
. "$IDF_PATH/export.sh"
XDG_CACHE_HOME="$PWD/build/cache" firmware/retro-go/build.sh build-img
```

A imagem sai em `build/cache/d-bmo/retro-go-4ced12066975/retro-go_<versão>_dbmo-v1.img`
(`rg_tool.py:224`; a versão padrão é `4ced120-dbmo`). Na CI roda a variante sem argumentos
(só o launcher); `build-img` nunca foi executado.

### Conferência sem ESP-IDF

```bash
firmware/retro-go/tests/host_check.sh
```

Prepara a árvore do Retro-Go (clone, cópia e registro do alvo) e passa o `config.h` pelo
compilador do computador com `-fsyntax-only`, em cada combinação de opções, contra o `rg_input.h`
do Retro-Go e substitutos dos símbolos do ESP-IDF. Confere sintaxe e estrutura. Não compila o
firmware.

## Como gravar

A gravação é pela imagem `.img`, na porta USB nativa (USB Serial/JTAG, GPIO 19/20):

```bash
firmware/retro-go/build.sh --port /dev/ttyACM0 install     # compila tudo, gera a imagem e grava
```

O `install` grava a imagem inteira a partir do endereço 0 (`rg_tool.py:144-150,222-231`), o que
apaga o firmware anterior (o Bruce, por exemplo). Para gravar à mão uma imagem pronta, o comando
do upstream é `esptool.py write_flash --flash_size detect 0x0 <imagem>` (`BUILDING.md:61`).
Depois da primeira gravação dá para regravar um app só: `build.sh --port <porta> flash launcher`,
e acompanhar o log com `build.sh --port <porta> monitor launcher` (`BUILDING.md:69-74`).

O nome da porta depende do sistema (`/dev/ttyACM0` é o usual no Linux). Se a placa não entrar em
modo de gravação sozinha, usar o procedimento da própria placa (na DevKitC, segurar BOOT e
apertar RESET). Essas duas informações não foram verificadas nesta sessão.

O cartão SD tem que estar em FAT32. O Retro-Go procura as ROMs em `/roms/<sistema>`, com o
nome curto de cada sistema: `nes`, `gb`, `gbc`, `sms`, `gg` e os demais
(`rg_storage.h:14`; `launcher/main/applications.c:680,692-702`).

### Teste só com a bancada de hoje

Com tela e três botões, sem SD:

```bash
DBMO_RG_BENCH=1 firmware/retro-go/build.sh --port /dev/ttyACM0 --fatsize 0x200000 install launcher
```

`--fatsize` cria a partição FAT interna `vfs` (`rg_tool.py:93-94`), que o perfil de bancada usa
quando o SD não monta (`rg_storage.c:168-183`). O valor tem que ser numérico: o `mkfw.py` o
converte com `int()` (`tools/mkfw.py:19`), apesar de a ajuda do `rg_tool.py` citar `500K` e `5M`.

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
3. **14 teclas para 16 botões.** Os cliques dos analógicos não têm tecla e não estão no
   primeiro MCP23017 (GPA7 e GPB7 são somente saída; ver a tabela de botões).
4. **Analógicos.** O Retro-Go não tem API de eixo analógico: a leitura é uma máscara de teclas
   (`rg_input.h:96`). O driver ADC converte faixas de tensão em teclas (`rg_input.c:110-125`), e é
   assim que o alvo `fri3d-2024` usa um joystick (`config.h:53-60`). Por isso só o analógico
   esquerdo entra, como D-pad, e atrás de uma opção. O direito não tem uso.
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

1. **Compilar.** O alvo nunca passou pelo ESP-IDF. O primeiro passo é um job de CI com
   `espressif/idf:v4.4.8` rodando `firmware/retro-go/build.sh build-img`.
2. **MCP23017 ausente.** Sem ele o perfil padrão não tem botões. Bloqueia o uso, não o build.
3. **SD no barramento compartilhado ainda não foi ligado nem testado.** O Retro-Go precisa de
   armazenamento para ROMs e configuração; sem ele o launcher mostra erro de cartão
   (`launcher/main/main.c:453-457`).
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

Nada foi compilado com o ESP-IDF e nada foi executado no hardware nesta sessão. O que segue foi
conferido por leitura do código, com arquivo e linha.

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

**Não feito:** build com o ESP-IDF; execução da ferramenta de configuração do ESP-IDF; qualquer
teste em hardware; conferência do `docker run` e do job de CI descritos acima.
