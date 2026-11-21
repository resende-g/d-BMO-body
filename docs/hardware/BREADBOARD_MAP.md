# Mapa da protoboard (perfil `dbmo-bench`)

| Campo | Valor |
|---|---|
| Versão | 0.1 (2026-11-21) |
| Escopo | Tela, 3 botões, receptor de IR e CC1101 (só recepção). Sem solda |
| Fora do escopo | SD da tela e PN532 (pinos não soldados), áudio (sem pinos no perfil `dbmo-bench`), transmissor de IR (circuito da placa não identificado), MCP23017 (não possui) |
| Pinagem | `firmware/config/include/dbmo/config/pins.h`, perfil `bench` ([`GPIO_MAP.md`](GPIO_MAP.md)) |
| Estado | **Não montado nem testado** com este mapa |

## 1. Princípio da montagem

- A **DevKitC fica fora da protoboard**, ligada por jumpers.
- A protoboard serve só para o que é **compartilhado**: trilho de 3,3 V, trilho de GND, os
  três sinais do SPI (SCK, MOSI, MISO), os botões e o receptor de IR.
- Sinal que liga só dois pontos (CS, DC, RESET, GDO0) vai **direto, com jumper fêmea-fêmea**,
  sem passar pela protoboard.
- O pino **5V da DevKitC não é usado**.

## 2. Desenho

