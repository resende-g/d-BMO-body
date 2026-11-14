<!--
SPDX-FileCopyrightText: 2026 Gabriel Almeida Resende
SPDX-License-Identifier: Apache-2.0
-->

# d-BMO-body v1 — Especificação mecânica (preliminar)

| Campo | Valor |
|---|---|
| Status | Preliminar. Prepara a modelagem da carcaça no FreeCAD; **não é um projeto de carcaça** |
| Data | 2026-11-12 (revisão 0.2) |
| Plataforma | ESP32-S3 N16R8 (definitiva para o v1) |
| Documentos irmãos | [COMPONENT_DIMENSIONS.md](COMPONENT_DIMENSIONS.md) (registro de dimensões), [ASSEMBLY_LAYOUT.md](ASSEMBLY_LAYOUT.md) (zonas e empilhamento), [`mechanical/README.md`](../../mechanical/README.md) (CAD paramétrico) |

## 1. Objetivo e limites deste documento

Este documento reúne, num só lugar, o que já está decidido sobre a mecânica do d-BMO-body v1 e o
que ainda falta medir ou decidir antes de modelar a carcaça. Ele:

- rastreia cada requisito até o `README.md` deste repositório ou até a especificação de marca;
- define **um** referencial (origem, eixos, planos de datum) usado por todos os documentos e pelo CAD;
- separa restrições conhecidas de restrições desconhecidas;
- lista as decisões em aberto e as inconsistências encontradas nas fontes.

Ele **não**:

- define dimensões externas da carcaça (largura, altura, espessura, raios): esses valores não
  existem ainda e aparecem aqui só como parâmetros nomeados, sem número;
- cria geometria funcional nova (posição de furos, bosses, encaixes). A seção 6 da especificação do
  personagem proíbe modificar ou inventar geometria funcional sem instrução e validação do
  mantenedor, e a seção 8 proíbe inferir essas informações de uma prancha de identidade;
- substitui o `README.md`, que continua sendo a fonte das decisões de projeto.

## 2. Fontes e convenção de evidência

### 2.1 Documentos de entrada

| Sigla | Documento | Natureza |
|---|---|---|
| **R** | `README.md` deste repositório (seções *Carcaça 3D*, *Carcaça v0*, *Especificação do cartucho*, *Dimensões conhecidas*, *Analógicos e gatilhos reaproveitados*, *Riscos da bateria e recomendações*, *Lista de componentes*, *Compras em fases*) | Decisões do projeto (fonte primária do projeto) |
| **M** | `docs/brand/CHARACTER_SPECIFICATION.md`, versão 2.0, do repositório `resende-g/d-BMO` (lido em cópia local nesta sessão) | Especificação visual do personagem (normativa para a identidade) |
| **T** | `docs/brand/DESIGN_TOKENS.md`, versão 1.0, do mesmo repositório | Tokens de cor (normativos para a paleta) |
| **F** | Documentos de fabricante abertos nesta sessão (lista na seção 2 de [COMPONENT_DIMENSIONS.md](COMPONENT_DIMENSIONS.md)) | Fonte primária externa |

### 2.2 Marcação usada no texto

- `[R: seção]`, `[M: §n]`, `[T: §n]`, `[F: id]` — afirmação tirada da fonte indicada.
- **(derivado)** — consequência direta de uma ou mais afirmações citadas; o raciocínio está escrito.
- **(interpretação)** — leitura deste documento sobre um texto ambíguo; pode ser revista pelo mantenedor.
- **(hipótese)** — suposição ainda sem evidência; precisa de teste ou de fonte.
- **não verificado nesta sessão** — informação conhecida de uso geral, mas sem fonte primária aberta
  nesta sessão; não usar como base de projeto sem conferir.

Os STATUS de dimensão (`CONFIRMADO`, `NOMINAL`, `PENDENTE`) estão definidos em
[COMPONENT_DIMENSIONS.md](COMPONENT_DIMENSIONS.md), seção 1.

## 3. Referencial comum (RC-dBMO)

Definido aqui uma única vez. [ASSEMBLY_LAYOUT.md](ASSEMBLY_LAYOUT.md) e o CAD usam exatamente este referencial.

### 3.1 Planos de datum, origem e eixos

Posição de referência: aparelho na vertical, face frontal (tela) voltada para o usuário, borda do
cartucho para cima.

| Elemento | Definição | Equação |
|---|---|---|
| Datum **A** (primário) | Plano da face frontal externa: o plano mais externo da frente, onde ficam a moldura da tela e os controles | Z = 0 |
| Datum **B** (secundário) | Plano médio vertical do corpo, que divide a silhueta em metade esquerda e metade direita | X = 0 |
| Datum **C** (terciário) | Plano da face inferior externa (a base, oposta à borda do cartucho) | Y = 0 |
| Origem **O** | Interseção de A, B e C: centro da aresta inferior da face frontal | (0, 0, 0) |

| Eixo | Sentido positivo |
|---|---|
| **+X** | Para a direita do usuário que olha a face frontal |
| **+Y** | Para cima, em direção à borda superior (fenda do cartucho) |
| **+Z** | Normal externa da face frontal, em direção ao usuário |

O sistema é destro (X × Y = Z). Unidade: milímetro. Ângulos em graus.

Consequências:

- o interior do corpo fica em **Z < 0**. A profundidade de um ponto abaixo da face frontal é `p = −Z`;
- "esquerda" e "direita" são sempre as do usuário olhando a frente: cruz direcional em X < 0, botões
  de ação em X > 0 `[M: §2]`;
