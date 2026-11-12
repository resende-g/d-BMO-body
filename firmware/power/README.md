# Power (energia)

Bateria, carregador, botão de energia, trava de alimentação e desligamento ordenado.
O circuito de energia do d-BMO **ainda não foi definido** (carregador, proteção, regulador,
trava): por isso cada peça de hardware é opcional e o que não existe aparece como
`Unknown`. Nenhum valor é inventado.

| Peça | Arquivo | Papel |
|---|---|---|
| `BatteryMonitor` | `battery_monitor.h` | Tensão da bateria por ADC + divisor, média de 8 amostras, níveis com histerese |
| `IChargerStatus`, `IPowerSwitch`, `ISleepControl` | `power_hal.h` | Contratos de hardware; `GpioChargerStatus` e `GpioPowerLatch` só dependem de GPIO |
| `PowerSubsystem` | `power_subsystem.h` | Junta tudo, publica eventos e conduz o desligamento |

## Leitura da bateria

- **Desabilitada por padrão** (`power.vbat_enabled = false`): o divisor ainda não existe na
  bancada. Desabilitado, o firmware nem lê o ADC.
- Uma amostra só vale se: o ADC respondeu, a tensão no pino está dentro da faixa calibrada
  do ADC (até 2900 mV, `docs/hardware/sources/ESP32-S3.md`) e o resultado está na faixa
  que uma célula 1S pode ter (2500 a 4350 mV). Fora disso o diagnóstico é `Saturated`,
  `Implausible` ou `ReadError`, e depois de três amostras ruins seguidas o nível volta a
  `Unknown`. Um pino solto **não** vira "bateria crítica".
- Não há porcentagem: ela exigiria a curva de descarga da célula real, que não foi medida.

## Configuração

| Chave | Padrão | Observação |
|---|---:|---|
| `power.vbat_enabled` | false | Ligar só com o divisor montado e conferido |
| `power.vbat_ratio_permille` | 2000 | Vbat = Vpino × razão / 1000 (divisor 100k/100k do README) |
| `power.low_mv` | 3500 | PROVISÓRIO: calibrar com a célula real |
| `power.critical_mv` | 3300 | PROVISÓRIO |
| `power.hysteresis_mv` | 100 | |
| `power.off_hold_ms` | 1500 | Tempo de toque longo para desligar |
| `power.auto_off_critical` | true | Desligar sozinho com bateria crítica confirmada |
| `power.sleep_after_s` | 0 | 0 = nunca; só publica `kIdleTimeout`, a aplicação decide |

## Desligamento

1. Um pedido (`requestShutdown`): toque longo no botão, bateria crítica por cinco amostras
   seguidas sem carregador, ou comando. Publica `kShutdownRequested`.
2. A aplicação encerra os outros subsistemas e chama `completeShutdown()`:
   devolve `Busy` enquanto o botão está apertado, depois solta a trava; sem trava, entra
   em sono profundo com o botão como fonte de despertar; sem nenhum dos dois, `Unsupported`.

O toque que **ligou** o aparelho é ignorado: o botão só passa a valer depois de solto.

## Estado

- Implementado e testado no computador: tudo acima (`tests/unit/power/`, 30 testes).
- Aguarda hardware e decisão: divisor da bateria, carregador (pinos de estado), trava de
  alimentação (GPIO 48 é proposta), limiares reais, consumo em sono. O botão de energia
  no GPIO 0 tem um risco aberto (pino de strap: segurá-lo no reset entra em modo de
  download), registrado em `docs/hardware/GPIO_MAP.md`.
