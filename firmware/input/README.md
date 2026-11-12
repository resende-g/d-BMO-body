# Input (controles)

Camada dos controles físicos: D-pad, quatro botões de ação, select/start, quatro gatilhos
de ombro, dois analógicos e os cliques deles.

## Peças

| Peça | Arquivo | Papel |
|---|---|---|
| Modelo lógico | `types.h` | `Button`, `ButtonAction`, `Stick`, `StickPosition` |
| `IButtonSource` | `button_source.h` | De onde vêm os botões. O resto do módulo não conhece o MCP23017 |
| `Mcp23017ButtonSource` | `button_source.h` | Botões no expansor (botão ao GND, pull-up interno) |
| `GpioButtonSource` | `button_source.h` | Botões direto em GPIO (os três da bancada de hoje) |
| `Mcp23017` | `mcp23017.h` | Driver mínimo do expansor |
| `ButtonProcessor` | `button_processor.h` | Debounce, clique, toque longo, repetição, combinações |
| `StickProcessor` | `stick_processor.h` | Calibração, zona morta radial, normalização |
| `InputSubsystem` | `input_subsystem.h` | Consulta as origens a cada 5 ms e publica eventos |

Trocar ou simular o controlador de botões é implementar `IButtonSource` (dois métodos).

## Comportamento dos botões

- **Debounce:** uma mudança só vale depois de `input.debounce_ms` com a leitura estável.
- **Clique:** apertou e soltou antes do tempo de toque longo.
- **Toque longo:** uma vez, depois de `input.long_press_ms`. Cancela o clique.
- **Repetição:** para os botões da máscara de repetição (padrão: D-pad), depois de
  `input.repeat_delay_ms` e então a cada `input.repeat_ms`. Esses botões não geram toque
  longo.
- **Combinações:** um conjunto de botões segurados dispara um evento próprio (com tempo
  mínimo e repetição opcionais). Os botões da combinação deixam de valer sozinhos até
  serem soltos. O projeto prevê select + cima/baixo para o volume.

## Analógicos

Cada analógico são dois canais do ADC1. O `StickProcessor` mede o centro com o analógico
solto, aprende o curso em um modo de calibração explícito, recusa calibrações sem curso
(peça com defeito) e aplica uma zona morta radial reescalada, de modo que a saída começa
em zero na borda da zona morta e ainda chega a ±1000 no fim do curso.

Não implementado: gravar a calibração no `ConfigStore` (a calibração pode ser lida e
aplicada por `calibrationX()`/`setCalibration()`, mas ninguém a persiste ainda).

## Eventos (domínio `Input`)

| Evento | `a` | `b` |
|---|---|---|
| `kButton` | `Button` | `ButtonAction` |
| `kCombo` | id da combinação | — |
| `kStick` | `Stick` | x e y empacotados (`packStick` / `unpackStick`) |
| `kSourceFault` | índice da origem | `Status` |
| `kSourceRecovered` | índice da origem | — |

## Configuração

| Chave | Padrão | Faixa |
|---|---:|---|
| `input.debounce_ms` | 20 | 1–200 |
| `input.long_press_ms` | 600 | 200–5000 |
| `input.repeat_delay_ms` | 400 | 100–3000 |
| `input.repeat_ms` | 120 | 30–1000 |
| `input.deadzone` | 120 | 0–900 (milésimos do curso) |

## Falhas

Se uma origem de botões para de responder, o módulo mantém o estado por algumas leituras
(falha passageira), depois solta todos os botões (nenhuma tecla fica presa), publica
`kSourceFault` uma vez e tenta reiniciar a origem a cada segundo. Ela só volta a valer
depois de uma leitura bem-sucedida.

## MCP23017: 14 entradas, não 16

A revisão D do datasheet (Microchip DS20001952D, junho de 2022) declara GPA7 e GPB7 como
somente saída no MCP23017 (`docs/hardware/sources/PERIPHERALS.md`, MCP-03 a MCP-06). O
driver recusa configurá-los como entrada e o mapa padrão (`pins.h`, `mcp_buttons`) não
põe botão neles: um expansor lê 14 botões. Os dois cliques dos analógicos ficam
**pendentes de decisão de hardware** (proposta: segundo MCP23017 em 0x21, `mcp_aux`).

Os endereços dos registradores usados (`IODIR` 0x00, `IPOL` 0x02, `IOCON` 0x0A, `GPPU`
0x0C, `GPIO` 0x12, `OLAT` 0x14, com `IOCON.BANK = 0`) não constam do registro de
evidências; foram conferidos contra o driver do MCP23017 do Retro-Go
(`components/retro-go/rg_i2c.c`), que é fonte secundária.

## Estado

- Implementado e testado no computador: tudo o que está na tabela de peças
  (`tests/unit/input/`, com um simulador do MCP23017 no nível dos registradores).
- Aguarda hardware: MCP23017 real (endereços, pull-ups, tempo de leitura a 100 kHz),
  ruído e curso reais dos analógicos tirados do controle de PS3, tempos de debounce
  adequados aos botões táteis usados.