- visto por trás, os lados se invertem: um item em X > 0 aparece à esquerda de quem olha a traseira.

### 3.2 Envelope externo como parâmetros (sem valores)

| Parâmetro | Significado | Extensão no referencial | Valor |
|---|---|---|---|
| `BODY_W` | Largura externa do corpo | X de −`BODY_W`/2 a +`BODY_W`/2 | **indefinido** |
| `BODY_H` | Altura externa do corpo (sem cartucho, sem antena) | Y de 0 a `BODY_H` | **indefinido** |
| `BODY_D` | Espessura externa do corpo | Z de −`BODY_D` a 0 | **indefinido** |

Nenhum desses três valores existe em fonte alguma consultada. Eles só podem ser fixados depois das
medições listadas em [COMPONENT_DIMENSIONS.md](COMPONENT_DIMENSIONS.md) e das decisões da seção 14.

### 3.3 Nomes das faces

| Sigla | Face | Plano |
|---|---|---|
| `FR` | Frontal | Z = 0 |
| `TR` | Traseira | Z = −`BODY_D` |
| `TP` | Topo (borda do cartucho) | Y = `BODY_H` |
| `BS` | Base | Y = 0 |
| `LE` | Lateral esquerda | X = −`BODY_W`/2 |
| `LD` | Lateral direita | X = +`BODY_W`/2 |

### 3.4 Uso no FreeCAD

- Modelar o corpo com o sistema global do FreeCAD coincidindo com o RC-dBMO. A vista frontal do
  aparelho corresponde à vista **Top** do FreeCAD (olhando de +Z para −Z, com X para a direita e Y
  para cima).
- Os envelopes STEP gerados em `mechanical/cad/out/` têm referencial **local** próprio, descrito em
  `mechanical/cad/components.yaml` (campo `local_frame`). A transformação local → RC-dBMO é dada por
  parâmetros de posicionamento definidos em [ASSEMBLY_LAYOUT.md](ASSEMBLY_LAYOUT.md).

## 4. Requisitos rastreados

Os identificadores `RM-nn` são deste documento. A coluna "Fonte" diz de onde o requisito vem; quando
há leitura deste documento, isso está marcado.

### 4.1 Silhueta, disposição e elementos obrigatórios

| ID | Requisito | Fonte |
|---|---|---|
| RM-01 | Corpo vertical, retangular, de cantos arredondados e espessura perceptível | `[M: §2]`, `[R: Carcaça 3D]` |
| RM-02 | Uma tela só, 4:3, grande, na metade superior, com moldura preta generosa | `[M: §2]`, `[R: Carcaça 3D]` |
| RM-03 | Cruz direcional à esquerda e quatro botões de ação em losango à direita, abaixo da tela | `[M: §2]`, `[R: Carcaça 3D]` |
| RM-04 | Dois analógicos na parte inferior, um de cada lado | `[M: §2, §2.1]`, `[R: Carcaça 3D]` |
| RM-05 | Botões pequenos de select e start no centro | `[M: §2]` ("centro inferior"), `[R: Carcaça 3D]` ("no centro") |
| RM-06 | Grade de alto-falante entre os analógicos | `[M: §2]`, `[R: Carcaça 3D]` (ver inconsistência I-2) |
| RM-07 | Gatilhos de ombro no topo | `[M: §2, §2.1]`, `[R: Carcaça 3D]` |
| RM-08 | Sem membros. Em repouso, o aparelho fica num suporte inclinado grafite, separado | `[M: §2, §2.1]`, `[R: Carcaça 3D]` |
| RM-09 | Sem teclado | `[R: Carcaça 3D]`, `[R: Versões do projeto]` |
| RM-10 | Silhueta reconhecível em vista frontal e em tamanho reduzido | `[M: §2]` |
| RM-11 | As vistas frontal, traseira, lateral e em perspectiva representam o mesmo objeto; detalhes traseiros ou mecânicos não definidos ficam marcados como pendentes | `[M: §3]` |
| RM-12 | Ergonomia por chanfro ou arredondamento das arestas inferiores, mantendo o corpo retangular | `[R: Carcaça 3D]` |
| RM-13 | Cada cartucho tem carcaça plástica não condutiva que envolve a PCB e o módulo funcional; a interface card edge permanece exposta apenas na extremidade | [R: Carcaça padrão dos cartuchos] |
| RM-14 | A carcaça dos cartuchos compartilha um envelope externo de referência e uma interface de encaixe comum; cavidade e aberturas funcionais podem variar por módulo | [R: Carcaça padrão dos cartuchos] |
| RM-15 | A língua card edge tem guarda lateral/rebaixo protetor, sem plástico cobrindo os pads ou invadindo a área de contato do soquete | [R: Carcaça padrão dos cartuchos] |
| RM-16 | A entrada superior do d-BMO tem boca guiada, canais laterais e chave mecânica compatível com o cartucho | [R: Carcaça padrão dos cartuchos] |
| RM-17 | Batente e retenção atuam sobre o corpo plástico; o encaixe não usa a ponta da PCB como batente nem o soquete como único suporte mecânico | [R: Carcaça padrão dos cartuchos] |
| RM-18 | O cartucho oferece superfície de pega para inserção e remoção sem tocar nos contatos ou componentes | [R: Carcaça padrão dos cartuchos] |

### 4.2 Elementos vedados

