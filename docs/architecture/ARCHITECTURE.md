# Arquitetura do firmware próprio do d-BMO v1

| Campo | Valor |
|---|---|
| Versão | 0.1 (2026-11-12) |
| Código | `firmware/` |
| Estado | Implementado e testado no computador; compila para o ESP32-S3 na CI; **não testado em hardware** |

## 1. Princípios

| Princípio do projeto | Como aparece no código |
|---|---|
| Local-first, sem nuvem | Nenhum módulo usa rede. `tools/check_repo.py` reprova inclusão de cabeçalhos de rede nas bibliotecas portáveis |
| Privacidade por projeto | Microfone, busca de NFC e transmissões vêm desligados; UID de etiqueta e gravação de áudio não vão para log nem para arquivo |
| Segurança por projeto | Transmissão de rádio e de infravermelho só com habilitação explícita e com orçamento de tempo; dados do cartão e do cartucho são validados como entrada não confiável |
| Modularidade | Um subsistema por pasta, com a mesma interface (`ISubsystem`); módulos conversam por eventos |
| Baixo consumo de recursos | Sem alocação dinâmica, sem exceções, sem RTTI; buffers de tamanho fixo; nada bloqueia o laço principal |
| Código auditável | Lógica separada do hardware e coberta por testes que rodam em qualquer computador |
| Sem telemetria | Não existe código de envio de dados |

## 2. Camadas

```
┌──────────────────────────────────────────────────────────────────────────┐
│ firmware/app            Application: cria e liga os subsistemas; console │
├──────────────────────────────────────────────────────────────────────────┤
│ firmware/<subsistema>   rosto · display · input · audio · radio · nfc ·  │
│                         infrared · storage · power · cartridges          │
├──────────────────────────────────────────────────────────────────────────┤
│ firmware/core           Status, HAL (interfaces), eventos, log, sistema, │
│                         configuração, registro de recursos               │
│ firmware/config         pins.h: mapa de pinos e endereços                │
├──────────────────────────────────────────────────────────────────────────┤
│ firmware/platform/esp32s3   implementação da HAL sobre o Arduino-ESP32   │
│ tests/mocks, tests/unit/*/sim   implementação da HAL para os testes      │
└──────────────────────────────────────────────────────────────────────────┘
```

Regras de dependência (a regra 3 é conferida automaticamente por `tools/check_repo.py`; as
demais valem por construção das dependências no CMake):

1. `core` e `config` não dependem de ninguém.
2. Um subsistema depende de `core`, de `config` e, só quando faz sentido, de outro
   subsistema por interface (`rosto` usa `display::IDisplay`).
3. **Só `firmware/platform/esp32s3` e `firmware/app/src/main.cpp` incluem cabeçalhos do
   Arduino.** Todo o resto compila com um compilador C++17 comum.
4. Subsistemas não se chamam diretamente: publicam e assinam eventos. Quem conhece todos é
   a `Application`.

Fora desta arquitetura, por serem projetos de terceiros com configuração do d-BMO:
`firmware/bruce` (AGPL-3.0) e `firmware/retro-go` (GPL-2.0). Eles compartilham com o
firmware próprio só o mapa de pinos, conferido por `tools/check_pinmap.py`.

## 3. Núcleo (`firmware/core`)

| Peça | Papel |
|---|---|
| `Status`, `Result<T>`, `DBMO_TRY` | Tratamento de erros sem exceções: toda falha volta como código |
| HAL (`IClock`, `IGpio`, `ISpiDevice`, `II2cBus`, `IAdcChannel`, `IPwmOutput`, `ICriticalSection`) | Contratos de hardware; o resto do firmware só conhece estas interfaces |
| `Logger`, `ILogSink`, `LogHistory` | Registro de eventos com vários destinos (serial, memória, arquivo) |
| `EventBus` | Fila de eventos assíncrona; comunicação entre módulos |
| `StateMachine` | Máquina de estados dirigida por tabela |
| `System` | Inicialização ordenada, estado global, falhas, tentativas, desligamento |
| `ConfigStore`, `IConfigBackend` | Configuração tipada, com faixa e padrão por chave |
| `ResourceRegistry` | Quem ocupa cada pino e cada endereço; detecta conflito |

### 3.1 Estado global

```
Off ─boot()─► Booting ─► Running ◄──► Degraded ──► Fault
                            │  ▲           │
                            ▼  │           │
                         LowPower          │
        (qualquer estado em operação) ─shutdown()─► ShuttingDown ─► Off
```

| Estado | Significado |
|---|---|
| `Running` | Todos os subsistemas prontos |
| `Degraded` | Falta ou falhou um subsistema **opcional**: o aparelho segue funcionando |
| `Fault` | Falhou um subsistema **obrigatório** (tela ou rosto) |

Cada subsistema tem o seu estado: `Ready`, `Degraded`, `Absent` (o hardware não
respondeu), `Failed`, `Stopped`. O comando `status` do console mostra todos.

### 3.2 Tratamento de falhas

