# Placa principal do d-BMO (uma face)

| Campo | Valor |
|---|---|
| Versão | 0.1 (2026-11-26) |
| Para quem | Tiago Batista, que fabrica a placa em casa (corrosão de uma face) |
| Ferramenta | EasyEDA Pro, projeto `dbmo` (esquemático e placa desenhados pelo MCP `easyeda-pro`) |
| Fonte das ligações | [`ligacoes.json`](ligacoes.json), conferido contra `firmware/config/include/dbmo/config/pins.h` (perfil `target`) por [`conferir_netlist.py`](conferir_netlist.py) |
| Estado | **Esquemático conferido contra `pins.h` (126 ligações). Layout em refação:** em 2026-11-26 o Gabriel pediu formato deitado, parecido com o Anbernic RG34XX (tela no meio, cartão SD saindo por cima, um IR em cada canto de cima, USB-C da DevKitC na lateral). A seção 5 descreve o layout em pé, já descartado; a rota e a lista de fios saem quando o novo layout fechar. Não corroer antes de fechar a seção 6 |

## 1. Escopo (decisões do Gabriel em 2026-11-25)

- Uma face: cobre só embaixo (Bottom), todas as peças de furo passante em cima. O lado das peças é
  a frente do aparelho: tela e botões ficam virados para quem usa.
- Encaixados em barras fêmea 2,54 mm: ESP32-S3 DevKitC-1 N16R8 inteira, tela MSP3218 (ILI9341,
  14 pinos, mais os 4 do SD), CC1101 Ebyte E07-M1101D (2×4), receptor e transmissor de IR (3 pinos
  cada), INMP441 (duas barras de 3), MAX98357A (7 pinos; o alto-falante vai no borne da placa
  dele) e PN532 (4 pinos, modo I²C).
- Botões táteis 6×6 mm PTH: cruz, A, B, X, Y, Start, Select e energia.
- Os 10 botões de jogo num MCP23017 em DIP-28 (soquete), endereço 0x20 (A0–A2 no GND). O botão
  de energia vai direto ao GPIO 2.
- Sem cartucho, sem analógicos, sem gatilhos. Sem circuito de energia: ele vai numa segunda placa
  (seção 3).

## 2. Arquivos

| Arquivo | O que é |
|---|---|
| `ligacoes.json` | Pino de cada peça → net. É a fonte; o esquemático foi feito a partir dele |
| `conferir_netlist.py` | Confere a netlist exportada do EasyEDA contra `ligacoes.json` e `pins.h`. Roda na CI |
| `esquematico.enet` | Netlist exportada do esquemático (formato do EasyEDA Pro), em 2026-11-26 |
| `fios.py` | Lê o Gerber exportado (zip) e lista os fios que faltam numa face; falha se houver curto ou trilha na camada de cima |
| `lista-de-pecas.tsv` | Lista de peças exportada do esquemático (separada por tabulação) |

Para conferir depois de mexer no esquemático: exportar a netlist (Exportar → Netlist, formato
EasyEDA Pro `.enet`) e rodar

```bash
python3 hardware/placa-principal/conferir_netlist.py placa-principal.enet
```

## 3. Conector para a placa de energia (J2)

Barra 1×8, 2,54 mm. A placa de energia ainda não tem peças compradas.

| Pino | Net | Direção (vista da principal) | GPIO | Para quê |
|---:|---|---|---:|---|
| 1 | `+5V` | entrada | — | Alimentação: vai ao pino 5V da DevKitC (J1-21), entrada do regulador de 3,3 V dela |
| 2 | `GND` | — | — | |
| 3 | `+3V3` | saída | — | 3,3 V do regulador da DevKitC, para pull-ups ou lógica da placa de energia |
| 4 | `POWER_BTN` | saída | 10 | O botão de energia (SW11) puxa ao GND. A placa de energia precisa ver o aperto para ligar |
| 5 | `POWER_HOLD` | saída | 2 | Trava: o firmware segura a alimentação ligada depois do aperto |
| 6 | `VBAT_SENSE` | entrada | 9 | Tensão da bateria pelo divisor 100k/100k, que fica na placa de energia (ADC1) |
| 7 | `CHG_STAT` | entrada | 8 | Estado do carregador (dreno aberto, ativo em baixo, hipótese). GPIO RTC: acorda o chip |
| 8 | `GND` | — | — | |

### Sem a placa de energia (como usar hoje)

- J2 fica vazio. A placa é alimentada pelo cabo USB da DevKitC, em qualquer uma das duas portas.
- O 3,3 V de todos os módulos sai do regulador da própria DevKitC (pino 3V3, J1-1).
- O amplificador fica sempre no 3,3 V (sem jumper, como na bancada). JP2 em **1-2**: luz de
  fundo da tela sempre acesa, como na bancada; em **2-3**, controlada pelo GPIO 14 (PWM).