| ID | Vedação | Fonte |
|---|---|---|
| RM-20 | Fenda de cartucho na face frontal | `[M: §2.2]`, `[R: Carcaça 3D]` |
| RM-21 | Corpo verde-água ou em tons de ciano | `[M: §2.2]` |
| RM-22 | Braços e pernas | `[M: §2.2]` |
| RM-23 | Botões coloridos de formas geométricas diferentes; símbolos △ ○ × □ nas tampas reaproveitadas | `[M: §2.2]`, `[R: Analógicos e gatilhos reaproveitados]` |
| RM-24 | Logotipo, texto ou detalhe de marca de console real | `[M: §2.2]` |
| RM-25 | Metal de verdade na frente e atrás do corpo (ver seção 7) | `[R: Carcaça 3D]` |
| RM-26 | Slot de microSD exposto por fora | `[R: Carcaça 3D]` |
| RM-27 | Dobradiça e ímã no fecho da tampa traseira | `[R: Carcaça 3D]` |

### 4.3 Traseira

| ID | Requisito | Fonte |
|---|---|---|
| RM-30 | Traseira em duas zonas: a de cima é fixa, parafusada, e segura a eletrônica | `[R: Carcaça 3D]` |
| RM-31 | A zona de baixo é uma tampa que dá acesso à bateria, aos berços dos cartuchos reserva e ao slot do microSD | `[R: Carcaça 3D]` |
| RM-32 | Fecho da tampa: um parafuso e, no lado oposto, um lábio que entra num rebaixo | `[R: Carcaça 3D]` |
| RM-33 | Parafusos de fechamento com rosca na traseira (heat-set ou standoff), não na frente | `[R: Carcaça 3D]` |

### 4.4 Carcaça v0 (protótipo) e carcaça v1

| ID | Requisito | Fonte |
|---|---|---|
| RM-40 | A v0 leva a ESP32-S3-DevKitC inteira, presa por suportes impressos, com os módulos ligados por fio | `[R: Carcaça v0]` |
| RM-41 | A DevKitC não tem furos de fixação: os suportes prendem a placa pelas bordas | `[R: Carcaça v0]` (a ausência de furos é afirmação do README; conferir na peça) |
| RM-42 | A v0 já tem a forma externa da variação 1; espessura, parafusos e encaixe interno podem mudar na v1 | `[R: Carcaça v0]` |
| RM-43 | A v0 já tem todos os furos e fendas; os que ainda não têm peça ficam com tampa cega | `[R: Carcaça v0]` |
| RM-44 | A v0 só é modelada depois da Fase 2 de compras, com tela, placa, bateria e analógicos em mãos | `[R: Compras em fases]` |
| RM-45 | Antes da v0, imprimir um corpo de prova com o encaixe dos standoffs e dos parafusos; contar com 2 ou 3 reimpressões | `[R: Carcaça v0]`, `[R: Carcaça 3D]` |
| RM-46 | A v1 é modelada em volta da PCB definitiva (módulo ESP32-S3-WROOM-1-N16R8, riser do card edge) | `[R: Compras em fases]` |

## 5. Restrições de envelope

### 5.1 O que se sabe

Só existem restrições **relativas** (o corpo tem de conter certas peças). Nenhuma delas fecha um
número, porque quase todas as entradas ainda são `NOMINAL` ou `PENDENTE`.

| ID | Restrição | Entradas e STATUS |
|---|---|---|
| EC-1 | Com a tela na horizontal (320 × 240), a largura interna tem de conter o lado maior da placa da tela | `display_msp3218.pcb_length` — NOMINAL |
| EC-2 | A zona da tampa traseira tem de conter a face da bateria, em uma de duas orientações (ver decisão D-3) | `battery_4070115.width`, `battery_4070115.length` — NOMINAL |
| EC-3 | O berço da bateria precisa de ~10% de folga na espessura `[R: Riscos da bateria, item 7]` | `battery_4070115.thickness` — NOMINAL e contraditório no anúncio |
| EC-4 | A borda superior tem de acomodar, sem colisão, a entrada guiada e o envelope externo do cartucho (centro), a saída SMA, o furo do microfone e os gatilhos de ombro [R: Carcaça 3D], [R: Carcaça padrão dos cartuchos] | Envelope externo do cartucho e demais itens PENDENTE |
| EC-5 | A espessura tem de conter o empilhamento mais alto entre as zonas (ver [ASSEMBLY_LAYOUT.md](ASSEMBLY_LAYOUT.md), seção 3) | Altura dos analógicos, espessura da tela, altura da DevKitC com pinos — todas PENDENTE |
| EC-6 | A região da antena do módulo ESP32-S3-WROOM-1 pede folga de pelo menos 15 mm em todas as direções dentro da carcaça `[F: ESPRESSIF_HDG_PCB_LAYOUT]` | Dimensões da região da antena — CONFIRMADO |
| EC-7 | A saliência externa do cartucho é definida pelo corpo plástico, pelo ombro/batente e pela profundidade real de engate da língua no soquete | Envelope da carcaça, slot_depth e posição da riser — PENDENTE |

Verificação de coerência (aritmética, não é confirmação): uma área ativa de 64,8 × 48,6 mm tem
diagonal de 81,0 mm, isto é, 3,19 polegadas, compatível com a designação "3,2 polegadas" e com a
proporção 4:3 (64,8 / 48,6 = 1,333).

### 5.2 O que não se sabe

