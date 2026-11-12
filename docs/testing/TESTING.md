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

Resultado: **46 executáveis de teste, 522 testes, 0 falhas, 0 ignorados.**

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
| `radio` | 4 | 59 |
| `nfc` | 3 | 43 |
| `infrared` | 3 | 48 |
| `audio` | 3 | 39 |
| Integração (`tests/integration`) | 1 | 21 |
| **Total** | **46** | **522** |

### GitHub Actions

Execução nº 16 do fluxo `CI`, commit `5cd8b84` (ramo `claude/dbmo-v1-firmware-dev-96rxin`):

| Tarefa | Resultado |
|---|---|
| Testes nativos (gcc + ASan/UBSan) | Aprovado |
| Testes nativos (clang, sem sanitizers) | Aprovado |
| Firmware próprio (ESP32-S3): compila `dbmo-bench` e `dbmo-target`; compila os testes de bancada sem gravar | Aprovado |
| Integridade do repositório | **Reprovado** naquela execução: link para `ASSEMBLY_GUIDE.md`, que ainda não existia. O guia foi escrito no commit seguinte; conferir a execução mais recente no histórico do ramo |

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
| Bruce para o d-BMO (`firmware/bruce/build.sh`) | Nunca compilado, nem aqui nem na CI |
| Retro-Go para o d-BMO (`firmware/retro-go/build.sh`) | Nunca compilado com o ESP-IDF; só a verificação de sintaxe da configuração no computador |
| Testes com clang + sanitizers | Não executado no computador de desenvolvimento (falta a biblioteca de runtime); a CI roda clang sem sanitizers |
| Medições elétricas, de consumo e de RF | Não executadas |
| Cobertura de código | Não medida |
