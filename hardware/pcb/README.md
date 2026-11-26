# Placa principal do d-BMO (uma face)

| Campo | Valor |
|---|---|
| Versão | 0.1 (2026-11-25) |
| Para quem | Tiago Batista, que fabrica a placa em casa (corrosão de uma face) e ensina EasyEDA |
| Fonte das ligações | `firmware/config/include/dbmo/config/pins.h`, perfil `target` |
| Ferramenta | EasyEDA Pro (decisão do Gabriel em 2026-11-25), pelo MCP `easyeda-pro` |
| Estado | Pinagem, ligações e lista de peças prontas. Esquemático e placa ainda não desenhados. Posições e regras de fabricação SUPOSTAS (seções 4 e 5) |

## 1. Escopo (decisões do Gabriel em 2026-11-25)

- O d-BMO passa a ter **duas placas**: esta, a **principal**, e uma **placa de energia** futura
  (bateria, carga e BMS, regulador, power-latch), que fica para quando as peças forem compradas.
  As duas se ligam pelo conector **CN1** (seção 3).
- A principal é de **uma face** (cobre só embaixo, limitação do Tiago), com tudo encaixado nela
  em barras fêmea: ESP32-S3 DevKitC-1 N16R8 inteira, tela MSP3218 (ILI9341 + SD), CC1101 (Ebyte
  E07-M1101D, barra 2×4), receptor e transmissor de IR (placas de 3 pinos), INMP441, MAX98357A (o
  alto-falante vai no borne da própria placa dele), PN532 (I²C) e os botões.
- **Sem** cartucho (os GPIOs 38, 43 e 44 ficaram livres), analógicos e gatilhos.
- Os sinais de energia **continuam reservados** e vão ao CN1: `VBAT_SENSE` (GPIO 1, ADC1),
  `POWER_HOLD` (GPIO 48) e, novo, `CHARGER_STATUS` (GPIO 4).
- Botões: cruz, A, B, X, Y, start e select num **MCP23017 em 0x20** (sem ressalva sobram só os
  GPIOs 5, 6 e 38, pouco para 10 botões; o driver existe no firmware próprio, no Retro-Go e no
  Bruce). O botão de energia fica **na principal**, no GPIO 2, e o mesmo nó vai ao CN1 para a
  placa de energia usar no power-latch.

## 2. Arquivos

| Arquivo | O que é |
|---|---|
| [`gerar_ligacoes.py`](gerar_ligacoes.py) | Gera os dois arquivos abaixo a partir de `pins.h` |
| [`LIGACOES.md`](LIGACOES.md) | Lista de ligações (netlist), CN1, lista de peças, posições sugeridas |
| [`ligacoes.json`](ligacoes.json) | O mesmo, para ferramentas: a sessão que desenha no EasyEDA lê daqui |

```bash
python3 hardware/pcb/gerar_ligacoes.py
```

A CI roda `python3 hardware/pcb/gerar_ligacoes.py --check`: se `pins.h` mudar e a lista de
ligações não for regenerada, falha.

## 3. Conector da placa de energia (CN1)

Barra macho 1×8 de 2,54 mm na principal; o par fêmea fica na placa de energia. GND nas duas
pontas: um conector encaixado ao contrário não põe alimentação no lugar do GND.

| Pino | Sinal | Para quê |
|---:|---|---|
| 1 | GND | |
| 2 | +5V | O nó do pino 5V da DevKitC (J1-21). Por aqui a placa de energia pode alimentar a DevKitC |
| 3 | +3V3 | O 3,3 V da DevKitC (J1-1). Alternativa de alimentação, ou só referência para a placa de energia |
| 4 | VBAT_SENSE | GPIO 1 (ADC1). O divisor 100k/100k fica na placa de energia |
| 5 | POWER_HOLD | GPIO 48. Segura o power-latch depois do clique no botão |
| 6 | POWER_BUTTON | O nó do botão SW11 e do GPIO 2. A placa de energia o lê (com diodo do lado dela) para ligar o latch |
| 7 | CHARGER_STATUS | GPIO 4. CHRG/STDBY do carregador, em coletor aberto (pull-up interno do ESP32-S3) |
| 8 | GND | |

**Sem a placa de energia** (CN1 em aberto), a principal funciona pelo USB da DevKitC: o 5 V do
USB e o regulador de 3,3 V da própria DevKitC alimentam tudo, o botão de energia continua
acordando o chip pelo GPIO 2, `VBAT_SENSE` fica solto (a leitura da bateria não vale nada até o
firmware saber que não há bateria) e `CHARGER_STATUS` lê nível alto. Nenhum jumper é necessário.

Pendente, para quando a placa de energia for projetada (hipóteses, sem fonte conferida):

- Por qual pino ela alimenta a DevKitC. O guia da DevKitC diz que USB, pino 5V e pino 3V3 são
  formas de alimentação "mutually exclusive" (DKC-13); o que acontece com o USB ligado ao mesmo
  tempo não foi conferido (DKC-14). A placa de energia precisa impedir a corrente de voltar para
  o USB (diodo ou chave de caminho de energia).
- A corrente que o regulador de 3,3 V da DevKitC aguenta com tela, CC1101 e PN532 (DKC-12).
- Como o latch lê o botão sem alimentar o GPIO 2 com o chip desligado.

## 4. Regras de fabricação (SUPOSTAS)

O Gabriel não sabia as regras do Tiago em 2026-11-25 e pediu um padrão conservador de corrosão
caseira. **Confirmar cada linha com o Tiago** e configurar no DRC do EasyEDA.

