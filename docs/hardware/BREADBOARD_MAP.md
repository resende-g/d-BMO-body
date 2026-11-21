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

Protoboard MB-102 vista de cima, colunas numeradas de 1 a 63 (impressas na placa).
Usar só o par de trilhos **de cima**.

```
             col: 1    5    10   13   16        25   27   32   34   39   41        50 51 52
                  |    |    |    |    |         |    |    |    |    |    |         |  |  |
 trilho (+) 3V3   ═══════════════════════════════════════════════════════════════════════════
 trilho (−) GND   ═══════════════════════════════════════════════════════════════════════════

       linha a    .    .    o    o    o    .    .    .    .    .    .    .    .    o  o  o
       linha b    .    .    o    o    o    .    o    .    .    o    .    .    o    o  o  o
       linha c    .    .    o    o    o    .    .    .    .    .    .    .    .   [R  R  R]
       linha d    .    .    o    o    o    .    .    .    .    .    .    .    .    .  .  .
       linha e    .    .    .    .    .   [B1--------]  [B2--------]  [B3--------] .  .  .
                            SCK  MOSI MISO   CIMA(39)      OK(2)        BAIXO(47)   OUT VCC GND
       ═══════════════════════ canal central ═══════════════════════════════════════════════
       linha f    .    .    .    .    .   [B1--------]  [B2--------]  [B3--------] .  .  .
       linha g    .    .    .    .    .    o    .    .    o    .    .    o    .    .  .  .
       linha h..j .    .    .    .    .    .    .    .    .    .    .    .    .    .  .  .

 o = furo usado por um jumper     [R R R] = pinos do receptor de IR     [B...] = botão
```

- **Colunas 10, 13 e 16 (linhas a–d):** são os "nós" do SPI. Cada coluna recebe 3 jumpers:
  DevKitC, tela e CC1101.
- **Botões:** cada um atravessa o canal central. Na metade de cima vai o fio para o GPIO;
  na metade de baixo, o fio para o GND (ver a seção 5, que define as pernas certas).
- **Receptor de IR:** pinos nas colunas 50 (OUT), 51 (VCC) e 52 (GND), linha c, com a parte
  do sensor virada para fora da placa.

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

Tipos: **FF** fêmea-fêmea, **FM** fêmea-macho, **MM** macho-macho. Cores sugeridas:
vermelho = 3,3 V, preto = GND, amarelo = SCK, verde = MOSI, azul = MISO, outras = sinais.

### Fase A: alimentação, tela e botões (teste T2)

| # | De | Para | Tipo | Cor |
|---:|---|---|---|---|
| A1 | DevKitC 3V3 | Trilho (+) | FM | vermelho |
| A2 | DevKitC GND | Trilho (−) | FM | preto |
| A3 | Tela VCC | Trilho (+) | FM | vermelho |
| A4 | Tela GND | Trilho (−) | FM | preto |
| A5 | Tela LED | Trilho (+) | FM | vermelho |
| A6 | DevKitC 12 | Coluna 10 (SCK) | FM | amarelo |
| A7 | Tela SCK | Coluna 10 (SCK) | FM | amarelo |
| A8 | DevKitC 11 | Coluna 13 (MOSI) | FM | verde |
| A9 | Tela SDI (MOSI) | Coluna 13 (MOSI) | FM | verde |
| A10 | DevKitC 13 | Coluna 16 (MISO) | FM | azul |
| A11 | Tela SDO (MISO) | Coluna 16 (MISO) | FM | azul |
| A12 | DevKitC 10 | Tela CS | FF | — |
| A13 | DevKitC 17 | Tela DC (ou RS) | FF | — |
| A14 | DevKitC 18 | Tela RESET | FF | — |
| A15 | DevKitC 39 | Botão CIMA, metade de cima | FM | — |
| A16 | DevKitC 2 | Botão OK, metade de cima | FM | — |
| A17 | DevKitC 47 | Botão BAIXO, metade de cima | FM | — |
| A18 | Botão CIMA, metade de baixo | Trilho (−) | MM | preto |
| A19 | Botão OK, metade de baixo | Trilho (−) | MM | preto |
| A20 | Botão BAIXO, metade de baixo | Trilho (−) | MM | preto |

Pinos T_CLK, T_CS, T_DIN, T_DO e T_IRQ da tela: **sem ligação**.

### Fase B: receptor de IR (teste T3)

| # | De | Para | Tipo | Cor |
|---:|---|---|---|---|
| B1 | Coluna 51 (VCC do receptor) | Trilho (+) | MM | vermelho |
| B2 | Coluna 52 (GND do receptor) | Trilho (−) | MM | preto |
| B3 | DevKitC 4 | Coluna 50 (OUT do receptor) | FM | — |

### Fase C: CC1101 E07-M1101D (teste T4)

O conector do módulo é 2×4: **não encaixar na protoboard**. Pino 1 marcado com "1 •".
Antes, fazer o teste T1 (pino 1 com continuidade para a rosca do conector da antena).

| # | Pino do módulo | Para | Tipo | Cor |
|---:|---|---|---|---|
| C1 | 1 GND | Trilho (−) | FM | preto |
| C2 | 2 VCC | Trilho (+) | FM | vermelho |
| C3 | 3 GDO0 | DevKitC 15 | FF | — |
| C4 | 4 CSN | DevKitC 14 | FF | — |
| C5 | 5 SCK | Coluna 10 (SCK) | FM | amarelo |
| C6 | 6 MOSI | Coluna 13 (MOSI) | FM | verde |
| C7 | 7 MISO | Coluna 16 (MISO) | FM | azul |
| C8 | 8 GDO2 | sem ligação | — | — |

Pinos 1 a 4 conforme o manual da Ebyte; 5 a 8 só por fonte secundária (ficha da LCSC do
E07-M1101D-TH). Antena rosqueada antes de energizar.

Total: 5 FF, 20 FM e 5 MM (mais 1 MM se o trilho for dividido).

## 5. Conferências com o multímetro

Todas com o **USB desligado**, salvo indicação.

| # | Quando | O que | Esperado |
|---|---|---|---|
| M1 | Protoboard vazia | Continuidade de ponta a ponta de cada trilho de cima | Bipa. Se não bipar, o trilho é dividido: ligar as duas metades com um MM na coluna 32 |
| BT | Antes de pôr os fios de cada botão | Continuidade entre um furo da metade de cima e um da metade de baixo, nas colunas das pernas do botão | Escolher o par que **só bipa com o botão apertado**. Se bipar sem apertar, as pernas estão ligadas por dentro: usar a perna da diagonal |
| M2 | Antes de cada energização | Continuidade entre o trilho (+) e o trilho (−) | Sem bip contínuo (um bip curto ao encostar é normal) |
| NÓ | Depois da fase A e da fase C | Continuidade entre o pino 12 da DevKitC e o pino SCK da tela e do CC1101; o mesmo para 11 e 13 | Bipa. Confirma os nós do SPI |
| M5 | USB ligado, fase A montada | Tensão DC entre trilho (−) e trilho (+) | Cerca de 3,3 V |

## 6. Ordem

1. M1, montar a fase A, BT, M2, ligar o USB, M5, gravar o firmware e fazer o teste T2.
2. Desligar o USB, montar a fase B, M2, teste T3.
3. Desligar o USB, teste T1 no CC1101, montar a fase C, M2, NÓ, teste T4.
