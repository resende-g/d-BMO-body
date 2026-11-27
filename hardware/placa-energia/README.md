# Placa de energia do d-BMO (uma face)

| Campo | Valor |
|---|---|
| Versão | 0.2 (2026-11-27) |
| Para quem | Tiago Batista, que fabrica a placa em casa (corrosão de uma face) |
| Ferramenta | EasyEDA Pro, projeto `dbmo`, placa `energia` (Schematic2 + PCB2) |
| Tamanho | 60,1 × 39,8 mm, contorno do Gerber (limite do Tiago: 70 × 70 mm) |
| Fonte das ligações | [`ligacoes.json`](ligacoes.json) |
| Estado | **Desenhada e roteada, 5 fios (4 de verdade), sem curto** (`fios.py` sobre o Gerber exportado do EasyEDA). Nenhuma peça comprada, nada montado. Não corroer antes de fechar a seção 6 |

## 1. O que a placa faz

Liga a bateria ao d-BMO por um cabo de 7 vias a partir do J2 (seção 4):

1. **Carrega a bateria pelo USB da DevKitC.** Com o cabo USB ligado, o pino 5V da DevKitC tem a
   tensão do USB; ela chega pelo J2-1 ao carregador (TP4056). Não há um terceiro conector USB.
2. **Alimenta o aparelho pela bateria** quando o USB está desligado: bateria → trava (Q1) →
   diodo D2 → J2-1 → pino 5V da DevKitC → regulador de 3,3 V da própria DevKitC.
3. **Trava liga/desliga** com o botão de energia da placa de botões da esquerda e o `POWER_HOLD` do firmware.
4. **Leitura da bateria** pelo divisor 100k/100k (`VBAT_SENSE`, ×2 no firmware), depois da trava:
   com o aparelho desligado o divisor não gasta nada.
5. **Estado do carregador** pelo `CHRG` do TP4056 (dreno aberto, em nível baixo durante a carga),
   ligado ao `CHG_STAT` com o pull-up interno do GPIO.

Sem a placa de energia, o d-BMO funciona só pelo USB da DevKitC.

## 2. Por que essa arquitetura

| Escolha | Motivo | Custo |
|---|---|---|
| Bateria direto no pino 5V da DevKitC, sem regulador próprio | O regulador de 3,3 V da DevKitC já existe; um buck-boost (o que `docs/hardware/POWER_ARCHITECTURE.md` recomenda) só existe em QFN, que não dá para soldar numa placa corroída em casa | Abaixo de ~3,5 V na bateria, o 3,3 V cai junto (regulador em dropout + 0,3 V do D2). Perde-se a cauda de descarga, ~10 % da carga (hipótese) |
| TP4056 (ESOP-8) | Passo de 1,27 mm cabe na isolação de 0,5 mm; tem `CHRG` em dreno aberto e entrada de NTC | Sem power-path de verdade: com USB ligado o aparelho roda pelo USB (DevKitC) e a bateria só carrega, o que dá o mesmo efeito |
| D2 (Schottky) entre a trava e o 5V | Impede a bateria de empurrar corrente para o USB e o USB de carregar a bateria por fora do TP4056 (corpo do Q1) | 0,3 V a menos na tensão de entrada do regulador |
| Trava com P-MOSFET (Q1) + N-MOSFET (Q2) | Dois transistores SOT-23 e três resistores; o firmware já espera `POWER_HOLD` ativo em alto e botão ativo em baixo | O botão precisa ficar apertado até o firmware segurar a trava (hipótese: menos de 0,5 s) |
| D1 da **placa de botões da esquerda** (1N4148 entre `POWER_BTN` e `PWR_KEY`) | Sem ele, com o aparelho desligado o GPIO sem alimentação grampeia o nó do botão em ~0,6 V, o que parece um aperto e liga o Q1 sozinho | O GPIO vê ~0,5 V com o botão apertado (limite de nível baixo: 0,83 V) |

## 3. Peças

