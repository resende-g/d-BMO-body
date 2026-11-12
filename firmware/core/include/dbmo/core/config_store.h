// Configurações do firmware: chaves tipadas, com padrão e faixa válida.
//
// Cada módulo declara as suas chaves com define(); o núcleo não conhece nenhuma. A
// persistência é em texto ("chave=valor" por linha) por um IConfigBackend, que o módulo
// de armazenamento (cartão) ou o porte (NVS) implementa.
#pragma once

#include <cstddef>
#include <cstdint>

#include "dbmo/core/status.h"

namespace dbmo {

enum class ConfigType : uint8_t { Bool, Int, String };

struct ConfigSpec {
  const char *key;  // "modulo.nome": letras minúsculas, dígitos, '_' e '.'
  ConfigType type;
  int32_t defaultInt;  // Bool usa 0 ou 1
  int32_t minInt;
  int32_t maxInt;
  const char *defaultString;

  static constexpr ConfigSpec boolean(const char *key, bool defaultValue) {
    return {key, ConfigType::Bool, defaultValue ? 1 : 0, 0, 1, nullptr};
  }
  static constexpr ConfigSpec integer(const char *key, int32_t defaultValue, int32_t minValue,
                                      int32_t maxValue) {
    return {key, ConfigType::Int, defaultValue, minValue, maxValue, nullptr};
  }
  static constexpr ConfigSpec string(const char *key, const char *defaultValue) {
    return {key, ConfigType::String, 0, 0, 0, defaultValue};
  }
};

struct ConfigLoadReport {
  uint16_t applied = 0;
  uint16_t unknownKeys = 0;     // linhas de chaves que este firmware não declara
  uint16_t invalidValues = 0;   // valor fora da faixa ou de tipo errado: fica o padrão
  uint16_t malformedLines = 0;  // linha sem '='
};

class IConfigBackend {
 public:
  virtual ~IConfigBackend() = default;
  // Lê o texto salvo. NotFound quando ainda não existe configuração salva.
  virtual Status load(char *buffer, size_t capacity, size_t *length) = 0;
  virtual Status save(const char *text, size_t length) = 0;
};

class ConfigStore {
 public:
  static constexpr size_t kMaxEntries = 48;
  static constexpr size_t kMaxKeyLength = 31;
  static constexpr size_t kMaxStringLength = 31;
  // Tamanho máximo do texto salvo. load() e save() usam um único buffer estático deste
  // tamanho (não ficam na pilha), então só podem ser chamados da tarefa principal.
  static constexpr size_t kSerializedMax = 2048;

  using ChangeObserver = void (*)(const char *key, void *context);

  // Declara uma chave com seu padrão. AlreadyInitialized se a chave já existe.
  Status define(const ConfigSpec &spec);
  bool has(const char *key) const;

  Result<int32_t> getInt(const char *key) const;
  Result<bool> getBool(const char *key) const;
  Status getString(const char *key, char *out, size_t capacity) const;

  // OutOfRange fora da faixa declarada; InvalidArgument se o tipo não confere.
  Status setInt(const char *key, int32_t value);
  Status setBool(const char *key, bool value);
  Status setString(const char *key, const char *value);
  // Converte o texto conforme o tipo da chave (console e arquivo).
  Status setFromText(const char *key, const char *value);

  void resetToDefaults();

  // Verdadeiro quando há alteração ainda não salva.
  bool dirty() const { return dirty_; }
  void clearDirty() { dirty_ = false; }

  size_t size() const { return count_; }
  const char *keyAt(size_t index) const { return entries_[index].spec.key; }

  // Chamado a cada alteração efetiva de valor.
  void setChangeObserver(ChangeObserver observer, void *context);

  // Escreve todas as chaves em texto. Devolve o tamanho; NoSpace se não coube.
  Result<size_t> serialize(char *out, size_t capacity) const;
  // Aplica o texto. As linhas válidas são sempre aplicadas; devolve Corrupt se alguma
  // linha era malformada ou tinha valor inválido. Chaves desconhecidas são ignoradas.
  Status deserialize(const char *text, size_t length, ConfigLoadReport *report = nullptr);

  Status load(IConfigBackend &backend, ConfigLoadReport *report = nullptr);
  Status save(IConfigBackend &backend);

 private:
  struct Entry {
    ConfigSpec spec;
    int32_t intValue;
    char stringValue[kMaxStringLength + 1];
  };

  Entry *findEntry(const char *key);
  const Entry *findEntry(const char *key) const;
  void changed(const Entry &entry);

  Entry entries_[kMaxEntries] = {};
  size_t count_ = 0;
  bool dirty_ = false;
  ChangeObserver observer_ = nullptr;
  void *observerContext_ = nullptr;
};

}  // namespace dbmo