O desenho interativo está no artifact "Bancada d-BMO" (https://claude.ai/artifact/Jcf6cXU6TdTkLStP5fy38u, privado). Resumo da disposição na protoboard MB-102 (colunas 1 a 63 impressas na placa; só o par de trilhos de cima é usado):

| Onde | O quê |
|---|---|
| Coluna 8, linhas a–d | Nó MOSI (DevKitC 11, tela SDI, CC1101 pino 6) |
| Coluna 11, linhas a–d | Nó SCK (DevKitC 12, tela SCK, CC1101 pino 5) |
| Coluna 14, linhas a–d | Nó MISO (DevKitC 13, tela SDO, CC1101 pino 7) |
| Colunas 24–26, atravessando o canal | Botão OK |
| Colunas 30–32, atravessando o canal | Botão CIMA |
| Colunas 36–38, atravessando o canal | Botão BAIXO |
| Colunas 50, 51, 52, linha e | Receptor de IR: OUT, VCC, GND |

Em cada botão, o fio do GND fica na metade de cima (coluna da esquerda, linha b) e o do GPIO na metade de baixo (coluna da direita, linha g): pernas em diagonal.

## 3. Pinos da DevKitC usados

A ordem abaixo segue o guia do usuário da DevKitC-1, conectores J1 e J3; foi confirmada na
fonte apenas para alguns pinos (DKC-07, DKC-10, DKC-01, DKC-02, DKC-18 em
`sources/ESP32-S3.md`). **Confira cada pino pelo nome impresso ao lado dele na placa.**

| Conector | Pino | Nome na placa | Uso aqui |
|---|---:|---|---|
| J1 | 1 ou 2 | 3V3 | Trilho (+) |
| J1 | 4 | 4 | Receptor de IR (OUT) |
| J1 | 8 | 15 | CC1101 GDO0 |
| J1 | 10 | 17 | Tela DC |
| J1 | 11 | 18 | Tela RESET |
| J1 | 16 | 10 | Tela CS |
| J1 | 17 | 11 | Nó MOSI |
| J1 | 18 | 12 | Nó SCK |
| J1 | 19 | 13 | Nó MISO |
| J1 | 20 | 14 | CC1101 CSN |
| J1 | 21 | **5V** | **Não usar.** Fica ao lado do 14: conferir duas vezes o jumper do CSN |
| J1 | 22 | G / GND | Trilho (−) |
| J3 | 5 | 2 | Botão OK |
| J3 | 9 | 39 | Botão CIMA |
| J3 | 17 | 47 | Botão BAIXO |

## 4. Lista de ligações

Tipos: **FF** fêmea-fêmea, **FM** fêmea-macho, **MM** macho-macho.

| # | Fase | De | Para | Tipo |
|---|---|---|---|---|
| A1 | A | DevKitC 3V3 | Trilho (+), col. 46 | FM |
| A2 | A | DevKitC GND | Trilho (−), col. 6 | FM |
| A3 | A | Tela VCC | Trilho (+), col. 2 | FM |
| A4 | A | Tela GND | Trilho (−), col. 4 | FM |
| A5 | A | Tela LED | Trilho (+), col. 12 | FM |
| A6 | A | Tela SDI (MOSI) | Col. 8, linha a | FM |
| A7 | A | Tela SCK | Col. 11, linha a | FM |
| A8 | A | Tela SDO (MISO) | Col. 14, linha a | FM |
| A9 | A | DevKitC 11 | Col. 8, linha d | FM |
| A10 | A | DevKitC 12 | Col. 11, linha d | FM |
| A11 | A | DevKitC 13 | Col. 14, linha d | FM |
| A12 | A | DevKitC 10 | Tela CS | FF |
| A13 | A | DevKitC 18 | Tela RESET | FF |
| A14 | A | DevKitC 17 | Tela DC | FF |
| A15 | A | Botão OK, col. 24 linha b | Trilho (−), col. 23 | MM |
| A16 | A | Botão CIMA, col. 30 linha b | Trilho (−), col. 29 | MM |
| A17 | A | Botão BAIXO, col. 36 linha b | Trilho (−), col. 35 | MM |
| A18 | A | DevKitC 2 | Botão OK, col. 26 linha g | FM |
| A19 | A | DevKitC 39 | Botão CIMA, col. 32 linha g | FM |
| A20 | A | DevKitC 47 | Botão BAIXO, col. 38 linha g | FM |
| B1 | B | IR VCC, col. 51 linha b | Trilho (+), col. 54 | MM |
| B2 | B | IR GND, col. 52 linha a | Trilho (−), col. 56 | MM |
| B3 | B | DevKitC 4 | IR OUT, col. 50 linha a | FM |
| C1 | C | CC1101 pino 1 GND | Trilho (−), col. 58 | FM |
| C2 | C | CC1101 pino 2 VCC | Trilho (+), col. 61 | FM |
| C3 | C | CC1101 pino 3 GDO0 | DevKitC 15 | FF |
| C4 | C | CC1101 pino 4 CSN | DevKitC 14 (ao lado do 5V) | FF |
| C5 | C | CC1101 pino 5 SCK | Col. 11, linha b | FM |
| C6 | C | CC1101 pino 6 MOSI | Col. 8, linha b | FM |
| C7 | C | CC1101 pino 7 MISO | Col. 14, linha b | FM |

CC1101 pino 8 (GDO2) e pinos T_* da tela: sem ligação. Pinos 1 a 4 do CC1101 conforme o manual da Ebyte; 5 a 8 só por fonte secundária (ficha da LCSC do E07-M1101D-TH).

Total: 5 FF, 20 FM e 5 MM (mais 1 MM se o trilho for dividido).

## 5. Conferências com o multímetro

Todas com o **USB desligado**, salvo indicação.

| # | Quando | O que | Esperado |
|---|---|---|---|
| M1 | Protoboard vazia | Continuidade de ponta a ponta de cada trilho de cima | Bipa. Se não bipar, o trilho é dividido: ligar as duas metades com um MM na coluna 32 |
| BT | Antes de pôr os fios de cada botão | Continuidade entre o furo do GND (metade de cima) e o do GPIO (metade de baixo) | **Só bipa com o botão apertado.** Se bipar sem apertar, girar o botão 90° |
| M2 | Antes de cada energização | Continuidade entre o trilho (+) e o trilho (−) | Sem bip contínuo (um bip curto ao encostar é normal) |
| NÓ | Depois da fase A e da fase C | Continuidade entre o pino 12 da DevKitC e o pino SCK da tela e do CC1101; o mesmo para 11 e 13 | Bipa. Confirma os nós do SPI |
| M5 | USB ligado, fase A montada | Tensão DC entre trilho (−) e trilho (+) | Cerca de 3,3 V |

## 6. Ordem

1. M1, montar a fase A, BT, M2, ligar o USB, M5, gravar o firmware e fazer o teste T2.
2. Desligar o USB, montar a fase B, M2, teste T3.
3. Desligar o USB, teste T1 no CC1101, montar a fase C, M2, NÓ, teste T4.
