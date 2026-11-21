# Mapa da protoboard (perfil `dbmo-bench`)

| Campo | Valor |
|---|---|
| Versão | 0.1 (2026-11-21) |
| Escopo | Tudo o que liga sem solda: tela, CC1101 (só recepção), receptor e transmissor de IR na protoboard grande; 3 botões numa protoboard pequena separada |
| Fora do escopo | SD da tela e PN532 (pinos não soldados), áudio (sem pinos no perfil `dbmo-bench`; pinagem dos módulos e ligação do alto-falante não conferidas), MCP23017 (não possui) |
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

Desenho interativo: artifact "Bancada d-BMO" (https://claude.ai/artifact/Jcf6cXU6TdTkLStP5fy38u, privado).

Protoboard grande (MB-102, só o par de trilhos de cima):

| Onde | O quê |
|---|---|
| Coluna 8, linhas a–d | Nó MOSI (DevKitC 11, tela SDI, CC1101 pino 6) |
| Coluna 11, linhas a–d | Nó SCK (DevKitC 12, tela SCK, CC1101 pino 5) |
| Coluna 14, linhas a–d | Nó MISO (DevKitC 13, tela SDO, CC1101 pino 7) |
| Colunas 40, 41, 42, linha e | Transmissor de IR: DAT, VCC, GND |
| Colunas 50, 51, 52, linha e | Receptor de IR: OUT, VCC, GND |

Protoboard pequena (desenhada com 30 colunas e trilhos, 400 pontos; só o trilho (−) de baixo é usado), botões atravessando o canal central: BAIXO nas colunas 6–8, CIMA 18–20, OK 24–26. Fio do GPIO na linha a (coluna da direita), fio do GND na linha j (coluna da esquerda): pernas em diagonal. Se a pequena for de 170 pontos, sem trilhos, cada perna de GND vai direto a um dos três pinos GND do J3 (1, 21, 22) com jumper fêmea-macho, no lugar de B1 e B5–B7.

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
| A2 | A | DevKitC GND (J1) | Trilho (−), col. 6 | FM |
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
| B1 | B | DevKitC GND (J3, pino 22) | Pequena: trilho (−) de baixo, col. 1 | FM |
| B2 | B | DevKitC 47 | Pequena: botão BAIXO, col. 8 linha a | FM |
| B3 | B | DevKitC 39 | Pequena: botão CIMA, col. 20 linha a | FM |
| B4 | B | DevKitC 2 | Pequena: botão OK, col. 26 linha a | FM |
| B5 | B | Pequena: botão BAIXO, col. 6 linha j | Trilho (−) de baixo, col. 5 | MM |
| B6 | B | Pequena: botão CIMA, col. 18 linha j | Trilho (−) de baixo, col. 17 | MM |
| B7 | B | Pequena: botão OK, col. 24 linha j | Trilho (−) de baixo, col. 23 | MM |
| C1 | C | IR RX VCC, col. 51 linha b | Trilho (+), col. 54 | MM |
| C2 | C | IR RX GND, col. 52 linha a | Trilho (−), col. 56 | MM |
| C3 | C | DevKitC 4 | IR RX OUT, col. 50 linha a | FM |
| C4 | C | IR TX VCC, col. 41 linha b | Trilho (+), col. 39 | MM |
| C5 | C | IR TX GND, col. 42 linha a | Trilho (−), col. 44 | MM |
| C6 | C | DevKitC 5 | IR TX DAT, col. 40 linha a | FM |
| D1 | D | CC1101 pino 1 GND | Trilho (−), col. 58 | FM |
| D2 | D | CC1101 pino 2 VCC | Trilho (+), col. 61 | FM |
| D3 | D | CC1101 pino 3 GDO0 | DevKitC 15 | FF |
| D4 | D | CC1101 pino 4 CSN | DevKitC 14 (ao lado do 5V) | FF |
| D5 | D | CC1101 pino 5 SCK | Col. 11, linha b | FM |
| D6 | D | CC1101 pino 6 MOSI | Col. 8, linha b | FM |
| D7 | D | CC1101 pino 7 MISO | Col. 14, linha b | FM |

CC1101 pino 8 (GDO2) e pinos T_* da tela: sem ligação. Pinos 1 a 4 do CC1101 conforme o manual da Ebyte; 5 a 8 só por fonte secundária (ficha da LCSC do E07-M1101D-TH). Transmissor de IR ligado, mas o envio continua bloqueado no firmware (`infrared.tx_enabled` = 0) até o circuito da plaquinha ser identificado.

Total: 5 FF, 21 FM e 8 MM (mais 1 MM por trilho dividido).

## 5. Conferências com o multímetro

Todas com o **USB desligado**, salvo indicação.

| # | Quando | O que | Esperado |
|---|---|---|---|
| M1 | Protoboard vazia | Continuidade de ponta a ponta de cada trilho de cima | Bipa. Se não bipar, o trilho é dividido: ligar as duas metades com um MM na coluna 32 |
| BT | Antes de pôr os fios de cada botão | Continuidade entre o furo do GPIO (linha a) e o do GND (linha j) | **Só bipa com o botão apertado.** Se bipar sem apertar, girar o botão 90° |
| M2 | Antes de cada energização | Continuidade entre o trilho (+) e o trilho (−) | Sem bip contínuo (um bip curto ao encostar é normal) |
| NÓ | Depois da fase A e da fase C | Continuidade entre o pino 12 da DevKitC e o pino SCK da tela e do CC1101; o mesmo para 11 e 13 | Bipa. Confirma os nós do SPI |
| M5 | USB ligado, fase A montada | Tensão DC entre trilho (−) e trilho (+) | Cerca de 3,3 V |

## 6. Ordem

Montagem completa de uma vez, a pedido do usuário (21/11/2026). Conferências M1, BT, M2, NÓ e o teste do pino 1 do CC1101 antes de ligar o USB; depois M5, gravar o firmware e rodar `status`. Esperado: `display`, `rosto`, `input`, `infrared` e `radio` em Ready; `storage` e `nfc` em Absent. Um módulo em Failed: desligar o USB e conferir só os fios daquele módulo.