- `BODY_W`, `BODY_H`, `BODY_D`, raios de canto, espessura de parede.
- Espessura total da tela com o painel de toque e contorno do vidro `[R: Dimensões conhecidas]`.
- Área e altura dos analógicos com a capa `[R: Analógicos e gatilhos reaproveitados]`.
- Dimensões reais da bateria, incluindo PCM, abas e fios.
- Comprimento, largura e altura da DevKitC com os pinos `[R: Dimensões conhecidas]`.
- Dimensões de todos os módulos genéricos (CC1101, PN532, INMP441, MAX98357A, conjunto IR,
  alto-falante), dos botões tácteis, dos standoffs e dos conectores USB.
- Altura livre acima dos controles `[R: Dimensões conhecidas]`.
- Regras do processo de impressão: espessura mínima de parede, tolerâncias da impressora, orientação
  de impressão e limites de suporte. A especificação do personagem exige essas informações antes de
  gerar STL ou STEP `[M: §6]`. Elas dependem da impressora e do material de quem vai imprimir e não
  constam do README.

## 6. Material e processo

| ID | Regra | Fonte |
|---|---|---|
| MP-1 | Impressão 3D por FDM | `[R: Carcaça 3D]` |
| MP-2 | Carcaça em **PETG ou ASA, não PLA**: o PLA amolece a cerca de 55 °C | `[R: Riscos da bateria, item 7]` (ver inconsistência I-1) |
| MP-3 | Acabamento prata **sem metal de verdade** na frente e atrás: filamento prata ou pintura | `[R: Carcaça 3D]` |
| MP-4 | Corpo usinado em alumínio está descartado, salvo com janelas plásticas sobre as antenas | `[R: Carcaça 3D]` |
| MP-5 | Folga de ~0,2 a 0,3 mm nos furos das tampas de botão, medida a partir do corpo e da aba de cada tampa | `[R: Analógicos e gatilhos reaproveitados]` |
| MP-6 | Detalhes mecânicos precisos saem do CAD, não de gerador de imagem; renders são referência estética, não prova dimensional | `[R: Carcaça 3D]`, `[M: §6]` |
| MP-7 | Validar malha, manifold, escala e tolerâncias no CAD antes de fabricar | `[M: §6]` |

Notas:

- A temperatura de 55 °C para o PLA é o valor do README; não foi conferida em ficha de fabricante de
  filamento nesta sessão.
- **(hipótese)** Tintas "prata metálica" podem conter partículas metálicas. Não foi encontrada, nesta
  sessão, fonte sobre o efeito dessas tintas em 13,56 MHz, sub-GHz ou 2,4 GHz. Antes de pintar a
  carcaça, medir o alcance do NFC e a recepção do CC1101 através de um corpo de prova pintado.
- A especificação do personagem admite "superfícies metálicas" no acabamento `[M: §5]`, mas condiciona
  a especificação final ao protótipo. Para a carcaça, vale a regra mais restritiva do README (MP-3).

## 7. Zonas de exclusão de RF (keep-outs)

### 7.1 Antena de trilha do ESP32-S3 (2,4 GHz)

Fatos:

- A DevKitC e o módulo WROOM-1 têm antena de trilha; na carcaça, a ponta da antena deve ficar longe de
  metal, da bateria e do alto-falante `[R: Lista de componentes]`.
- Na v0, a ponta da DevKitC com a antena fica perto de uma borda da carcaça `[R: Carcaça v0]`.
- No footprint oficial da Espressif, a região da antena do ESP32-S3-WROOM-1 ocupa toda a largura do
  módulo (18 mm) e 5,95 mm do comprimento, numa das pontas `[F: ESPRESSIF_KICAD_WROOM1_FP]`. A figura
  de folga das diretrizes de hardware cota essa profundidade como 6 mm `[F: ESPRESSIF_HDG_CLEARANCE_FIG]`.
- As diretrizes de hardware da Espressif recomendam, para o produto final: "Ensure that the PCB antenna
  on the base board also has a sufficiently large clearance area inside the housing. A clearance of at
  least 15 mm is recommended in all directions." e pedem teste de vazão e alcance no produto final
  `[F: ESPRESSIF_HDG_PCB_LAYOUT]`.
- As mesmas diretrizes recomendam posicionar a antena para fora da placa-base, com o ponto de
  alimentação da antena junto à borda `[F: ESPRESSIF_HDG_PCB_LAYOUT]`.

Regra para a carcaça:

- `KO-ESP`: volume formado pela região da antena acrescida de 15 mm em todas as direções. Dentro dele
  não entram bateria, alto-falante, parafusos, standoffs, insertos, blindagens, chicotes de fio nem
  outras placas.
- **(interpretação)** A recomendação de 15 mm é lida aqui como regra para condutores e componentes. A
  parede plástica da carcaça inevitavelmente passa a menos de 15 mm; ela deve ficar o mais longe que
  a forma permitir e o resultado deve ser validado por medição de alcance, como o fabricante pede.

### 7.2 Antena do NFC (PN532, 13,56 MHz)

Fatos:

- A antena do PN532 fica virada para fora, sem metal atrás `[R: Carcaça 3D]`.
- Corpo metálico bloqueia o NFC `[R: Carcaça 3D]`.

Regra para a carcaça:

- `KO-NFC`: a antena (a própria placa do módulo PN532, de dimensões `PENDENTE`) fica paralela a uma
  parede externa, o mais perto possível dela, sem metal entre a antena e o exterior e sem massa
  metálica logo atrás.
- **(derivado)** São massas metálicas, para este fim: a bateria (invólucro de folha metálica), a
  moldura metálica da tela, o ímã e a carcaça do alto-falante, blindagens, standoffs, insertos e
  parafusos.
