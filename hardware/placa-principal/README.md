# Placa principal do d-BMO (uma face)

| Campo | Valor |
|---|---|
| Versão | 0.1 (2026-11-26) |
| Para quem | Tiago Batista, que fabrica a placa em casa (corrosão de uma face) |
| Ferramenta | EasyEDA Pro, projeto `dbmo` (esquemático e placa desenhados pelo MCP `easyeda-pro`) |
| Fonte das ligações | [`ligacoes.json`](ligacoes.json), conferido contra `firmware/config/include/dbmo/config/pins.h` (perfil `target`) por [`conferir_netlist.py`](conferir_netlist.py) |
| Estado | **Esquemático pronto e conferido (129 ligações, 0 divergências). Placa ainda não criada:** a API do EasyEDA Pro conectado não cria o documento de PCB (seção 5). Não corroer antes de fechar a seção 6 |

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
| 4 | `POWER_BTN` | saída | 2 | O botão de energia (SW11) puxa ao GND. A placa de energia precisa ver o aperto para ligar |
| 5 | `POWER_HOLD` | saída | 48 | Trava: o firmware segura a alimentação ligada depois do aperto |
| 6 | `VBAT_SENSE` | entrada | 1 | Tensão da bateria pelo divisor 100k/100k, que fica na placa de energia (ADC1) |
| 7 | `CHG_STAT` | entrada | 6 | Estado do carregador (dreno aberto, ativo em baixo, hipótese). GPIO RTC: acorda o chip |
| 8 | `GND` | — | — | |

### Sem a placa de energia (como usar hoje)

- J2 fica vazio. A placa é alimentada pelo cabo USB da DevKitC, em qualquer uma das duas portas.
- O 3,3 V de todos os módulos sai do regulador da própria DevKitC (pino 3V3, J1-1).
- JP1 em **2-3**: amplificador no 3,3 V, como na bancada. JP2 em **1-2**: luz de fundo da tela
  sempre acesa, como na bancada.
- `POWER_HOLD`, `VBAT_SENSE` e `CHG_STAT` ficam sem nada do outro lado. O GPIO 1 lê um valor sem
  sentido (o firmware do perfil `target` mostra uma bateria qualquer). O botão de energia funciona:
  o GPIO 2 tem pull-up interno e acorda o chip do deep sleep.

## 4. Jumpers de escolha

| Jumper | 1-2 | 2-3 | Por quê |
|---|---|---|---|
| JP1 | MAX98357A no `+5V` | MAX98357A no `+3V3` | Na bancada o amplificador rodou no 3,3 V. O 5 V dá mais volume, mas o 5 V no pino J1-21 com o USB ligado não foi conferido em fonte (DKC-14) |
| JP2 | Luz de fundo no `+3V3` (sempre acesa) | Luz de fundo no GPIO 16 (PWM) | A corrente da luz de fundo da MSP3218 não foi conferida (MSP-05); na bancada ela está no 3,3 V |

## 5. O que falta para a placa (bloqueado num clique)

A extensão do MCP (0.5.1) com o EasyEDA Pro 3.2 não cria o documento de PCB
(`pcb_Document.createBoard` indisponível; `createPcbFromSchematic` devolveu
`capability_unavailable`). Falta um clique humano:

1. No EasyEDA Pro, projeto `dbmo`: **Design → Converter esquemático em PCB** (ou *Atualizar PCB*),
   1 camada de cobre, e aceitar as mudanças. Se estiver aberta, fechar a janela de exportação de
   PDF que o MCP pediu (a exportação do PDF do esquemático expirou esperando por ela).
2. Retomar a sessão do Claude que desenhou o esquemático. Com a placa criada, tudo o resto passa
   pelo MCP: contorno, posições da tabela abaixo, regras da seção 6, autoroteamento só na camada
   de baixo, DRC e exportação de Gerber, lista de peças e netlist da placa para esta pasta.

Posições planejadas, em mm, origem no canto de cima à esquerda, vista do lado das peças (frente).
Placa de 100 × 175 mm (cabe numa placa virgem de 15 × 20 cm). As zonas seguem a ordem de
`docs/mechanical/ASSEMBLY_LAYOUT.md`: topo, placa principal, tela, controles, alto-falante.

| Ref | Peça | Centro (x, y) | Como |
|---|---|---|---|
| J10, J11 | IR RX e IR TX | (10, 4) e (22, 4) | Barras na borda de cima; os módulos ficam em balanço para fora |
| SW11 | Energia | (36, 8) | No topo, longe dos botões de jogo |
| J7, J8 | INMP441 | (52, 5) e (52, 15) | Duas barras de 3 frente a frente (distância é hipótese, seção 6) |
| J5 | CC1101 | (84, 10) | SMA para fora pela borda de cima |
| U1 | DevKitC | fileiras em y = 32 e y = 54,86; pino 1 em x = 10 | Antena na borda esquerda, sem cobre embaixo; USB para a direita |
| J2 | Placa de energia | x = 96, y de 28 a 46 | Vertical, na borda direita |
| JP2 | Luz de fundo | (88, 58) | Fora da sombra da tela, para dar para trocar o jumper |
| J3, J4 | Tela (14 pinos) e SD (4 pinos) | x = 92 e x = 8, centro em y = 93 | Tela deitada; nada embaixo dela além de trilhas |
| SW9, SW10 | Select, Start | (40, 128), (60, 128) | |
| SW1–SW4 | Cruz | centro (18, 148), botões a 10 mm | Cima, baixo, esquerda, direita |
| SW5–SW8 | A, B, X, Y | centro (82, 148), botões a 10 mm | X em cima, A à direita, B embaixo, Y à esquerda |
| U2, R1, R2, C1 | MCP23017 e passivos | U2 vertical em (50, 150); R1, R2 em (37, 146) e (41, 146); C1 junto do VDD | |
| J6 | PN532 | (60, 170) | Conector de cabo: o PN532 vai na traseira (seção 6, item 11) |
| J9, JP1 | MAX98357A e jumper | (12, 170) e (28, 170) | Módulo em balanço para fora da borda de baixo; alto-falante no borne dele |

## 6. Pendente antes de fabricar

| # | Item | Situação | Como fechar |
|---|---|---|---|
| 1 | Ordem dos pinos da DevKitC | **Fato:** o símbolo da LCSC (ESP32-S3-DEVKITC-1-N8R8, mesma placa e pinagem da N16R8) bate com o guia v1.1 da Espressif; `conferir_netlist.py` confere cada GPIO pelo nome do pino | — |
| 2 | Pegada da DevKitC | **Pendente.** A pegada da LCSC chama `CONN-TH_ESP32-S3-DEVKITC-1U-N8`; a distância entre fileiras precisa dar 22,86 mm (`COMPONENT_DIMENSIONS.md`) | Conferir os pads quando a placa existir e com paquímetro na placa em mãos |
| 3 | Distância entre a barra de 14 pinos e a do SD na MSP3218 | **Hipótese:** cerca de 84 mm (placa de 89,3 mm, nominal) | Paquímetro, antes de corroer |
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
