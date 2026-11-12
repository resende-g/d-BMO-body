// Catálogo das expressões do rosto. Os desenhos ficam em firmware/rosto/rosto_faces.h
// (a mesma tabela que o sketch de bancada usa); aqui está o acesso tipado a ela.
#pragma once

#include <cstddef>
#include <cstdint>

#include "dbmo/core/status.h"

namespace dbmo::rosto {

inline constexpr int kGridWidth = 16;
inline constexpr int kGridHeight = 12;

// Um caractere por bloco do quadro.
inline constexpr char kPixelSignal = '#';      // vermelho sinal
inline constexpr char kPixelDark = '+';        // vermelho escuro
inline constexpr char kPixelBackground = '.';  // fundo

// Os dez primeiros são os estados da especificação do personagem (v2.0), na ordem dela.
// Os sete seguintes são extensões do firmware, ainda não incorporadas à especificação.
enum class Expression : uint8_t {
  Idle = 0,
  Thinking,
  Scanning,
  Analyzing,
  Success,
  Alert,
  Error,
  Protecting,
  Curious,
  Offline,
  Happy,
  Sad,
  Angry,
  Listening,
  Speaking,
  Warning,
  Sleep,
};
inline constexpr uint8_t kExpressionCount = 17;

struct FrameView {
  const char *const *rows;  // kGridHeight linhas de kGridWidth caracteres
  uint16_t durationMs;
};

// Nome em maiúsculas, como na especificação ("IDLE", "THINKING"...).
const char *name(Expression expression);
// Aceita o nome sem diferenciar maiúsculas de minúsculas. NotFound se não existe.
Result<Expression> expressionFromName(const char *text);
// OutOfRange se index >= kExpressionCount.
Result<Expression> expressionFromIndex(uint32_t index);

uint8_t frameCount(Expression expression);
// Requer index < frameCount(expression).
FrameView frame(Expression expression, uint8_t index);

// Escreve o quadro como texto, uma linha por fileira, terminadas em '\n'. Devolve o
// tamanho escrito, ou 0 se não coube (precisa de kGridHeight * (kGridWidth + 1) + 1).
size_t renderText(const FrameView &view, char *out, size_t capacity);

}  // namespace dbmo::rosto
