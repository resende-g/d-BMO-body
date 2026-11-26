# Inventário físico do hardware

| Campo | Valor |
|---|---|
| Versão | 0.2 (2026-11-25) |
| Estado | Em preenchimento. Atualizado em 25/11/2026 com o que a bancada testou de 21 a 25/11 ([`../testing/TESTING.md`](../testing/TESTING.md)) |
| Fonte da lista | `README.md`, "Lista de componentes (BOM)" |

Estados possíveis, nesta ordem: **esperado** → **recebido** (está na mão) → **identificado**
(marcação do chip confere com o esperado) → **testado** (funcionou em bancada) → **aprovado**
ou **reprovado**. Um item só avança com evidência registrada (foto, leitura, teste).

## 1. Compra de 02/11/2026

| Item | Variação comprada | O que conferir na peça | Foto | Estado | Observação |
|---|---|---|---|---|---|
| PN532 V3 NFC | "PN532 Set" | Marcação "PN532" no CI; DIP switch e posições para I²C; o que veio no kit (cartão, chaveiro) | | recebido | Barra de pinos **não soldada** (20/11): bloqueado até haver ferro de solda |
| Conjunto IR 38 kHz | "1set" | Se há **transmissor e receptor**; marcação do receptor (ex.: família 1838/TSOP) e do LED | | testado (receptor) | Duas placas com 3 pinos em fileira única (fotos de 21/11). Receptor "V1222": OUT, VCC, GND; **decodificou NEC na bancada** em 21 e 22/11 (`nec:0004:17` e outros); em 25/11 não leu o controle (fiação a conferir). Transmissor "V1221": DAT, VCC, GND; não se sabe se há transistor (componentes não identificados); nunca enviou |
| MAX98357A | "Small Chip Welding" | Marcação do CI (pode ser ilegível no encapsulamento pequeno); pinos já soldados | | identificado (placa) | Pinos soldados (20/11). Alto-falante por **borne de parafuso**, sem solda (fotos de 22/11). Serigrafia: LRC, BCLK, DIN, GAIN, SD, GND, Vin; no verso, "MAX98357A I2S Mono Amp", "vin: 2.5-5.5V", "Default 9db Gain & (L+R)/2 out". Ainda não ligado |
| Alto-falante 2030 | 8 Ω 2 W | Dimensões com paquímetro; resistência DC (fica abaixo de 8 Ω, é normal) | | recebido | Retangular, cabo vermelho e preto trançado com conector branco de 2 vias (foto de 22/11). Impedância, dimensões e resistência não medidas. O conector não entra no borne do MAX98357A |
| INMP441 | — | Marcação do microfone; furo acústico; pinos | | testado | Pinos soldados (20/11). Serigrafia: L/R, WS, SCK; GND, VDD, SD; furo do som no lado da serigrafia (fotos de 22/11). Gravou na bancada em 22/11 (palmas saturam) e em 25/11 (8 s) |
| Jumpers Dupont | 20 cm, F-F/M-M/M-F, 40 de cada | Quantidade por tipo | | recebido | Em uso na bancada desde 21/11; quantidade por tipo não contada |
| Protoboard MB-102 | branca, 830 pontos | Trilhos de alimentação contínuos ou partidos ao meio | | recebido | Em uso desde 21/11. Trilho (+) de cima contínuo entre as colunas 10 e 46 (o LED da tela acende pelo trilho); continuidade de ponta a ponta (M1) não medida |
| Resistores | kit 1 %, 30 valores, 300 peças | Presença de 4,7 kΩ, 100 kΩ, 1 kΩ, 5,1 kΩ | | esperado | Potência 1/4 W: não servem como carga de teste |
| Capacitores 100 nF | 100 peças "104" | Quantidade aproximada | | esperado | |

## 2. Itens recebidos antes

| Item | O que conferir | Estado | Observação |
|---|---|---|---|
| CC1101 + antena SMA | Marcação "CC1101"; frequência do cristal (26 MHz é o usual); **banda do casamento de antena** (o anúncio diz 433 MHz; módulos de 315/868 MHz são parecidos) | testado | Respondeu na bancada (`PARTNUM=0x00`, `VERSION=0x14`) em 21, 22 e 25/11 e entrou em recepção; o cristal de 26 MHz não foi verificado e nunca transmitiu. Ebyte **E07-M1101D V2.0**, serigrafia "433M" (fotos de 21/11). Barra **2×4** em 2,54 mm: **não vai na protoboard**, só com jumpers fêmea. Pinagem: 1 GND, 2 VCC, 3 GDO0, 4 CSN (manual Ebyte); 5 SCK, 6 MOSI, 7 MISO/GDO1, 8 GDO2 (só fonte secundária: ficha LCSC do E07-M1101D-TH). Conferir pino 1 = GND por continuidade com a carcaça do SMA. Marcação do CI e do cristal não legíveis na foto |
| Tela MSP3218 (ILI9341) | — | testado | Validada (ID 0x9341). Conector do SD soldado em 24/11 e **testado em 25/11**: o cartão monta, grava e lê no SPI dividido com a tela e o CC1101 |
| ESP32-S3 DevKitC-1 N16R8 | — | testado | Validada com Bruce |
| MCP23017 | — | **não possui** | Informado em 21/11; o README dizia o contrário por erro. A comprar |
| Placa do controle genérico de PS3 | Tampas dos botões (os sticks saíram do v1 em 2026-11-25) | identificado (placa) | Placa **BM-901 VER1.1**, CI marcado "ShanWan BM-901SWQP174 003" (fotos de 21/11; sem datasheet localizado). Dois sticks soldados na placa, cada um com dois potenciômetros de 3 pinos. Serigrafia: "R1 ADC R2", "L1 ADC L2", USB_CON (5V, D−, D+, GND, GND), fileira de furos vazios "Y B A X ADC PS ST SE GND AD AL AR AU", pontos "V5", "V3V", "GND". Os sticks não serão dessoldados: o v1 não tem analógicos nem gatilhos |
| Power bank AJ-C15 | Ver [`POWERBANK_AJ-C15.md`](POWERBANK_AJ-C15.md) | recebido | Em investigação |

## 3. Situação a confirmar com o usuário

| Item | Dúvida |
|---|---|
| Bateria LiPo 4000 mAh | O README diz "compra não confirmada". Foi comprada? |
| Soquete card edge | Comprado ou não? |
| Fio 30 AWG, malha, fluxo | Comprados no Brasil? |
| Multímetro e ferramentas | Em 20/11: só multímetro e kit de chaves. Em 24/11 o conector do SD da tela foi soldado, então já há ferro de solda; malha e fluxo não confirmados |
| Outros itens | Algo chegou que não está nesta lista? |
