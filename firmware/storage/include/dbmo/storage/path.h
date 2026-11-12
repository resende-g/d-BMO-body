// Validação de caminhos. Todo caminho que chega de fora (nome de arquivo de código IR,
// identificador de cartucho) passa por aqui antes de tocar o cartão.
#pragma once

#include <cstddef>
#include <cstdint>

#include "dbmo/core/status.h"

namespace dbmo::storage {

inline constexpr size_t kMaxPath = 96;       // com o terminador
inline constexpr size_t kMaxComponent = 40;  // sem o terminador

// Estrutura de pastas do d-BMO no cartão.
inline constexpr char kRootDir[] = "/dbmo";
inline constexpr char kConfigDir[] = "/dbmo/config";
inline constexpr char kLogDir[] = "/dbmo/logs";
inline constexpr char kCartridgeDir[] = "/dbmo/cartridges";
inline constexpr char kIrDir[] = "/dbmo/ir";
inline constexpr char kConfigFile[] = "/dbmo/config/config.txt";
inline constexpr char kLogFile[] = "/dbmo/logs/log.txt";
inline constexpr char kLogFileOld[] = "/dbmo/logs/log.1.txt";

// Um nome de arquivo ou pasta: 1 a kMaxComponent caracteres de [A-Za-z0-9._-], e que
// não seja "." nem "..".
bool isValidName(const char *name);

// Caminho absoluto: começa com '/', componentes válidos separados por uma única '/',
// sem '/' no fim (a raiz "/" é a exceção) e que caiba em kMaxPath.
bool isValidPath(const char *path);

// directory + "/" + name, validando o resultado. InvalidArgument ou NoSpace.
Status joinPath(char *out, size_t capacity, const char *directory, const char *name);

// Pasta de dados de um cartucho: /dbmo/cartridges/<vvvv>-<pppp> (hexadecimal).
Status cartridgeDataDir(uint16_t vendorId, uint16_t productId, char *out, size_t capacity);

}  // namespace dbmo::storage
