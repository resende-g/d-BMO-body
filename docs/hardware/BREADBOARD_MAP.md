# Mapa da protoboard (perfil `dbmo-bench`)

| Campo | Valor |
|---|---|
| Versão | 0.4 (2026-11-25) |
| Escopo | Tela ligada direto na DevKitC; trilho da protoboard grande para o LED da tela, o receptor e o transmissor de IR, o CC1101 (só recepção), o microfone INMP441 e o amplificador MAX98357A; SD da tela no SPI dividido; 3 botões numa protoboard pequena separada |
| Fora do escopo | PN532 (pinos não soldados), MCP23017 (não possui) |
| Pinagem | `firmware/config/include/dbmo/config/pins.h`, perfil `bench` ([`GPIO_MAP.md`](GPIO_MAP.md)) |
| Estado | Fases A a F montadas e testadas de 21 a 25/11/2026: tela, CC1101 e SD dividindo o SPI, microfone funcionando (ver [`../testing/TESTING.md`](../testing/TESTING.md)). Em 25/11 o receptor de IR deixou de ler o controle (fiação a conferir). **Fase G (alto-falante) ainda não montada.** A v0.1 (tela inteira alimentada pelo trilho grande) deixou a tela apagada, com 2,65 V medidos no 3,3 V |

## 1. Princípio da montagem

- A **DevKitC fica fora da protoboard**, ligada por jumpers.
- A **tela é alimentada direto pela DevKitC**: VCC no J1 pino 1 (3V3), GND no J1 pino 22. Foi a
  montagem que funcionou depois da falha da v0.1.
- O **trilho grande** recebe 3,3 V do J1 pino 2 e GND do J3 pino 1, e alimenta o LED da tela, os
  módulos de IR e o CC1101. Com o LED da tela no trilho, a luz de fundo mostra se o trilho está
  bom: se ele falhar, a luz apaga e a tela continua funcionando.
- **SPI:** até a fase C, SDI, SCK e SDO da tela vão direto aos pinos 11, 12 e 13 (fêmea-fêmea).
  Na fase D, para dividir o barramento com o CC1101, os três passam a nós da protoboard grande,
  porque cada pino macho da DevKitC aceita um só jumper fêmea.
- Sinal que liga só dois pontos (CS, DC, RESET, GDO0, CSN) vai **direto, com jumper fêmea-fêmea**.
- O pino **5V da DevKitC não é usado**.
- Os dois pinos 3V3 do J1 têm o mesmo nome na tabela do guia (DKC-18), mas o esquema da placa não
  foi aberto: que são o mesmo ponto elétrico é conferido em M3.

## 2. Desenho

