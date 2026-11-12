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

### Testes no computador (2026-11-12)

Comando:

```bash
cmake -S . -B build/lead -G Ninja -DDBMO_SANITIZE=ON && cmake --build build/lead && ctest --test-dir build/lead
```

Ambiente: g++ 13.3.0, CMake 3.28.3, Ninja, AddressSanitizer + UndefinedBehaviorSanitizer.

Resultado: **46 executáveis de teste, 525 testes, 0 falhas, 0 ignorados.**

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

### O que NÃO foi executado

| Item | Situação |
|---|---|
| Qualquer teste em hardware (`tests/hardware/`, firmware próprio na placa) | Não executado: sem acesso à bancada. A CI só prova que compila |
| Bruce e Retro-Go na placa | Compilados na CI; **nunca gravados nem executados** |
| Emuladores do Retro-Go e imagem completa (`build-img`) | Não compilados; só o launcher |
| Testes com clang + sanitizers | Não executado no computador de desenvolvimento (falta a biblioteca de runtime); a CI roda clang sem sanitizers |
| Medições elétricas, de consumo e de RF | Não executadas |
| Cobertura de ramos | Não medida (só a de linhas, acima) |
