// Códigos de erro comuns a todo o firmware e o tipo Result<T>.
// O firmware compila sem exceções: toda falha volta como Status.
#pragma once

#include <cstdint>

namespace dbmo {

enum class [[nodiscard]] Status : uint8_t {
  Ok = 0,
  InvalidArgument,
  OutOfRange,
  NotInitialized,
  AlreadyInitialized,
  NotFound,  // arquivo, chave ou registro inexistente
  Timeout,
  Busy,
  IoError,  // falha de barramento, leitura ou escrita
  CrcError,
  ProtocolError,  // resposta malformada do periférico
  Corrupt,        // dado persistido inválido
  NoSpace,        // buffer, fila ou armazenamento cheio
  Unsupported,
  NotPermitted,    // bloqueado por política (ex.: transmissão fora da faixa permitida)
  Incompatible,    // versão ou capacidade incompatível
  HardwareAbsent,  // o periférico não respondeu
  InternalError,
};

constexpr bool isOk(Status status) { return status == Status::Ok; }

// Nome estável do código, para log e testes.
const char *toString(Status status);

// Valor ou erro. Quando !ok(), value() devolve um T{} e não deve ser usado.
template <typename T>
class [[nodiscard]] Result {
 public:
  Result(T value) : value_(value), status_(Status::Ok) {}  // NOLINT(google-explicit-constructor)
  Result(Status error) : value_(), status_(error) {}       // NOLINT(google-explicit-constructor)

  bool ok() const { return status_ == Status::Ok; }
  Status status() const { return status_; }
  const T &value() const { return value_; }
  T valueOr(T fallback) const { return ok() ? value_ : fallback; }

 private:
  T value_;
  Status status_;
};

}  // namespace dbmo

// Propaga o erro para quem chamou.
#define DBMO_TRY(expr)                                                   \
  do {                                                                   \
    const ::dbmo::Status dbmo_try_status_ = (expr);                      \
    if (dbmo_try_status_ != ::dbmo::Status::Ok) return dbmo_try_status_; \
  } while (0)