| Ref | Peça | Valor | Pacote | Lado | Para quê |
|---|---|---|---|---|---|
| U1 | TP4056 | — | ESOP-8 | cobre (SMD) | Carregador de 1 célula, 4,2 V |
| R1 | Resistor | 2,43 kΩ (MFR-25, E96) | PTH | peças | `PROG`: corrente de carga = 1200 / R1 ≈ 490 mA. Trocar para 1,2 kΩ (1 A) com carregador de parede |
| R2 | Resistor | 5,6 kΩ | PTH | peças | Com o NTC de 10 kΩ da bateria: carga só entre ~6 °C e ~45 °C |
| C1, C2 | Capacitor eletrolítico | 10 µF / 25 V | PTH (5 mm, passo 2,0) | peças | Entrada (5V) e saída (bateria) do TP4056. Hipótese: pino 1 = positivo, conferir a serigrafia |
| J1 | Bateria | Barra 1×3, 2,54 mm | PTH | peças | `BAT+`, `GND`, NTC. O JST-PH (passo 2,0 mm) deixa 0,4 mm entre os pads, abaixo da isolação de 0,5 mm: a bateria entra por adaptador JST-PH → 2,54 mm ou fios crimpados |
| JP1 | Jumper | 2 pinos | PTH | peças | Fechar só se a bateria não tiver NTC (desliga a proteção de temperatura) |
| Q1 | P-MOSFET AO3401A | −30 V, −4 A | SOT-23 | cobre (SMD) | Trava: liga a bateria ao sistema |
| Q2 | N-MOSFET BSS138 | 50 V | SOT-23 | cobre (SMD) | `POWER_HOLD` segura o Q1 ligado |
| R3, R4 | Resistor | 100 kΩ | PTH | peças | Q1 desligado por padrão; `POWER_HOLD` em baixo no reset |
| D1 | 1N4148 | — | DO-35 | peças | Botão de energia puxa o gate do Q1 |
| D2 | Schottky SS34 | 3 A | SMA | cobre (SMD) | Sistema → pino 5V da DevKitC, sem volta |
| R5, R6 | Resistor | 100 kΩ | PTH | peças | Divisor da bateria |
| C3 | Capacitor | 100 nF | PTH | peças | No pino do ADC (S3-ADC-10) |
| J2 | Barra 1×8 | 2,54 mm | PTH | peças | Cabo para a placa-base e a placa de botões |

## 4. Conector J2 (cabo até a placa-base e a placa de botões)

Desde 2026-11-27 a CPU é uma placa-base com barras de pinos (`../placa-principal/README.md`, seção 4):
cada via do J2 vai num fio Dupont até a coluna certa da placa-base, menos o `PWR_KEY`, que vai ao
JE-5 da placa de botões da esquerda (o botão de energia e o D1 ficam lá).

| Pino | Net | Na placa de energia | Vai para |
|---:|---|---|---|
| 1 | `+5V` | Entrada do TP4056 (vem do USB) e saída do D2 (vai para o regulador da DevKitC) | Placa-base JT·2 (pino 5V da DevKitC) |
| 2, 8 | `GND` | | Placa-base, qualquer GND (os dois fios juntam lá, por isso o fio W4 é dispensável) |
| 3 | — | Sem ligação | — |
| 4 | `PWR_KEY` | Botão de energia (puxa a GND); liga o Q1 pelo D1 | Placa de botões da esquerda, JE-5 |
| 5 | `POWER_HOLD` | Gate do Q2 | Placa-base JB·18 (GPIO 2) |
| 6 | `VBAT_SENSE` | Divisor R5/R6 | Placa-base JT·8 (GPIO 9) |
| 7 | `CHG_STAT` | `CHRG` do TP4056 | Placa-base JT·11 (GPIO 8) |

## 4.1 Rota e fios

Rota pelo Freerouting 2.5.0 só na camada de baixo (trilha 0,6 mm, mínimo 0,4 mm, isolação 0,5 mm),
importada no PCB2 pela API. Medido no Gerber exportado (`fios.md`): isolação mínima de 0,52 mm; toda
trilha com 0,6 mm, menos 38,9 mm com 0,45 mm (`GND`, `POWER_HOLD`, `PWR_KEY` e `TEMP`); cobre a
2,50 mm da borda no ponto mais perto (pad R3.1). `fios.md` lista 5 fios; o W4 (GND J2-2 ↔ J2-8) não precisa ser feito,
porque os dois GND do cabo se juntam na placa-base. O W1 (`BAT+`) e o fio do `CHG_STAT` passam por
cima do TP4056. Um fio é inevitável nesta topologia: o CE (pino 8) é `+5V` e fica na diagonal oposta
do VCC (pino 4), e com o NTC ligado qualquer volta em torno do chip tranca o `TEMP` ou o `CHRG`.

| Arquivo | O que é |
|---|---|
| `ligacoes.json` | Pino de cada peça → net |
| `esquematico.enet` | Netlist do Schematic2 (conferida: `../placa-principal/conferir_netlist.py esquematico.enet ligacoes.json`) |
| `gerber.zip` | Gerber do PCB2 (SMD na camada de baixo, junto com o cobre) |
| `cobre-espelhada.png` | Para o Tiago gravar (lado do cobre visto de frente, 1:1, 600 dpi) |
| `cobre-vista-de-cima.png` | Como o EasyEDA mostra |
| `lista-de-pecas.tsv` | Lista de peças exportada do PCB |
| `fios.md` | Fios, isolação, borda e trilhas finas (`../placa-principal/fios.py`) |

