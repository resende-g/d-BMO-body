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
| Qualquer teste em hardware (`tests/hardware/`, firmware próprio na placa) | Não executado: sem acesso à bancada. A CI só prova que compila |
| Bruce e Retro-Go na placa | Compilados na CI; **nunca gravados nem executados** |
| Emuladores do Retro-Go e imagem completa (`build-img`) | Não compilados; só o launcher |
| Testes com clang + sanitizers | Não executado no computador de desenvolvimento (falta a biblioteca de runtime); a CI roda clang sem sanitizers |
| Medições elétricas, de consumo e de RF | Não executadas |
| Cobertura de ramos | Não medida (só a de linhas, acima) |
