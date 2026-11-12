# Arquitetura de alimentação do d-BMO v1

| Campo | Valor |
|---|---|
| Versão | 0.1 (2026-11-12) |
| Estado | **Proposta com decisões em aberto.** Não existe esquemático, nenhum CI foi escolhido e nada foi montado |
| Evidências | [`sources/ESP32-S3.md`](sources/ESP32-S3.md), [`sources/PERIPHERALS.md`](sources/PERIPHERALS.md), `README.md` ("Alimentação", "Riscos da bateria") |

Este documento **não define um circuito**. Ele reúne o que está decidido, os requisitos
que as fontes confirmam e as decisões que faltam. A pesquisa de CIs de carga, proteção e
regulação **não foi feita nesta sessão**; os candidatos citados vêm do `README.md` e não
tiveram datasheet verificado.

## 1. O que está decidido (README)

| Decisão | Origem |
|---|---|
| Bateria LiPo de 1 célula (selecionada: anúncio de 4000 mAh; compra não confirmada) | README, "Compras" |
| Carga por USB-C, proteção e regulação como componentes SMD na PCB própria | README, "Alimentação" |
| Liga e desliga por botão momentâneo com circuito de trava (power-latch) | README, "Compras" |
| Lógica em 3,3 V; nunca expor a tensão crua da bateria aos periféricos de 3,3 V | README, "Alimentação" |
| MAX98357A alimentado direto da bateria | README, "Riscos da bateria", item 6 |
| Meta de corrente com o aparelho desligado: cerca de 10 µA | README, "Riscos da bateria", item 4 |

## 2. Diagrama de blocos proposto

```
 USB-C ──► carregador com power-path ──► célula LiPo 1S (com PCM e NTC)
                    │                          │
                    └──────── nó de sistema ◄──┘
                                   │
                         trava de alimentação  ◄── botão de energia
                                   │               ◄── GPIO HOLD (firmware)
                                   ▼
                     regulador de 3,3 V (buck-boost)
                                   │
        ┌──────────────┬───────────┼───────────────┬──────────────┐
     ESP32-S3        tela      CC1101, PN532,   microfone      cartucho
                               MCP23017, SD,                  (limitado)
                               IR
   nó de sistema ──► MAX98357A (2,5 V a 5,5 V)
   nó de sistema ──► divisor ──► ADC (leitura da bateria)
```

Cada bloco abaixo traz o requisito confirmado e o que está em aberto.

## 3. Requisitos confirmados em fonte

| Bloco | Requisito | Evidência |
|---|---|---|
| Trilho de 3,3 V | O ESP32-S3 pede fonte de 3,3 V com pelo menos 500 mA, e 3,0 V a 3,6 V nos pinos | S3-PWR-01, S3-PWR-02 |
| Trilho de 3,3 V | Manter acima de 3,0 V para a flash e a PSRAM | S3-PWR-05 |
| Entrada do módulo | Proteção ESD e pelo menos 10 µF na entrada; mais 10 µF por causa do surto de transmissão | S3-PWR-04 |
| Reset (EN) | O módulo não traz o RC: 10 kΩ + 1 µF recomendados. Com fonte de subida lenta ou liga/desliga frequente (caso de bateria), o RC pode não bastar | MOD-10, S3-PWR-09, S3-PWR-10 |
| GPIO 0 | Pull-up; sem capacitor de valor alto | S3-STRAP-08 |
| Tela | Lógica de 1,65 V a 3,3 V; analógica de 2,5 V a 3,3 V | ILI-01, ILI-02 |
| CC1101 | 1,8 V a 3,6 V; máximo absoluto de 3,9 V | CC-01, CC-02 |
| PN532 | VBAT de 2,7 V a 5,5 V; interface de 1,6 V a 3,6 V | PN-01, PN-02 |
| MCP23017 | 1,8 V a 5,5 V | MCP-08 |
| INMP441 | 1,62 V a 3,63 V | MIC-01 |
| MAX98357A | 2,5 V a 5,5 V | AMP-01 |
| Receptor de IR (família TSOP48xx) | 2,5 V a 5,5 V | IR-01 |

Consequência: **nada no v1 exige 5 V**. O que o `README.md` chama de "boost de 5 V
provavelmente desnecessário" é coerente com as faixas acima.

## 4. Decisões em aberto

