// Rostos do d-BMO: os 10 estados da spec (resende-g/d-BMO, docs/brand/CHARACTER_SPECIFICATION.md)
// e mais 7 estados do firmware (HAPPY, SAD, ANGRY, LISTENING, SPEAKING, WARNING, SLEEP).
// Grade fixa de 16 × 12 blocos (20 px cada na tela de 320 × 240). Um caractere por bloco:
//   '#' vermelho sinal (#D92332)   '+' vermelho escuro (#8F1722)   '.' fundo (#090A0C)
// Sem dependência do Arduino, para o teste_faces.cpp compilar no computador.
#pragma once
#include <stdint.h>

constexpr int GRID_W = 16;
constexpr int GRID_H = 12;

struct Frame {
  const char *rows[GRID_H];
  uint16_t ms;  // quanto tempo o quadro fica na tela
};

struct Face {
  const char *name;
  const Frame *frames;
  uint8_t count;
};

#define VAZIO "................"

// Olhos de bloco 2 × 3 e sorriso curto: o rosto da referência conceitual v1.
const Frame ROSTO_IDLE[] = {
    {{VAZIO, VAZIO, VAZIO, "....##....##....", "....##....##....", "....##....##....", VAZIO, VAZIO,
      ".....#....#.....", "......####......", VAZIO, VAZIO},
     3000},
    {{VAZIO, VAZIO, VAZIO, VAZIO, VAZIO,  // piscada
      "....##....##....", VAZIO, VAZIO, ".....#....#.....", "......####......", VAZIO, VAZIO},
     150},
};

// Olhando para cima e reticências crescendo.
const Frame ROSTO_THINKING[] = {
    {{VAZIO, VAZIO, ".....##....##...", ".....##....##...", ".....##....##...", VAZIO, VAZIO, VAZIO,
      ".......###......", VAZIO, ".......+........", VAZIO},
     400},
    {{VAZIO, VAZIO, ".....##....##...", ".....##....##...", ".....##....##...", VAZIO, VAZIO, VAZIO,
      ".......###......", VAZIO, ".......+.+......", VAZIO},
     400},
    {{VAZIO, VAZIO, ".....##....##...", ".....##....##...", ".....##....##...", VAZIO, VAZIO, VAZIO,
      ".......###......", VAZIO, ".......+.+.+....", VAZIO},
     400},
};

// Olhos em faixa varrendo de um lado para o outro.
const Frame ROSTO_SCANNING[] = {
    {{VAZIO, VAZIO, VAZIO, VAZIO, "..###....###....", VAZIO, VAZIO, VAZIO, VAZIO,
      ".....######.....", VAZIO, VAZIO},
     300},
    {{VAZIO, VAZIO, VAZIO, VAZIO, "...###....###...", VAZIO, VAZIO, VAZIO, VAZIO,
      ".....######.....", VAZIO, VAZIO},
     300},
    {{VAZIO, VAZIO, VAZIO, VAZIO, "....###....###..", VAZIO, VAZIO, VAZIO, VAZIO,
      ".....######.....", VAZIO, VAZIO},
     300},
    {{VAZIO, VAZIO, VAZIO, VAZIO, "...###....###...", VAZIO, VAZIO, VAZIO, VAZIO,
      ".....######.....", VAZIO, VAZIO},
     300},
};

// Um olho apertado de cada vez, boca de lado.
const Frame ROSTO_ANALYZING[] = {
    {{VAZIO, VAZIO, VAZIO, "..........##....", "..........##....", "....##....##....", VAZIO, VAZIO,
      VAZIO, "........##......", VAZIO, VAZIO},
     600},
    {{VAZIO, VAZIO, VAZIO, "....##..........", "....##..........", "....##....##....", VAZIO, VAZIO,
      VAZIO, "......##........", VAZIO, VAZIO},
     600},
};

