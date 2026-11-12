// Contrato de sistema de arquivos do cartão microSD. O porte para o ESP32-S3 implementa
// sobre a biblioteca SD; os testes usam um sistema de arquivos em memória.
// Sem alocação dinâmica: arquivos abertos são identificados por um número.
#pragma once

#include <cstddef>
#include <cstdint>

#include "dbmo/core/status.h"

namespace dbmo::storage {

enum class OpenMode : uint8_t {
  Read,    // NotFound se não existe
  Write,   // cria ou zera
  Append,  // cria se não existe; escreve no fim
};

using FileHandle = int8_t;
inline constexpr FileHandle kInvalidFile = -1;

struct DirEntry {
  const char *name;  // só o nome, sem a pasta; válido durante a chamada
  bool isDirectory;
  uint32_t size;
};
// Devolver false interrompe a listagem.
using DirVisitor = bool (*)(const DirEntry &entry, void *context);

class IFileSystem {
 public:
  virtual ~IFileSystem() = default;

  // HardwareAbsent quando não há cartão (ou ele não respondeu).
  virtual Status mount() = 0;
  virtual void unmount() = 0;
  virtual bool mounted() const = 0;

  virtual Result<FileHandle> open(const char *path, OpenMode mode) = 0;
  // Devolve quantos bytes foram lidos; 0 no fim do arquivo.
  virtual Result<size_t> read(FileHandle file, void *buffer, size_t n) = 0;
  // Devolve quantos bytes foram escritos; menos que n significa cartão cheio.
  virtual Result<size_t> write(FileHandle file, const void *data, size_t n) = 0;
  virtual Status sync(FileHandle file) = 0;
  virtual Status close(FileHandle file) = 0;

  virtual bool exists(const char *path) = 0;
  virtual Result<uint32_t> fileSize(const char *path) = 0;
  virtual Status remove(const char *path) = 0;
  // Falha se o destino já existe (comportamento do FAT): apagar o destino antes.
  virtual Status rename(const char *from, const char *to) = 0;
  virtual Status makeDir(const char *path) = 0;
  virtual Status list(const char *path, DirVisitor visitor, void *context) = 0;
};

}  // namespace dbmo::storage
