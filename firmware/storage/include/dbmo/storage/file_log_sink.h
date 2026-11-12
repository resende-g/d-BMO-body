// Destino de log em arquivo. write() só copia a linha para a memória; quem grava no
// cartão é flush(), chamado pelo StorageSubsystem fora do caminho de quem registrou.
// O log fica no cartão do próprio aparelho e não é enviado a lugar nenhum.
#pragma once

#include <cstddef>
#include <cstdint>

#include "dbmo/core/log.h"
#include "dbmo/storage/filesystem.h"

namespace dbmo::storage {

class FileLogSink final : public ILogSink {
 public:
  static constexpr size_t kBufferSize = 2048;

  explicit FileLogSink(IFileSystem &fs) : fs_(fs) {}

  // Tamanho em que o arquivo é rotacionado (log.txt vira log.1.txt). O cartão guarda,
  // no máximo, cerca de duas vezes este valor.
  void setMaxBytes(uint32_t maxBytes) { maxBytes_ = maxBytes; }
  void setMinLevel(LogLevel level) { minLevel_ = level; }
  void setEnabled(bool enabled);

  // Só da tarefa principal (não chamar de interrupção).
  void write(const LogRecord &record) override;

  bool pending() const { return used_ > 0; }
  // Grava o que está na memória. Em caso de erro as linhas continuam guardadas para a
  // próxima tentativa (até a memória encher).
  Status flush();

  // Linhas descartadas porque a memória encheu antes de um flush() bem-sucedido.
  uint32_t droppedLines() const { return dropped_; }

 private:
  Status rotateIfNeeded();

  IFileSystem &fs_;
  char buffer_[kBufferSize] = {};
  size_t used_ = 0;
  uint32_t maxBytes_ = 256 * 1024;
  LogLevel minLevel_ = LogLevel::Info;
  bool enabled_ = true;
  uint32_t dropped_ = 0;
  uint32_t reportedDropped_ = 0;
};

}  // namespace dbmo::storage
