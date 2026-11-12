# Orçamento de energia do d-BMO v1

| Campo | Valor |
|---|---|
| Versão | 0.1 (2026-11-12) |
| Estado | **Parcial.** Só há correntes de datasheet; nenhuma corrente foi medida no projeto |
| Evidências | [`sources/ESP32-S3.md`](sources/ESP32-S3.md), [`sources/PERIPHERALS.md`](sources/PERIPHERALS.md) |

Regra deste documento: **uma corrente só entra na tabela com fonte**. Onde não há fonte, a
célula diz "sem dado", e os totais são apresentados como **somas parciais**, isto é,
limites inferiores do consumo real. Nenhuma autonomia é estimada: faltam os números que
mais pesam (rádio do ESP32-S3 e backlight da tela).

## 1. Correntes por componente

Condições das fontes: valores típicos, a 25 °C, na tensão de ensaio de cada datasheet
(indicada quando difere de 3,3 V).

| Componente | Estado | Corrente | Evidência |
|---|---|---:|---|
| **ESP32-S3** | Ativo, sem rádio (exemplo da Espressif, variante de memória não informada) | ≈ 23,9 mA | S3-PWR-15 |
| | Wi-Fi conectado, modem-sleep, CPU a 160 MHz: média | 38,2 a 40,1 mA | S3-PWR-11 |
| | Idem: máximo medido | 235,8 a 267,2 mA | S3-PWR-11 |
| | Pico de transmissão e recepção de Wi-Fi e Bluetooth | **sem dado** | S3-PWR-16 |
| | Deep sleep | 6,8 a 8,14 µA | S3-PWR-14, S3-PWR-15 |
| | Requisito de fonte | ≥ 500 mA | S3-PWR-01 |
| **Tela ILI9341** (lógica) | Em operação | **sem dado** | — |
| **Backlight da tela** | Ligado | **sem dado** (o valor de 95 mA encontrado em busca é de outro produto) | MSP-05 |
| **CC1101** | Recepção, 433 MHz | 15,0 a 17,1 mA (a 3,0 V) | CC-15 |
| | Transmissão, 433 MHz, 0 dBm / +10 dBm | 16,0 / 29,2 mA (a 3,0 V) | CC-16 |
| | Repouso (IDLE) | 1,7 mA | CC-19 |
| | Sono (SLEEP) | 0,2 µA típico, 1 µA máximo | CC-19 |
| **PN532** | Digital + analógico | 25 + 6 = 31 mA (VBAT = 5 V) | PN-06 |
| | Campo de RF ligado | + 60 mA típico, 150 mA máximo | PN-06 |
| | Soft-power-down | 22 µA típico, 45 µA máximo | PN-07 |
| | Hard-power-down | 1 µA típico, 2 µA máximo | PN-07 |
| **MCP23017** | Ativo | 1 mA máximo | MCP-15 |
| | Repouso | 1 µA máximo | MCP-15 |
| **INMP441** | Operação normal | 2,2 mA típico, 2,5 mA máximo | MIC-03 |
| | Standby (clock parado) | 0,8 mA máximo | MIC-03 |
| | Power-down (CHIPEN baixo) | 4,5 µA máximo | MIC-03 |
| **MAX98357A** | Repouso com clocks | 2,4 mA típico (a 3,7 V) | AMP-04 |
| | Standby (sem BCLK) | 340 µA típico | AMP-04 |
| | Desligado (SD_MODE em 0 V) | 0,6 µA típico, 2 µA máximo | AMP-04 |
| | Tocando | Depende do volume; potência máxima de saída de 0,77 W em 8 Ω a 3,7 V (1 % de THD). A corrente de alimentação não está no registro | AMP-03 |
| **Receptor de IR** (família TSOP48xx) | Ligado | 0,7 mA típico (a 5 V) | IR-02 |
| **LED de IR** | Emitindo | **sem dado** (peça não identificada) | IR-09 |
| **Cartão SD** | Leitura e escrita | **sem dado** | — |
| **EEPROM do cartucho** | Leitura e escrita | **sem dado** | — |
| **Analógicos** (4 potenciômetros) | Ligados | **sem dado** (resistência não medida) | — |
| **Divisor da bateria** (100k + 100k) | Sempre ligado | 21 µA a 4,2 V | Calculado |
| **Cartucho** | Limite adotado pelo firmware | 100 mA (provisório) | `cartridge.max_ma` |

## 2. O que o firmware faz para poupar energia

