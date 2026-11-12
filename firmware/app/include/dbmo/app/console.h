// Console de linha de comando da bancada (serial). Lógica portável: recebe caracteres,
// separa a linha em argumentos e chama o comando registrado. Não conhece a UART.
#pragma once

#include <cstdarg>
#include <cstddef>

#include "dbmo/core/status.h"

namespace dbmo::app {

class IConsoleOutput {
 public:
  virtual ~IConsoleOutput() = default;
  virtual void write(const char *text) = 0;
};

class Console {
 public:
  static constexpr size_t kLineMax = 96;
  static constexpr size_t kMaxArgs = 6;
  static constexpr size_t kMaxCommands = 24;
  static constexpr size_t kPrintMax = 160;

  // argv[0] é o nome do comando. As strings só valem durante a chamada.
  using Handler = void (*)(Console &console, int argc, const char *const *argv, void *context);

  struct Command {
    const char *name;
    const char *help;
    Handler handler;
    void *context;
  };

  explicit Console(IConsoleOutput &output);

  // O comando "help" já vem registrado e lista os demais.
  Status registerCommand(const Command &command);

  // Entrega um caractere recebido. '\r' ou '\n' executa a linha; backspace apaga.
  void feed(char c);
  void feed(const char *text);

  // Executa uma linha inteira. NotFound se o comando não existe; InvalidArgument se há
  // argumentos demais ou aspas sem fechar. Linha vazia devolve Ok sem fazer nada.
  Status execute(const char *line);

  void print(const char *text);
  void printf(const char *format, ...) __attribute__((format(printf, 2, 3)));

  // Repete na saída o que é digitado (terminais sem eco local).
  void setEcho(bool enabled) { echo_ = enabled; }

  size_t commandCount() const { return commandCount_; }

 private:
  static void helpCommand(Console &console, int argc, const char *const *argv, void *context);
  const Command *find(const char *name) const;

  IConsoleOutput &output_;
  Command commands_[kMaxCommands] = {};
  size_t commandCount_ = 0;
  char line_[kLineMax + 1] = {};
  size_t length_ = 0;
  bool overflow_ = false;
  bool echo_ = false;
  bool lastWasCr_ = false;
};

}  // namespace dbmo::app
