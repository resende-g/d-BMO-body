# Placa-base da DevKitC (CPU do d-BMO)

| Campo | Valor |
|---|---|
| Versão | 0.2 (2026-11-27) |
| Para quem | Tiago Batista, que fabrica a placa em casa (laser + percloreto, uma face) |
| Ferramenta | EasyEDA Pro, projeto `dbmo`, Board1 (Schematic1 + PCB1) |
| Tamanho | 64 × 53 mm (limite do Tiago: 70 × 70 mm) |
| Fonte das ligações | [`ligacoes.json`](ligacoes.json), conferido contra `firmware/config/include/dbmo/config/pins.h` (perfil `target`) por [`conferir_netlist.py`](conferir_netlist.py) |
| Estado | **Desenhada e roteada: 0 fio, sem curto** (`fios.py` sobre o Gerber exportado do EasyEDA). Nada montado |

## 1. O que é

Em 2026-11-27 o d-BMO virou módulos de até 65 × 65 mm, porque o Tiago só fabrica placas de até
70 × 70 mm (a placa única deitada tinha 170 × 115 mm). Esta placa só leva a DevKitC: cada pino dela
vai em linha reta para barras de pinos machos, e **todos os módulos chegam por cabo Dupont**:

- **JT1, JT2, JT3** (1×22, em cima): três cópias da fileira J1 da DevKitC (3V3, 5V, SPI e a maior parte
  dos GPIOs). Três cópias porque o SPI vai para a tela, o SD e o CC1101.
- **JB1, JB2** (1×22, embaixo): duas cópias da fileira J3 da DevKitC.
- **J12** (1×4): mais 3V3. **J13** (1×4): mais GND.

Como cada pino da DevKitC tem a sua coluna, a rota é reta e não sobra fio. Os GPIOs são os de
`pins.h` (perfil `target`, de 2026-11-26): a placa-base não muda nenhum.

Decisões do Gabriel em 2026-11-27: módulos para o Tiago; a CPU como placa-base, com tudo por cabo
(a alternativa, MAX98357A, INMP441 e CC1101 montados na CPU, deixava 18 fios); o MCP23017 foi para a
placa de botões da esquerda.

## 2. Arquivos

| Arquivo | O que é |
|---|---|
| `ligacoes.json` | Pino de cada peça → net. É a fonte |
| `esquematico.enet` | Netlist exportada do esquemático (EasyEDA Pro) |
| `conferir_netlist.py` | Confere a netlist contra `ligacoes.json` e `pins.h`. Roda na CI. Aceita o `ligacoes.json` das outras placas como segundo argumento |
| `gerber.zip` | Gerber exportado do EasyEDA (cobre só na camada de baixo) |
| `cobre-espelhada.png` / `.svg` | **Para o Tiago gravar**: a camada de cobre vista do lado do cobre, 1:1, 600 dpi, furos marcados |
| `cobre-vista-de-cima.png` / `.svg` | A mesma camada como o EasyEDA mostra (olhando pelo lado das peças) |
| `lista-de-pecas.tsv` | Lista de peças exportada do PCB |
| `fios.md` | Saída do `fios.py`: nenhum fio, isolação mínima de 0,60 mm, cobre a 2,03 mm da borda, toda trilha com 0,8 mm |
| `fios.py` | Lê o Gerber e lista os fios, a isolação mínima, o cobre mais perto da borda e as trilhas abaixo de 0,8 mm; falha se houver curto, trilha na camada de cima ou isolação abaixo de 0,5 mm. Serve às quatro placas |
| `dsn.py`, `rotear.py` | Gera o DSN (Specctra) a partir do Gerber e roteia com o Freerouting; o SES volta ao EasyEDA pelo MCP |
| `png_tiago.py` | Gera as imagens para o Tiago a partir do Gerber |

Como foi feito: esquemático e posições pelo MCP `easyeda-pro`; rota pelo Freerouting 2.5.0 (Java 25+),
só na camada de baixo, trilha 0,8 mm, isolação 0,5 mm, 2 mm de cobre até a borda e keepout sob a antena;
o SES voltou ao EasyEDA pela extensão MCP Bridge com uma correção local (a 0.5.1 não sabia criar trilha:
usa `pcb_PrimitiveLine.create`). Medido de novo pelo Gerber exportado.

## 3. Colunas das barras

