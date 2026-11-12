// Sistema de arquivos em memória para os testes, com o comportamento que o firmware
// precisa enfrentar num cartão de verdade: pasta-mãe obrigatória, rename que falha se o
// destino existe, cartão cheio (escrita parcial), cartão retirado e falhas injetadas.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "dbmo/storage/filesystem.h"
#include "dbmo/storage/path.h"

namespace dbmo::sim {

class MemoryFileSystem final : public storage::IFileSystem {
 public:
  static constexpr size_t kMaxNodes = 24;
  static constexpr size_t kMaxFileSize = 8192;
  static constexpr size_t kMaxOpen = 4;

  Status mount() override {
    ++mountCount;
    if (!cardPresent) return Status::HardwareAbsent;
    mounted_ = true;
    return Status::Ok;
  }
  void unmount() override {
    mounted_ = false;
    for (auto &open : open_) open.used = false;
  }
  bool mounted() const override { return mounted_; }

  Result<storage::FileHandle> open(const char *path, storage::OpenMode mode) override {
    if (!usable()) return Status::IoError;
    if (takeFault(failOpen)) return Status::IoError;
    if (!storage::isValidPath(path)) return Status::InvalidArgument;
    Node *node = find(path);
    if (node != nullptr && node->directory) return Status::InvalidArgument;
    if (node == nullptr) {
      if (mode == storage::OpenMode::Read) return Status::NotFound;
      if (!parentExists(path)) return Status::NotFound;
      node = allocate(path, false);
      if (node == nullptr) return Status::NoSpace;
    } else if (mode == storage::OpenMode::Write) {
      node->size = 0;
    }
    for (size_t i = 0; i < kMaxOpen; ++i) {
      if (open_[i].used) continue;
      open_[i] = Open{true, node, mode, mode == storage::OpenMode::Append ? node->size : 0};
      return static_cast<storage::FileHandle>(i);
    }
    return Status::Busy;
  }

  Result<size_t> read(storage::FileHandle file, void *buffer, size_t n) override {
    Open *open = handle(file);
    if (open == nullptr) return Status::InvalidArgument;
    if (!usable() || takeFault(failRead)) return Status::IoError;
    const size_t available = open->node->size - open->position;
    const size_t count = n < available ? n : available;
    memcpy(buffer, open->node->data + open->position, count);
    open->position += count;
    return count;
  }

  Result<size_t> write(storage::FileHandle file, const void *data, size_t n) override {
    Open *open = handle(file);
    if (open == nullptr || open->mode == storage::OpenMode::Read) return Status::InvalidArgument;
    if (!usable() || takeFault(failWrite)) return Status::IoError;
    ++writeCount;
    size_t count = n;
    if (open->position + count > kMaxFileSize) count = kMaxFileSize - open->position;
    const size_t used = usedBytes();
    const size_t freeBytes = used < capacityBytes ? capacityBytes - used : 0;
    const size_t growth =
        open->position + count > open->node->size ? open->position + count - open->node->size : 0;
    if (growth > freeBytes) count -= growth - freeBytes;  // cartão cheio: escrita parcial
    memcpy(open->node->data + open->position, data, count);
    open->position += count;
    if (open->position > open->node->size) open->node->size = open->position;
    return count;
  }

  Status sync(storage::FileHandle file) override {
    if (handle(file) == nullptr) return Status::InvalidArgument;
    if (!usable() || takeFault(failSync)) return Status::IoError;
    ++syncCount;
    return Status::Ok;
  }

  Status close(storage::FileHandle file) override {
    Open *open = handle(file);
    if (open == nullptr) return Status::InvalidArgument;
    open->used = false;
    return Status::Ok;
  }

  bool exists(const char *path) override { return usable() && find(path) != nullptr; }

  Result<uint32_t> fileSize(const char *path) override {
    if (!usable()) return Status::IoError;
    const Node *node = find(path);
    if (node == nullptr || node->directory) return Status::NotFound;
    return static_cast<uint32_t>(node->size);
  }

  Status remove(const char *path) override {
    if (!usable() || takeFault(failRemove)) return Status::IoError;
    Node *node = find(path);
    if (node == nullptr) return Status::NotFound;
    node->used = false;
    return Status::Ok;
  }

  Status rename(const char *from, const char *to) override {
    if (!usable()) return Status::IoError;
    ++renameCount;
    if (renameFaultDelay > 0) {
      --renameFaultDelay;
    } else if (takeFault(failRename)) {
      return Status::IoError;
    }
    Node *node = find(from);
    if (node == nullptr) return Status::NotFound;
    if (!storage::isValidPath(to) || find(to) != nullptr || !parentExists(to))
      return Status::IoError;
    snprintf(node->path, sizeof(node->path), "%s", to);
    return Status::Ok;
  }