- `POWER_HOLD`, `VBAT_SENSE` e `CHG_STAT` ficam sem nada do outro lado. O GPIO 9 lê um valor sem
  sentido (o firmware do perfil `target` mostra uma bateria qualquer). O botão de energia funciona:
  o GPIO 10 tem pull-up interno e acorda o chip do deep sleep.

## 4. Jumper de escolha

| Jumper | 1-2 | 2-3 | Por quê |
|---|---|---|---|
| JP2 | Luz de fundo no `+3V3` (sempre acesa) | Luz de fundo no GPIO 14 (PWM) | A corrente da luz de fundo da MSP3218 não foi conferida (MSP-05); na bancada ela está no 3,3 V |

O JP1 (amplificador no 5 V ou no 3,3 V) saiu em 2026-11-26: o MAX98357A fica fixo no 3,3 V, que
foi o validado na bancada.

## 5. Como a placa foi feita (layout em pé, descartado em 2026-11-26)

1. **Esquemático** pelo MCP, a partir de `ligacoes.json`, conferido por `conferir_netlist.py`.
2. **Posições e GPIOs** escolhidos para a placa de uma face precisar de poucos fios: um modelo
   simples (cruzamentos das linhas de ligação, sem rotear) comparou em pé e deitado e escolheu
   posição e rotação das peças pequenas, o mapa dos botões no MCP23017 e quais GPIOs da DevKitC
   ficam com cada sinal. Em pé ganhou nas duas restrições testadas (63 contra 237 com a tela no
   topo). Restrições fixas: tela encostada na borda de cima com a barra do SD à direita (o cartão
   sai por cima, pedido do Gabriel em 2026-11-26), DevKitC logo abaixo com a antena na borda
   esquerda, cruz e ABXY embaixo, IR e CC1101 em bordas com o módulo para fora, nada no caminho do
   cabo USB.