Barras na horizontal, coluna 1 à esquerda olhando pelo lado das peças, com o USB da DevKitC na borda
esquerda. JT1 e JB1 são as fileiras mais perto da DevKitC; todas as fileiras de uma coluna têm a mesma
net.

| Coluna | JT (cima): pino DevKitC · função · net | JB (baixo): pino DevKitC · função · net |
|---:|---|---|
| 1 | J1-22 · GND · `GND` | J3-22 · GND · `GND` |
| 2 | J1-21 · 5V · `+5V` | J3-21 · GND · `GND` |
| 3 | J1-20 · GPIO14 · `DISPLAY_BACKLIGHT` | J3-20 · GPIO19 · `USB_DM` |
| 4 | J1-19 · GPIO13 · `SPI_MISO` | J3-19 · GPIO20 · `USB_DP` |
| 5 | J1-18 · GPIO12 · `SPI_SCK` | J3-18 · GPIO21 · `CC1101_GDO0` |
| 6 | J1-17 · GPIO11 · `SPI_MOSI` | J3-17 · GPIO47 · `SD_CS` |
| 7 | J1-16 · GPIO10 · `POWER_BTN` | J3-16 · GPIO48 · `CC1101_CS` |
| 8 | J1-15 · GPIO9 · `VBAT_SENSE` | J3-15 · GPIO45 · `IO45` |
| 9 | J1-14 · GPIO46 · `IO46` | J3-14 · GPIO0 · `IO0` |
| 10 | J1-13 · GPIO3 · `IO3` | J3-13 · GPIO35 · `IO35` |
| 11 | J1-12 · GPIO8 · `CHG_STAT` | J3-12 · GPIO36 · `IO36` |
| 12 | J1-11 · GPIO18 · `I2S_LRCLK` | J3-11 · GPIO37 · `IO37` |
| 13 | J1-10 · GPIO17 · `I2S_BCLK` | J3-10 · GPIO38 · `IO38` |
| 14 | J1-9 · GPIO16 · `I2S_SPK_DATA` | J3-9 · GPIO39 · `DISPLAY_DC` |
| 15 | J1-8 · GPIO15 · `AMP_SHUTDOWN` | J3-8 · GPIO40 · `DISPLAY_RESET` |
| 16 | J1-7 · GPIO7 · `I2S_MIC_DATA` | J3-7 · GPIO41 · `DISPLAY_CS` |
| 17 | J1-6 · GPIO6 · `I2C_SDA` | J3-6 · GPIO42 · `IO42` |
| 18 | J1-5 · GPIO5 · `IR_TX` | J3-5 · GPIO2 · `POWER_HOLD` |
| 19 | J1-4 · GPIO4 · `IR_RX` | J3-4 · GPIO1 · `I2C_SCL` |
| 20 | J1-3 · EN · `EN` | J3-3 · RX (GPIO44) · `U0RXD` |
| 21 | J1-2 · 3V3 · `+3V3` | J3-2 · TX (GPIO43) · `U0TXD` |
| 22 | J1-1 · 3V3 · `+3V3` | J3-1 · GND · `GND` |

Não ligar nada em `IO0`, `IO3`, `IO45`, `IO46` (strapping), `IO35`–`IO37` (PSRAM da N16R8), `IO38`
(LED RGB), `USB_DM`/`USB_DP` e `U0RXD`/`U0TXD` (USB e serial da DevKitC). `IO42` está livre.

## 4. Cabos (de cada módulo para a placa-base)

`JT·n` = qualquer fileira da coluna n de cima; `JB·n` = coluna n de baixo. 3V3: J12 ou JT·21/22.
GND: J13, JT·1, JB·1, JB·2 ou JB·22.

