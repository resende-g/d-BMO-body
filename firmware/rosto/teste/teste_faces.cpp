// SPDX-License-Identifier: Apache-2.0
// Confere a tabela de rostos no computador, sem placa:
//   c++ -std=c++17 firmware/rosto/teste/teste_faces.cpp -o /tmp/teste_faces && /tmp/teste_faces
// (fica fora da raiz do sketch porque o Arduino compila todo .cpp da raiz junto)
// Com "--ver", imprime todos os quadros em texto para revisar o desenho.
#include <cstdio>
#include <cstring>
#include "../rosto_faces.h"

int main(int argc, char **argv) {
  bool ver = argc > 1 && strcmp(argv[1], "--ver") == 0;
  // Os dez estados da spec (resende-g/d-BMO, CHARACTER_SPECIFICATION.md), nesta ordem.
  // Sem assert aqui e abaixo: o teste tem que falhar também com -DNDEBUG.
  const char *SPEC[] = {"IDLE", "THINKING", "SCANNING", "ANALYZING", "SUCCESS",
                        "ALERT", "ERROR", "PROTECTING", "CURIOUS", "OFFLINE"};
  const int SPEC_COUNT = sizeof(SPEC) / sizeof(SPEC[0]);
  if (FACE_COUNT != SPEC_COUNT) {
    fprintf(stderr, "%d rostos na tabela, a spec tem %d\n", FACE_COUNT, SPEC_COUNT);
    return 1;
  }
  for (int f = 0; f < FACE_COUNT; f++)
    if (strcmp(FACES[f].name, SPEC[f]) != 0) {
      fprintf(stderr, "estado %d: \"%s\", a spec diz \"%s\"\n", f, FACES[f].name, SPEC[f]);
      return 1;
    }
  for (int f = 0; f < FACE_COUNT; f++) {
    const Face &face = FACES[f];
    if (face.count < 1) {
      fprintf(stderr, "%s: sem quadros\n", face.name);
      return 1;
    }
    for (int q = 0; q < face.count; q++) {
      const Frame &fr = face.frames[q];
      if (fr.ms == 0) {
        fprintf(stderr, "%s quadro %d: duração zero\n", face.name, q);
        return 1;
      }
      if (ver) printf("%s, quadro %d (%d ms)\n", face.name, q, fr.ms);
      for (int y = 0; y < GRID_H; y++) {
        const char *row = fr.rows[y];
        if (!row) {
          fprintf(stderr, "%s quadro %d linha %d: faltando\n", face.name, q, y);
          return 1;
        }
        if (strlen(row) != GRID_W) {
          fprintf(stderr, "%s quadro %d linha %d: %zu colunas, esperado %d\n",
                  face.name, q, y, strlen(row), GRID_W);
          return 1;
        }
        if (strspn(row, "#+.") != GRID_W) {
          fprintf(stderr, "%s quadro %d linha %d: caractere fora de '#', '+' e '.'\n", face.name, q, y);
          return 1;
        }
        if (ver) printf("  %s\n", row);
      }
    }
  }
  printf("ok: %d rostos conferidos\n", FACE_COUNT);
}
