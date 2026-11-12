<!-- Licença: AGPL-3.0, a mesma do Bruce (ver firmware/bruce/LICENSE). -->

# Bruce para o d-BMO v1

Build próprio do [Bruce](https://github.com/pr3y/Bruce) (tag **1.16.1**) para o d-BMO v1:
ESP32-S3 N16R8, tela ILI9341 320x240, CC1101, PN532, microSD e infravermelho. O binário
oficial da DevKitC não tem tela, por isso esta pasta acrescenta uma "placa" ao Bruce, sem
alterar o código dele: um arquivo de ambiente do PlatformIO, a interface da placa e um script
que junta tudo no clone do Bruce e compila.

> **Estado desta revisão.** Nada aqui foi compilado nem executado na sessão em que foi escrito
> (o PlatformIO não instala no ambiente usado). O que foi conferido, e como, está em
> [Verificação](#verificação). A compilação precisa ser provada pela CI e o comportamento, na
> bancada.

## Arquivos

| Arquivo | Vai para (no clone do Bruce) | Função |
|---|---|---|
| `bmo.ini` | `boards/bmo/bmo.ini` | Ambientes `bmo-v1` e `bmo-v1-mcp` do PlatformIO |
| `interface.cpp` | `boards/bmo/interface.cpp` | Interface da placa: GPIO, botões, brilho, bateria, energia |
| `disabled_features.cpp` | `boards/bmo/disabled_features.cpp` | Aviso no lugar dos bloqueadores de sinal retirados do build |
| `../config/include/dbmo/config/pins.h` | `boards/bmo/dbmo/config/pins.h` | Mapa de pinos canônico; o `interface.cpp` usa dele o mapa de botões e o endereço do MCP23017 |
| `build.sh` | — | Baixa ou confere o Bruce, copia os arquivos acima e chama o PlatformIO |
| `LICENSE` | — | AGPL-3.0, a licença do Bruce |

O Bruce encontra o `bmo.ini` porque o `platformio.ini` dele carrega `boards/*/*.ini`
(`platformio.ini:86-88`), e a placa `ESP-General` já define os pinos SPI 10/11/12/13 e I2C 8/9
(`boards/ESP-General/pins_arduino.h:25-31`).

## Compilar e gravar

```bash
firmware/bruce/build.sh                       # compila bmo-v1 (baixa o Bruce em ~/.cache na 1ª vez)
firmware/bruce/build.sh "" upload             # compila e grava pela USB
BRUCE_ENV=bmo-v1-mcp firmware/bruce/build.sh  # compila o ambiente dos botões no MCP23017
firmware/bruce/build.sh /caminho/do/clone     # usa um clone existente do Bruce
```

- **PlatformIO:** o script usa `$PIO` se a variável existir; senão `~/.platformio/penv/bin/pio`
  se existir; senão o `pio` do `PATH`. A plataforma do Bruce exige Python 3.10 a 3.14.
- **Versão fixada:** uma pasta de clone que já existe precisa estar exatamente na tag 1.16.1;
  caso contrário o script para com uma mensagem, sem tocar nela.
- **Saída:** além do `firmware.bin` do PlatformIO, o Bruce gera `Bruce-<ambiente>.bin` (imagem
  única) na raiz do clone (`build.py:36`).
- **Dependências do sistema** (as da CI do próprio Bruce, `.github/workflows/PR_check.yml:77-101`):
  Python 3.13, `gcc-multilib` e `libc6-dev-i386` (o Bruce compila no computador, em 32 bits, um
  gerador de cabeçalhos: `gen_mqjs_headers.py:25-29`) e
  `pip install platformio requests esptool intelhex` (`patch.py:3` importa `requests`).

Comando para a CI, um por ambiente (com `pio` no `PATH`):

```bash
BRUCE_ENV=bmo-v1 firmware/bruce/build.sh "$RUNNER_TEMP/bruce"
BRUCE_ENV=bmo-v1-mcp firmware/bruce/build.sh "$RUNNER_TEMP/bruce"
```

## Ambientes

| Ambiente | Entrada | Estado |
|---|---|---|
| `bmo-v1` | 3 botões diretos (UP 39, OK 2, DOWN 47) | Tela e botões validados na bancada com a versão anterior destes arquivos; esta revisão ainda não foi regravada |
| `bmo-v1-mcp` | 14 botões no MCP23017 (I2C 0x20) | **Não testado em hardware**; a peça ainda não foi ligada |

## Flags `DBMO_*`

Todas ficam **desligadas por padrão**. Para ligar uma, acrescente a linha `-D...` ao
`build_flags` do ambiente no `bmo.ini`. Sem nenhuma delas, a leitura dos botões e o uso da
tela são os mesmos da versão anterior.

| Flag | Padrão | O que liga | Hardware necessário | Estado |
|---|---|---|---|---|
| `DBMO_INPUT_MCP23017=1` | desligada (ligada só em `bmo-v1-mcp`) | Botões lidos do MCP23017 por I2C, no lugar dos 3 GPIOs. Exige também `HAS_5_BUTTONS`, `SYS_I2C_SDA/SCL` e a retirada de `HAS_3_BUTTONS` (o ambiente `bmo-v1-mcp` já faz isso) | MCP23017 em 0x20 no I2C 8/9, botões ao GND | Escrito; não compilado; não testado |
| `DBMO_VBAT_ADC_PIN=<gpio>` | desligada | `getBattery()` lê o divisor da bateria no ADC e devolve 1 a 100 | Divisor 100k/100k da bateria no GPIO 1 (ADC1) | Escrito; não compilado; curva **não calibrada** |
| `DBMO_CHG_STAT_PIN=<gpio>` + `DBMO_CHG_STAT_ACTIVE=<HIGH\|LOW>` | desligadas | `isCharging()` lê o pino de status do carregador | CI de carga com saída de status. **CI e pino ainda não escolhidos** | Só o gancho; sem valor padrão de propósito |
| `DBMO_POWER_HOLD_PIN=<gpio>` + `DBMO_POWER_HOLD_ACTIVE=<HIGH\|LOW>` | desligadas | Segura o power-latch no boot e o solta em `powerOff()` | Circuito de power-latch (GPIO 48 no perfil target). **Circuito ainda não definido** | Só o gancho; sem valor padrão de propósito |

Os pares precisam ser definidos juntos: o `interface.cpp` falha na compilação se vier só um.

## O que esta revisão muda no ambiente `bmo-v1`

Mesmo com todas as flags `DBMO_*` desligadas, o binário muda nestes pontos. Nenhum GPIO mudou.

1. **Nome:** `DEVICE_NAME` passou de "BMO v1" para "d-BMO v1" (aparece no comando `info` da
   serial e nos scripts: `src/core/serial_commands/util_commands.cpp:130`).
2. **`-DTOUCH_CS=-1`** (ver achado A2).
3. **Deep sleep e desligar:** os itens "Deep Sleep" e "Power Off" do menu agora apagam a tela e
   dormem, acordando pelo botão OK (ver [Energia](#energia-poweroff-gotodeepsleep-checkreboot)).
   Antes, "Power Off" não fazia nada.
4. **Módulos padrão:** CC1101 para RF e PN532 por I2C para NFC (achado A7). Só vale em aparelho
   sem `/brucePins.conf` salvo; no aparelho da bancada, trocar em Config ou apagar o arquivo.
5. **Bloqueadores de sinal fora do build** (ver [Funções retiradas](#funções-retiradas-do-build)).

## Funções da interface

### Bateria: `getBattery()` e `isCharging()`

- O Bruce trata **0 como "sem informação de bateria"**: `drawStatusBar()` só desenha o ícone
  com valor maior que zero (`src/core/display.cpp:921-923`), e `isCharging()` só é consultado
  nesse caso (`src/core/display.cpp:1074-1077`). Sem a flag, `getBattery()` devolve 0, como a
  placa `ESP-General` (`boards/ESP-General/interface.cpp:16`).
- Com `DBMO_VBAT_ADC_PIN`: média de 8 leituras de `analogReadMilliVolts()` (a mesma função do
  Bruce em `src/core/utils.cpp:55`), vezes 2 pelo divisor. Abaixo de 2500 mV na bateria devolve
  0 (sem célula no divisor). De 3300 mV a 4200 mV, uma **reta**: é uma aproximação simples e
  **não calibrada**; a descarga de uma LiPo não é linear e a tensão cai sob carga. Uma leitura
  válida nunca devolve menos de 1, para não virar "sem informação".
- O Bruce já tem um mecanismo equivalente (`ANALOG_BAT_PIN`, `src/core/utils.cpp:46-63`), mas
  ele fica inativo porque o `getBattery()` desta placa substitui a versão fraca do Bruce. A
  versão daqui acrescenta a média e o caso "sem célula".
- `isCharging()` sem a flag devolve `false`. Com a flag, o pino recebe pull contrário ao nível
  ativo, para que um pino solto leia "não carregando".

### Energia: `powerOff()`, `goToDeepSleep()`, `checkReboot()`

Como o Bruce chama cada uma:

| Função | Quem chama | Versão fraca do Bruce |
|---|---|---|
| `goToDeepSleep()` | Menu Config > "Deep Sleep" (`src/core/menu_items/ConfigMenu.cpp:243`) | ext0 em `DEEPSLEEP_WAKEUP_PIN` e dorme, sem tocar na tela (`src/core/mykeyboard.cpp:1379-1391`) |
| `powerOff()` | Menu "Power Off" depois da confirmação (`ConfigMenu.cpp:252`) e comando serial `poweroff` (`src/core/serial_commands/power_commands.cpp:5-7`) | Mostra "Not available" (`mykeyboard.cpp:1378`) |
| `checkReboot()` | A cada volta do menu principal (`src/core/display.cpp:568-569`) | Vazia (`mykeyboard.cpp:1393`) |
| `_pre_storage_gpio()` | Depois do primeiro uso da tela, antes do armazenamento (`src/main.cpp:492`) | Vazia (`src/main.cpp:219-220`). Só a CoreS3 a usa; o d-BMO não precisa e não a define |

O que esta placa faz:

- **`goToDeepSleep()`** segue o padrão das placas do Bruce (`boards/lilygo-t-display-s3/interface.cpp:195-203`,
  `boards/ES3C28P/interface.cpp:239-249`): espera soltar o botão de acordar, apaga o backlight,
  manda o comando SLPIN (0x10) ao painel, arma o ext0 e dorme. Dois cuidados a mais:
  - espera soltar o botão, porque no `bmo-v1` o botão que acorda (OK) é o mesmo que escolhe a
    opção no menu;
  - liga o pull-up **RTC** do pino (`rtc_gpio_pullup_en`). No ESP32-S3 o pull-up do `pinMode()`
    não vale quando o pino passa ao domínio RTC, e a documentação do ESP-IDF manda configurá-lo
    antes de dormir (`docs/en/api-reference/system/sleep_modes.rst:205-207`). Nenhuma placa do
    Bruce faz isso; elas têm pull-up externo no botão.
- **`powerOff()`** sem a flag do latch é o mesmo deep sleep; a `ES3C28P` também trata as duas
  funções como uma só (`boards/ES3C28P/interface.cpp:239-255`). Com `DBMO_POWER_HOLD_PIN`, solta o latch,
  espera 500 ms e, se a placa continuar viva (USB), dorme. O padrão é o do M5StickC Plus2
  (`boards/m5stack-cplus2/interface.cpp:90-94`).
- **`checkReboot()`** fica vazia. Nas placas de três botões do Bruce, quem a implementa usa um
  botão de energia dedicado (`m5stack-cplus2/interface.cpp:101-129`) ou cima+baixo por 3 s
  (`lilygo-t-display-s3/interface.cpp:206-242`). No d-BMO, cima+baixo já é "voltar", e o botão
  de energia (GPIO 0 no perfil target) depende do circuito de latch, ainda não definido. A
  placa que serviu de base também a deixa vazia (`boards/ESP32-C5-tft/interface.cpp:193`).

Limites conhecidos do deep sleep: só o painel e o ESP32 dormem; CC1101, PN532 e os demais
módulos continuam alimentados. O estado do pino do backlight durante o sono não é mantido por
software.

## Entrada pelo MCP23017 (`bmo-v1-mcp`)

**Não testado em hardware.**

- **Leitura:** uma transação I2C por consulta lê GPIOA e GPIOB (registradores 0x12 e 0x13). Na
  inicialização, IODIR e GPPU (0x00/0x01 e 0x0C/0x0D) configuram entradas com pull-up interno.
  Usa só o `Wire`, que já faz parte do build do Bruce.
- **GPA7 e GPB7 não são lidos.** O datasheet atual do MCP23017 (DS20001952D) declara esses dois
  pinos **somente saída** e manda programar o bit 7 de IODIR como saída
  (`docs/hardware/sources/PERIPHERALS.md`, MCP-03 a MCP-05). O código segue o fabricante: os
  bits 7 e 15 ficam como saída e fora da leitura, e o mapa de `pins.h` não põe botão neles
  (14 botões no primeiro MCP23017). Os cliques dos analógicos ficam pendentes de decisão de
  hardware (proposta: segundo MCP23017, `pins.h mcp_aux`) e não são lidos neste ambiente. Um
  `static_assert` impede que um botão de navegação seja remapeado para esses bits.
- **Mapa para as variáveis do Bruce** (as de `src/main.cpp:40-49`), no perfil `HAS_5_BUTTONS`,
  o mesmo da `smoochiee-board` (`boards/smoochiee-board/interface.cpp:95-125`):

  | Botão (`pins.h`) | Variável | Efeito nos menus (`src/core/display.cpp:664-713`) |
  |---|---|---|
  | D-pad esquerda | `PrevPress` | Item anterior; segurar por mais de 0,7 s sai do submenu |
  | D-pad direita | `NextPress` | Próximo item |
  | D-pad cima / baixo | `UpPress` / `DownPress` | Item anterior / próximo |
  | A | `SelPress` | Selecionar |
  | B | `EscPress` | Voltar |
  | L1 / R1 | `PrevPagePress` / `NextPagePress` | Página anterior / próxima (lista de arquivos) |
  | X, Select, Start, L2, R2, clique do analógico esquerdo | só `AnyKeyPress` | Acordam a tela, pulam a animação de boot |

  No teclado de tela, o perfil de 5 botões move o cursor nas quatro direções
  (`src/core/mykeyboard.cpp:1056-1140`); o de 3 botões não.
- **Barramento compartilhado:** o MCP23017 e o PN532 usam os mesmos pinos. As funções do Bruce
  pegam o I2C com `acquireI2CBus()` e, ao sair, o **desligam** com `releaseI2CBus()`
  (`src/core/bus_HAL.cpp:207-216`), a não ser que os pinos sejam os do `sys_i2c`
  (`bus_HAL.cpp:164-186`). Por isso o ambiente define `SYS_I2C_SDA=8` e `SYS_I2C_SCL=9` e o
  `interface.cpp` chama `setSysI2CBus(&Wire)`, como a `ES3C28P`
  (`boards/ES3C28P/pins_arduino.h:29-30`, `interface.cpp:107-108`). Sem isso, os botões
  morreriam depois da primeira leitura de NFC. Com o `sys_i2c` definido, o Bruce também passa a
  vigiar o barramento preso (`bus_HAL.cpp:218-298`).
- **Contexto de tarefa:** `InputHandler()` roda na tarefa "InputHandler"
  (`src/main.cpp:84-113`, criada em `src/main.cpp:526-533`), enquanto o PN532 é usado pela tarefa
  principal. O código conta com a trava interna do `Wire` do core do Arduino para separar as
  transações, como fazem as placas do Bruce que leem teclado ou touch por I2C nessa tarefa
  (`boards/ES3C28P/interface.cpp:60-83`, `boards/lilygo-t-deck/interface.cpp:174-180`). Essa
  trava **não foi conferida por leitura** (o core não está disponível aqui).
- **Sem resposta do MCP23017:** nenhum botão é reportado e a configuração é refeita na
  tentativa seguinte (5 por segundo). O aparelho fica sem entrada, mas não trava.
- **Acordar do deep sleep:** os botões do MCP não acordam o chip. O ambiente usa o GPIO 0
  (botão BOOT da DevKitC; botão de energia do perfil target), como várias placas S3 do Bruce
  (`boards/ES3C28P/pins_arduino.h:219`, `boards/lilygo-t-deck/pins_arduino.h:74`).
- **Troca de flags por herança:** `build_unflags` com o texto exato da flag herdada é o padrão
  do Bruce (`boards/m5stack-cplus1_1/m5stack-cplus1_1.ini:152-153`,
  `boards/elecrow/elecrow.ini:246-251`). Se a retirada de `HAS_3_BUTTONS` falhar, o
  `interface.cpp` para a compilação com `#error`.

## Auditoria do `bmo.ini`

Comparado com `boards/ESP-General/ESP-General.ini`, com as outras placas ESP32-S3 e com o uso
de cada flag em `src/`, `include/` e `lib/` do Bruce 1.16.1.

| ID | Achado | Evidência (Bruce 1.16.1) | Ação |
|---|---|---|---|
| A1 | Nenhuma das 55 flags `-D` do `bmo.ini` tem nome que o Bruce não reconheça. `MINBRIGHT`, `UP_BTN`, `SEL_BTN`, `DW_BTN` e `BTN_ACT` só existem para o `interface.cpp` das placas, o que é o esperado | Busca por palavra inteira em `src/`, `include/`, `lib/` e `boards/` | Nenhuma |
| A2 | Faltava `TOUCH_CS`. Sem a flag, a biblioteca TFT_eSPI compila a classe `TFT_eSPI` sem a extensão de touch, e o `src/` a vê com a extensão (o valor -1 vem de um cabeçalho que a biblioteca não inclui). O Bruce compila com LTO e `-Werror=odr`. Todas as placas do Bruce definem `TOUCH_CS` para o build inteiro | `include/precompiler_flags.h:123-125`; `lib/TFT_eSPI/TFT_eSPI.h:1019-1031`; `platformio.ini:132,141`; `boards/ESP-General/ESP-General.ini:71` | **Acrescentado `-DTOUCH_CS=-1`** |
| A3 | `USE_CC1101_VIA_SPI`, presente nas placas do Bruce, não é lida por nenhum código | `boards/ESP-General/ESP-General.ini:97`; zero ocorrências em `src/`, `include/`, `lib/` | Nenhuma (não é necessária) |
| A4 | `TFT_DISPON` e `TFT_DISPOFF` repetem o que o driver já define, com os mesmos valores | `lib/TFT_eSPI/TFT_Drivers/ILI9341_Defines.h:46-47` | Mantidas (inofensivas) |
| A5 | Comentário antigo: o microSD era "módulo separado". O plano atual é o slot da tela, CS 21, no mesmo SPI. Os nomes `SDCARD_*` estão certos e o Bruce reaproveita o SPI da tela quando o MOSI coincide | `src/core/sd_functions.cpp:52-80`; `src/core/bus_HAL.cpp:330-334` | **Comentário corrigido** |
| A6 | Comentário antigo: "IR (README: RX 4, TX 5)". O README hoje dá 39/47 como alvo | `README.md` > Pinagem | **Comentário corrigido**; pinos mantidos (ver Migração) |
| A7 | O módulo de RF padrão do Bruce é o de um pino só, com TX/RX nos pinos do I2C (8/9), e o de NFC é o RFID2. As placas com CC1101 e PN532 trocam o padrão em `_setup_gpio()` | `src/core/configPins.h:228-230,239`; `boards/ES3C28P/interface.cpp:116-117`; `boards/lilygo-t-display-s3/interface.cpp:103-104` | **Padrões definidos no `interface.cpp`** |
| A8 | `ALLOW_ALL_GPIO_FOR_IR_RF` oferece no menu os GPIO 0 a 44, menos os da tela e do SD. A lista inclui 22 a 37 (inexistentes, flash ou PSRAM no N16R8) e os pinos do I2C, do CC1101 e da USB; e **não inclui 45 a 48**, logo o IR TX 47 do perfil target não pode ser escolhido pelo menu | `include/precompiler_flags.h:156-163,177-187`; `src/core/settings.cpp:1073-1077` | Mantido; trocar por listas explícitas na etapa do IR (ver Migração) |
| A9 | A serial e o GPS do Bruce usam por padrão os pinos do I2C (8/9) | `include/precompiler_flags.h:139-153` | Mantido; não usar GPS/serial externa pelo menu até a etapa da UART |
| A10 | `SPI_SS_PIN=14` é o mesmo CS do CC1101. O leitor RC522 por SPI usaria esse CS. O upstream faz igual (`SPI_SS_PIN` = `CC1101_SS_PIN` = 10) | `src/modules/rfid/RFID2.h:43`; `boards/ESP-General/ESP-General.ini:99,116` | Mantido; no perfil target o CS de expansão é o do cartucho (38) |
| A11 | Áudio I2S não configurado. O Bruce espera `HAS_NS4168_SPKR` com `BCLK`, `WCLK`, `DOUT`, `MCLK` para o alto-falante e `MIC_INMP441` com `PIN_CLK`, `PIN_WS`, `PIN_DATA` para o microfone; o pino de shutdown do amplificador entraria pelo gancho `_setup_codec_speaker()` | `src/modules/others/audio.cpp:8,18-19,132`; `src/modules/others/mic.cpp:2,141-156`; `boards/smoochiee-board/smoochiee-board.ini:27` | Não acrescentado: MAX98357A e INMP441 ainda não ligados |
| A12 | O pino padrão do iButton é o GPIO 0, o botão de energia do perfil target | `src/core/configPins.h:236` | Mantido; anotado |
| A13 | A placa `ESP-General` declara 8 MB de flash; o `bmo.ini` troca para 16 MB e para a tabela `custom_16Mb.csv` (aplicativo de 0x470000 bytes), sem trocar `maximum_size` | `boards/_boards_json/ESP-General.json:45-47`; `custom_16Mb.csv:3` | Mantido (é a configuração que rodou na bancada); o limite de tamanho efetivo fica para a CI mostrar |
| A14 | `lib_ignore = FastLED` é coerente: o Bruce só inclui a FastLED com `HAS_RGB_LED` | `src/core/led_control.cpp:6-10` | Nenhuma |
| A15 | `powerOff()` vazio escondia o aviso "Not available" do Bruce; o deep sleep padrão não apagava a tela nem ligava o pull-up RTC | `src/core/mykeyboard.cpp:1378-1391` | **Implementados** (ver Energia) |

## Migração dos pinos por etapas

O `bmo.ini` descreve a **bancada de hoje** (perfil `bench` de `pins.h`). O perfil `target` usa
alguns desses GPIOs para outra coisa:

| GPIO | `bmo.ini` hoje | Perfil target (`pins.h`) |
|---:|---|---|
| 2 | Botão OK e pino que acorda do deep sleep | Analógico esquerdo X |
| 4 | IR RX | Analógico esquerdo Y |
| 5 | IR TX | Analógico direito X |
| 39 | Botão UP | IR RX |
| 47 | Botão DOWN | IR TX |
| 0 | (iButton padrão do Bruce) | Botão de energia |

Os pinos validados na bancada não foram alterados. A troca é por etapas, cada uma só quando o
hardware correspondente estiver ligado:

| Etapa | Quando | O que muda no Bruce | Estado |
|---|---|---|---|
| 0 | Hoje | Ambiente `bmo-v1`: tela, 3 botões diretos, IR em 4/5, CC1101 em 14/15, SD em 21 | Tela e botões validados; o resto ainda não ligado |
| 1 | MCP23017 ligado | Ambiente `bmo-v1-mcp`. Libera 39, 47 e 2; quem acorda passa a ser o GPIO 0 | Escrito; não testado |
| 2 | Módulo de IR ligado em 39/47 | `-DRXLED=39`, `-DTXLED=47`; trocar `ALLOW_ALL_GPIO_FOR_IR_RF` por `IR_TX_PINS`, `IR_RX_PINS`, `RF_TX_PINS` e `RF_RX_PINS` explícitos (as quatro juntas; formato em `boards/ESP-General/ESP-General.ini:44-52`), por causa do achado A8. Libera 4 e 5 | Pendente |
| 3 | Analógicos ligados em 2/4/5/6 | Nada: o Bruce não lê analógicos. Basta as etapas 1 e 2 terem liberado os pinos | Pendente |
| 4 | Divisor da bateria no GPIO 1 | `-DDBMO_VBAT_ADC_PIN=1` | Escrito; não testado |
| 5 | Carregador e power-latch definidos | Flags `DBMO_CHG_STAT_*` e `DBMO_POWER_HOLD_*` | Só os ganchos |
| 6 | MAX98357A e INMP441 ligados | Flags de áudio do achado A11 (BCLK 40, LRCLK 41, DOUT 42, microfone 7, shutdown 3). O valor de `MCLK` para um amplificador sem MCLK não foi definido | Pendente |
| 7 | UART do cartucho | `-DSERIAL_TX=43`, `-DSERIAL_RX=44`, `-DGPS_SERIAL_TX=43`, `-DGPS_SERIAL_RX=44` (achado A9). Em `pins.h` esses pinos estão marcados como de risco | Pendente |
| 8 | PCB | Avaliar subir `SPI_FREQUENCY` (hoje 10 MHz, o que rodou nos jumpers e o limite de escrita do datasheet do ILI9341; acima disso é fora de especificação) | Pendente; exige validação na placa |

Atenção: IR, RF e NFC têm os pinos e módulos **salvos em `/brucePins.conf`**; o valor salvo
prevalece sobre o do `bmo.ini`. Depois de mudar um padrão, ajustar no menu Config ou apagar o
arquivo.

## Funções retiradas do build

O projeto não admite bloqueio de sinal. O `bmo.ini` tira do build os três arquivos do Bruce
dedicados a isso (`src/modules/rf/rf_jammer.cpp`, `src/modules/ir/ir_jammer.cpp`,
`src/modules/NRF24/nrf_jammer.cpp`), e o `disabled_features.cpp` fornece os três pontos de
entrada que os menus chamam (`src/core/menu_items/RFMenu.cpp:31`, `IRMenu.cpp:20`,
`NRF24.cpp:16`), mostrando só o aviso "Disabled on d-BMO". Não há flag para religar. Fora
desses menus, nenhum outro código do Bruce usa esses arquivos (busca pelos símbolos em `src/`).

Isto **não torna o build inteiro conforme** à política do projeto. O Bruce é uma ferramenta de
teste de segurança e traz outras funções de uso duplo em WiFi, BLE, RF e NFC que esta pasta não
remove nem acrescenta. Decidir o que mais sai do build é uma pendência do projeto; ver
`docs/hardware/RF_REGULATORY_NOTES.md`.

## Pendências

1. **Compilar** os dois ambientes na CI. Nada foi compilado.
2. **Regravar e revalidar o `bmo-v1`** na bancada: tela, 3 botões, e os itens novos (Deep Sleep
   e Power Off acordando pelo OK; o botão OK funcionando depois de acordar).
3. **MCP23017:** testar o `bmo-v1-mcp`; decidir o destino dos botões em GPA7 e GPB7.
4. **Carregador:** escolher o CI de carga, o pino de status e o nível ativo.
5. **Power-latch:** definir o circuito, o pino e o nível. O GPIO 48 não é RTC: sem mais código,
   o nível do latch não se mantém durante o deep sleep, e dormir equivaleria a desligar.
6. **Bateria:** calibrar a curva com medidas reais; conferir se a impedância do divisor
   100k/100k permite leitura estável no ADC (capacitor no nó, se preciso).
7. **Deep sleep:** medir o consumo; conferir o backlight durante o sono; decidir se CC1101 e
   PN532 devem ser postos em repouso antes.
8. **`checkReboot()`:** implementar quando existir o botão de energia.
9. **Etapas 2, 6 e 7** da migração.
10. **Política de funções do Bruce** além dos bloqueadores (seção anterior).
11. **microSD no SPI da tela:** conferir na bancada a convivência de tela, SD e CC1101 no mesmo
    MISO.

## Verificação

**Nada foi compilado nem executado em hardware nesta sessão.** O PlatformIO e o ESP-IDF não
instalam no ambiente usado. A CI deve provar a compilação dos dois ambientes; a bancada, o
comportamento.

O que foi feito:

- **Leitura do Bruce 1.16.1** (clone na tag, commit `ba519c9`): cada símbolo, macro, flag e
  caminho citado neste arquivo foi localizado no código, nas linhas indicadas. Em particular:
  as variáveis de navegação (`src/main.cpp:40-49`, `include/globals.h:199-217`); as assinaturas
  de `include/interface.h`; `tft.writecommand` (`lib/HAL/display/tftespi.h:63`);
  `setSysI2CBus`/`acquireI2CBus`/`releaseI2CBus` (`src/core/bus_HAL.h:19-33`);
  `displayWarning` (`src/core/display.h:130`); a classe `RFJammer`
  (`src/modules/rf/rf_jammer.h:15-18`), `startIrJammer` (`src/modules/ir/ir_jammer.cpp:698`) e
  `nrf_jammer` (`src/modules/NRF24/nrf_jammer.cpp:16`); as chamadas de Arduino usadas, todas
  com precedente no Bruce (`analogReadMilliVolts` em `src/core/utils.cpp:55`,
  `Wire.requestFrom((uint8_t), (uint8_t))` em `boards/ES3C28P/interface.cpp:331`,
  `esp_sleep_enable_ext0_wakeup` e `esp_deep_sleep_start` em `src/core/mykeyboard.cpp:1383-1387`,
  `INPUT_PULLDOWN` em `src/modules/bjs_interpreter/gpio_js.cpp:157`, `"driver/gpio.h"` em
  `src/modules/others/mic.cpp:6`).
- **ESP-IDF:** a regra do pull-up RTC para o ext0 foi lida na documentação do ESP-IDF
  (`sleep_modes.rst:205-217`), e `SOC_PM_SUPPORT_EXT0_WAKEUP` e `SOC_HP_I2C_NUM` no `soc_caps.h`
  do ESP32-S3. As assinaturas de `rtc_gpio_pullup_en` e `rtc_gpio_pulldown_dis` só puderam ser
  lidas no cabeçalho do **ESP-IDF 4.4.8** (`driver/rtc_io.h:144,186`), não no da versão que o
  Bruce usa; a existência de `driver/rtc_io.h` na versão atual foi vista só na listagem de
  arquivos do repositório. O mesmo vale para a afirmação de que `pinMode()` devolve o pino ao
  modo digital depois de acordar (lida em `gpio_config()` do 4.4.8).
- **MCP23017:** os endereços dos registradores foram conferidos com o driver do Retro-Go
  (`components/retro-go/rg_i2c.c:155-160`); a restrição de GPA7/GPB7, com
  `docs/hardware/sources/PERIPHERALS.md`. O avanço automático do ponteiro de registrador e o
  estado de IOCON após o reset **não foram conferidos no datasheet**.
- **Não conferido por leitura:** o core do Arduino para ESP32 (trava do `Wire`, atenuação
  padrão do ADC) e o PlatformIO (`build_unflags`, `build_src_filter`), que não estão
  disponíveis aqui. Para esses, a evidência é o uso idêntico em arquivos do próprio Bruce.
- **Checagens feitas no computador, que não substituem a compilação real:**
  - `bash -n build.sh`, e o `build.sh` executado com um PlatformIO de mentira sobre uma cópia
    do clone, cobrindo: ambiente padrão, `upload`, `BRUCE_ENV`, ambiente inexistente,
    PlatformIO ausente, clone fora da tag e pasta que não é repositório;
  - sintaxe de `interface.cpp` (padrão, MCP e todas as flags) e de `disabled_features.cpp` com
    `g++ -fsyntax-only` contra **cabeçalhos substitutos escritos à mão** a partir das
    assinaturas lidas no Bruce. Isso pega erro de digitação e de tipo no código desta pasta;
    não prova que ele compila com os cabeçalhos verdadeiros;
  - estrutura do `bmo.ini` lida com o `configparser` do Python (seções, chaves, flags
    duplicadas, texto exato das flags de `build_unflags`).