| Componente | Estado padrão no firmware | Onde |
|---|---|---|
| CC1101 | Sono (`SPWD`) sempre que não está recebendo nem transmitindo | `RadioSubsystem` |
| PN532 | Campo de RF desligado; busca desligada por padrão | `NfcSubsystem` |
| MAX98357A | Desligado pelo pino SD_MODE em repouso; desliga 50 ms depois de tocar | `AudioSubsystem` |
| INMP441 | Entrada I2S só ligada durante uma gravação | `AudioSubsystem` |
| Tela | Escurece e apaga por inatividade (`display.dim_after_s`, `display.off_after_s`), se houver controle de backlight | `DisplaySubsystem` |
| Aparelho | Desliga por inatividade se `power.sleep_after_s` > 0 (padrão: desligado) | `PowerSubsystem` |

Limite conhecido: o microfone divide os clocks com o amplificador, então ele sai de
standby sempre que o áudio toca, mesmo sem gravação (2,2 mA em vez de 0,8 mA).

## 3. Somas parciais por cenário

Cada soma traz **só o que tem fonte**. As lacunas estão na última coluna e não são
pequenas.

| Cenário | O que está ligado | Soma parcial | Fora da soma (sem dado) |
|---|---|---:|---|
| Rosto na tela, sem rádio | ESP32-S3 ativo (23,9) + MCP23017 (1) + receptor de IR (0,7) + CC1101 dormindo + PN532 em soft-power-down | ≈ 25,6 mA | Tela, **backlight**, cartão, analógicos |
| Recebendo em 433 MHz | Anterior + CC1101 em recepção (17,1) | ≈ 42,7 mA | Idem |
| Buscando etiqueta NFC | Rosto na tela + PN532 com campo (31 + 60) | ≈ 116,6 mA | Idem; a medida do PN532 é a 5 V |
| Tocando áudio | Rosto na tela + MAX98357A em repouso (2,4) + INMP441 (2,2) | ≈ 30,2 mA | Idem, **mais a potência entregue ao alto-falante** |
| Wi-Fi conectado | ESP32-S3 em modem-sleep (40,1 de média) + periféricos do primeiro cenário | ≈ 41,8 mA de média | Idem; picos de até 267 mA medidos, pico de transmissão sem dado |
| Desligado em deep sleep | ESP32-S3 (8,14 µA) + divisor (21 µA) + CC1101 (0,2 µA) + MCP23017 (1 µA) + PN532 em soft-power-down (22 µA) + INMP441 em standby (800 µA) | **≈ 852 µA** | Regulador, carregador, tela, cartão |

O último cenário mostra um problema de projeto, não uma medida: **com os periféricos
alimentados, o aparelho "desligado" passa muito da meta de 10 µA** do `README.md`. Só o
microfone em standby e o PN532 já a excedem. A meta só é alcançável cortando a alimentação
dos periféricos (trava antes do regulador) ou com chave de alimentação para eles.

## 4. Pico de corrente a prever no regulador

Não é possível fechar o número. O que as fontes dão:

- o ESP32-S3 sozinho exige fonte de pelo menos 500 mA (S3-PWR-01);
- o PN532 pode chegar a 150 mA no transmissor, mais 31 mA (PN-06);
- o CC1101 transmite com 29,2 mA a +10 dBm (CC-16);
- o cartucho pode levar até o limite que for definido (100 mA provisórios);
- o amplificador não entra nesta conta se for alimentado direto da bateria.

Somando só esses máximos: 500 + 181 + 29 + 100 = **810 mA**, sem tela, backlight, cartão e
LED de IR. É um piso para o dimensionamento, não um valor de projeto.

## 5. Medições que faltam

| # | Medição | Como | Para quê |
|---:|---|---|---|
| 1 | Corrente da DevKitC + tela, com o rosto na tela | Medidor USB em série, ou fonte de bancada no pino 5V | Base de todos os cenários |
| 2 | Corrente do backlight | Amperímetro em série com o pino LED da tela | Decide se o GPIO aciona direto ou por transistor |
| 3 | Pico com Wi-Fi transmitindo | Osciloscópio em resistor shunt | Dimensionar o regulador e os capacitores |
| 4 | Cada módulo isolado (CC1101, PN532, áudio, cartão) | Amperímetro em série com o VCC do módulo | Conferir os datasheets e os módulos reais |
| 5 | Deep sleep da montagem completa | Microamperímetro | Decidir entre deep sleep e corte de energia |
| 6 | Curva de descarga da célula escolhida | Carga constante, registrando a tensão | Limiares de bateria baixa e crítica; estimar autonomia |

Só depois das medições 1 a 3 faz sentido estimar autonomia. Até lá, qualquer número de
horas de uso seria palpite.
