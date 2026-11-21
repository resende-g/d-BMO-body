# Mapa da protoboard (perfil `dbmo-bench`)

| Campo | Valor |
|---|---|
| Versão | 0.2 (2026-11-21) |
| Escopo | Tela ligada direto na DevKitC; trilho da protoboard grande para o LED da tela, o receptor e o transmissor de IR e o CC1101 (só recepção); 3 botões numa protoboard pequena separada |
| Fora do escopo | SD da tela e PN532 (pinos não soldados), áudio (sem pinos no perfil `dbmo-bench`; pinagem dos módulos e ligação do alto-falante não conferidas), MCP23017 (não possui) |
| Pinagem | `firmware/config/include/dbmo/config/pins.h`, perfil `bench` ([`GPIO_MAP.md`](GPIO_MAP.md)) |
| Estado | **v0.2 não montada.** A v0.1 (tela inteira alimentada pelo trilho grande) foi montada em 21/11/2026: a tela ficou apagada, com 2,65 V medidos no 3,3 V (ver [`../testing/TESTING.md`](../testing/TESTING.md)). Em uso agora: montagem mínima (tela direto na DevKitC e botões; IR e CC1101 desligados) |

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
| Coluna 8, linhas a–d | Nó MOSI, só na fase D (DevKitC 11, tela SDI, CC1101 pino 6) |
| Coluna 11, linhas a–d | Nó SCK, só na fase D (DevKitC 12, tela SCK, CC1101 pino 5) |
| Coluna 14, linhas a–d | Nó MISO, só na fase D (DevKitC 13, tela SDO, CC1101 pino 7) |
| Colunas 40, 41, 42, linha e | Transmissor de IR: DAT, VCC, GND |
| Colunas 50, 51, 52, linha e | Receptor de IR: OUT, VCC, GND |

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
| J3 | 9 | 39 | Botão CIMA |
| J3 | 17 | 47 | Botão BAIXO |
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

CC1101 pino 8 (GDO2) e pinos T_* da tela: sem ligação. Pinos 1 a 4 do CC1101 conforme o manual da Ebyte; 5 a 8 só por fonte secundária (ficha da LCSC do E07-M1101D-TH). Transmissor de IR ligado, mas o envio continua bloqueado no firmware (`infrared.tx_enabled` = 0) até o circuito da plaquinha ser identificado.

Montagem completa (fases A a D, já sem A6–A8): 7 FF, 20 FM e 7 MM, mais 1 MM por trilho dividido.

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

Um módulo em Failed, ou tensão caindo: desligar o USB e conferir só os fios daquele módulo.