| Módulo | Pino do módulo → placa-base |
|---|---|
| Tela MSP3218 (barra de 14) | VCC → 3V3 · GND → GND · CS → JB·16 · RESET → JB·15 · DC → JB·14 · SDI (MOSI) → JT·6 · SCK → JT·5 · LED → JT·3 (luz de fundo por PWM) **ou** 3V3 (sempre acesa) · SDO (MISO) → JT·4 · T_* → nada |
| SD da tela (barra de 4) | SD_CS → JB·6 · MOSI → JT·6 · MISO → JT·4 · SCK → JT·5 |
| CC1101 E07-M1101D | 1 GND → GND · 2 VCC → 3V3 · 3 GDO0 → JB·5 · 4 CS → JB·7 · 5 SCK → JT·5 · 6 MOSI → JT·6 · 7 MISO → JT·4 · 8 GDO2 → nada |
| INMP441 | L/R → GND · WS → JT·12 · SCK → JT·13 · GND → GND · VDD → 3V3 · SD → JT·16 |
| MAX98357A | LRC → JT·12 · BCLK → JT·13 · DIN → JT·14 · GAIN → nada (9 dB) · SD → JT·15 · GND → GND · Vin → 3V3 · alto-falante no borne |
| Receptor de IR | S → JT·19 · + → 3V3 · − → GND |
| Transmissor de IR | S → JT·18 · + → 3V3 · − → GND |
| PN532 (modo I²C) | GND → GND · VCC → 3V3 · SDA → JT·17 · SCL → JB·19 |
| Placa de botões da esquerda (JE) | 1 SDA → JT·17 · 2 SCL → JB·19 · 3 3V3 → 3V3 · 4 GND → GND · 5 `PWR_KEY` → placa de energia J2-4 · 6 `POWER_BTN` → JT·7 |
| Placa de energia (J2) | 1 `+5V` → JT·2 · 2 e 8 GND → GND · 3 nada · 4 `PWR_KEY` → JE-5 da placa da esquerda · 5 `POWER_HOLD` → JB·18 · 6 `VBAT_SENSE` → JT·8 · 7 `CHG_STAT` → JT·11 |

Cópias que bastam: SPI 3 (tela, SD, CC1101), I2S_LRCLK/BCLK 2 (INMP441, MAX98357A), SDA 3 e SCL 2
(PN532, botões), 3V3 10, GND 13. Sem a placa de energia, tudo roda pelo USB da DevKitC; o botão de
energia funciona pelo D1 da placa da esquerda.

## 5. Pendente antes de fabricar

| # | Item | Situação | Como fechar |
|---|---|---|---|
| 1 | Ordem dos pinos da DevKitC | **Fato:** símbolo e pegada da LCSC (ESP32-S3-DEVKITC-1-N8R8, mesma pinagem da N16R8) batem com o guia v1.1 da Espressif; `conferir_netlist.py` confere cada GPIO pelo nome do pino | — |
| 2 | Pegada da DevKitC | **Fato:** fileiras a 22,86 mm e pinos 1 e 22 a 53,34 mm, como no footprint oficial. **Hipótese:** a DevKitC (62,7 mm) fica com o USB rente à borda esquerda | Paquímetro na placa em mãos e na impressão 1:1 |
| 3 | Dupont ao lado da DevKitC | **Hipótese:** a fileira JT1/JB1 fica a 5,08 mm da fileira da DevKitC (uma posição vazia no meio), o bastante para a carcaça Dupont não encostar na placa da DevKitC, que fica ~11 mm acima | Encaixar um cabo antes de soldar tudo |
| 4 | Antena | **Fato:** sem cobre embaixo da ponta da antena (keepout na rota). **Pendente:** teste de alcance do Wi-Fi montado | Teste |
| 5 | Regras do Tiago | **Fato (Tiago, 2026-11-26):** trilha de 0,8 a 1 mm serve; placa até 70 × 70 mm; margem de 2–3 mm. **Fato (`fios.md`):** toda trilha com 0,8 mm (os 2 trechos de `GND` de 0,6 mm junto do JB2.20 foram alargados em 2026-11-27), isolação mínima de 0,60 mm, cobre a 2,03 mm da borda. **Hipótese:** isolação de 0,5 mm basta (ele engrossa tudo no Paint depois) | Ver a primeira placa gravada |
| 6 | Furos | **Fato (Tiago):** brocas de 1,0 e 0,6 mm. As barras de pinos pedem 1,0 mm | — |
| 7 | 5V e USB juntos | **Hipótese:** o pino 5V da DevKitC recebe o VBUS com o USB ligado; a placa de energia tem um Schottky (D2) para não haver duas fontes em paralelo | Multímetro no pino 5V com o USB ligado |
| 8 | Imagem para o Tiago | `cobre-espelhada.png` é o lado do cobre visto de frente (cobre na camada de baixo). Ele engrossa trilhas e pads no Paint | Confirmar com o Tiago na primeira placa |