| Situação | Comportamento |
|---|---|
| Periférico ausente no boot | O subsistema devolve `HardwareAbsent`; o sistema fica em `Degraded` e segue |
| Periférico para de responder em uso | O subsistema conta erros seguidos, publica um evento de falha **uma vez**, e tenta de novo em intervalos |
| Periférico ligado depois do boot | NFC e rádio são tentados de novo a cada 15 s; o cartão, só a pedido (`sd`), porque montar um cartão ausente pode travar por mais de um segundo |
| Dado inválido de fora (cartão, EEPROM de cartucho, quadro do PN532, pacote de rádio) | Validado campo a campo; recusado sem afetar o resto |
| Driver de cartucho que falha | Parado; o conector fica em `Faulted`; os recursos reservados são liberados |

## 4. Aplicação (`firmware/app`)

`Application` recebe uma estrutura `Hardware` com ponteiros para as implementações de HAL
que existem na montagem. O que falta é substituído por um objeto "ausente", e o subsistema
correspondente aparece como `Absent`, sem caso especial.

### 4.1 Ordem de inicialização

| # | Subsistema | Obrigatório | Motivo da posição |
|---:|---|---|---|
| 1 | `power` | não | Segura a trava de alimentação antes de qualquer coisa demorada |
| 2 | `storage` | não | O cartão vem antes da configuração |
| 3 | `config` | não | Carrega a configuração (do cartão, ou da memória interna) antes de quem a lê |
| 4 | `display` | **sim** | |
| 5 | `rosto` | **sim** (depende de `display`) | |
| 6–11 | `input`, `audio`, `infrared`, `nfc`, `radio`, `cartridges` | não | |

O desligamento percorre a ordem inversa.

### 4.2 Reações entre módulos

| Evento | Reação |
|---|---|
| Qualquer evento de controles | Reinicia os temporizadores de tela e de inatividade |
| Etiqueta NFC detectada, pacote de rádio recebido, código IR recebido ou aprendido | Rosto SUCCESS por um tempo |
| Transmissão negada pela política (rádio ou IR) | Rosto ALERT |
| Bateria baixa / crítica | Rosto WARNING / ALERT |
| Cartucho ativo ou identificado / recusado ou em falha / sem identificação | Rosto HAPPY / ERROR / CURIOUS |
| Cartão retirado em uso | Rosto WARNING |
| Falha de subsistema | Rosto ERROR |
| Gravando áudio | Fundo LISTENING |
| Buscando NFC, recebendo rádio ou aprendendo IR | Fundo SCANNING |
| Desligamento pedido | Fundo SLEEP; encerra tudo em ordem |

### 4.3 Desligamento

1. Pedido: toque longo no botão, bateria crítica confirmada, inatividade ou comando.
2. A aplicação encerra os subsistemas na ordem inversa: a configuração e o log pendentes
   vão para o cartão, o rádio e o NFC dormem, o amplificador desliga.
3. A energia só é cortada depois que o botão é solto. Sem trava de alimentação, o chip
   entra em deep sleep com o botão como despertar.

## 5. Modelo de execução

- Um laço principal, cooperativo: `Application::loop()` → `System::tick()` → `tick()` de
  cada subsistema. **Nenhum `tick()` bloqueia.** Operações longas (comando do PN532,
  transmissão de rádio, reprodução de áudio) são máquinas de estado acompanhadas por
  consulta.
- As únicas esperas limitadas estão na inicialização (por exemplo, até 150 ms por comando
  do PN532).
- Eventos publicados durante um `tick()` são entregues no começo do próximo.
- O firmware não cria tarefas do FreeRTOS. A fila de eventos aceita publicação a partir de
  interrupção (com `ICriticalSection`), mas nenhum módulo usa interrupção hoje.

## 6. Testabilidade

| O que | Como é testado |
|---|---|
| Lógica de cada módulo | Testes de unidade contra mocks da HAL |
| Protocolo de cada chip | Simuladores no nível do barramento: MCP23017, 24C02, CC1101 (SPI), PN532 (I2C), cartão (sistema de arquivos em memória) |
| O conjunto | `tests/integration`: a `Application` inteira sobre os simuladores, 21 cenários |
| Compilação para o alvo | PlatformIO na CI, nos perfis de bancada e de alvo |
| Hardware real | `tests/hardware`: testes de bancada, **ainda não executados** |

Limite: um simulador prova que o firmware faz o que o autor entendeu do datasheet, não
que o chip real se comporta assim. Cada README de módulo lista o que foi conferido em
fonte e o que não foi.

Detalhes: [`../testing/TESTING.md`](../testing/TESTING.md).

## 7. Como acrescentar um subsistema

1. Criar `firmware/<nome>/` com `CMakeLists.txt` (`dbmo_add_library`), `library.json`,
   `README.md`, `include/dbmo/<nome>/` e `src/`.
2. Definir os contratos de hardware que ele precisa como interfaces; implementar a lógica
   só sobre elas.
3. Implementar `ISubsystem` (`init`, `tick`, `shutdown`); devolver `HardwareAbsent` quando
   o periférico não responde.
4. Declarar as chaves de configuração em `registerConfig()` e os eventos em um domínio de
   `EventDomain`.
5. Criar `tests/unit/<nome>/` com os testes e, se houver chip, um simulador.
6. Acrescentar o nome a `DBMO_SUBSYSTEMS` (CMake), a `SUBSYSTEMS` (`tools/check_repo.py`)
   e a `lib_deps` (`platformio.ini`); ligar na `Application`.

Para um periférico de cartucho não é preciso nada disso: basta um driver de cartucho
(`docs/cartridges/CARTRIDGE_SPECIFICATION.md`, seção 9).