## 5. Como funciona

- **Ligar:** o botão puxa `PWR_KEY` a GND; pelo D1 o gate do Q1 desce e a bateria chega ao
  regulador da DevKitC. O firmware sobe, põe `POWER_HOLD` em alto (`PowerSubsystem::init()`,
  primeira coisa) e o Q2 passa a segurar o gate do Q1. O botão pode ser solto.
- **Desligar:** toque longo; o firmware espera o botão ser solto e põe `POWER_HOLD` em baixo. O
  R3 fecha o Q1. Corrente desligado: fuga do Q1 e do TP4056 (hipótese: poucos µA).
- **USB ligado:** a DevKitC roda pelo USB mesmo com a trava aberta; o D2 fica reverso. O TP4056
  carrega a bateria pelo J2-1.
- **Sem USB:** a entrada do TP4056 fica com a tensão da bateria menos o D2, abaixo da bateria, e
  ele dorme (não descarrega a bateria).

## 6. Pendente antes de fabricar

| # | Item | Situação | Como fechar |
|---|---|---|---|
| 1 | Pino 5V da DevKitC com USB ligado | **Hipótese:** tem a tensão do USB (direto ou por diodo), o que permite carregar pelo USB da DevKitC. `DKC-13` só diz que as formas de alimentar são "mutuamente exclusivas" | Multímetro no pino 5V com o USB ligado e nada mais; se não tiver tensão, a carga precisa de um USB próprio na placa de energia |
| 2 | Regulador da DevKitC em dropout | **Hipótese:** com 3,3–3,5 V na entrada ele entrega perto de 3,0–3,2 V (modelo não verificado, `DKC-12`) | Fonte de bancada no pino 5V de 3,0 a 4,2 V, medindo o 3V3 com o rádio ligado. Ajustar os limiares de bateria do firmware (hoje 3500/3300 mV) |
| 3 | Tempo do botão para ligar | **Hipótese:** o firmware segura a trava em menos de 0,5 s | Medir na placa montada |
| 4 | Pinos da bateria e NTC | **Pendente:** ordem dos fios do JST e o B do NTC (3435 ou 3950) | Ver a bateria comprada; trocar R2 se a janela de temperatura sair de 0–45 °C |
| 5 | Dissipação do TP4056 | **Hipótese:** a 500 mA a ilha térmica sem solda basta (o CI reduz a corrente sozinho acima de ~120 °C) | Se esquentar demais, furo na ilha para soldar por baixo |
| 6 | Regras do Tiago | **Decisão do desenho:** trilha de 0,6 mm (mínimo 0,4 mm) e isolação de 0,5 mm nesta placa. **Fato (`fios.md`):** 0,6 mm em quase tudo, 0,45 mm em 38,9 mm, isolação mínima de 0,52 mm. Com 0,8 mm o autoroteador não sai dos pinos do TP4056 (ESOP-8, passo 1,27 mm, pads de 0,6 mm) nem dos SOT-23 | Perguntar ao Tiago se 0,6 mm (e 0,45 mm junto do TP4056) sai na corrosão dele |
| 7 | Pino 6 do TP4056 | **Decisão do desenho:** o pad do STDBY (sem ligação) sai da pegada, para abrir passagem entre os pinos 5 e 7 | Editar a pegada no EasyEDA (Gabriel) |
| 8 | Corrente com USB de computador | **Hipótese:** carga de 500 mA mais o aparelho passa dos 500 mA de uma porta USB 2.0 | Carregar com carregador de parede ou trocar R1 por 3 kΩ (400 mA) |
| 9 | EN da DevKitC | **Pendente:** supervisor de tensão no EN (`S3-PWR-10`) não foi incluído | Avaliar depois das medições do item 2 |
| 10 | Borda | **Fechado em 2026-11-27:** o contorno antigo (57,5 × 36,4 mm) deixava o cobre a 1,05 mm da borda. O Gabriel redesenhou o contorno do PCB2 (60,1 × 39,8 mm); agora o cobre fica a 2,50 mm da borda nos lados e a 4,2 mm embaixo | — |
| 11 | Fios | **Fato:** 5 fios (`fios.md`), sendo o W4 (`GND` J2-2 ↔ J2-8) dispensável | Soldar W1, W2, W3 e W5 na montagem |