- Não há, nas fontes consultadas, distância mínima entre a antena e o metal. Os parâmetros
  `NFC_WALL_GAP` e `NFC_METAL_GAP` ([ASSEMBLY_LAYOUT.md](ASSEMBLY_LAYOUT.md)) ficam em aberto e são
  fechados por teste de leitura com as tags reais, com a tela e a bateria montadas.
- A parede sobre a antena é a decisão em aberto D-6.

### 7.3 Antena SMA do CC1101 (sub-GHz)

Fatos:

- A antena SMA é externa, sai pelo topo e serve só ao CC1101 `[R: Carcaça 3D]`, `[R: Lista de componentes]`.
- Topo: cartucho no centro, antena SMA e furo do microfone distribuídos para não colidir com os
  gatilhos de ombro `[R: Carcaça 3D]`.
- Corpo metálico atenua sub-GHz `[R: Carcaça 3D]`.

Regra para a carcaça:

- `KO-SMA`: o conector atravessa a parede do topo; a haste da antena fica livre, fora do corpo.
- A posição em X depende da largura do cartucho, do curso dos gatilhos e da geometria da placa do
  CC1101 (todos `PENDENTE`). Ver decisão D-7.
- **(derivado)** Um cartucho de rádio (por exemplo o LoRa de 915 MHz com antena própria, citado em
  `[R: Cartuchos de expansão]`) fica a poucos centímetros da antena SMA. A convivência entre as duas
  antenas não foi avaliada em nenhuma fonte consultada.

### 7.4 Outras zonas sensíveis

| Item | Regra | Fonte |
|---|---|---|
| Alto-falante | Longe da antena do ESP32; cavidade acústica a conferir com a peça em mãos | `[R: Lista de componentes]` |
| Janela do IR | LED e receptor com visada livre; janela ou furo previsto já na v0 | `[R: Carcaça 3D]`, `[R: Carcaça v0]` |
| Microfone | Furo no topo | `[R: Carcaça 3D]` |
| Fixadores metálicos | Fora de `KO-ESP` e de `KO-NFC` | (derivado de 7.1 e 7.2) |

## 8. Berço da bateria

Regras de `[R: Riscos da bateria e recomendações]`, item 7, e de `[R: Lista de componentes]`:

| ID | Regra | Fonte |
|---|---|---|
| BT-1 | Cerca de 10% de folga de espessura no berço | `[R: Riscos da bateria, item 7]` |
| BT-2 | Sem cola rígida | idem |
| BT-3 | Nenhuma aresta encostando na célula | idem |
| BT-4 | Longe do regulador, do amplificador e da S3 | idem |
| BT-5 | Carcaça em PETG ou ASA, não PLA | idem |
| BT-6 | O NTC, quando avulso, é colado na célula com fita kapton, sem criar ponto de pressão | `[R: Lista de componentes]` |
| BT-7 | A bateria fica na zona da tampa traseira, acessível ao abrir a tampa | `[R: Carcaça 3D]` |
| BT-8 | Pressão mecânica é uma das causas de inchaço: o berço não comprime a célula | `[R: Riscos da bateria]` (parágrafo inicial) |
| BT-9 | A bateria fica fora de `KO-ESP` e não fica atrás da antena do NFC | `[R: Lista de componentes]` para a antena do ESP32; (derivado) de `[R: Carcaça 3D]` para o NFC, por ser a célula uma massa metálica |

Consequências **(derivado)**:

- A folga de BT-1 é calculada sobre a espessura **medida** da célula, não sobre o valor do anúncio. O
  anúncio traz 0,4 mm e a designação 4070115 sugere 4,0 mm `[R: Lista de componentes]`; os dois são
  `NOMINAL`.
- O parafuso da tampa e os demais fixadores não podem alcançar a célula, com qualquer comprimento de
  parafuso que caiba no furo (leitura de BT-3 aplicada aos fixadores).
- O berço precisa de passagem para os fios da bateria (2 ou 3 fios, conforme haja NTC) sem dobra
  forçada na saída da célula.
- Pela mesma regra de BT-3, bosses, nervuras e berços dos cartuchos reserva não tocam a célula.

## 9. Interface do cartucho e fenda de acomodação (borda superior)

A interface tem dois estágios físicos: a abertura guiada da carcaça principal recebe
e orienta o corpo plástico; dentro dela, a língua card edge entra no DK920A-20M. A abertura
externa não deve ser dimensionada apenas pela espessura da PCB.

| ID | Regra | Fonte |
|---|---|---|
| CT-1 | O cartucho entra pela borda superior, paralelo ao corpo; nunca pela face frontal | [R: Carcaça 3D], [M: §2.2] |
| CT-2 | Soquete escolhido: DK920A-20M, card edge de 2,54 mm, 2 × 10 contatos, vertical, de furo passante | [R: Carcaça 3D] |
| CT-3 | Por ser vertical, o soquete vai numa placa riser perpendicular à PCB principal, no topo | [R: Carcaça 3D] (ver inconsistência I-6) |
| CT-4 | O soquete não tem abas de fixação conhecidas; a estrutura da carcaça principal apoia o corpo do soquete e a riser | [R: Especificação do cartucho] |
| CT-5 | A língua é deslocada lateralmente; a guarda plástica e a boca de entrada repetem a chave para impedir inserção invertida | [R: Carcaça padrão dos cartuchos] |
| CT-6 | A entrada do d-BMO tem boca chanfrada, canais laterais e transição que centraliza a língua nua na fenda elétrica | [R: Carcaça padrão dos cartuchos] |
| CT-7 | A guarda protege as bordas e enquadra os contatos; nenhum plástico cobre os pads ou invade a área de varredura dos contatos do soquete | [R: Carcaça padrão dos cartuchos] |
| CT-8 | Ombro/batente no corpo plástico limita a inserção antes de a ponta da PCB atingir o fundo do conector | [R: Carcaça padrão dos cartuchos] |
| CT-9 | A retenção secundária e a área de pega devem permitir remoção sem alavancar o cartucho pelo PCB | [R: Carcaça padrão dos cartuchos] |
| CT-10 | Os esforços de inserção/retirada são distribuídos entre corpo plástico, guias, batente e apoio estrutural do soquete; as soldas não são o único suporte | [R: Carcaça padrão dos cartuchos] |
| CT-11 | Até a Fase 4 de compras, a abertura fica com tampa cega | [R: Compras em fases], [R: Carcaça v0] |
| CT-12 | A referência atual da PCB é 52 × 52 mm, 1,6 mm, com língua de ~27,8 mm; o envelope plástico externo é independente e ainda não cotado | [R: Especificação do cartucho] (ver D-8) |
| CT-13 | O corpo plástico segue um envelope comum; cavidades internas e janelas funcionais podem variar por cartucho | [R: Carcaça padrão dos cartuchos] |