  Status makeDir(const char *path) override {
    if (!usable() || takeFault(failMakeDir)) return Status::IoError;
    if (!storage::isValidPath(path) || find(path) != nullptr || !parentExists(path))
      return Status::IoError;
    return allocate(path, true) != nullptr ? Status::Ok : Status::NoSpace;
  }

  Status list(const char *path, storage::DirVisitor visitor, void *context) override {
    if (!usable()) return Status::IoError;
    const bool root = strcmp(path, "/") == 0;
    if (!root) {
      const Node *dir = find(path);
      if (dir == nullptr || !dir->directory) return Status::NotFound;
    }
    const size_t prefix = root ? 0 : strlen(path);
    for (const Node &node : nodes_) {
      if (!node.used || strncmp(node.path, path, prefix) != 0 || node.path[prefix] != '/') continue;
      const char *name = node.path + prefix + 1;
      if (*name == '\0' || strchr(name, '/') != nullptr) continue;
      const storage::DirEntry entry{name, node.directory, static_cast<uint32_t>(node.size)};
      if (!visitor(entry, context)) break;
    }
    return Status::Ok;
  }

  // ---- Controle do teste ----

  // Conteúdo de um arquivo como texto (vazio se não existe).
  const char *text(const char *path) {
    static char copy[kMaxFileSize + 1];
    const Node *node = find(path);
    const size_t size = node != nullptr ? node->size : 0;
    if (node != nullptr) memcpy(copy, node->data, size);
    copy[size] = '\0';
    return copy;
  }
  // Cria (ou substitui) um arquivo diretamente, criando as pastas-mãe.
  void put(const char *path, const char *content) {
    char partial[storage::kMaxPath];
    for (size_t i = 1; path[i] != '\0'; ++i) {
      if (path[i] != '/') continue;
      memcpy(partial, path, i);
      partial[i] = '\0';
      if (find(partial) == nullptr) (void)allocate(partial, true);
    }
    Node *node = find(path);
    if (node == nullptr) node = allocate(path, false);
    node->size = strlen(content);
    memcpy(node->data, content, node->size);
  }
  // Simula o cartão sendo puxado: o que estava gravado continua nele.
  void ejectCard() {
    cardPresent = false;
    for (auto &open : open_) open.used = false;
  }
  void insertCard() { cardPresent = true; }
  size_t usedBytes() const {
    size_t total = 0;
    for (const Node &node : nodes_)
      if (node.used) total += node.size;
    return total;
  }

  bool cardPresent = true;
  size_t capacityBytes = kMaxNodes * kMaxFileSize;
  // Quantas das próximas chamadas de cada tipo falham com IoError.
  unsigned failOpen = 0;
  unsigned failRead = 0;
  unsigned failWrite = 0;
  unsigned failSync = 0;
  unsigned failRemove = 0;
  unsigned failRename = 0;
  // Quantos rename() ainda funcionam antes de failRename começar a valer.
  unsigned renameFaultDelay = 0;
  unsigned failMakeDir = 0;
  unsigned mountCount = 0;
  unsigned writeCount = 0;
  unsigned syncCount = 0;
  unsigned renameCount = 0;

 private:
  struct Node {
    bool used = false;
    bool directory = false;
    char path[storage::kMaxPath] = {};
    size_t size = 0;
    uint8_t data[kMaxFileSize] = {};
  };
  struct Open {
    bool used = false;
    Node *node = nullptr;
    storage::OpenMode mode = storage::OpenMode::Read;
    size_t position = 0;
  };

  bool usable() const { return mounted_ && cardPresent; }
  static bool takeFault(unsigned &counter) {
    if (counter == 0) return false;
    --counter;
    return true;
  }
  Node *find(const char *path) {
    for (Node &node : nodes_)
      if (node.used && strcmp(node.path, path) == 0) return &node;
    return nullptr;
  }
  bool parentExists(const char *path) {
    const char *slash = strrchr(path, '/');
    if (slash == nullptr) return false;
    if (slash == path) return true;  // direto na raiz
    char parent[storage::kMaxPath];
    const size_t length = static_cast<size_t>(slash - path);
    memcpy(parent, path, length);
    parent[length] = '\0';
    const Node *node = find(parent);
    return node != nullptr && node->directory;
  }
  Node *allocate(const char *path, bool directory) {
    for (Node &node : nodes_) {
      if (node.used) continue;
      node.used = true;
      node.directory = directory;
      node.size = 0;
      snprintf(node.path, sizeof(node.path), "%s", path);
      return &node;
    }
    return nullptr;
  }
  Open *handle(storage::FileHandle file) {
    if (file < 0 || static_cast<size_t>(file) >= kMaxOpen || !open_[file].used) return nullptr;
    return &open_[file];
  }

  bool mounted_ = false;
  Node nodes_[kMaxNodes];
  Open open_[kMaxOpen];
};

}  // namespace dbmo::sim
