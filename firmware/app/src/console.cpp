#include "dbmo/app/console.h"

#include <cstdio>
#include <cstring>

namespace dbmo::app {

Console::Console(IConsoleOutput &output) : output_(output) {
  (void)registerCommand({"help", "lista os comandos", &Console::helpCommand, nullptr});
}

Status Console::registerCommand(const Command &command) {
  if (command.name == nullptr || command.name[0] == '\0' || command.handler == nullptr)
    return Status::InvalidArgument;
  if (find(command.name) != nullptr) return Status::AlreadyInitialized;
  if (commandCount_ == kMaxCommands) return Status::NoSpace;
  commands_[commandCount_++] = command;
  return Status::Ok;
}

const Console::Command *Console::find(const char *name) const {
  for (size_t i = 0; i < commandCount_; ++i)
    if (strcmp(commands_[i].name, name) == 0) return &commands_[i];
  return nullptr;
}

void Console::print(const char *text) {
  if (text != nullptr) output_.write(text);
}

void Console::printf(const char *format, ...) {
  char buffer[kPrintMax];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  output_.write(buffer);
}

void Console::helpCommand(Console &console, int, const char *const *, void *) {
  for (size_t i = 0; i < console.commandCount_; ++i)
    console.printf("%-10s %s\r\n", console.commands_[i].name,
                   console.commands_[i].help != nullptr ? console.commands_[i].help : "");
}

void Console::feed(const char *text) {
  if (text == nullptr) return;
  for (; *text != '\0'; ++text) feed(*text);
}

void Console::feed(char c) {
  if (c == '\r' || c == '\n') {
    // "\r\n" conta como um fim de linha só.
    const bool tailOfCrLf = c == '\n' && lastWasCr_;
    lastWasCr_ = c == '\r';
    if (tailOfCrLf) return;

    if (echo_) output_.write("\r\n");
    if (overflow_) {
      output_.write("linha longa demais\r\n");
    } else {
      line_[length_] = '\0';
      const Status status = execute(line_);
      if (status == Status::NotFound) output_.write("comando desconhecido (digite help)\r\n");
      if (status == Status::InvalidArgument) output_.write("argumentos invalidos\r\n");
    }
    length_ = 0;
    overflow_ = false;
    return;
  }
  lastWasCr_ = false;

  if (c == '\b' || c == 0x7F) {
    if (length_ > 0) {
      --length_;
      if (echo_) output_.write("\b \b");
    }
    return;
  }
  if (c < 0x20 || c > 0x7E) return;  // só ASCII imprimível

  if (length_ == kLineMax) {
    overflow_ = true;
    return;
  }
  line_[length_++] = c;
  if (echo_) {
    const char echoed[2] = {c, '\0'};
    output_.write(echoed);
  }
}

Status Console::execute(const char *line) {
  if (line == nullptr) return Status::InvalidArgument;

  // Cópia local: a separação em argumentos escreve terminadores na linha.
  char buffer[kLineMax + 1];
  const size_t length = strlen(line);
  if (length > kLineMax) return Status::InvalidArgument;
  memcpy(buffer, line, length + 1);

  const char *argv[kMaxArgs];
  int argc = 0;
  char *cursor = buffer;
  while (true) {
    while (*cursor == ' ' || *cursor == '\t') ++cursor;
    if (*cursor == '\0') break;
    if (argc == static_cast<int>(kMaxArgs)) return Status::InvalidArgument;

    if (*cursor == '"') {
      // Argumento entre aspas: pode conter espaços.
      argv[argc++] = ++cursor;
      while (*cursor != '\0' && *cursor != '"') ++cursor;
      if (*cursor == '\0') return Status::InvalidArgument;
    } else {
      argv[argc++] = cursor;
      while (*cursor != '\0' && *cursor != ' ' && *cursor != '\t') ++cursor;
      if (*cursor == '\0') break;
    }
    *cursor++ = '\0';
  }
  if (argc == 0) return Status::Ok;

  const Command *command = find(argv[0]);
  if (command == nullptr) return Status::NotFound;
  command->handler(*this, argc, argv, command->context);
  return Status::Ok;
}

}  // namespace dbmo::app