Pendências: medir largura, profundidade, altura e tolerâncias do DK920A-20M; medir o corpo e a
profundidade de engate do soquete; definir dimensões da guarda, do canal, do batente, da retenção
e da superfície de pega; medir os módulos reais; definir o método de união da carcaça.
## 10. Aberturas

Lista do que o README pede. A posição exata de cada abertura é parâmetro de
[ASSEMBLY_LAYOUT.md](ASSEMBLY_LAYOUT.md); nenhuma está cotada.

| ID | Abertura | Face | Fonte | Situação |
|---|---|---|---|---|
| AB-01 | Janela da tela, com moldura preta | `FR` | `[R: Carcaça 3D]`, `[M: §2]` | Contorno do vidro `PENDENTE` |
| AB-02 | Cruz direcional (peça única ou quatro tampas) | `FR` | `[R: Analógicos e gatilhos reaproveitados]` | Decisão D-10 |
| AB-03 | Quatro botões de ação em losango | `FR` | `[R: Carcaça 3D]` | Tampas `PENDENTE` |
| AB-04 | Select e start | `FR` | `[R: Carcaça 3D]` | Botões `PENDENTE` |
| AB-05 | Dois furos dos analógicos | `FR` | `[R: Carcaça 3D]` | Sticks `PENDENTE` |
| AB-06 | Grade do alto-falante | `FR` | `[R: Carcaça 3D]` | Ver inconsistência I-2 |
| AB-07 | Janela do IR (LED emissor e receptor) | `FR` ou `TP` | `[R: Carcaça 3D]` | Ver inconsistência I-8 e decisão D-9 |
| AB-08 | LED de estado entre a cruz e os botões de ação | `FR` | `[M: §8]` | Função ainda não definida na especificação do personagem |
| AB-09 | Fenda do cartucho | `TP` | seção 9 | — |
| AB-10 | Saída do conector SMA | `TP` | `[R: Carcaça 3D]` | Decisão D-7 |
| AB-11 | Furo do microfone | `TP` | `[R: Carcaça 3D]` | Módulo `PENDENTE` |
| AB-12 | Gatilhos de ombro L1/R1 (e L2/R2, se couberem) | `TP` | `[R: Analógicos e gatilhos reaproveitados]` | Decisão D-11 |
| AB-13 | Porta USB-C de dados | `LE` ou `LD` | `[R: Carcaça 3D]` | Decisão D-4 |
| AB-14 | Porta USB-C só de carga, com moldura ou anel em volta, ícone de raio em relevo e LED de carga ao lado | `LE` ou `LD` | `[R: Conector de carga USB-C dedicado]` | Decisão D-4 |
| AB-15 | Botão de energia momentâneo | `LE` ou `LD` | `[R: Lista de componentes]`, `[R: Carcaça 3D]` | Ver inconsistência I-3 |
| AB-16 | Tampa traseira inferior | `TR` | `[R: Carcaça 3D]` | Linha de divisão `PENDENTE` |
| AB-17 | Furos dos parafusos de fechamento | `TR` | `[R: Carcaça 3D]` | Quantidade e posição `PENDENTE` |

Não há abertura para o microSD: na v0 o cartão fica no slot SD do verso do módulo da tela; na PCB
final, num slot próprio dentro da zona da tampa traseira `[R: Lista de componentes]`, `[R: Carcaça 3D]`.

## 11. Fixadores

| ID | Regra | Fonte |
|---|---|---|
| FX-1 | Rosca da tampa: standoff hexagonal de metal reaproveitado (de placa-mãe), embutido num furo hexagonal justo | `[R: Carcaça 3D]` |
| FX-2 | Plano B, se o standoff girar: inserto heat-set M2 ou M3 | `[R: Carcaça 3D]` |
| FX-3 | Demais parafusos de fechamento também com heat-set ou standoff na traseira | `[R: Carcaça 3D]` |
| FX-4 | Testar o encaixe hexagonal num corpo de prova antes de fechar o desenho | `[R: Carcaça 3D]` |
| FX-5 | Na v0, a DevKitC é presa por suportes impressos | `[R: Carcaça v0]` |
| FX-6 | Apoio do pivô impresso para as alavancas L2/R2, se forem usadas | `[R: Analógicos e gatilhos reaproveitados]` |

