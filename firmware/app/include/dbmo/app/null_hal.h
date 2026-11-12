// Implementações "ausentes" dos contratos de hardware. A aplicação usa uma delas quando
// um periférico não foi ligado: o subsistema correspondente recebe HardwareAbsent na
// inicialização e o sistema segue sem ele, sem nenhum caso especial no código.
#pragma once

#include "dbmo/core/hal/i2c.h"
#include "dbmo/core/hal/spi.h"
#include "dbmo/storage/filesystem.h"

namespace dbmo::app {

class NullI2cBus final : public II2cBus {
 public:
  Status write(uint8_t, const uint8_t *, size_t) override { return Status::HardwareAbsent; }
  Status read(uint8_t, uint8_t *, size_t) override { return Status::HardwareAbsent; }
  Status writeRead(uint8_t, const uint8_t *, size_t, uint8_t *, size_t) override {
    return Status::HardwareAbsent;
  }
  Status probe(uint8_t) override { return Status::HardwareAbsent; }
};

class NullSpiDevice final : public ISpiDevice {
 public:
  Status select() override { return Status::HardwareAbsent; }
  Status transfer(const uint8_t *, uint8_t *, size_t) override { return Status::HardwareAbsent; }
  void deselect() override {}
};

class NullFileSystem final : public storage::IFileSystem {
 public:
  Status mount() override { return Status::HardwareAbsent; }
  void unmount() override {}
  bool mounted() const override { return false; }
  Result<storage::FileHandle> open(const char *, storage::OpenMode) override {
    return Status::HardwareAbsent;
  }
  Result<size_t> read(storage::FileHandle, void *, size_t) override {
    return Status::HardwareAbsent;
  }
  Result<size_t> write(storage::FileHandle, const void *, size_t) override {
    return Status::HardwareAbsent;
  }
  Status sync(storage::FileHandle) override { return Status::HardwareAbsent; }
  Status close(storage::FileHandle) override { return Status::HardwareAbsent; }
  bool exists(const char *) override { return false; }
  Result<uint32_t> fileSize(const char *) override { return Status::HardwareAbsent; }
  Status remove(const char *) override { return Status::HardwareAbsent; }
  Status rename(const char *, const char *) override { return Status::HardwareAbsent; }
  Status makeDir(const char *) override { return Status::HardwareAbsent; }
  Status list(const char *, storage::DirVisitor, void *) override { return Status::HardwareAbsent; }
};

}  // namespace dbmo::app
