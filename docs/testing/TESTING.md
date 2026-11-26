# Testes e verificação

> Estado em 2026-11-12. Este documento descreve o que existe no repositório e como
> reproduzir cada verificação. O que foi de fato executado, e com qual resultado, está na
> seção [Evidência de execução](#evidência-de-execução).

## 1. Níveis de teste

| Nível | Onde fica | Roda em | O que prova | O que não prova |
|---|---|---|---|---|
| Unitário | `tests/unit/<subsistema>/` | Computador (CMake + Unity) | A lógica de cada módulo, contra mocks de HAL e simuladores dos chips | Que o chip real se comporta como o simulador |
| Integração | `tests/integration/` | Computador (CMake + Unity) | O firmware inteiro montado com os simuladores: ordem de inicialização, eventos entre módulos, degradação com hardware ausente | Temporização real, elétrica, barramentos reais |
| Bancada | `tests/hardware/` | ESP32-S3 com o periférico ligado (PlatformIO + Unity) | Que o periférico real responde como o datasheet diz | Só roda com hardware; o CI apenas compila esses testes |
| Integridade | `tools/check_*.py`, `tools/check_format.sh` | Computador | Coerência entre `pins.h`, Bruce, Retro-Go e documentação; links; regras de portabilidade e privacidade; formatação | Comportamento do código |
| Compilação para o alvo | `platformio.ini`, `firmware/bruce/build.sh`, `firmware/retro-go/build.sh` | CI (GitHub Actions) | Que o código compila e liga para o ESP32-S3 | Que funciona no aparelho |

Regra do projeto: um teste só é dado como aprovado se foi executado. Compilar para o
ESP32-S3 não é evidência de funcionamento no hardware.

## 2. Testes nativos (computador)

Requisitos: CMake 3.16 ou mais novo, Ninja (ou Make) e um compilador C++17 (GCC ou
Clang). Nenhuma dependência é baixada: o Unity está em `tests/third_party/unity/`.

```bash
cmake -S . -B build/host -G Ninja -DDBMO_SANITIZE=ON
cmake --build build/host
ctest --test-dir build/host --output-on-failure
```

Opções do CMake:

| Opção | Padrão | Efeito |
|---|---|---|
| `DBMO_SANITIZE` | `OFF` | AddressSanitizer + UndefinedBehaviorSanitizer |
| `DBMO_WERROR` | `ON` | Avisos viram erro |
| `DBMO_COVERAGE` | `OFF` | Instrumentação para `gcov`/`gcovr` |
| `DBMO_SUBSYSTEMS` | todos | Lista de subsistemas a compilar, por exemplo `-DDBMO_SUBSYSTEMS="config;core;radio"` |

Cada arquivo `test_*.cpp` vira um executável com o seu próprio `main()`; para ver os
casos de um deles, basta executá-lo (`build/host/tests/unit/core/core_test_system`).

O build nativo usa as mesmas restrições do firmware: `-fno-exceptions -fno-rtti`, sem
alocação dinâmica no código portável (conferido também por `tools/check_repo.py`).

### Mocks e simuladores

- `tests/mocks/include/dbmo/mock/mock_hal.h`: relógio manual, GPIO, barramento I2C com
  dispositivos simulados, dispositivo SPI, ADC, PWM, coletor de log e persistência de
  configuração em memória. Todos permitem injetar falhas.
- Simuladores de chip ficam junto dos testes do subsistema que os usa
  (`tests/unit/<subsistema>/`) e são bibliotecas `dbmo_<subsistema>_sim`, reutilizadas
  pelos testes de integração. Eles são escritos a partir do datasheet, não a partir do
  driver; os valores esperados nos testes vêm de exemplos do datasheet ou de contas
  feitas à mão.

Limite conhecido: um simulador reproduz a leitura que se fez do datasheet. Um erro de
interpretação do datasheet passa nos testes nativos e só aparece na bancada. Por isso
cada módulo lista no seu `README.md` o que aguarda hardware.

## 3. Testes de bancada (hardware)

Ficam em `tests/hardware/test_<nome>/` e rodam na placa pelo PlatformIO:

```bash
pio test -e dbmo-bench -f test_<nome>     # compila, grava e roda um teste
pio test -e dbmo-bench --without-uploading --without-testing   # só compila (é o que o CI faz)
```

Cada teste de bancada verifica um periférico por vez, na ordem do guia de montagem
([`docs/hardware/ASSEMBLY_GUIDE.md`](../hardware/ASSEMBLY_GUIDE.md)). Nenhum deles
transmite por rádio ou por infravermelho.

## 4. Verificações de integridade

```bash
python3 tools/check_pinmap.py    # pins.h x bmo.ini x alvo do Retro-Go x blocos gerados da documentação
python3 tools/check_links.py     # links relativos e âncoras dos Markdown
python3 tools/check_repo.py      # estrutura, portabilidade, ausência de rede e de segredos, sintaxe de shell
tools/check_format.sh            # clang-format 18 (use --fix para aplicar)
```

`tools/check_pinmap.py --write` regrava as tabelas de pinos geradas dentro dos Markdown
(blocos entre os marcadores `BEGIN pinmap` e `END pinmap`). A fonte de verdade dos
pinos é `firmware/config/include/dbmo/config/pins.h`.

## 5. Integração contínua

`.github/workflows/ci.yml` roda em todo pull request e em pushes para `main` e para
ramos `claude/**`. As actions são fixadas por commit.

| Job | O que faz |
|---|---|
| Testes nativos (gcc + ASan/UBSan) | Compila e roda todos os testes unitários e de integração com sanitizers |
| Testes nativos (clang) | O mesmo, com Clang, sem sanitizers |
| Integridade do repositório | As quatro verificações da seção 4 |
| Firmware próprio (ESP32-S3) | `pio run` nos perfis `dbmo-bench` e `dbmo-target`, compila os testes de bancada e publica os binários como artefato |

Os jobs de compilação do Bruce, do Retro-Go e dos modelos CAD estão descritos na seção
de evidência, com o estado de cada um.

## Evidência de execução

Registro do que foi **executado de fato** nesta etapa de desenvolvimento. O que não foi
executado está na última tabela, sem resultado atribuído.

### Retirada dos analógicos e dos gatilhos (2026-11-25, no computador)

Executado num macOS (Darwin 27), sem placa:

| O que | Resultado |
|---|---|
| `ctest` com `-DDBMO_SANITIZE=OFF` (CMake e Ninja instalados por `pip`) | **45 de 45 executáveis aprovados** |
| `ctest` com `-DDBMO_SANITIZE=ON` | **Não executado.** Neste macOS, todo executável trava antes do `main`, dentro de `__asan::InitializeShadowMemory` (pilha obtida com `sample`). É problema do ambiente: o sanitizer continua só na CI |
| `clang-format` 18.1.8, `--dry-run --Werror`, nos arquivos C++ alterados | Sem diferenças. O `tools/check_format.sh` não roda no bash 3.2 do macOS (`mapfile`) |
| `tools/check_pinmap.py`, `check_links.py`, `check_repo.py` | Aprovados: 27 sinais no perfil `target`, 19 no `bench`; 0 links quebrados; 0 problemas |
| `firmware/retro-go/tests/host_check.sh` | Todas as conferências passaram (mapa I2C com 10 botões) |
| `mechanical/cad/test_generate_models.py` | 19 ok, 0 falhas, 3 ignorados (geometria: sem cadquery) |
| `pio run -e dbmo-target -e dbmo-bench` | Os dois perfis compilam (`SUCCESS`) |

Não executado: compilação do Bruce e do Retro-Go com o ESP-IDF (ficam para a CI) e qualquer
teste na placa.

### Sem placa: alto-falante no perfil bench e logs de boot (2026-11-25, no computador)

Feito depois do vídeo, sem hardware novo e sem a placa:

| Mudança | Commit | Verificação |
|---|---|---|
| Saída do MAX98357A no perfil `bench` (DIN no GPIO 42, sem pino de SD_MODE) | `efca1de` | Os dois perfis compilam; **nada foi ligado nem tocado** |
| Mapa da protoboard v0.4: fase F (SD) e fase G (alto-falante, ainda não montada) | `c330fb4` | `check_links.py` e `check_repo.py` |
| Log de boot nomeia as chaves da memoria interna quando o cartão não tem configuração | `4a1191c` | Teste novo falhou antes (`usando os padroes` aparecia) e passa depois |
| Periférico ausente vai ao log uma vez, não a cada nova tentativa | `5ab03a8` | Teste novo: 5 avisos em 65 s antes, 1 depois; a ligação depois do boot continua coberta pelo teste existente |
| Pinos e peças com evidência de bancada | `3bf353c` | `check_pinmap.py` (27 sinais no `target`, 23 no `bench`) |
| Botão de energia do `target` do GPIO 0 para o GPIO 2 (RTC, fora do strapping); a bancada continua no BOOT (GPIO 0) | `8e988b7` | `test_power_button_is_an_rtc_gpio_outside_the_strapping_pins` falhou antes (a bancada nem tinha botão de energia próprio) e passa; `static_assert` novo em `pins.h`; `check_pinmap.py` (27 sinais no `target`, 24 no `bench`) |
| Pull-up de RTC no pino que acorda do sono profundo | `ba8be0e` | Só compilado: o sono profundo não roda no computador e não foi testado na placa |

Testes no computador: 47 executáveis aprovados (`-DDBMO_SANITIZE=OFF`); `clang-format` 18
sem diferenças; `pio run -e dbmo-bench -e dbmo-target` compila; `host_check.sh` do Retro-Go
passa. Não executado: nada na placa.

### Vídeo de demonstração: rádio, microfone, IR e Retro-Go na placa (2026-11-25)

Mesma montagem da entrada seguinte (fases A a E e o SD da tela, cartão `DBMO` inserido). Duas
tomadas: a 1ª com o firmware próprio, a 2ª com o Retro-Go gravado por cima.

**Microfone até 10 s (`a6a5739`).** O comando `mic` passou a usar um buffer de 160000 amostras
(10 s a 16 kHz) reservado na PSRAM por `main.cpp`; sem PSRAM, continua com 1 s. No computador:
47 executáveis de teste aprovados (`-DDBMO_SANITIZE=OFF`, ver a entrada "Retirada dos
analógicos"); os perfis `dbmo-bench` e `dbmo-target` compilam. Na placa: `mic 10001` responde
`uso: mic [ms], ate 10000; mic last`.

**Tomada 1, firmware próprio (`a6a5739`).** Um roteiro mandou os comandos pelo console em tempos
fixos; um ensaio e uma tomada, com o mesmo resultado:

| Item | Comando | Saída | Leitura |
|---|---|---|---|
| Rádio | `radio rx on` e, 10 s depois, `radio rx off` | `recepcao: Ok` nos dois | Rosto "procurando" durante a recepção, relatado pelo Gabriel |
| Microfone | `config audio.mic_enabled 1`; `mic 8000` | `gravacao encerrada: 126560 amostras, pico 32768` | Rosto "ouvindo" por 8 s, relatado pelo Gabriel |
| Controle remoto | Menu Infravermelho, teclas do controle | **Nenhum código recebido**; 15 vezes `aprender infravermelho: Ok` sem resultado | **Falhou** |

Com a recepção do rádio ligada, o GPIO 4 registrou de 14 a 33 `infravermelho: sinal
desconhecido` por segundo, e nenhum fora desse intervalo. Em 22/11 isso não acontecia e o
controle era lido (`nec:0004:...`). **Hipótese, não verificada:** o receptor de IR ficou sem
alimentação ou com o OUT solto depois da religação de 24/11; com a linha solta, o GPIO 4 pega o
ruído da fiação do rádio. Pendente: conferir C1 a C3 e a tensão nas pernas do receptor.

**Tomada 2, Retro-Go.** Primeira vez na placa. Imagem de bancada da CI gravada com
`esptool.py --chip esp32s3 write-flash --flash-size detect 0x0` (6.750.464 bytes, 35 s):

| Etapa | Saída | Leitura |
|---|---|---|
| Imagem da execução 45712889400 (SHA-256 `a3858c90…632d`) | `Storage mounted at /sd`, `Input ready`, `Display ready`, `Retro-Go ready`, abas criadas | Launcher funciona; tela **de cabeça para baixo e espelhada** em relação ao firmware próprio |
| Causa | Os dois enviam MADCTL `0x28`; o Retro-Go enviava `0xB6` = `0x0A, 0xA2` (SS=1), o Adafruit `0x08, 0x82` (SS=0) | Correção `c3aad60`: `0xB6` = `0x0A, 0x82` |
| Imagem da execução 45724562378 (SHA-256 `9f1c6082…de2d`) | Mesmo boot | Orientação igual à do firmware próprio, relatado pelo Gabriel |
| ROM | `/roms/gb`, `/roms/gbc`, `/roms/nes` e `/roms/pce` existiam vazias (criadas pelo Retro-Go). A ROM de Game Boy Color do Gabriel estava num `.7z`, que o Retro-Go não abre: descompactada no Mac e copiada para `/roms/gbc` (1.048.576 bytes, SHA-256 igual ao do original) | — |
| Jogo | `Switching to app retro-core (gbc)`; `160x144@1.111 => 266x240`; frameskip entre 1 e 3; rodou de 31 s a pelo menos 183 s do log, sem reinício | Jogo de Game Boy Color rodando na placa; "rodou perfeito", relatado pelo Gabriel |

Na troca do launcher para o jogo, o log registrou `spi_deinit: Failed to properly terminate SPI
driver!`, sem efeito visível.

Limites da evidência:

- O vídeo não foi visto por mim: o rosto, a orientação da tela e o jogo vêm do relato do
  Gabriel e do log.
- O controle remoto não foi lido nesta sessão.
- Na bancada não há Start, Select nem esquerda (3 botões); sem amplificador, sem som; o SPI da
  tela a 10 MHz obriga o frameskip.
- A placa ficou com o Retro-Go: o firmware próprio volta com
  `pio run -e dbmo-bench -t upload`.
- O `config audio.mic_enabled 1` da tomada 1 foi gravado sozinho no cartão: a configuração do
  cartão está com o microfone habilitado até ser desfeita.

### Tela branca resolvida e SD da tela lendo e gravando (2026-11-25)

**Montagem:** fases A a E do [mapa v0.3](../hardware/BREADBOARD_MAP.md), mais os 4 fios do SD
da tela, cujos pinos foram soldados em 24/11: SD_MOSI, SD_SCK e SD_MISO na linha c das colunas
8, 11 e 14 (os nós do SPI) e SD_CS no GPIO 21 (J3 pino 18). Essa ligação está só no artefato
da montagem com o SD; o `BREADBOARD_MAP.md` ainda não a traz. Placa em
`/dev/cu.usbmodem2101`, com o firmware que já estava gravado (`version`:
`d-BMO v1 firmware 0.1.0`; o commit não foi registrado). Console pelo script pyserial.
Cartão: SD de 2 GB em FAT32, volume `DBMO`, colocado e tirado com o USB desligado.

| Etapa | Montagem | Saída | Leitura |
|---|---|---|---|
| 1 | Sem cartão; LED da tela ligado no lugar errado (posição não registrada); 10, 11 e 12 da DevKitC sem fio | `ID do controlador 0xFFFF`; `CC1101 nao respondeu: Timeout`; `sdCommand(): Card Failed!` | Nenhum dos três responde pelo MISO |
| 2 | LED corrigido; 10, 11 e 12 ainda sem fio | `ID do controlador 0x0000`; `CC1101 nao respondeu: HardwareAbsent` | Continua sem SPI |
| 3 | 10 (CS da tela), 11 (MOSI) e 12 (SCK) ligados; sem cartão | `controlador 0x9341`; `CC1101 PARTNUM=0x00 VERSION=0x14`; `radio: Idle, 433920000 Hz`; `storage: Absent` | Tela e CC1101 normais com os fios do SD no barramento |
| 4 | Com o cartão | `cartao: Ok`; `cartao montado`; `storage: Ready (Ok)`; `sd` respondeu `cartao: Ok`; tela e CC1101 `Ready` | O SD responde no SPI dividido |
| 5 | `save` e reinício | `gravar configuracao: Ok`; no boot seguinte, `configuracao carregada (35 chaves, cartao)` | Escrita e leitura no cartão |

**Leitura.** Nesta sessão, as falhas vieram dos fios faltando: sem o SCK e o MOSI da DevKitC,
nenhum dos três dispositivos do SPI recebe nada, e sem o CS a tela não é selecionada. Se a
tela branca de 24/11 teve a mesma causa, não se sabe.

**Microfone habilitado sem querer.** No boot sem cartão, a configuração veio da memória interna
(`configuracao carregada (34 chaves, memoria interna)`) com `microfone: habilitado`. A causa
foram os `config audio.mic_enabled 1` de 22/11: toda chave alterada é gravada sozinha 2 s
depois (`kSaveDelayMs`), mesmo sem `save`. Com o cartão sem arquivo de configuração, o boot
registrou `nenhuma configuracao salva: usando os padroes`, mas o microfone continuou
habilitado: `Application::begin` carrega a configuração da memória interna antes da do cartão,
e o log não diz isso (imprecisão do log, não corrigida). O `save` da etapa 5 gravou
`audio.mic_enabled=1` no cartão. Em seguida, `config audio.mic_enabled 0` foi gravado sozinho
no cartão, e no boot seguinte apareceu `microfone: desabilitado` e `audio.mic_enabled=0`.

Limites da evidência:

- O rosto aparece na tela, relatado pelo Gabriel depois das etapas acima, sem foto.
- A memória interna continua com `audio.mic_enabled=1`; ela só é regravada num boot sem
  cartão.
- No cartão, só o arquivo de configuração foi gravado e lido. O log no cartão e a pasta de IR
  não foram conferidos, e o cartão não foi lido no Mac depois.
- A posição em que o fio do LED estava antes não foi registrada.

### Primeira gravação na placa (2026-11-21)

Firmware próprio, perfil `dbmo-bench`, gravado pela USB nativa da DevKitC-1 N16R8
(`/dev/cu.usbmodem1101`, VID:PID 303A:1001) num macOS. Montagem conforme
[`../hardware/BREADBOARD_MAP.md`](../hardware/BREADBOARD_MAP.md): tela, 3 botões, receptor e
transmissor de IR, CC1101. Log do console copiado pelo usuário, a partir de 10,7 s (o
monitor foi aberto depois do início do boot; as linhas anteriores não foram vistas).

| Subsistema | Resultado no log | Leitura |
|---|---|---|
| `display` | Ready; aviso "backlight sem controle" | Esperado: o LED da tela está direto no 3,3 V |
| `rosto` | Ready | Rosto na tela não confirmado visualmente no registro |
| `input` | Ready | Botões ainda não apertados no registro |
| `infrared` | Ready; "receptor: sim; emissor: sim; envio desabilitado" | Recepção ainda não testada com controle remoto |
| `radio` | **Absent (HardwareAbsent)**, repetido em nova tentativa aos 25,9 s | CC1101 ligado e não identificado: em diagnóstico |
| `nfc` | Absent | Esperado: PN532 sem pinos soldados |
| `audio` | Absent | Esperado: sem pinos no perfil `dbmo-bench` |
| `cartridges` | Ready | Sem soquete; o subsistema não exige hardware para iniciar |
| `storage` | não aparece no trecho copiado | Pendente |
| Sistema | Booting → Degraded | Esperado com periféricos ausentes |

Primeira execução do firmware próprio no hardware. O `HardwareAbsent` do rádio (e não
`Timeout`) indica que a espera pelo "pronto" do chip passou e que a leitura de
PARTNUM/VERSION devolveu um valor de barramento sem chip (0x00 ou 0xFF em VERSION) ou
PARTNUM diferente de 0x00 (`firmware/radio/src/cc1101.cpp`, `identify`).

### Tela apagada e primeiro teste de bancada aprovado (2026-11-21)

Depois da primeira gravação, a tela estava **sem luz de fundo**, embora o log mostrasse
`display: Ready` (o firmware não tem como verificar a alimentação da tela). Medição do
usuário com multímetro digital: **2,65 V** no 3,3 V (ponto exato não registrado). O usuário
encontrou o GND do receptor de IR no lugar errado e o corrigiu; a tela continuou apagada.

Montagem mínima: tela ligada **direto na DevKitC** (9 jumpers fêmea-fêmea, VCC e LED nos
dois pinos 3V3), botões na protoboard pequena, IR e CC1101 desligados. **A luz de fundo
acendeu.** Conclusão: tela e DevKitC em ordem; a falha estava no caminho de alimentação pela
protoboard grande (trilho dividido, mau contato de jumper ou a carga do receptor ligado
errado). Causa exata não isolada.

`pio test -e dbmo-bench -f test_display`, primeira tentativa: erro de compilação no
computador ("Implicit dependency ... unity.h not found", biblioteca Unity instalada durante
a compilação). Resolvido apagando `.pio/build/dbmo-bench`. Segunda execução:

| Teste | Resultado |
|---|---|
| `test_controller_id_is_ili9341` | Aprovado |
| `test_landscape_resolution` | Aprovado |
| `test_fill_and_sleep_commands_are_accepted` | Aprovado |
| `test_backlight_is_reported_as_not_controlled` | Aprovado |

**4 de 4 aprovados, 29,6 s.** Primeiro teste de bancada do firmware próprio aprovado em
hardware.

### Firmware próprio com a montagem mínima (2026-11-21)

Tela direto na DevKitC e 3 botões na protoboard pequena; IR, CC1101, PN532 e SD
desligados. Rosto do d-BMO **visível na tela** (confirmado pelo usuário). Saída do `status`
(console aberto com `--echo --filter send_on_enter --eol CRLF`; sem essas opções o comando
não foi respondido, causa não investigada):

```
aplicacao: Running; sistema: Degraded
  power        Ready         Ok
  storage      Absent        HardwareAbsent
  config       Ready         Ok
  display      Ready         Ok
  rosto        Ready         Ok
  input        Ready         Ok
  audio        Absent        HardwareAbsent
  infrared     Ready         Ok
  nfc          Absent        HardwareAbsent
  radio        Absent        HardwareAbsent
  cartridges   Ready         Ok
bateria: sem leitura (Disabled)
```

Limites desta evidência: `input: Ready` só indica pinos configurados; o firmware não
registra botões no log, então a leitura dos botões **não foi verificada**. `infrared: Ready`
aparece mesmo com os módulos de IR desligados. Com o CC1101 desligado, a primeira tentativa
leu VERSION 0xE0 (MISO solto) e relatou `Failed (IoError)` antes de passar a `Absent`:
classificação imprecisa do firmware, sem risco ao hardware.

### Botões lidos pelo firmware próprio (2026-11-21)

Montagem mínima (seção anterior): botões na protoboard pequena pelo J3, CIMA no 39, OK no 2,
BAIXO no 47, GND do J3 (pino 22) no trilho (−); IR, CC1101, PN532 e SD desligados. Firmware
`dbmo-bench` no commit `66a6723` (registro de botões no log desde `56cd461`), gravado com
`pio run -e dbmo-bench -t upload --upload-port /dev/cu.usbmodem1101` (hash verificado). Console
lido por um script pyserial a 115200 (DTR e RTS em 0, comandos terminados em CRLF), não pelo
`pio device monitor`. Abrir a porta reinicia a placa: cada captura começa com o boot completo
(`rst:0x15 (USB_UART_CHIP_RESET)`), sem precisar do RST.

| Tentativa | Montagem | Resultado no log |
|---|---|---|
| 1 (capturas de 240 s e 900 s) | Como estava | Nenhum evento de botão; não se sabe se os botões foram apertados nessas janelas |
| 2 (300 s) | Como estava; usuário apertou os três | Só `botao down: apertou / soltou / clique` (11,4 s). CIMA e OK sem nenhuma linha |
| 3 (300 s) | Usuário desligou o USB e realinhou os jumpers dos botões | **Os três botões registrados** |

Saída da tentativa 3 (linhas periódicas do PN532 e do CC1101 omitidas):

```
[   31163][I][app] botao down: apertou
[   31418][I][app] botao down: soltou
[   31418][I][app] botao down: clique
[   32358][I][app] botao up: apertou
[   32618][I][app] botao up: soltou
[   32618][I][app] botao up: clique
[   33403][I][app] botao a: apertou
[   33648][I][app] botao a: soltou
[   33648][I][app] botao a: clique
```

Leitura: o firmware próprio lê os três botões (GPIO 39, 2 e 47, com pull-up interno). Cada
toque curto gerou um único apertou/soltou/clique, sem leitura duplicada. Os três pinos passam
pela mesma rotina (`GpioButtonSource`, `firmware/app/src/main.cpp`), o que apontava a falha da
tentativa 2 para a fiação; ela sumiu depois do realinhamento. Qual jumper ou furo estava errado
não foi registrado.

Limites desta evidência: um toque curto por botão, numa única rodada; toque longo e repetição
não testados; continuidade com o multímetro não registrada.

Rádio, na captura de 240 s, com o CC1101 desligado: aos 197 s o firmware leu
`VERSION 0x0B fora das documentadas (0x04, 0x14)`, passou a `radio: Failed (IoError)` e voltou a
`Absent` 15 s depois. É a mesma imprecisão já registrada com 0xE0 (MISO solto); não corrigida.

### Trilho da protoboard grande e receptor de IR (2026-11-21)

Montagem conforme [`../hardware/BREADBOARD_MAP.md`](../hardware/BREADBOARD_MAP.md) v0.2: fase A
(VCC e GND da tela direto na DevKitC; trilho (+) no J1 pino 2; trilho (−) no J3 pino 1; LED da
tela no trilho) e fase C (receptor de IR com OUT no GPIO 4, transmissor com DAT no GPIO 5, os
dois alimentados pelo trilho); CC1101 desligado. Firmware `dbmo-bench` já gravado (seção
anterior). Console pelo mesmo script pyserial, com `ir last` enviado depois de cada tentativa.

| Etapa | Observação | Leitura |
|---|---|---|
| Primeira energização com o IR | Boot e `status` iguais aos da montagem mínima; depois de uma tecla do controle, `ir last` → `nada recebido` | — |
| R1: VCC contra GND, nas pernas do receptor (USB ligado) | **0 V** | Receptor sem alimentação |
| GND do receptor contra GND da DevKitC | **2,5 V** (sinal da leitura não registrado) | O GND do receptor não chegava ao GND da placa |
| Troca dos jumpers de VCC (amarelo) e de GND (marrom) do receptor | `ir last` → `nada recebido` | Não resolveu |
| Usuário encontrou o fio A2 (J3 pino 1 → trilho (−)) **não ligado** e o ligou | — | O trilho (−) estava sem GND |
| Nova tentativa com o controle | `ir last` → **`nec:0004:17`** | Receptor recebeu e decodificou um quadro NEC |

Leitura: o receptor de IR funciona com o firmware próprio no GPIO 4. É a primeira execução em
hardware do driver de recepção (`RmtIrReceiver`, `firmware/platform/esp32s3/`) e do
decodificador NEC. A hipótese de que este receptor tivesse sido danificado pela ligação errada do
GND na v0.1 fica descartada. A causa da falha nesta montagem foi o fio A2 ausente; não se sabe se
os jumpers trocados tinham algum defeito.

Limites desta evidência: um controle (não identificado), uma tecla, uma leitura; R1 não foi
medido de novo depois da correção; conferências M1 a M5 do mapa não registradas, então não se
sabe se o trilho de cima é dividido; mudança do rosto na tela não confirmada; transmissor ligado
mas não testado (envio bloqueado, `infrared.tx_enabled` = 0). Numa das capturas, o log registrou
`botao up` e `botao a` (apertou, soltou, clique) aos 126–127 s sem confirmação de que os botões
foram apertados. Em outra, com o CC1101 desligado, o rádio relatou `CC1101 nao respondeu: Timeout`
em vez de `HardwareAbsent`: a mesma leitura de MISO solto já registrada.

### CC1101 sozinho no SPI (2026-11-21)

Passo anterior à fase D do mapa, para tirar a tela do caminho: os fios de SPI da tela (A6–A8)
saíram da DevKitC e o CC1101 (Ebyte E07-M1101D, 433M, antena rosqueada) foi ligado direto, com
jumpers fêmea: pino 1 GND e pino 2 VCC no trilho, 3 GDO0 → 15, 4 CSN → 14, 5 SCK → 12,
6 MOSI → 11, 7 MISO → 13; pino 8 sem ligação. IR e botões continuaram ligados. Desenho:
artifact "Ligação do CC1101" (https://claude.ai/artifact/KKHCduhNsNV6nBJWRAmhbM, privado).

Medições do usuário (multímetro digital, faixa 20 V; continuidade na escala 200 Ω):

| # | Pontas | Leitura | Leitura do resultado |
|---|---|---|---|
| M5 | Trilho (+) contra trilho (−) | 3,24 V | Dentro da faixa do CC1101 (CC-01, CC-02) |
| V1 | Pino 2 contra pino 1 do módulo | 3,24 V | Alimentado, polaridade certa |
| V2 | Pino 3 (GDO0) contra pino 1 | 1,61 V | Média do clock de fábrica no GDO0 (CC-12): cristal oscilando; confirma que o pino 3 é o GDO0 |
| V3 | Pino 4 (CSN) contra pino 1 | 3,24 V | CSN chega ao módulo, em alto no repouso |
| S5, S6 | Pino 12 da DevKitC contra pino 5; pino 11 contra pino 6 | 0,8 Ω cada | Fios de SCK e MOSI contínuos |

| Tentativa | Resultado | Leitura |
|---|---|---|
| 1 | Boot: `radio: Absent (HardwareAbsent)`. `pio test -e dbmo-bench -f test_cc1101`: 1 de 4 (reset aprovado; identificação falhou com "VERSION 0x00 ou 0xFF"; configuração e recepção falharam; sono aprovado) | O fio do MISO (pino 7 → 13) **não estava ligado**, como o usuário encontrou ao medir S7. O GPIO 13 solto leu 0x00, e por isso a espera de "pronto" (MISO baixo) passou mesmo sem chip no barramento |
| 2 (MISO ligado) | `test_cc1101`: **4 de 4 aprovados**, 19,0 s. Firmware regravado; boot: `CC1101 PARTNUM=0x00 VERSION=0x14; transmissao desabilitada`, `radio: Ready (Ok)`; comando `radio`: `Idle, 433920000 Hz` | CC1101 identificado (valores de CC-13) |

Leitura: o CC1101 responde pelo driver do firmware próprio. A comunicação com a ligação acima
confirma, na prática, a função dos pinos 5 (SCK), 6 (MOSI) e 7 (MISO), que até aqui vinham só da
ficha da LCSC; o pino 8 (GDO2) continua sem verificação. Nada foi transmitido: o teste não
transmite e o firmware relata a transmissão desabilitada.

Limites desta evidência: a frequência do cristal **não** foi verificada. O teste de configuração
relê FREQ2 = 0x10, valor que o próprio firmware calcula supondo 26 MHz, e por isso só prova a
escrita e a leitura do registrador. O RSSI foi conferido só dentro da faixa plausível (−140 a 0);
o valor não foi registrado. A tela ficou sem SPI durante o teste: o firmware leu o ID `0xFFFF` e
depois `0x0000` e mesmo assim declarou `display: Ready` (imprecisão do firmware, não corrigida).
O barramento dividido com a tela (fase D do mapa) ainda não foi testado.

### Correções da bancada e menu, só no computador (2026-11-21)

Branch `claude/firmware-menu`, a partir de `claude/adoring-heisenberg-b9zyh3` (`7b208bd`), numa
sessão **sem a placa**: nada desta entrada foi executado em hardware. Itens do
[`../HANDOFF_FIRMWARE.md`](../HANDOFF_FIRMWARE.md): correções F1 a F5 dos problemas vistos na
bancada nesta data e o menu de 3 botões.

Ambiente: macOS, Apple clang 17.0.0, CMake 4.4.5 e Ninja 1.13 (instalados por `pip` num
ambiente isolado), PlatformIO com a plataforma de `platformio.ini`.

| Verificação | Comando | Resultado |
|---|---|---|
| Linha de base, antes das mudanças | `cmake -S . -B build/plain -G Ninja && cmake --build build/plain && ctest --test-dir build/plain` | 46 executáveis, 539 testes, todos aprovados |
| Testes com AddressSanitizer neste computador | `-DDBMO_SANITIZE=ON` | **Não executados:** o runtime do ASan do Apple clang 17 trava antes do `main()` neste macOS (amostra do processo parada em `__asan::InitializeShadowMemory`), já na linha de base. O ASan roda na CI (gcc + ASan/UBSan) |
| Testes com UndefinedBehaviorSanitizer | `cmake -S . -B build/ubsan -G Ninja -DCMAKE_CXX_FLAGS="-fsanitize=undefined -fno-sanitize-recover=undefined" -DCMAKE_C_FLAGS=-fsanitize=undefined -DCMAKE_EXE_LINKER_FLAGS=-fsanitize=undefined` e `ctest` | 48 executáveis, 571 testes, todos aprovados |
| Compilação do firmware | `pio run -e dbmo-bench` e `pio run -e dbmo-target` | Os dois perfis compilam. RAM 13,1 % e 13,4 %; flash 8,0 % e 8,8 % |
| Integridade | `tools/check_pinmap.py`, `check_links.py`, `check_repo.py`; formatação com clang-format 18.1.8 nos mesmos arquivos de `tools/check_format.sh` | Sem problemas (o `check_format.sh` usa `mapfile`, que o bash 3.2 do macOS não tem; a lista de arquivos foi passada ao clang-format à mão) |
| CI da branch | GitHub Actions, execução 44973165827, commit `9408729` | As 9 tarefas aprovadas, inclusive "Testes nativos (gcc + ASan/UBSan)" e "Firmware próprio (ESP32-S3)" (os dois perfis e os testes de bancada compilados, sem gravar) |

Testes novos e o que cada um mostra (todos no computador, contra simuladores):

| Item | Testes | O que foi conferido |
|---|---|---|
| F1, rádio com barramento solto | `test_floating_miso_with_odd_version_is_absent`, `test_documented_versions_are_accepted_without_the_write_check` (`test_cc1101`); `test_floating_miso_is_absent_and_never_failed`, `test_unknown_version_that_keeps_writes_is_accepted_with_a_warning` (`test_radio_subsystem`) | VERSION 0xE0 e 0x0B com um barramento que não guarda escrita: `Absent (HardwareAbsent)`, sem falha registrada. 0x14 e 0x04: aceitos. VERSION desconhecida que guarda a escrita em ADDR: aceita, com aviso no log. Os testes falharam antes da correção (`Failed` em vez de `Absent`) |
| F2, ID da tela | `test_controller_id_is_checked_without_blocking_the_screen`; `test_known_restriction_degrades_without_a_fault` (`test_system`); `test_display_with_unexpected_id_is_degraded_and_still_draws` (integração) | ID 0xFFFF ou 0x0000: aviso no log, `display Degraded IoError` no `status`, rosto desenhado, sem registro de falha e sem o rosto de erro. Outro ID: `Incompatible`. Tela sem ID: sem conferência |
| F3, IR no log | `test_received_infrared_code_is_written_to_the_log_once` (integração) | `infravermelho: nec:0004:17` no log; a repetição logo em seguida não entra; sinal desconhecido registra só o tamanho, uma vez por tecla segurada (9 quadros a cada 114 ms dão uma linha; outro tamanho logo em seguida conta como sinal novo) |
| F4, console | `test_keystrokes_one_by_one_like_the_default_monitor` | Tecla a tecla com CRLF, só CR e só LF executam o comando, com eco: o console não explica a falha de 2026-11-21 |
| F5, NVS | — | Código do porte: só compilado |
| Texto na tela | `test_text` (6 testes) | Fonte com 95 caracteres distintos, desenho na escala certa, uma chamada de `fillRect` por sequência de pixels, só as linhas alteradas são redesenhadas |
| Rosto escondido | `test_hidden_face_keeps_changing_but_does_not_draw` | Escondido, o rosto muda de expressão e não desenha; ao voltar, repinta a tela |
| Lógica do menu | `test_menu` (11 testes) | Abrir, navegar com volta ao início, CIMA e BAIXO segurados sem passar do item ao soltar, itens ausentes pulados, entrar e voltar (toque longo no OK e B), ações só de recepção, histórico de botões, volta ao rosto por inatividade |
| Menu na aplicação | 5 cenários de integração `test_menu_*` e `test_idle_menu_*` | Aprender um código pelo menu, Estado com `ausente` e sem os itens de rádio e IR quando faltam, recepção ligada e desligada pelo menu sem nenhuma transmissão, telas Botoes e Sobre, nenhum redesenho com o menu parado, botão no menu conta como atividade, volta ao rosto depois de 30 s |

Revisão independente: quatro revisores por área (correções F1 a F5, menu, emissão e escopo,
porte para o ESP32-S3), cada achado seguido de um verificador que tentou refutá-lo com
reprodução no computador. Emissão e escopo: nenhum achado. Confirmados e corrigidos, com teste:
o cursor passava um item além ao soltar CIMA ou BAIXO segurados (o `ButtonProcessor` manda um
clique ao soltar depois das repetições); a F2 usava `reportFault`, o que punha um erro no log,
um registro de falha e o rosto de erro por 2,5 s a cada boot sem o SDO da tela (passou a usar
o novo `System::reportDegraded`, só aviso); um sinal de IR de protocolo desconhecido com a
tecla segurada gerava uma linha a cada ~110 ms. Refutado: um MISO da tela solto lendo IDs
mistos (rotulados como "controlador diferente"); a bancada só registrou 0xFFFF e 0x0000. Uma
segunda leitura das correções achou que a supressão do IR desconhecido engolia outra tecla
apertada em menos de 200 ms; corrigido (tamanho diferente conta como sinal novo), com teste.

Duas mutações conferiram que os testes do menu pegam erros: sem esconder o rosto ao abrir o
menu, dois cenários falham; repintando a tela inteira a cada `render()`, falham o cenário de
redesenho e o teste da `TextScreen`. As telas do menu também foram renderizadas num
`FramebufferDisplay` e conferidas visualmente (imagem fora do repositório).

Hipóteses da F4 para a bancada, sem confirmação: o firmware recebe os mesmos bytes nos dois
modos do monitor, então a diferença deve estar no caminho USB ou no terminal (por exemplo,
teclas digitadas durante o boot que a abertura da porta provoca). Como contorno, o
`platformio.ini` passou a usar a combinação que funcionou.

Não testado na placa (pendente de bancada): tudo desta entrada. Em particular, o desenho e a
legibilidade do menu na ILI9341, os botões navegando o menu, a leitura real do ID da tela
pelo porte, a identificação do CC1101 com VERSION fora de 0x14 e 0x04, o aprendizado de IR
pelo menu, a recepção do rádio ligada pelo menu, a ausência do `nvs_open failed` no primeiro
boot e o `pio device monitor` com as opções novas.

### Menu na placa, SPI dividido e tela branca por inatividade (2026-11-22)

**Montagem:** fase D do [mapa v0.2](../hardware/BREADBOARD_MAP.md) completa, com a tela e o
CC1101 dividindo SCK, MOSI e MISO pelos nós das colunas 8, 11 e 14 (D1 a D13), mais
alimentação (A), botões (B) e IR (C). A tela tinha sido tirada inteira ao fim de 21/11 e foi
religada nesta data. O M2 foi relatado como "zerado", sem o valor do visor; as conferências
NÓ não foram relatadas.

**Firmware:** `claude/firmware-menu` (`f508cb6`), gravado com
`pio run -e dbmo-bench -t upload --upload-port /dev/cu.usbmodem1101`. O console foi lido por
um script pyserial (DTR e RTS em 0; linhas com CRLF); abrir a porta reinicia a placa.

| Etapa | Comando ou ação | Saída | Leitura |
|---|---|---|---|
| Boot, firmware anterior (`7b208bd`) | Abrir a porta e `status` | `tela: id 0x9341, 320x240`; `CC1101 PARTNUM=0x00 VERSION=0x14`; `display Ready Ok`; `radio Ready Ok`; `nvs_open failed: NOT_FOUND` 2 vezes; `sdCommand(): Card Failed!` 3 vezes | Tela e CC1101 respondem no barramento dividido |
| Boot, `f508cb6` | `status` e `radio` | `controlador 0x9341`; `display: Ready (Ok)`; `CC1101 PARTNUM=0x00 VERSION=0x14`; `radio: Idle, 433920000 Hz; pacotes 0, crc 0, negados 0`; sem `nvs_open failed`; sem `sdCommand(): Card Failed!` (restam 2 `f_mount failed`) | F5 confirmada neste boot |
| Menu | Roteiro de 9 passos com os 3 botões, relatado pelo Gabriel | Rosto; OK abre; CIMA e BAIXO andam e repetem ao segurar; Estado; Infravermelho; Radio; Botoes; Sobre; volta ao rosto depois de 30 s parado: todos relatados como funcionando | O log mostra `botao up/down/a` com `apertou`, `soltou`, `clique` e `toque longo` |
| Rádio pelo menu | OK no item Radio | `recepcao do radio ligada: Ok` e `recepcao do radio desligada: Ok`, 8 vezes alternadas | Só recepção; o menu não tem envio |
| IR pelo menu e no log | OK no item Infravermelho (aprender) e teclas de um controle | `aprender infravermelho: Ok` 4 vezes; `infravermelho: nec:0004:44`, `:41`, `:07`, `:06`, `:40`, `:14`; 3 `sinal desconhecido` (67, 21 e 11 durações) | F3: o código entra no log sem `ir last` |

**Tela branca.** Depois de uns 2 minutos sem botão, a tela ficou toda branca e o rosto sumiu;
CIMA trouxe o rosto de volta. Causa no código: `display.off_after_s` (120 s) levava a tela ao
estado `Off`, que chama `setSleep(true)` (`ILI9341_DISPOFF` e `ILI9341_SLPIN`) mesmo sem
controle do backlight. Com o LED no trilho de 3,3 V aceso, o painel em repouso fica branco. O
comportamento existia desde 2026-11-12 e um teste o exigia.

Correção `6073bcf`: sem controle do backlight, o `DisplaySubsystem::tick` não esmaece nem
apaga a tela. O teste `test_uncontrolled_backlight_never_dims_or_sleeps` falhou antes da
correção (linha 150) e passou depois. Testes com UBSan no computador: 48 executáveis, todos
aprovados (ASan só na CI). `check_pinmap.py`, `check_links.py` e `check_repo.py` sem
problemas. Na placa, gravado o `6073bcf`: 170 s sem nenhum botão no log e o rosto continuou na
tela.

Nesse boot a leitura do ID deu `ID do controlador 0x0000, esperado 0x9341 (MISO da tela sem
resposta)` e o `status` passou a `display: Degraded (IoError)`, com o rosto desenhado: é o
comportamento esperado da F2, visto por acaso. Nos 3 boots anteriores, com a mesma montagem, o
ID foi `0x9341`.

Limites da evidência:

- F1 (rádio com o barramento sem chip) não foi exercitada: o CC1101 respondeu em todos os
  boots. O teste tirando o D13 foi adiado.
- A leitura do ID da tela variou entre boots (`0x9341` e `0x0000`). Hipótese: mau contato no
  D3 ou no D6, ou disputa no MISO dividido. Não verificado.
- F4: os comandos foram só pelo script; o `pio device monitor` com as opções novas não foi
  testado digitando à mão.
- Cada código de IR apareceu de 2 a 4 vezes, com uns 0,5 s entre eles: pelo log não dá para
  separar toques seguidos de repetições. Os 3 `sinal desconhecido` não foram investigados.
- A legibilidade do texto na tela foi relatada como boa, sem foto.
- O NFC registra `PN532 nao respondeu: HardwareAbsent` e `nfc: hardware ausente` a cada 15 s
  (ruído no log, não corrigido). Segurar um botão repete na tela, mas o log não registra as
  repetições.

### Microfone INMP441 no perfil bench (2026-11-22)

**Montagem:** fase E do [mapa v0.3](../hardware/BREADBOARD_MAP.md), sobre as fases A a D. O
INMP441 fica fora da protoboard, ligado por jumpers: VDD no trilho (+), GND e L/R no trilho
(−), SCK no GPIO 40, WS no 41 e SD no 7. Pinagem pela serigrafia da placa (fotos de 22/11):
L/R, WS e SCK de um lado; GND, VDD e SD do outro. As conferências MIC e MV e o valor de L/R
contra GND não foram relatados.

**Firmware:** `d33f036` (comando `mic`), gravado com
`pio run -e dbmo-bench -t upload --upload-port /dev/cu.usbmodem1101`; console pelo script
pyserial. O microfone foi habilitado em cada sessão com `config audio.mic_enabled 1`, sem
`save`. **Correção em 2026-11-25:** o texto original dizia que, sem `save`, a configuração não
ficou gravada. Está errado: toda chave alterada é gravada sozinha 2 s depois
(`kSaveDelayMs`), e o microfone ficou habilitado na memória interna (ver a entrada de
2026-11-25).

| Etapa | Comando | Saída | Leitura |
|---|---|---|---|
| Sem o microfone ligado | `mic 500`, duas vezes | `gravacao encerrada: 6320 amostras, pico 0` | Base: com o SD solto, só zeros |
| Com o microfone, em silêncio | `mic 1000`, duas vezes | `14240 amostras, pico 21277` e `pico 17773` | Chegam dados pelo SD |
| Estatística por trecho, em silêncio | `mic 1000` e `mic last`, 3 vezes | Na 1ª: `media por 100 ms: 11754 2832 -1493 -3085 -3432 -3353 -2867 -2445 -1931`; `rms por 100 ms: 3584 1780 855 285 184 195 212 198 217`. As outras duas, com o mesmo formato | Cada gravação começa com um desvio lento, igual em todas |
| Palmas | `mic 1000` e `mic last` a cada 2,2 s por 2 minutos; o Gabriel bateu 5 a 10 palmas | Na gravação dos 82 s, pico (já sem a média) de `35359`, `35177` e `35089` nos trechos 3, 6 e 9 de 100 ms, com `min -32638; max 32767`. Nas gravações sem palma, os trechos do meio ficaram entre 3000 e 5000 nessa medida | O microfone responde ao som até o fundo de escala |

**Leitura.** O INMP441 funciona: sem ele o firmware lê só zeros, com ele chegam dados, e as
palmas levam o sinal ao fundo de escala. Cada gravação, porém, começa com um desvio lento que
não é som: a média por trecho de 100 ms vai de cerca de +10000 a −5500 e volta a −2000 ao fim de
1 s, e o RMS dentro do trecho só cai para cerca de 200 depois de 300 ms. O desvio se repete
quase igual de uma gravação para outra.

**Hipótese, não verificada:** entre gravações o firmware desliga a entrada I2S (escolha de
privacidade do subsistema de áudio), o clock para e o microfone entra em espera (MIC-09). Ao
voltar, a saída dele leva mais de 1 s para assentar, e o firmware descarta só os primeiros
100 ms (`kMicStartupMs`). A causa não foi confirmada; o efeito foi corrigido em seguida pelo filtro
abaixo.

**Correção `8f1ffd0`: passa-altas na gravação.** Filtro de 2ª ordem (Butterworth) com corte em
`audio.mic_highpass_hz` = 60 Hz, o canto inferior do próprio INMP441; o estado do filtro começa
como se o sinal já estivesse parado no primeiro valor. No computador: um desvio de 2 Hz sobre DC
de 10000 sai abaixo de 50 depois de 50 ms, e um tom de 1 kHz sobre DC de 5000 passa com o pico
entre 7800 e 8200 (de 8000). Testes com UBSan: 48 executáveis aprovados; o perfil `dbmo-target`
compila. Na placa, em silêncio, 3 gravações de 1 s:

| Medida | Antes do filtro | Com o filtro |
|---|---|---|
| Pico no log (`gravacao encerrada`) | 17773 a 21277 | 279, 406 e 431 |
| Média por trecho de 100 ms | de +11754 a −5500 | de −7 a +3 |
| RMS por trecho de 100 ms | de 4280 (1º trecho) a cerca de 200 | de 50 a 92 |

Limites da evidência:

- As palmas aparecem nítidas em uma gravação só: cada gravação cobre cerca de 45 % do tempo,
  e não se sabe quando cada palma foi batida.
- O RMS de cerca de 200 no fim inclui a parte final do desvio lento: não é o ruído próprio do
  microfone.
- Nada foi ouvido: não há alto-falante ligado. Taxa de 16 kHz, palavras de 32 bits, só o canal
  esquerdo.
- Os pinos I2S do perfil `bench` continuam como `Proposed` em `pins.h`.
- As palmas não foram repetidas com o filtro: a passagem de som na faixa da voz foi conferida só
  no computador (tom de 1 kHz).

### Testes no computador (2026-11-12)

Comando:

```bash
cmake -S . -B build/lead -G Ninja -DDBMO_SANITIZE=ON && cmake --build build/lead && ctest --test-dir build/lead
```

Ambiente: g++ 13.3.0, CMake 3.28.3, Ninja, AddressSanitizer + UndefinedBehaviorSanitizer.

Resultado: **46 executáveis de teste, 538 testes, 0 falhas, 0 ignorados** (contagem final, depois das correções da revisão adversarial; a tabela por área abaixo é a da medição anterior, com 525).

| Área | Executáveis | Testes |
|---|---:|---:|
| `core` (HAL, eventos, log, configuração, sistema, mocks) | 11 | 82 |
| `config` (mapa de pinos) | 1 | 10 |
| `app` (console) | 1 | 10 |
| `display` | 2 | 17 |
| `rosto` | 4 | 41 |
| `input` | 4 | 45 |
| `cartridges` | 3 | 33 |
| `power` | 2 | 30 |
| `storage` | 4 | 44 |
| `radio` | 4 | 61 |
| `nfc` | 3 | 44 |
| `infrared` | 3 | 48 |
| `audio` | 3 | 39 |
| Integração (`tests/integration`) | 1 | 21 |
| **Total** | **46** | **525** |

### Cobertura de linhas (2026-11-12)

Medida com `-DDBMO_COVERAGE=ON` (gcc `--coverage`, sem otimização) e `gcov`, somando todos
os executáveis de teste. Conta o firmware portável: 102 arquivos de `firmware/`. Ficam de
fora, porque não compilam no computador, `firmware/app/src/main.cpp` e
`firmware/platform/esp32s3`; e, porque não são executados pelos testes, os utilitários de
`firmware/*/tools`.

| Módulo | Linhas executadas / executáveis | Cobertura |
|---|---:|---:|
| `core` | 718 / 739 | 97,2 % |
| `config` | 6 / 6 | 100 % |
| `display` | 139 / 140 | 99,3 % |
| `rosto` | 208 / 208 | 100 % |
| `input` | 440 / 445 | 98,9 % |
| `audio` | 245 / 245 | 100 % |
| `radio` | 517 / 542 | 95,4 % |
| `nfc` | 302 / 311 | 97,1 % |
| `infrared` | 395 / 406 | 97,3 % |
| `storage` | 267 / 271 | 98,5 % |
| `power` | 276 / 296 | 93,2 % |
| `cartridges` | 353 / 365 | 96,7 % |
| `app` | 477 / 570 | 83,7 % |
| **Total** | **4343 / 4544** | **95,6 %** |

Menos cobertos: `app/null_hal.h` (16 %: os objetos "ausentes" só têm os métodos chamados
no boot exercitados), `app_commands.cpp` (66 %: nem todo ramo de cada comando do console é
testado) e as funções `toString` de vários módulos. Cobertura de linhas não mede a
qualidade das asserções nem substitui o teste em hardware.

### GitHub Actions

Execução nº 20 do fluxo `CI`, commit `79cee67` (ramo `claude/dbmo-v1-firmware-dev-96rxin`),
em 2026-11-12: **as nove tarefas aprovadas**.

| Tarefa | Resultado | O que prova |
|---|---|---|
| Testes nativos (gcc + ASan/UBSan) | Aprovado | Os 46 executáveis de teste passam |
| Testes nativos (clang, sem sanitizers) | Aprovado | Idem, com outro compilador |
| Integridade do repositório | Aprovado | Mapa de pinos coerente entre `pins.h`, Bruce, Retro-Go e documentação; links; portabilidade; formatação |
| Firmware próprio (ESP32-S3) | Aprovado | Compila nos perfis `dbmo-bench` e `dbmo-target`; os cinco testes de bancada compilam (sem gravar) |
| Bruce para o d-BMO (`bmo-v1`) | Aprovado | Bruce 1.16.1 compila com a placa do d-BMO (`bmo-v1 SUCCESS` no log do PlatformIO, 8 min 36 s) |
| Bruce para o d-BMO (`bmo-v1-mcp`) | Aprovado | Idem, com os botões no MCP23017 |
| Retro-Go, conferência da configuração | Aprovado | Sintaxe do alvo `dbmo-v1` em todas as combinações de opções |
| Retro-Go para o d-BMO (launcher) | Aprovado | O launcher compila com o ESP-IDF 4.4.8 para o alvo `dbmo-v1` (815 passos; `launcher.bin` gerado para ESP32-S3) |
| CAD paramétrico | Aprovado | Autoteste do gerador de modelos com as versões registradas do cadquery |

Foi a **primeira vez** que o Bruce e o Retro-Go foram compilados para o d-BMO. Compilar não
é funcionar: nenhum dos três firmwares foi gravado na placa nesta etapa. Do Retro-Go só o
launcher foi compilado; os emuladores não.

Histórico relevante das execuções anteriores (falhas reais, corrigidas, não escondidas):
a plataforma exigia PlatformIO 6.1.19; a biblioteca `display` não tinha manifesto; o link
para o guia de montagem apontava para um arquivo que ainda não existia.

Bugs encontrados pelos próprios testes durante o desenvolvimento e corrigidos antes do
commit: perda de evento ao cancelar inscrição durante a entrega (`EventBus`); origem de
botões que oscilava entre falha e recuperação (`InputSubsystem`); remontagem imediata e
repetida de um cartão que não grava (`StorageSubsystem`); expressão de fundo do rosto que
não acompanhava o aprendizado de infravermelho (`Application`).

### Verificações de integridade (2026-11-12, no computador)

| Verificação | Resultado |
|---|---|
| `tools/check_pinmap.py` | 31 sinais no perfil `target`, 19 no `bench`; 28 flags do Bruce, 17 macros do Retro-Go e 2 blocos gerados conferidos |
| `tools/check_links.py` | 34 arquivos Markdown, 0 links quebrados |
| `tools/check_repo.py` | 12 subsistemas, fontes portáveis e scripts conferidos; 0 problemas |
| `tools/check_format.sh` | Sem diferenças (clang-format 18.1.3) |

### Revisão adversarial independente (2026-11-12)

Cinco revisores por área, cada um seguido de um verificador que tenta refutar ou reproduzir
os achados. **A revisão ficou incompleta:** o limite de gasto da conta interrompeu a
execução. Resultado por área:

| Área | Revisão | Verificação independente |
|---|---|---|
| Emissão (rádio e infravermelho) | Concluída: 8 achados, com testes de reprodução do próprio revisor | Não executada |
| Entrada não confiável (NFC, cartuchos, caminhos, códigos IR) | Concluída: 8 achados | Não executada |
| Energia e dados | **Não executada** | — |
| Porte ESP32-S3 | **Não executada** | — |
| Núcleo e interface | **Não executada** | — |

Os 16 achados foram conferidos pelo autor contra o código e tratados assim:

| Achado | Tratamento |
|---|---|
| Registradores de canal e de deslocamento do CC1101 fora da lista protegida (portadora fora da faixa autorizada) | Corrigido, com teste |
| Reconfiguração que falha deixava o rádio "configurado" com registradores misturados | Corrigido, com teste |
| Autorização de transmissão reutilizável e válida para outra configuração | Corrigido (uso único; amarrada a frequência, banda e tempo no ar), com teste |
| Troca de cristal ou de potência só em memória, sem regravar o chip | Corrigido |
| Janela de tempo no ar do rádio permitia o dobro do orçamento em 60 s | Corrigido, com teste |
| OOK: tabela de potência perdida quando o chip dorme | Corrigido, com teste (o comportamento do chip real vem do datasheet, de memória: validar na bancada) |
| Orçamento de infravermelho em janela fixa permitia o dobro na virada do minuto | Corrigido, com teste |
| NEC estendido com byte alto zero era reenviado com outro endereço | Corrigido (`necx:` no formato de texto), com teste |
| `stopScan()` do NFC podia deixar o campo de RF ligado | Corrigido, com teste |
| PN532 que para de confirmar comandos nunca chegava a `Fault` | Corrigido, com teste |
| Gravação atômica sem cópia de segurança podia perder a versão antiga e a nova | Corrigido, com teste |
| Cartucho ficava em `Unknown` para sempre depois de uma leitura ruim na inserção | Corrigido; **sem teste novo** |
| Cabeçalho de cartucho aceito com fornecedor zero ou bytes reservados não nulos | Corrigido, com teste |
| Remoção de cartucho ativo relatada como falha do driver | Corrigido; **sem teste novo** |
| Recuperação do NFC em `Fault` espera dentro de um `tick()` (até ~0,5 s a cada 5 s) | **Não corrigido**: limitação registrada em `firmware/nfc/README.md` |
| (duplicado do NEC estendido, relatado pelas duas áreas) | — |

Três das cinco áreas não foram revisadas por um segundo par de olhos, e nenhuma correção
passou por verificação independente. Isso deve ser refeito antes de confiar no firmware.

### O que NÃO foi executado

| Item | Situação |
|---|---|
| Firmware próprio na placa | **Executado** de 21 a 25/11/2026 (entradas acima): tela, botões, menu, IR (recepção), CC1101 (identificação e recepção), microfone e SD. Ainda não: transmissão de IR, alto-falante, sono profundo e despertar, F1 com o barramento sem chip, F4 com o monitor digitando à mão |
| Testes de `tests/hardware/` | `test_display` e `test_cc1101` rodaram na placa em 21/11 (4/4 cada); os demais não |
| Bruce e Retro-Go na placa | Retro-Go **gravado e executado** em 25/11/2026 (imagem de bancada: launcher, SD e um jogo de Game Boy Color). Bruce gravado e usado antes do firmware próprio (ver `README.md`), não nesta série |
| Emuladores do Retro-Go e imagem completa (`build-img`) | Compilados na CI no perfil de bancada; na placa, só o launcher e o emulador de Game Boy Color foram usados (25/11/2026). Os outros emuladores nunca rodaram |
| Testes com clang + sanitizers | Não executado no computador de desenvolvimento (falta a biblioteca de runtime); a CI roda clang sem sanitizers |
| Medições elétricas, de consumo e de RF | Só tensões de bancada (trilho de 3,3 V e pinos do CC1101, 21/11/2026). Consumo e RF: não executados |
| Cobertura de ramos | Não medida (só a de linhas, acima) |