Pendências: rosca, distância entre faces e comprimento do standoff (medir a peça); escolha entre M2
e M3; quantidade e posição dos parafusos. A rosca de standoffs de placa-mãe varia; não assumir M3
sem medir (**não verificado nesta sessão**).

## 12. Identidade visual

Direção: geometria limpa, aparência de equipamento industrial, acabamento sóbrio. A leitura
principal é a de um dispositivo eletrônico real `[M: §1]`, com cantos arredondados moderados e
contornos discretos `[T: §4]`.

### 12.1 Paleta por peça

| Peça | Token | HEX | Fonte |
|---|---|---|---|
| Corpo (frente, traseira, laterais) | `color.silver.metallic` ou `color.silver.industrial` | `#D5D7D9` / `#AEB3B8` | `[T: §1]`, `[M: §5]`, `[R: Carcaça 3D]` |
| Moldura da tela | `color.black` (o README admite grafite ou preto) | `#090A0C` | `[M: §2]` ("moldura preta"), `[R: Carcaça 3D]`, `[T: §1]` |
| Controles (cruz, botões de ação, select/start, analógicos, gatilhos) | `color.graphite` (ou prata) | `#24272B` | `[M: §2.1]`, `[R: Carcaça 3D]` |
| Junções e detalhes | `color.graphite` ou `color.black` | `#24272B` / `#090A0C` | `[M: §5]`, `[R: Carcaça 3D]` |
| Suporte inclinado | `color.graphite` | `#24272B` | `[M: §2]`, `[R: Carcaça 3D]` |
| Acentos pequenos | `color.red.signal` | `#D92332` | `[T: §1]`, `[R: Carcaça 3D]` |
| Marcações que exijam legibilidade | `color.offwhite` | `#F2F0EB` | `[M: §5]` ("apenas quando necessário para legibilidade") |

### 12.2 Regras

- Vermelho é cor de sinalização e se usa com parcimônia `[T: §1]`. No corpo, só em acentos pequenos;
  o rosto vermelho é da tela, não da carcaça `[R: Carcaça 3D]`.
- Não depender só da cor para comunicar estado `[T: §1]`. Por isso a porta de carga é identificada
  por cor **e** por ícone em relevo `[R: Conector de carga USB-C dedicado]`.
- Botões monocromáticos, em grafite ou prata `[M: §2.1]`.
- Nenhum símbolo, texto ou detalhe de console real `[M: §2.2]`. As tampas reaproveitadas do controle
  de PS3 são lixadas e pintadas de grafite, ou servem só de molde de medida
  `[R: Analógicos e gatilhos reaproveitados]`.
- Cores fora da paleta não entram como cores centrais `[T: §1]`.
- O README não define a cor da moldura da porta de carga. Dentro da paleta, a opção coerente é o
  vermelho de sinalização como acento pequeno **(interpretação)**; decisão D-12.
- Os valores HEX são alvo de tela. Filamento e tinta não reproduzem HEX exato: escolher o material
  comparando amostras físicas com a referência e registrar marca, linha e cor escolhidas (D-13).

## 13. Suporte inclinado

O suporte é uma peça separada, grafite, onde o aparelho repousa `[M: §2]`, `[R: Carcaça 3D]`. Ângulo,
forma e interface com o corpo não estão definidos em nenhuma fonte. Fica fora do escopo da carcaça
v0 até haver definição (D-14).

## 14. Decisões em aberto

| ID | Decisão | Quem ou o que fecha | Origem |
|---|---|---|---|
| D-1 | Dimensões externas `BODY_W`, `BODY_H`, `BODY_D` e raios | Medições da Fase 2 e as decisões abaixo | `[R: Compras em fases]`, `[M: §8]` |
| D-2 | Orientação da tela. O firmware do rosto trata a tela como horizontal (320 × 240) e registra "ajustar quando a orientação da carcaça estiver fechada" (`firmware/rosto/rosto.ino`); o README pede medir a altura livre acima dos controles | Medição e mantenedor | `[R: Dimensões conhecidas]` |
| D-3 | Orientação da bateria na zona da tampa (lado maior em Y ou em X) | Medição da bateria e largura do corpo | (derivado de EC-2) |
| D-4 | Uma porta USB-C (dados e carga) ou duas | Projeto elétrico | `[R: Riscos da bateria, item 5]` |
| D-5 | Carcaça impressa pelo Tiago Efraim ou fabricada na JLCPCB | Mantenedor | `[R: Fabricação]`, `[R: Roadmap]` |
| D-6 | Parede em que fica a antena do NFC e como evitar metal atrás dela | Teste de leitura com a peça | `[R: Carcaça 3D]` |
| D-7 | Posição em X da saída SMA e do furo do microfone no topo; placa do CC1101 junto à parede do topo ou ligada por cabo | Medição do módulo CC1101 e dos gatilhos | `[R: Carcaça 3D]` |
| D-8 | Formato final da PCB-cartucho (referência 52 × 52 mm ou variante curta) e compatibilidade com a cavidade da carcaça comum | Resposta da JLCPCB sobre o chanfro, módulos reais e protótipo da carcaça | [R: Especificação do cartucho] |
| D-9 | Face da janela do IR | Mantenedor | `[R: Carcaça 3D]` |
| D-10 | Cruz direcional de peça única ou quatro tampas; tampas do controle de PS3 ou impressas | Medição das tampas | `[R: Analógicos e gatilhos reaproveitados]` |
| D-11 | L2/R2 com as alavancas do controle, com tampas impressas, ou só L1/R1 na v0 | Medição das alavancas | `[R: Analógicos e gatilhos reaproveitados]` |
| D-12 | Cor da moldura da porta de carga | Mantenedor | `[R: Conector de carga USB-C dedicado]` |
| D-13 | Filamento ou tinta para cada token de cor; teste de RF se a tinta for metálica | Amostras físicas | seção 6 |
| D-14 | Suporte inclinado: ângulo e interface | Mantenedor | `[M: §2]` |
| D-15 | Espessura mínima de parede, tolerâncias, orientação de impressão e limites de suporte | Quem imprime, com a impressora e o material reais | `[M: §6]` |
| D-16 | Rosca: standoff reaproveitado ou heat-set; M2 ou M3 | Corpo de prova | `[R: Carcaça 3D]` |
| D-17 | LED de estado na frente (ponto vermelho da referência v1): manter como LED pequeno e embutido ou remover | Mantenedor da marca | `[M: §8]` |
| D-18 | Linha de divisão entre a zona traseira fixa e a tampa | Medição da bateria e da eletrônica | `[R: Carcaça 3D]` |
| D-19 | Onde ficam os berços dos cartuchos reserva e quantos são | Dimensões finais do cartucho (D-8) | `[R: Carcaça 3D]` |
| D-20 | Método de união e abertura da carcaça dos cartuchos (clips, parafusos ou combinação) | Processo de impressão, manutenção e corpo de prova | [R: Carcaça padrão dos cartuchos] |
| D-21 | Geometria da guarda/rebaixo da língua, chave, batente e retenção secundária | Medição do DK920A-20M e protótipo conjunto da entrada do d-BMO | [R: Carcaça padrão dos cartuchos] |