// Olhos em arco e sorriso largo.
const Frame ROSTO_SUCCESS[] = {
    {{VAZIO, VAZIO, VAZIO, "....##....##....", "...#..#..#..#...", VAZIO, VAZIO, "...#........#...",
      "....#......#....", ".....######.....", VAZIO, VAZIO},
     1000},
};

// Olhos arregalados e boca aberta, piscando entre as duas cores.
const Frame ROSTO_ALERT[] = {
    {{VAZIO, VAZIO, "....##....##....", "....##....##....", "....##....##....", "....##....##....",
      VAZIO, VAZIO, ".......##.......", "......#..#......", ".......##.......", VAZIO},
     400},
    {{VAZIO, VAZIO, "....++....++....", "....++....++....", "....++....++....", "....++....++....",
      VAZIO, VAZIO, ".......++.......", "......+..+......", ".......++.......", VAZIO},
     400},
};

// Olhos em X e boca triste.
const Frame ROSTO_ERROR[] = {
    {{VAZIO, VAZIO, VAZIO, "...#.#....#.#...", "....#......#....", "...#.#....#.#...", VAZIO, VAZIO,
      "......####......", ".....#....#.....", VAZIO, VAZIO},
     1000},
};

// Olhos firmes, boca reta e uma moldura que pulsa.
const Frame ROSTO_PROTECTING[] = {
    {{VAZIO, VAZIO, VAZIO, "...##......##...", "...###....###...", "...###....###...", VAZIO, VAZIO,
      VAZIO, ".....######.....", VAZIO, VAZIO},
     700},
    {{"++++++++++++++++", "+..............+", "+..............+", "+..##......##..+",
      "+..###....###..+", "+..###....###..+", "+..............+", "+..............+",
      "+..............+", "+....######....+", "+..............+", "++++++++++++++++"},
     700},
};

// Um olho grande, outro pequeno e erguido, boquinha de lado.
const Frame ROSTO_CURIOUS[] = {
    {{VAZIO, VAZIO, "...........#....", "....##.....#....", "....##..........", "....##..........",
      VAZIO, VAZIO, ".........##.....", ".........##.....", VAZIO, VAZIO},
     1000},
};

// Olhos fechados em vermelho escuro e um "z" de vez em quando.
const Frame ROSTO_OFFLINE[] = {
    {{VAZIO, VAZIO, VAZIO, VAZIO, VAZIO, "....++....++....", VAZIO, VAZIO, VAZIO, VAZIO, VAZIO,
      VAZIO},
     1200},
    {{VAZIO, "............+++.", ".............+..", "............+++.", VAZIO, "....++....++....",
      VAZIO, VAZIO, VAZIO, VAZIO, VAZIO, VAZIO},
     1200},
};

// ---------------------------------------------------------------------------
// Estados acrescentados pelo firmware (ainda não constam da especificação do personagem,
// versão 2.0, que lista os dez acima). Mesma grade, mesmos três caracteres.
// ---------------------------------------------------------------------------

// Olhos cheios que sobem e descem um bloco e boca aberta em sorriso largo.
const Frame ROSTO_HAPPY[] = {
    {{VAZIO, VAZIO, VAZIO, "....##....##....", "....##....##....", VAZIO, VAZIO, "...#........#...",
      "...##########...", "....########....", ".....######.....", VAZIO},
     500},
    {{VAZIO, VAZIO, "....##....##....", "....##....##....", VAZIO, VAZIO, VAZIO, "...#........#...",
      "...##########...", "....########....", ".....######.....", VAZIO},
     500},
};

// Olhos caídos para fora, boca para baixo e uma lágrima que escorre.
const Frame ROSTO_SAD[] = {
    {{VAZIO, VAZIO, VAZIO, VAZIO, "....##....##....", "...###....###...", "...+............", VAZIO,
      VAZIO, "......####......", ".....#....#.....", VAZIO},
     700},
    {{VAZIO, VAZIO, VAZIO, VAZIO, "....##....##....", "...###....###...", VAZIO, "...+............",
      VAZIO, "......####......", ".....#....#.....", VAZIO},
     700},
};

