// Operações de arquivo inteiro sobre IFileSystem.
#pragma once

#include <cstddef>

#include "dbmo/core/status.h"
#include "dbmo/storage/filesystem.h"

namespace dbmo::storage {

// Lê o arquivo inteiro. NoSpace se ele é maior que o buffer; NotFound se não existe.
Result<size_t> readFile(IFileSystem &fs, const char *path, void *buffer, size_t capacity);

// Como readFile(), mas se o arquivo não existe tenta <path>.bak (o que sobra quando a
// energia cai no meio de writeFileAtomic()).
Result<size_t> readFileOrBackup(IFileSystem &fs, const char *path, void *buffer, size_t capacity);

// Grava de forma que uma queda de energia nunca deixe o arquivo pela metade:
// escreve em <path>.tmp, sincroniza e só então troca pelo definitivo. A versão anterior vai
// para <path>.bak durante a troca e só é apagada depois de a nova estar no lugar (com
// keepBackup ela fica).
Status writeFileAtomic(IFileSystem &fs, const char *path, const void *data, size_t n,
                       bool keepBackup);

// Cria as pastas do d-BMO que ainda não existem.
Status ensureLayout(IFileSystem &fs);

}  // namespace dbmo::storage