## 15. Inconsistências encontradas nas fontes

Registradas para decisão do mantenedor. Este documento não as resolve, salvo onde indicado.

| ID | Inconsistência | Tratamento aqui |
|---|---|---|
| I-1 | `[R: Carcaça 3D]` admite "PLA/PETG prata ou pintura"; `[R: Riscos da bateria, item 7]` diz "PETG ou ASA, não PLA" | Adotada a regra mais restritiva (MP-2) |
| I-2 | `[R: Carcaça 3D]` põe a grade do alto-falante "entre" os analógicos e, em outro item, "no canto inferior" | Adotada a posição entre os analógicos, que coincide com `[M: §2]` |
| I-3 | `[R: Carcaça 3D]` cita "2× USB-C + chave numa lateral"; `[R: Lista de componentes]` determina botão momentâneo, não chave deslizante | Tratado como botão momentâneo (AB-15) |
| I-4 | `[M: §3]` pede manter os "pontos de conexão dos braços e pernas", mas `[M: §2]` e `[M: §2.2]` definem o personagem sem membros | Nenhum ponto de conexão de membro é previsto. Corrigir na especificação do personagem |
| I-5 | Espessura da bateria: 0,4 mm no anúncio e 4,0 mm pela designação 4070115 | Os dois valores ficam `NOMINAL`; medir |
| I-6 | `[R: Carcaça 3D]` diz que o soquete vai numa riser perpendicular e, no item seguinte, "soldado na PCB principal" | Adotada a riser (CT-3), que é a consequência geométrica de um soquete vertical com cartucho paralelo ao corpo |
| I-7 | Placa da tela: 89,3 × 55,04 mm ("datasheet") e 88 × 54 mm (régua) `[R: Dimensões conhecidas]` | Os dois valores ficam `NOMINAL`; medir com paquímetro |
| I-8 | `[R: Carcaça 3D]` põe "LED + receptor IR na frente"; o texto alternativo do render lista o LED de IR junto dos itens do topo | Decisão D-9 |
| I-9 | A DevKitC oficial documentada pela Espressif tem duas portas **Micro-USB** `[F: ESPRESSIF_DEVKITC_GUIDE]`; o README fala em aberturas USB-C na v0 | Conferir o conector da placa em mãos. Se for USB-C, a placa não é a revisão documentada e o padrão de furos oficial precisa ser conferido na peça |

## 16. Condições para iniciar a modelagem da carcaça v0

A modelagem começa quando, no registro de dimensões, as linhas abaixo deixarem de ser `NOMINAL` ou
`PENDENTE` (lista mínima, derivada das seções 5 a 11):

1. Tela: contorno e espessura da placa, espessura total com o painel de toque, contorno do vidro,
   posição da área ativa em relação à placa, posição do slot SD e dos pinos.
2. DevKitC: comprimento, largura, altura com pinos, posição dos conectores USB e dos botões.
3. Bateria: espessura, largura e comprimento reais, com PCM e saída dos fios.
4. Analógicos: área da base, altura total com a capa, diâmetro da capa e curso.
5. Módulos: contorno, espessura e posição do componente funcional (antena, conector SMA, furo
   acústico do microfone, LED e receptor de IR) de cada módulo.
6. Alto-falante: contorno, espessura e posição dos terminais.
7. Botões tácteis: corpo e altura do atuador de cada modelo usado, inclusive os de ângulo reto.
8. Standoff: rosca, distância entre faces e comprimento.
9. Regras de processo de quem imprime (D-15).
10. Cartucho: medir o DK920A-20M real e fechar o contorno da língua, a guarda lateral, a
    área livre dos pads, o batente, a retenção e o canal da carcaça principal. O corpo plástico
    comum precisa ser prototipado com pelo menos uma PCB e um módulo reais antes de definir
    seus berços internos e os berços de reserva do d-BMO.

Decisões D-2, D-3, D-6, D-7, D-9 e D-18 precisam estar fechadas antes de fixar `BODY_W`, `BODY_H` e
`BODY_D`.
