// Persistência da configuração em um arquivo de texto no cartão, editável à mão.
#pragma once

#include <cstddef>

#include "dbmo/core/config_store.h"
#include "dbmo/storage/filesystem.h"
#include "dbmo/storage/path.h"

namespace dbmo::storage {

class FileConfigBackend final : public IConfigBackend {
 public:
  explicit FileConfigBackend(IFileSystem &fs, const char *path = kConfigFile)
      : fs_(fs), path_(path) {}

  // Lê o arquivo; se ele não existe, está vazio ou não pôde ser lido, tenta a cópia
  // anterior (.bak). NotFound quando nenhum dos dois existe.
  Status load(char *buffer, size_t capacity, size_t *length) override;
  // Gravação atômica, mantendo a versão anterior em .bak.
  Status save(const char *text, size_t length) override;

  // A última leitura veio da cópia de segurança?
  bool loadedFromBackup() const { return fromBackup_; }

 private:
  IFileSystem &fs_;
  const char *path_;
  bool fromBackup_ = false;
};

}  // namespace dbmo::storage