// Sobrancelhas em diagonal para o centro e boca larga virada para baixo.
const Frame ROSTO_ANGRY[] = {
    {{VAZIO, VAZIO, "...#........#...", "...##......##...", "....##....##....", "....##....##....",
      "....##....##....", VAZIO, VAZIO, ".....######.....", "....#......#....", VAZIO},
     1000},
};

// Ondas de som nas laterais enquanto o microfone capta.
const Frame ROSTO_LISTENING[] = {
    {{VAZIO, VAZIO, VAZIO, "....##....##....", ".+..##....##..+.", ".+..##....##..+.", VAZIO, VAZIO,
      VAZIO, ".......##.......", VAZIO, VAZIO},
     400},
    {{VAZIO, VAZIO, VAZIO, "+...##....##...+", "+.+.##....##.+.+", "+.+.##....##.+.+",
      "+..............+", VAZIO, VAZIO, ".......##.......", VAZIO, VAZIO},
     400},
};

// Boca que abre e fecha em três tempos.
const Frame ROSTO_SPEAKING[] = {
    {{VAZIO, VAZIO, VAZIO, "....##....##....", "....##....##....", "....##....##....", VAZIO, VAZIO,
      VAZIO, "......####......", VAZIO, VAZIO},
     180},
    {{VAZIO, VAZIO, VAZIO, "....##....##....", "....##....##....", "....##....##....", VAZIO, VAZIO,
      "......####......", "......#..#......", "......####......", VAZIO},
     180},
    {{VAZIO, VAZIO, VAZIO, "....##....##....", "....##....##....", "....##....##....", VAZIO, VAZIO,
      ".....######.....", ".....#....#.....", ".....######.....", VAZIO},
     180},
};

// Ponto de exclamação entre os olhos, alternando entre as duas cores.
const Frame ROSTO_WARNING[] = {
    {{VAZIO, ".......##.......", ".......##.......", "...##..##..##...", "...##..##..##...",
      ".......##.......", VAZIO, ".......##.......", VAZIO, ".....######.....", VAZIO, VAZIO},
     500},
    {{VAZIO, ".......++.......", ".......++.......", "...##..++..##...", "...##..++..##...",
      ".......++.......", VAZIO, ".......++.......", VAZIO, ".....######.....", VAZIO, VAZIO},
     500},
};

// Olhos fechados em arco para baixo e uma respiração lenta na boca.
const Frame ROSTO_SLEEP[] = {
    {{VAZIO, VAZIO, VAZIO, VAZIO, "...#..#..#..#...", "....##....##....", VAZIO, VAZIO, VAZIO,
      ".......##.......", VAZIO, VAZIO},
     1500},
    {{VAZIO, VAZIO, VAZIO, VAZIO, "...#..#..#..#...", "....##....##....", VAZIO, VAZIO, VAZIO,
      "......####......", VAZIO, VAZIO},
     1500},
};

#define FACE(nome) \
  { #nome, ROSTO_##nome, sizeof(ROSTO_##nome) / sizeof(ROSTO_##nome[0]) }

// A ordem é parte do contrato: os dez primeiros são os estados da especificação e os
// índices são usados pelo sketch de bancada e por dbmo::rosto::Expression.
const Face FACES[] = {
    FACE(IDLE),  FACE(THINKING),   FACE(SCANNING), FACE(ANALYZING), FACE(SUCCESS), FACE(ALERT),
    FACE(ERROR), FACE(PROTECTING), FACE(CURIOUS),  FACE(OFFLINE),   FACE(HAPPY),   FACE(SAD),
    FACE(ANGRY), FACE(LISTENING),  FACE(SPEAKING), FACE(WARNING),   FACE(SLEEP),
};
constexpr int FACE_COUNT = sizeof(FACES) / sizeof(FACES[0]);

#undef FACE
#undef VAZIO