| Regra | Valor |
|---|---|
| Camadas de cobre | 1 (embaixo). Componentes PTH do lado de cima |
| Largura de trilha | 0,8 mm (sinal e alimentação) |
| Isolação entre cobres | 0,5 mm |
| Ilha (pad) | 2,0 mm, redonda |
| Furo | 0,9 mm (barras e CI), 1,0 mm (pernas dos botões), 0,8 mm (jumpers) |
| Cobre até a borda | 1,0 mm |
| Placa | 100 × 176 mm sugeridos (cabe numa placa virgem de 15 × 20 cm) |
| Jumpers | Fio rígido por cima da placa, onde as trilhas se cruzarem; soldados antes dos módulos |

Duas lições do primeiro roteamento, feito por script antes da troca para o EasyEDA (o script
foi descartado): com a DevKitC encaixada inteira, cada pino da barra é uma ilha que bloqueia a
passagem, e o roteamento chegou a 60 jumpers. Para reduzir:

1. **Pino sem ligação não precisa de ilha.** Nas barras fêmea da DevKitC, da tela e do
   MAX98357A, tirar o contato das posições sem ligação (lista em [`LIGACOES.md`](LIGACOES.md)) ou
   cortar a barra em pedaços; no EasyEDA, usar um footprint sem esses pads. Pelos vãos passam
   trilhas. O MCP23017 fica com os 28 pinos, por causa do soquete.
2. **Botão lido em diagonal.** Sinal numa perna e GND na perna da diagonal oposta: ficam sempre
   em lados opostos da chave, qualquer que seja a posição do botão. Vale qualquer uma das duas
   diagonais; escolher a que encurta a trilha.

Ordem de montagem: jumpers, soquete do MCP23017, resistores e capacitor, botões, barras fêmea e
macho; os módulos por último.

## 5. Pendente antes de fabricar

Fato com fonte, hipótese e pendente, item por item.

| # | Item | Situação | Como fechar |
|---|---|---|---|
| 1 | Ordem dos pinos J1 e J3 da DevKitC | **Fato:** guia do usuário v1.1 da Espressif, tabelas J1 e J3 (DKC-07, DKC-18) | — |
| 2 | Lado do J1 (à esquerda, vista de cima com a antena para cima) | **Hipótese** (figura de pinagem v1.1) | Ler a serigrafia ao lado de cada pino na placa |
| 3 | Distância entre J1 e J3 (22,86 mm), do pino 1 à ponta da antena e comprimento da DevKitC | 22,86 vem de `docs/mechanical`; o resto é SUPOSTO (DKC-20) | Paquímetro |
| 4 | Pinagem do MCP23017 em DIP-28 | **Fato:** DS20001952D, Table 2-1, p. 11 | — |
| 5 | Tela MSP3218: tamanho da placa, lado do conector de 14 pinos com o rosto em pé, pino 1 (VCC) em cima, posição dos 4 pinos do SD | **Hipótese.** Os nomes dos 14 pinos são os da serigrafia usada na bancada; lado, sentido e posição do SD são SUPOSTOS | Na bancada, com o rosto em pé: de que lado fica o conector? Medir a placa e a posição do SD |
| 6 | CC1101 E07-M1101D: pinos 1–4 | **Fato:** manual da Ebyte (1 GND, 2 VCC, 3 GDO0, 4 CSN) | — |
| 7 | CC1101: pinos 5–8 e arranjo da barra 2×4 (ímpares numa fileira, pares na outra) | 5–8 só por fonte secundária (ficha da LCSC); arranjo SUPOSTO | Pino 1 = GND por continuidade com a rosca da SMA (teste CC do `BREADBOARD_MAP.md`); conferir onde ficam 2 e 3 |
| 8 | INMP441: distância entre as fileiras (10,16 mm) e quais pinos ficam frente a frente | Nomes pela serigrafia (fato); geometria SUPOSTA | Paquímetro |
| 9 | PN532: ordem dos 4 pinos de I²C | **Hipótese** (GND, VCC, SDA, SCL). O manual da Elechouse só diz que os nomes estão impressos na frente | Ler a serigrafia. A chave fica em I²C: canal 1 ON, canal 2 OFF (ELE-06). A barra de 4 pinos sozinha não segura o módulo: prever espaçador nos furos dele |
| 10 | IR: ordem dos pinos | **Fato:** serigrafia (OUT, VCC, GND; DAT, VCC, GND) | Conferir se o LED e o receptor ficam para fora da borda |
| 11 | MAX98357A: ordem dos pinos | **Fato:** serigrafia (LRC, BCLK, DIN, GAIN, SD, GND, Vin) | Medir o tamanho da placa |
| 12 | Botões 6 × 6 mm com pernas a 6,5 × 4,5 mm | **Hipótese** (padrão do formato) | Conferir no botão comprado |
| 13 | Regras do Tiago | SUPOSTAS (seção 4) | Perguntar ao Tiago |
| 14 | 5 V no J1-21 com o USB ligado | Não conferido (DKC-14) | JP1 liga o MAX98357A no 5 V ou no 3V3; medir na bancada |
| 15 | Luz de fundo da tela no GPIO 16 | Corrente e circuito não conferidos (MSP-05) | JP2: sempre acesa (3V3) ou pelo GPIO 16 |
| 16 | Antena do ESP32-S3 | Sem cobre entre as fileiras até o pino 4 é o mínimo; o recomendado (15 mm em volta, `KO-ESP`) não cabe | Testar o alcance do Wi-Fi com a placa montada |
| 17 | Placa de energia | Não projetada | Seção 3 |

Peças a comprar: MCP23017-E/SP (DIP-28) e soquete, 11 botões 6 × 6 mm, barras fêmea e macho e 2
jumpers de encaixe (quantidades em [`LIGACOES.md`](LIGACOES.md)).