Desenho interativo: artifact "Bancada d-BMO" (https://claude.ai/artifact/Jcf6cXU6TdTkLStP5fy38u, privado). A fase D começa desligada no desenho.

Protoboard grande (MB-102, só o par de trilhos de cima):

| Onde | O quê |
|---|---|
| Coluna 8, linhas a–d | Nó MOSI: tela SDI (a), CC1101 pino 6 (b), SD_MOSI (c), DevKitC 11 (d) |
| Coluna 11, linhas a–d | Nó SCK: tela SCK (a), CC1101 pino 5 (b), SD_SCK (c), DevKitC 12 (d) |
| Coluna 14, linhas a–d | Nó MISO: tela SDO (a), CC1101 pino 7 (b), SD_MISO (c), DevKitC 13 (d) |
| Colunas 40, 41, 42, linha e | Transmissor de IR: DAT, VCC, GND |
| Colunas 50, 51, 52, linha e | Receptor de IR: OUT, VCC, GND |
| Colunas 30 a 36, linha j (metade de baixo) | MAX98357A, fase G: LRC (30), BCLK (31), DIN (32), GAIN (33), SD (34), GND (35), Vin (36). A coluna 30 é o nó do LRCLK e a 31 o do BCLK, divididos com o microfone |

Protoboard pequena (desenhada com 30 colunas e trilhos, 400 pontos; só o trilho (−) de baixo é usado), botões atravessando o canal central: BAIXO nas colunas 6–8, CIMA 18–20, OK 24–26. Fio do GPIO na linha a (coluna da direita), fio do GND na linha j (coluna da esquerda): pernas em diagonal. Se a pequena for de 170 pontos, sem trilhos, cada perna de GND vai direto a um dos pinos GND livres do J3 com jumper fêmea-macho, no lugar de B1 e B5–B7.

## 3. Pinos da DevKitC usados

A ordem abaixo segue o guia do usuário da DevKitC-1, conectores J1 e J3; foi confirmada na
fonte apenas para alguns pinos (DKC-07, DKC-10, DKC-01, DKC-02, DKC-18 em
`sources/ESP32-S3.md`). **Confira cada pino pelo nome impresso ao lado dele na placa.**

| Conector | Pino | Nome na placa | Uso aqui |
|---|---:|---|---|
| J1 | 1 | 3V3 | Tela VCC |
| J1 | 2 | 3V3 | Trilho (+) |
| J1 | 4 | 4 | Receptor de IR (OUT) |
| J1 | 5 | 5 | Transmissor de IR (DAT) |
| J1 | 7 | 7 | INMP441 SD (fase E) |
| J1 | 8 | 15 | CC1101 GDO0 (fase D) |
| J1 | 10 | 17 | Tela DC |
| J1 | 11 | 18 | Tela RESET |
| J1 | 16 | 10 | Tela CS |
| J1 | 17 | 11 | Tela SDI; nó MOSI na fase D |
| J1 | 18 | 12 | Tela SCK; nó SCK na fase D |
| J1 | 19 | 13 | Tela SDO; nó MISO na fase D |
| J1 | 20 | 14 | CC1101 CSN (fase D) |
| J1 | 21 | **5V** | **Não usar.** Fica ao lado do 14: conferir duas vezes o jumper do CSN |
| J1 | 22 | G / GND | Tela GND |
| J3 | 1 | G / GND | Trilho (−) da protoboard grande |
| J3 | 5 | 2 | Botão OK |
| J3 | 6 | 42 | MAX98357A DIN (fase G); fica ao lado do 2 (OK) |
| J3 | 7 | 41 | INMP441 WS (fase E); nó do LRCLK na fase G |
| J3 | 8 | 40 | INMP441 SCK (fase E); nó do BCLK na fase G; fica ao lado do 39 (CIMA) |
| J3 | 9 | 39 | Botão CIMA |
| J3 | 17 | 47 | Botão BAIXO |
| J3 | 18 | 21 | SD_CS da tela (fase F); fica entre o 47 e o 20 (USB D+) |
| J3 | 22 | G / GND | Trilho (−) da protoboard pequena |

## 4. Lista de ligações

Tipos: **FF** fêmea-fêmea, **FM** fêmea-macho, **MM** macho-macho.

| # | Fase | De | Para | Tipo |
|---|---|---|---|---|
| A1 | A | DevKitC 3V3 (J1, pino 2) | Trilho (+), col. 46 | FM |
| A2 | A | DevKitC GND (J3, pino 1) | Trilho (−), col. 47 | FM |
| A3 | A | Tela VCC | DevKitC 3V3 (J1, pino 1) | FF |
| A4 | A | Tela GND | DevKitC GND (J1, pino 22) | FF |
| A5 | A | Tela LED | Trilho (+), col. 10 | FM |
| A6 | A | Tela SDI (MOSI) | DevKitC 11 (sai na fase D) | FF |
| A7 | A | Tela SCK | DevKitC 12 (sai na fase D) | FF |
| A8 | A | Tela SDO (MISO) | DevKitC 13 (sai na fase D) | FF |
| A9 | A | DevKitC 10 | Tela CS | FF |
| A10 | A | DevKitC 18 | Tela RESET | FF |
| A11 | A | DevKitC 17 | Tela DC | FF |
| B1 | B | DevKitC GND (J3, pino 22) | Pequena: trilho (−) de baixo, col. 1 | FM |
| B2 | B | DevKitC 47 | Pequena: botão BAIXO, col. 8 linha a | FM |
| B3 | B | DevKitC 39 | Pequena: botão CIMA, col. 20 linha a | FM |
| B4 | B | DevKitC 2 | Pequena: botão OK, col. 26 linha a | FM |
| B5 | B | Pequena: botão BAIXO, col. 6 linha j | Trilho (−) de baixo, col. 5 | MM |
| B6 | B | Pequena: botão CIMA, col. 18 linha j | Trilho (−) de baixo, col. 17 | MM |
| B7 | B | Pequena: botão OK, col. 24 linha j | Trilho (−) de baixo, col. 23 | MM |
| C1 | C | IR RX VCC, col. 51 linha b | Trilho (+), col. 53 | MM |
| C2 | C | IR RX GND, col. 52 linha a | Trilho (−), col. 56 | MM |
| C3 | C | DevKitC 4 | IR RX OUT, col. 50 linha a | FM |
| C4 | C | IR TX VCC, col. 41 linha b | Trilho (+), col. 39 | MM |
| C5 | C | IR TX GND, col. 42 linha a | Trilho (−), col. 44 | MM |
| C6 | C | DevKitC 5 | IR TX DAT, col. 40 linha a | FM |
| D1 | D | Tela SDI (MOSI) | Col. 8, linha a (substitui A6) | FM |
| D2 | D | Tela SCK | Col. 11, linha a (substitui A7) | FM |
| D3 | D | Tela SDO (MISO) | Col. 14, linha a (substitui A8) | FM |
| D4 | D | DevKitC 11 | Col. 8, linha d | FM |
| D5 | D | DevKitC 12 | Col. 11, linha d | FM |
| D6 | D | DevKitC 13 | Col. 14, linha d | FM |
| D7 | D | CC1101 pino 1 GND | Trilho (−), col. 58 | FM |
| D8 | D | CC1101 pino 2 VCC | Trilho (+), col. 61 | FM |
| D9 | D | CC1101 pino 3 GDO0 | DevKitC 15 | FF |
| D10 | D | CC1101 pino 4 CSN | DevKitC 14 (ao lado do 5V) | FF |
| D11 | D | CC1101 pino 5 SCK | Col. 11, linha b | FM |
| D12 | D | CC1101 pino 6 MOSI | Col. 8, linha b | FM |
| D13 | D | CC1101 pino 7 MISO | Col. 14, linha b | FM |
| E1 | E | INMP441 VDD | Trilho (+), col. 45 | FM |
| E2 | E | INMP441 GND | Trilho (−), col. 49 | FM |
| E3 | E | INMP441 L/R | Trilho (−), col. 50 | FM |
| E4 | E | INMP441 SCK | DevKitC 40 (J3, pino 8; sai na fase G) | FF |
| E5 | E | INMP441 WS | DevKitC 41 (J3, pino 7; sai na fase G) | FF |
| E6 | E | INMP441 SD | DevKitC 7 (J1, pino 7) | FF |
| F1 | F | Tela SD_MOSI | Col. 8, linha c | FM |
| F2 | F | Tela SD_SCK | Col. 11, linha c | FM |
| F3 | F | Tela SD_MISO | Col. 14, linha c | FM |
| F4 | F | Tela SD_CS | DevKitC 21 (J3, pino 18) | FF |
| G1 | G | DevKitC 41 (J3, pino 7) | Col. 30, linha f (nó do LRCLK) | FM |
| G2 | G | DevKitC 40 (J3, pino 8) | Col. 31, linha f (nó do BCLK) | FM |
| G3 | G | INMP441 WS | Col. 30, linha g (substitui E5) | FM |
| G4 | G | INMP441 SCK | Col. 31, linha g (substitui E4) | FM |
| G5 | G | DevKitC 42 (J3, pino 6) | Col. 32, linha f (DIN) | FM |
| G6 | G | Col. 35, linha f (GND do MAX98357A) | Trilho (−), col. 37 | MM |
| G7 | G | Col. 36, linha f (Vin do MAX98357A) | Trilho (+), col. 38 | MM |

CC1101 pino 8 (GDO2) e pinos T_* da tela: sem ligação. Pinos 1 a 4 do CC1101 conforme o manual da Ebyte; 5 a 8 só por fonte secundária (ficha da LCSC do E07-M1101D-TH). Transmissor de IR ligado, mas o envio continua bloqueado no firmware (`infrared.tx_enabled` = 0) até o circuito da plaquinha ser identificado.

INMP441 (fase E) fora da protoboard, como o CC1101: seus pinos ficam em duas fileiras de 3, e uma fileira encaixada caberia num mesmo grupo de 5 furos, pondo os 3 pinos em curto. Pinagem pela serigrafia da placa (foto de 22/11): L/R, WS e SCK de um lado; GND, VDD e SD do outro, com o furo do som no meio, do lado da serigrafia. L/R vai ao GND: o microfone usa o canal esquerdo, que é o que o firmware lê (MIC-06 em `sources/PERIPHERALS.md`). A placa não expõe o CHIPEN.

SD da tela (fase F, soldado em 24/11): os 4 pinos ficam na borda oposta ao conector principal; ordem SD_CS, SD_MOSI, SD_MISO, SD_SCK, a típica do módulo, sem confirmação no esquema (MSP-06). O cartão é alimentado pela própria placa da tela (VCC no J1 pino 1); só os 4 sinais saem. Cada nó do SPI recebe 4 fios: tela (a), CC1101 (b), SD (c), DevKitC (d).

MAX98357A (fase G): placa com barra de 7 pinos já soldada e borne de parafuso para o alto-falante. Pinagem pela serigrafia (fotos de 22/11): LRC, BCLK, DIN, GAIN, SD, GND, Vin; no verso, "vin: 2.5-5.5V" e "Default 9db Gain & (L+R)/2 out". GAIN e SD ficam soltos: 9 dB (AMP-11) e a placa mantém o amplificador ligado na mistura (L+R)/2 (AMP-06; valores dos resistores da placa não conferidos). Os pinos de dado aceitam até 6 V (AMP-02). Como cada pino da DevKitC aceita um só jumper fêmea, o BCLK e o LRCLK passam a nós na metade de baixo (colunas 31 e 30), e o microfone vai para esses nós (G3 e G4 no lugar de E4 e E5). Alimentação pelo trilho de 3,3 V (AMP-01: 2,5 V a 5,5 V), nada no 5V. Alto-falante no borne: os dois fios dele, sem polaridade a respeitar com um alto-falante só; o conector branco do cabo não entra no borne (encaixar dois jumpers macho nele ou, só com decisão do Gabriel, cortar o conector).

Montagem completa (fases A a G, já sem A6–A8, E4 e E5): 9 FF, 31 FM e 9 MM, mais 1 MM por trilho dividido.

## 5. Conferências com o multímetro

Todas com o **USB desligado**, salvo indicação. Resistência na escala 200 Ω ("1" = aberto);
tensão na faixa 20 V.

| # | Quando | O que | Esperado |
|---|---|---|---|
| M1 | Protoboard grande vazia | Continuidade de ponta a ponta de cada trilho de cima, da primeira à última coluna | Perto de 0 Ω. Se der "1", o trilho é dividido: ligar as duas metades com um MM na coluna 32 e medir de novo |
| M2 | Antes de cada energização | Trilho (+) contra trilho (−) | Sem continuidade (com módulos ligados, uma leitura breve ao encostar é normal) |
| M3 | Antes da fase A | Solda do J1 pino 1 contra a solda do J1 pino 2 | Perto de 0 Ω: os dois 3V3 são o mesmo ponto |
| M4 | Antes da fase A | Cada jumper de alimentação (A1, A2, A5), de ponta a ponta | Perto de 0 Ω e estável ao mexer no fio; separar o que oscilar |
| BT | Antes de pôr os fios de cada botão | Continuidade entre o furo do GPIO (linha a) e o do GND (linha j) | **Só fecha com o botão apertado.** Se fechar sem apertar, girar o botão 90° |
| NÓ | Fase D | Continuidade entre o pino 12 da DevKitC e o pino SCK da tela e do CC1101; o mesmo para 11 e 13 | Perto de 0 Ω. Confirma os nós do SPI |
| CC | Fase D, antes de ligar o CC1101 | Pino 1 do CC1101 contra a rosca do SMA; pino 2 contra a rosca | Pino 1 fecha; pino 2 não |
| M5 | USB ligado, fase A montada | Tensão no J1 pino 1 contra GND; trilho (+) contra trilho (−) perto do A1 e na ponta oposta | Cerca de 3,3 V nos três pontos, praticamente iguais. Trilho abaixo do pino indica perda no caminho (jumper ou contato): desligar e refazer M1 e M4 |
| IR | USB ligado, fase C (receptor) | Tensão entre VCC e GND do receptor, nas pernas dele | Cerca de 3,3 V |
| MIC | Fase E, antes de ligar o USB | Continuidade entre o furo com o nome (lado da serigrafia) e a outra ponta de cada fio (E1–E6); L/R contra GND do módulo, antes do E3 | Perto de 0 Ω nos 6 fios. L/R contra GND: anotar o valor |
| MV | USB ligado, fase E | Tensão entre VDD e GND do INMP441, nas pernas dele | Cerca de 3,3 V |
| SOLDA | Fase F, antes de ligar | Os 4 pinos do SD entre si e cada um contra o GND do conector principal da tela | Aberto ("1") em todos |
| AF | Fase G, antes de ligar | Resistência do alto-falante, nos dois fios | Pouco abaixo da impedância nominal (um de 8 Ω mede cerca de 6 a 7 Ω). Perto de 0 Ω: não ligar |
| AV | USB ligado, fase G, durante `tone` | Trilho (+) contra trilho (−) | Cerca de 3,3 V, sem cair. Se cair, desligar o USB: o trilho não está dando conta do amplificador |

## 6. Ordem

Um módulo por vez, sempre com o USB desligado para mexer em fio. A fase B (botões) já está
montada e testada (TESTING.md, 21/11/2026).

1. M1, M2, M3 e M4 com a protoboard grande vazia.
2. Fase A: tirar o LED da tela do J1 pino 2, ligar A1 e A2 e levar o LED ao trilho (A5). M2;
   USB; M5. Esperado: luz de fundo acesa, rosto na tela, `status` igual ao da montagem mínima.
3. Fase C, receptor (C1–C3) sozinho: M2; USB; IR; apontar um controle remoto e rodar `ir last`.
4. Fase C, transmissor (C4–C6): só ligar; o envio fica bloqueado até identificar o circuito da
   plaquinha.
5. Fase D: trocar A6–A8 por D1–D6 e conferir a tela (rosto e `status`) antes do CC1101; depois
   CC, D7–D13, NÓ, M2, USB e `pio test -e dbmo-bench -f test_cc1101`.
6. Fase E, microfone (E1–E6): MIC; M2; USB; MV; no console, `config audio.mic_enabled 1` e
   `mic 1000` em silêncio e com som perto. Sem o microfone ligado, o firmware registra `pico 0`.
7. Fase F, SD da tela (F1–F4): SOLDA; tela e CC1101 primeiro, sem o SD; depois F1–F4, NÓ, M2, USB
   e, no boot, `cartao: Ok`. Cartão em FAT32, colocado e tirado com o USB desligado.
8. Fase G, alto-falante (G1–G7): AF; tirar E4 e E5 e ligar G1 a G7; NÓ nas colunas 30 e 31; M2;
   USB; no console, `config audio.volume 20` e `tone 440 500`, com AV durante o tom. Só então
   subir o volume. Depois, `mic 1000` para conferir que o microfone continua lendo pelos nós.

Um módulo em Failed, ou tensão caindo: desligar o USB e conferir só os fios daquele módulo.
