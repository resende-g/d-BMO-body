# Inventário físico do hardware

| Campo | Valor |
|---|---|
| Versão | 0.1 (2026-11-20) |
| Estado | **Modelo para preencher.** Em 20/11/2026 o usuário informou que as compras do AliExpress chegaram; nenhum item foi conferido peça a peça |
| Fonte da lista | `README.md`, "Lista de componentes (BOM)" |

Estados possíveis, nesta ordem: **esperado** → **recebido** (está na mão) → **identificado**
(marcação do chip confere com o esperado) → **testado** (funcionou em bancada) → **aprovado**
ou **reprovado**. Um item só avança com evidência registrada (foto, leitura, teste).

## 1. Compra de 02/11/2026

| Item | Variação comprada | O que conferir na peça | Foto | Estado | Observação |
|---|---|---|---|---|---|
| PN532 V3 NFC | "PN532 Set" | Marcação "PN532" no CI; DIP switch e posições para I²C; o que veio no kit (cartão, chaveiro) | | recebido | Barra de pinos **não soldada** (20/11): bloqueado até haver ferro de solda |
| Conjunto IR 38 kHz | "1set" | Se há **transmissor e receptor**; marcação do receptor (ex.: família 1838/TSOP) e do LED | | recebido | Duas placas com 3 pinos em fileira única (fotos de 21/11). Receptor "V1222": OUT, VCC, GND. Transmissor "V1221": DAT, VCC, GND; não se sabe se há transistor (componentes não identificados) |
| MAX98357A | "Small Chip Welding" | Marcação do CI (pode ser ilegível no encapsulamento pequeno); pinos já soldados | | recebido | Pinos soldados (20/11). Ligação do alto-falante a confirmar (borne ou furo) |
| Alto-falante 2030 | 8 Ω 2 W | Dimensões com paquímetro; resistência DC (fica abaixo de 8 Ω, é normal) | | esperado | |
| INMP441 | — | Marcação do microfone; furo acústico; pinos | | recebido | Pinos soldados (20/11) |
| Jumpers Dupont | 20 cm, F-F/M-M/M-F, 40 de cada | Quantidade por tipo | | esperado | |
| Protoboard MB-102 | branca, 830 pontos | Trilhos de alimentação contínuos ou partidos ao meio | | esperado | |
| Resistores | kit 1 %, 30 valores, 300 peças | Presença de 4,7 kΩ, 100 kΩ, 1 kΩ, 5,1 kΩ | | esperado | Potência 1/4 W: não servem como carga de teste |
| Capacitores 100 nF | 100 peças "104" | Quantidade aproximada | | esperado | |

## 2. Itens recebidos antes

| Item | O que conferir | Estado | Observação |
|---|---|---|---|
| CC1101 + antena SMA | Marcação "CC1101"; frequência do cristal (26 MHz é o usual); **banda do casamento de antena** (o anúncio diz 433 MHz; módulos de 315/868 MHz são parecidos) | identificado (placa) | Ebyte **E07-M1101D V2.0**, serigrafia "433M" (fotos de 21/11). Barra **2×4** em 2,54 mm: **não vai na protoboard**, só com jumpers fêmea. Pinagem: 1 GND, 2 VCC, 3 GDO0, 4 CSN (manual Ebyte); 5 SCK, 6 MOSI, 7 MISO/GDO1, 8 GDO2 (só fonte secundária: ficha LCSC do E07-M1101D-TH). Conferir pino 1 = GND por continuidade com a carcaça do SMA. Marcação do CI e do cristal não legíveis na foto |
| Tela MSP3218 (ILI9341) | — | testado | Validada (ID 0x9341). Conector do SD (4 pinos) **só com furos** (20/11): SD bloqueado até haver solda |
| ESP32-S3 DevKitC-1 N16R8 | — | testado | Validada com Bruce |
| MCP23017 | — | **não possui** | Informado em 21/11; o README dizia o contrário por erro. A comprar |
| Power bank AJ-C15 | Ver [`POWERBANK_AJ-C15.md`](POWERBANK_AJ-C15.md) | recebido | Em investigação |

## 3. Situação a confirmar com o usuário

| Item | Dúvida |
|---|---|
| Bateria LiPo 4000 mAh | O README diz "compra não confirmada". Foi comprada? |
| Soquete card edge | Comprado ou não? |
| Fio 30 AWG, malha, fluxo | Comprados no Brasil? |
| Multímetro e ferramentas | Em 20/11: só multímetro e kit de chaves; ferro de solda a caminho |
| Outros itens | Algo chegou que não está nesta lista? |
