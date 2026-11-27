# Placa de botões da esquerda (cruz, Select/Start, energia e MCP23017)

| Campo | Valor |
|---|---|
| Versão | 0.1 (2026-11-27) |
| Para quem | Tiago Batista (laser + percloreto, uma face) |
| Ferramenta | EasyEDA Pro, projeto `dbmo`, placa `botoes-esquerda` (Schematic3 + PCB3) |
| Tamanho | 52 × 64 mm |
| Estado | **Desenhada e roteada: 4 fios, sem curto** (`fios.py` sobre o Gerber exportado do EasyEDA). Nenhuma peça comprada, nada montado |

## 1. O que leva

- Cruz (SW1 ↑, SW2 ↓, SW3 ←, SW4 →, a 11 mm do centro), Select (SW9), Start (SW10) e energia (SW11).
- **MCP23017** (U2, DIP-28, 0x20: A0–A2 no GND, RESET no 3V3) com C1 (100 nF) e os pull-ups do I²C
  (R1 no SDA, R2 no SCL, 4,7 kΩ). Recebe os 4 botões da placa da direita pelo JD.
- **D1** (1N4148, anodo em `POWER_BTN`, catodo em `PWR_KEY`): o botão puxa `PWR_KEY` a GND, a
  placa de energia vê o aperto e liga; o GPIO 10 lê o botão pelo D1. Com o aparelho desligado, o D1
  impede que o GPIO sem alimentação grampeie o nó do botão (ligaria sozinho).

Saiu da placa-base em 2026-11-27: com o MCP aqui, a placa-base só manda I²C, 3V3, GND e o botão de
energia (6 vias) em vez das 13 linhas de botão.

## 2. Mapa dos botões no MCP23017

Escolhido em 2026-11-27 para a placa rotear com menos fios; é o `mcp_buttons` de `pins.h`.

| Botão | Pino do MCP23017 | Porta | Bit |
|---|---:|---|---:|
| → | 21 | GPA0 | 0 |
| Start | 22 | GPA1 | 1 |
| ↓ | 25 | GPA4 | 4 |
| Select | 26 | GPA5 | 5 |
| ← | 27 | GPA6 | 6 |
| Y | 1 | GPB0 | 8 |
| A | 3 | GPB2 | 10 |
| B | 4 | GPB3 | 11 |
| X | 5 | GPB4 | 12 |
| ↑ | 7 | GPB6 | 14 |

## 3. Conectores

| JE (para a placa-base e a de energia) | Net | Vai para |
|---:|---|---|
| 1 | `I2C_SDA` | Placa-base JT·17 |
| 2 | `I2C_SCL` | Placa-base JB·19 |
| 3 | `+3V3` | Placa-base, 3V3 (J12) |
| 4 | `GND` | Placa-base, GND (J13) |
| 5 | `PWR_KEY` | Placa de energia, J2-4 |
| 6 | `POWER_BTN` | Placa-base JT·7 (GPIO 10) |

| JD (daqui) | Net | JD da placa da direita |
|---:|---|---:|
| 1 | `BTN_B` | 2 |
| 2 | `GND` | 5 |
| 3 | `BTN_X` | 3 |
| 4 | `BTN_Y` | 4 |
| 5 | `BTN_A` | 1 |

As duas pontas do cabo do JD têm ordens diferentes (cada placa ficou na ordem que rota melhor): o
cabo é feito com fios Dupont soltos, pino a pino, pela tabela acima.

## 4. Rota e fios

Rota pelo Freerouting 2.5.0 só na camada de baixo (trilha 0,8 mm, isolação 0,5 mm, 2 mm de borda),
importada no PCB3 pela API. Medido no Gerber exportado (`fios.md`):

| Medida | Valor |
|---|---|
| Fios | 4 (95 mm no total): W1 `+3V3` C1.1–U2.18, W2 `BTN_UP` SW1.1–U2.7, W3 `BTN_X` JD.3–U2.5, W4 `PWR_KEY` D1.2–JE.5 |
| Isolação mínima | 0,52 mm (trilhas `BTN_B` × `BTN_A`) |
| Cobre mais perto da borda | 1,94 mm (trilha `BTN_Y` e pad JE.1) |
| Trilhas abaixo de 0,8 mm | 2 lugares: `BTN_Y` 0,6 mm × 28,1 mm e `GND` 0,4 mm × 1,4 mm (seção 6, item 6) |

O Freerouting estreitou 22 trechos (0,4 a 0,6 mm), quase todos na saída dos pads. Em 2026-11-27,
19 deles foram alargados para 0,8 mm no PCB3 pela API, onde a isolação continuava em 0,5 mm ou mais;
os 3 que sobram (o de `BTN_Y` e dois seguidos de `GND`) não cabem.

## 5. Arquivos

| Arquivo | O que é |
|---|---|
| `ligacoes.json` | Pino de cada peça → net |
| `esquematico.enet` | Netlist do Schematic3, conferida na CI por `../placa-principal/conferir_netlist.py`: 51 pinos, nomes dos pinos do MCP23017 e o mapa dos botões contra `mcp_buttons` de `pins.h`, 0 divergências |
| `gerber.zip` | Gerber do PCB3 (cobre só na camada de baixo) |
| `cobre-espelhada.png` / `.svg` | Para o Tiago gravar (lado do cobre visto de frente, 1:1, 600 dpi) |
| `cobre-vista-de-cima.png` / `.svg` | Como o EasyEDA mostra |
| `lista-de-pecas.tsv` | Lista de peças exportada do PCB3 (14 peças, códigos LCSC) |
| `fios.md` | Fios, isolação, borda e trilhas finas (`../placa-principal/fios.py`) |

## 6. Pendente antes de fabricar

| # | Item | Situação | Como fechar |
|---|---|---|---|
| 1 | Botões 6 × 6 | **Hipótese:** pinos 1 e 4 em lados opostos da chave (diagonal) | Multímetro num botão comprado |
| 2 | MCP23017 na frente | **Hipótese:** o DIP-28 (com soquete) fica entre os botões, do lado das peças, abaixo da cruz; a tampa precisa de folga para ele | Conferir na caixa |
| 3 | Conectores na frente | **Hipótese:** JE e JD (barras retas) ficam do lado das peças; os cabos saem pelas bordas | Conferir na caixa; se atrapalhar, barra em 90° |
| 4 | D1 | **Fato:** 1N4148 com o pino 1 = anodo na peça da LCSC (C14516). **Hipótese:** o GPIO lê ~0,5 V com o botão apertado (limite de nível baixo 0,83 V) | Medir na montagem |
| 5 | Regras do Tiago | **Fato:** trilha de 0,8 mm (menos o item 6), isolação mínima de 0,52 mm, cobre a 1,94 mm da borda no ponto mais perto. **Hipótese:** isolação de 0,5 mm basta para o laser dele | Primeira placa gravada |
| 6 | Trilhas finas | **Fato:** `BTN_Y` com 0,6 mm por 28,1 mm, rente à borda esquerda da vista de cima (x = 2,3 mm, de y = 7,7 a 35,8 mm); a 0,8 mm a isolação até a trilha `BTN_A` cairia para 0,42 mm e a borda para 1,9 mm. `GND` com 0,4 mm por 1,4 mm junto do R2.2 (`+3V3`); a 0,8 mm a isolação cairia para 0,35 mm | Perguntar ao Tiago se 0,6 e 0,4 mm saem na corrosão dele; se não, rerrotear esses dois trechos (pode custar um fio) |
| 7 | Fios | **Fato:** 4 fios (W1–W4, `fios.md`) | Soldar na montagem |