| # | Decisão | O que se sabe | O que falta |
|---:|---|---|---|
| 1 | CI de carga | O README pede power-path e entrada de NTC e cita candidatos (BQ24074, BQ25895/BQ25601, MCP73871) | Nenhum datasheet foi aberto. Corrente de carga, sinais de estado, dissipação |
| 2 | Proteção da célula | Célula com PCM e NTC; proteção adicional na PCB | Escolha do CI; limiares de subtensão e sobrecorrente |
| 3 | Regulador de 3,3 V | Precisa de ≥ 500 mA só para o ESP32-S3, mais as cargas da seção 3. Com célula de 3,0 V a 4,2 V, a saída de 3,3 V fica no meio da faixa: o README recomenda buck-boost | Escolha do CI; corrente de pico real (rádio transmitindo); corrente em repouso |
| 4 | Trava de alimentação | Botão liga; firmware segura (`POWER_HOLD`) e solta | Topologia. Interage com o risco do GPIO 0 ([`GPIO_MAP.md`](GPIO_MAP.md), risco 1) e com o GPIO 48 não ser de RTC |
| 5 | Desligar cortando a energia ou em deep sleep | Deep sleep medido pela Espressif: 6,8 a 8,14 µA só no módulo (S3-PWR-14, S3-PWR-15), quase toda a meta de 10 µA | Corrente dos demais blocos desligados; se os straps são relidos ao acordar (S3-STRAP-21) |
| 6 | Linha de 5 V no cartucho | README: decisão em aberto | Decidir; hoje o firmware a trata como não oferecida |
| 7 | Limitador de corrente do cartucho | README: limitador ou polyfuse | Valor; o firmware usa 100 mA provisórios (`cartridge.max_ma`) |
| 8 | Supervisor de tensão no EN | Recomendado pelo fabricante em uso com bateria (S3-PWR-10) | Escolha e limiar |
| 9 | USB-C | README: resistores de 5,1 kΩ separados em CC1 e CC2 | Conferir no esquemático; fonte primária não aberta |
| 10 | Alimentação do amplificador | Direto do nó de sistema (bateria ou USB) | Ruído do amplificador no trilho; estado do SD_MODE no boot (AMP-07) |

## 5. Leitura da bateria

| Item | Valor | Estado |
|---|---|---|
| Pino | GPIO 1 (ADC1_CH0) | Proposto |
| Divisor | 100 kΩ / 100 kΩ (×2 no firmware) | Proposto (README) |
| Tensão no pino com 4,2 V | 2,1 V, dentro da faixa calibrada de 0 a 2900 mV | Derivado de S3-ADC-09 |
| Erro esperado | ±50 mV no pino, cerca de ±100 mV na bateria | Derivado de S3-ADC-09 |
| Capacitor no pino | 0,1 µF | S3-ADC-10 |
| Corrente do divisor | 4,2 V / 200 kΩ = 21 µA, **contínuos** se ligado direto à bateria | Calculado |

O último ponto conflita com a meta de 10 µA desligado: o divisor sozinho consumiria o
dobro. Opções (sem decisão): resistores maiores (piora a exatidão por causa da impedância
de fonte do ADC, não verificada: S3-ADC-11), divisor depois da trava, ou divisor chaveado.

No firmware a leitura vem **desabilitada** (`power.vbat_enabled = false`) e os limiares
(3500 mV baixo, 3300 mV crítico) são provisórios. Sem leitura válida o nível é
"desconhecido"; nenhum valor é inventado (`firmware/power/README.md`).

## 6. O que o firmware já assume

| Suposição | Onde | Se a decisão de hardware for outra |
|---|---|---|
| A trava é um GPIO em nível alto para manter ligado | `GpioPowerLatch` | Inverter o parâmetro `activeHigh` ou implementar outro `IPowerSwitch` |
| O botão de energia é ativo em nível baixo e pode ser lido por GPIO | `PowerSubsystem::setPowerButton` | Trocar pino e polaridade na montagem do `Hardware` |
| Sem trava, "desligar" é deep sleep com despertar pelo botão | `DeepSleepControl` | Nenhuma mudança: a trava tem precedência |
| O carregador informa o estado por um ou dois pinos em dreno aberto | `GpioChargerStatus` | Implementar `IChargerStatus` para um CI com I2C |
| A energia só é cortada depois que o botão é solto | `completeShutdown()` | Depende da topologia da trava; revisar junto com o item 4 |

## 7. Bancada de hoje

A DevKitC é alimentada pelo USB, com o LDO da própria placa (DKC-11). As três formas de
alimentar a placa são mutuamente exclusivas (DKC-13): **não alimentar pelo pino 3V3 com o
USB conectado**. O modelo e a corrente do LDO da placa não foram verificados (DKC-12), o
que limita quantos periféricos podem ser ligados ao 3V3 da placa ao mesmo tempo
([`POWER_BUDGET.md`](POWER_BUDGET.md)).