3. **Rota** pelo autoroteador do EasyEDA, só na camada de baixo, modo "Speed first" (o "Completion
   first" cruza nets para fechar 100%). O que ele não consegue ligar vira fio: `fios.py` lê o
   Gerber exportado e lista os fios em `fios.md`.

Placa de 110 × 185 mm (cabe numa placa virgem de 15 × 20 cm). Posições em mm, origem no canto de
baixo à esquerda, vista do lado das peças:

| Ref | Peça | Centro (x, y) | Rotação | Como |
|---|---|---|---:|---|
| J3, J4 | Tela (14 pinos) e SD (4 pinos) | (13, 154,5) e (97, 154,5) | 90 | Tela encostada no topo; nada embaixo dela além de trilhas |
| U1 | DevKitC | (37,67, 108) | 90 | Pino 1 em x = 11; antena na borda esquerda; USB para a direita |
| J10, J11 | IR RX e IR TX | (4, 86,8) e (4, 60,7) | 90 e 270 | Borda esquerda, módulos para fora |
| J5 | CC1101 | (105,05, 81,2) | 90 | Borda direita, SMA para fora |
| J7, J8 | INMP441 | (54,7, 69,4) e (54,7, 59,2) | 180 | |
| J9 | MAX98357A | (52,2, 76,2) | 180 | Alto-falante por fio no borne |
| J2 | Placa de energia | (74,6, 72,5) | 180 | Cabo até a placa de energia |
| JP2 | Luz de fundo | (92, 92) | 180 | Abaixo da DevKitC, fora do caminho do cabo USB |
| SW11 | Energia | (71,4, 82,2) | 0 | |
| SW9, SW10 | Select, Start | (35,8, 53,9) e (74,7, 54) | 180 | |
| SW1–SW4 | Cruz | centro (20, 32), botões a 10 mm | — | |
| SW5–SW8 | A, B, X, Y | centro (90, 32), botões a 10 mm | — | X em cima, A à direita |
| U2, C1 | MCP23017 e 100 nF | (59,4, 26,2) e (49,7, 31,2) | 90 e 180 | |
| R1, R2 | Pull-ups do I²C | (43,3, 57) e (26,3, 68,1) | 270 | |
| J6 | PN532 | (96,2, 61,2) | 180 | Cabo até o PN532 na traseira |

**Pads tirados das pegadas** (pinos sem ligação; na montagem, tirar o contato da barra fêmea ou
cortar a barra): U1 2, 3, 13, 14, 23, 24, 25, 26, 30, 31, 32, 33, 34, 35, 39, 42, 43; J3 10 a 14
(toque); J9 4 (GAIN, solto = 9 dB); J5 8 (GDO2).

## 6. Pendente antes de fabricar

| # | Item | Situação | Como fechar |
|---|---|---|---|
| 1 | Ordem dos pinos da DevKitC | **Fato:** o símbolo da LCSC (ESP32-S3-DEVKITC-1-N8R8, mesma placa e pinagem da N16R8) bate com o guia v1.1 da Espressif; `conferir_netlist.py` confere cada GPIO pelo nome do pino | — |
| 2 | Pegada da DevKitC | **Fato:** na pegada da LCSC (`CONN-TH_ESP32-S3-DEVKITC-1U-N8`) as fileiras ficam a 22,86 mm e os pinos 1 e 22 a 53,34 mm, como no footprint oficial da Espressif. Os pads livres saíram (seção 5) | Paquímetro na placa em mãos |
| 3 | Tela MSP3218 | **Fato (Gabriel, 2026-11-26):** barra do SD à direita, cartão saindo por cima. **Hipótese:** cerca de 84 mm entre as barras (placa de 89,3 mm, nominal) e o pino 1 das duas barras embaixo | Paquímetro e conferir o pino 1, antes de corroer |
| 4 | CC1101 E07-M1101D | **Fato:** pinos 1–4 pelo manual da Ebyte. **Hipótese:** 5–8 (fonte secundária) e ímpares numa fileira, pares na outra | Continuidade do pino 1 com a rosca da SMA |
| 5 | INMP441 | **Fato:** nomes pela serigrafia (L/R, WS, SCK; GND, VDD, SD). **Hipótese:** distância entre fileiras e quais pinos se encaram | Paquímetro |
| 6 | PN532 em I²C | **Hipótese:** ordem GND, VCC, SDA, SCL; chave em I²C (canal 1 ON, canal 2 OFF) | Ler a serigrafia; como vai por cabo, a ordem se ajusta no cabo |
| 7 | IR, MAX98357A, tela | **Fato:** ordem pela serigrafia usada na bancada (`BREADBOARD_MAP.md`) | — |
| 8 | Botões 6×6 | **Hipótese:** pinos 1 e 4 ficam em lados opostos da chave (diagonal), seja o par interno 1-3/2-4 ou 1-2/3-4 | Multímetro num botão comprado |
| 9 | 5 V da DevKitC | **Pendente:** o 5 V no pino J1-21 com o USB ligado (DKC-14) e se há diodo entre o VBUS e esse pino. Sem diodo, ligar USB e placa de energia juntos põe duas fontes em paralelo | Esquema da DevKitC v1.1 e multímetro |
| 10 | Regras do Tiago | **Hipótese conservadora** (Gabriel, 2026-11-25): trilha 0,8 mm, isolação 0,5 mm, ilha 2,0 mm, furo 1,0 mm, 1 mm de cobre até a borda | Perguntar ao Tiago |
| 11 | PN532 por cabo | **Decisão desta sessão, a confirmar:** encaixado na placa, o módulo cobriria a tela ou os botões, e a antena precisa ficar na traseira sem metal atrás (AD-03) | Gabriel |
| 12 | Barras na lista de peças | As barras da lista são machos da LCSC, escolhidas só pela pegada. Para os módulos, comprar **barras fêmea** de 2,54 mm; machos só em J2, JP1 e JP2 (com dois jumpers de encaixe) | — |
| 13 | `CHG_STAT` | **Hipótese:** dreno aberto, ativo em baixo, como os carregadores comuns. Sem pull-up na principal: o firmware liga o pull-up interno quando passar a ler o pino | Escolha do carregador da placa de energia |
| 14 | Antena do ESP32-S3 | **Pendente:** os 15 mm sem metal recomendados (`KO-ESP`) não cabem; a antena fica na borda esquerda, sem cobre embaixo | Teste de alcance do Wi-Fi com a placa montada |
| 15 | GPIOs novos | **Hipótese:** a troca de GPIOs de 2026-11-26 (README, Pinagem) segue as restrições do ESP32-S3 (ADC1 na bateria, RTC fora do strapping no botão, no carregador e na trava; SPI no IO_MUX), mas só o SPI, o microfone (7) e o IR RX (4) já rodaram nesses pinos | Testar cada módulo na placa montada |
| 16 | Corpo dos módulos | **Hipótese:** o MAX98357A (J9) e o INMP441 (J7/J8) ficam no meio da placa; o lado para onde o corpo de cada módulo se estende a partir da barra não foi medido | Encaixar os módulos de verdade sobre a impressão em papel 1:1 antes de corroer |
