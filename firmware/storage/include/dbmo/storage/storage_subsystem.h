// Subsistema de armazenamento: monta o cartão, garante a estrutura de pastas, grava o log
// em segundo plano e acompanha a saúde do cartão. O aparelho funciona sem cartão: a
// configuração usa os valores padrão e o log fica só na memória.
#pragma once

#include <cstdint>

#include "dbmo/core/config_store.h"
#include "dbmo/core/event_bus.h"
#include "dbmo/core/subsystem.h"
#include "dbmo/storage/file_config_backend.h"
#include "dbmo/storage/file_log_sink.h"
#include "dbmo/storage/filesystem.h"

namespace dbmo::storage {

namespace storage_event {
// O cartão ficou disponível (no boot ou depois de reinserido).
inline constexpr EventId kAvailable = makeEventId(EventDomain::Storage, 1);
// O cartão deixou de responder. a = Status da falha.
inline constexpr EventId kUnavailable = makeEventId(EventDomain::Storage, 2);
}  // namespace storage_event

namespace config_key {
inline constexpr char kLogEnabled[] = "storage.log_enabled";
inline constexpr char kLogMaxKb[] = "storage.log_max_kb";
}  // namespace config_key

class StorageSubsystem final : public ISubsystem {
 public:
  static constexpr uint32_t kFlushPeriodMs = 2000;
  static constexpr uint32_t kHealthPeriodMs = 5000;
  static constexpr uint32_t kRemountPeriodMs = 5000;
  // Um cartão que monta mas não grava é tentado cada vez mais devagar, até este limite.
  static constexpr uint32_t kMaxRemountPeriodMs = 5 * 60 * 1000;
  // Falhas seguidas de gravação até o cartão ser dado como indisponível.
  static constexpr uint8_t kFaultThreshold = 3;

  StorageSubsystem(IFileSystem &fs, EventBus &bus, ConfigStore &config);

  static Status registerConfig(ConfigStore &config);

  const char *name() const override { return "storage"; }
  // HardwareAbsent sem cartão: o sistema segue e pode tentar de novo depois.
  Status init() override;
  void tick(uint32_t nowMs) override;
  // Grava o que falta do log e desmonta o cartão.
  Status shutdown() override;

  bool available() const { return available_; }
  IFileSystem &fileSystem() { return fs_; }
  FileConfigBackend &configBackend() { return configBackend_; }
  FileLogSink &logSink() { return logSink_; }

  void applyConfig();

 private:
  Status bringUp();
  void markUnavailable(Status reason, uint32_t nowMs);

  IFileSystem &fs_;
  EventBus &bus_;
  ConfigStore &config_;
  FileConfigBackend configBackend_;
  FileLogSink logSink_;
  bool initialized_ = false;
  bool available_ = false;
  uint8_t failures_ = 0;
  bool wroteSinceMount_ = false;
  uint32_t remountDelayMs_ = kRemountPeriodMs;
  bool timed_ = false;
  uint32_t lastFlushMs_ = 0;
  uint32_t lastHealthMs_ = 0;
  uint32_t lastRemountMs_ = 0;
};

}  // namespace dbmo::storage
