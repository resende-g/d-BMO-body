#include "dbmo/storage/storage_subsystem.h"

#include "dbmo/core/hal/clock.h"
#include "dbmo/core/log.h"
#include "dbmo/storage/file_utils.h"
#include "dbmo/storage/path.h"

namespace dbmo::storage {

namespace {
constexpr char kTag[] = "storage";
}

StorageSubsystem::StorageSubsystem(IFileSystem &fs, EventBus &bus, ConfigStore &config)
    : fs_(fs), bus_(bus), config_(config), configBackend_(fs), logSink_(fs) {}

Status StorageSubsystem::registerConfig(ConfigStore &config) {
  DBMO_TRY(config.define(ConfigSpec::boolean(config_key::kLogEnabled, true)));
  DBMO_TRY(config.define(ConfigSpec::integer(config_key::kLogMaxKb, 256, 16, 4096)));
  return Status::Ok;
}

void StorageSubsystem::applyConfig() {
  logSink_.setEnabled(config_.getBool(config_key::kLogEnabled).valueOr(true));
  logSink_.setMaxBytes(static_cast<uint32_t>(config_.getInt(config_key::kLogMaxKb).valueOr(256)) *
                       1024);
}

Status StorageSubsystem::bringUp() {
  DBMO_TRY(fs_.mount());
  const Status layout = ensureLayout(fs_);
  if (!isOk(layout)) {
    fs_.unmount();
    return layout;
  }
  available_ = true;
  failures_ = 0;
  wroteSinceMount_ = false;
  return Status::Ok;
}

Status StorageSubsystem::init() {
  applyConfig();
  available_ = false;
  timed_ = false;
  remountDelayMs_ = kRemountPeriodMs;
  initialized_ = true;
  const Status status = bringUp();
  if (!isOk(status)) {
    DBMO_LOGW(kTag, "cartao indisponivel: %s", toString(status));
    return status;
  }
  DBMO_LOGI(kTag, "cartao montado");
  (void)bus_.publish(storage_event::kAvailable);
  return Status::Ok;
}

void StorageSubsystem::markUnavailable(Status reason, uint32_t nowMs) {
  if (!available_) return;
  available_ = false;
  fs_.unmount();
  lastRemountMs_ = nowMs;
  // Montou e não conseguiu gravar nada: espera mais antes da próxima tentativa, para um
  // cartão defeituoso ou protegido contra escrita não gerar um evento a cada poucos
  // segundos.
  if (wroteSinceMount_) {
    remountDelayMs_ = kRemountPeriodMs;
  } else if (remountDelayMs_ < kMaxRemountPeriodMs) {
    remountDelayMs_ =
        remountDelayMs_ * 2 < kMaxRemountPeriodMs ? remountDelayMs_ * 2 : kMaxRemountPeriodMs;
  }
  DBMO_LOGW(kTag, "cartao parou de responder: %s", toString(reason));
  (void)bus_.publish(storage_event::kUnavailable, static_cast<uint32_t>(reason));
}

void StorageSubsystem::tick(uint32_t nowMs) {
  if (!initialized_) return;
  if (!timed_) {
    timed_ = true;
    lastFlushMs_ = lastHealthMs_ = lastRemountMs_ = nowMs;
  }

  if (!available_) {
    // Cartão retirado em uso: tenta montar de novo de tempos em tempos.
    if (elapsedMs(nowMs, lastRemountMs_) < remountDelayMs_) return;
    lastRemountMs_ = nowMs;
    if (isOk(bringUp())) {
      DBMO_LOGI(kTag, "cartao montado de novo");
      (void)bus_.publish(storage_event::kAvailable);
    }
    return;
  }

  if (elapsedMs(nowMs, lastFlushMs_) >= kFlushPeriodMs) {
    lastFlushMs_ = nowMs;
    if (logSink_.pending()) {
      const Status status = logSink_.flush();
      if (isOk(status)) {
        failures_ = 0;
        wroteSinceMount_ = true;
        remountDelayMs_ = kRemountPeriodMs;
      } else if (++failures_ >= kFaultThreshold) {
        markUnavailable(status, nowMs);
        return;
      }
    }
  }

  if (elapsedMs(nowMs, lastHealthMs_) >= kHealthPeriodMs) {
    lastHealthMs_ = nowMs;
    if (!fs_.exists(kRootDir)) {
      // Cartão retirado: não é defeito de gravação, então a espera não aumenta.
      wroteSinceMount_ = true;
      markUnavailable(Status::HardwareAbsent, nowMs);
    }
  }
}

Status StorageSubsystem::shutdown() {
  Status status = Status::Ok;
  if (available_) {
    status = logSink_.flush();
    fs_.unmount();
    available_ = false;
  }
  initialized_ = false;
  return status;
}

}  // namespace dbmo::storage
