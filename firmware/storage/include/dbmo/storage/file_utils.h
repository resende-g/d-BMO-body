// Operações de arquivo inteiro sobre IFileSystem.
#pragma once

#include <cstddef>

#include "dbmo/core/status.h"
#include "dbmo/storage/filesystem.h"

namespace dbmo::storage {

// Lê o arquivo inteiro. NoSpace se ele é maior que o buffer; NotFound se não existe.
Result<size_t> readFile(IFileSystem &fs, const char *path, void *buffer, size_t capacity);

// Grava de forma que uma queda de energia nunca deixe o arquivo pela metade:
// escreve em <path>.tmp, sincroniza e só então troca pelo definitivo. Com keepBackup, a
// versão anterior fica em <path>.bak.
Status writeFileAtomic(IFileSystem &fs, const char *path, const void *data, size_t n,
                       bool keepBackup);

// Cria as pastas do d-BMO que ainda não existem.
Status ensureLayout(IFileSystem &fs);

}  // namespace dbmo::storage
