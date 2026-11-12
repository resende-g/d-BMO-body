// Biblioteca de códigos: formato de texto de um código e a guarda por nome.
// O módulo não conhece o cartão: grava e lê por IIrCodeStorage, que a aplicação
// implementa sobre o armazenamento (um arquivo por código em /dbmo/ir).
#pragma once

#include <cstddef>

#include "dbmo/core/status.h"
#include "dbmo/infrared/ir_types.h"

namespace dbmo::infrared {

inline constexpr size_t kMaxCodeNameLength = 24;
// Maior texto de um código: "raw:" + portadora + 128 durações.
inline constexpr size_t kMaxCodeText = 800;

// Formatos (uma linha):
//   nec:<endereço hex>:<comando hex>
//   necx:<endereço estendido de 16 bits, hex>:<comando hex>
//   sony:<bits>:<endereço hex>:<comando hex>
//   raw:<portadora Hz>:<d0>,<d1>,...   (microssegundos, começando por uma marca)
Result<size_t> formatCode(const StoredCode &code, char *out, size_t capacity);
// Corrupt se o texto não é um código válido.
Status parseCode(const char *text, size_t length, StoredCode *out);

// Nome de código: 1 a 24 caracteres de [a-z0-9_-].
bool isValidCodeName(const char *name);

class IIrCodeStorage {
 public:
  virtual ~IIrCodeStorage() = default;
  virtual Status save(const char *name, const char *text, size_t length) = 0;
  // NotFound se o código não existe.
  virtual Result<size_t> load(const char *name, char *buffer, size_t capacity) = 0;
  virtual Status remove(const char *name) = 0;
};

class IrLibrary {
 public:
  explicit IrLibrary(IIrCodeStorage &storage) : storage_(storage) {}

  Status save(const char *name, const StoredCode &code);
  Status load(const char *name, StoredCode *out);
  Status remove(const char *name);

 private:
  IIrCodeStorage &storage_;
};

}  // namespace dbmo::infrared
