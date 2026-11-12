#include "dbmo/core/system.h"

#include <cstring>

#include "dbmo/core/log.h"

namespace dbmo {

namespace {
constexpr char kTag[] = "system";
}

const System::Machine::Transition System::kTransitions[] = {
    {SystemState::Off, Trigger::Boot, SystemState::Booting},
    {SystemState::Booting, Trigger::BootOk, SystemState::Running},
    {SystemState::Booting, Trigger::BootDegraded, SystemState::Degraded},
    {SystemState::Booting, Trigger::BootFailed, SystemState::Fault},
    {SystemState::Running, Trigger::SubsystemLost, SystemState::Degraded},
    {SystemState::Degraded, Trigger::AllHealthy, SystemState::Running},
    {SystemState::Running, Trigger::EnterLowPower, SystemState::LowPower},
    {SystemState::Degraded, Trigger::EnterLowPower, SystemState::LowPower},
    {SystemState::LowPower, Trigger::ExitLowPower, SystemState::Running},
    {SystemState::Running, Trigger::FatalError, SystemState::Fault},
    {SystemState::Degraded, Trigger::FatalError, SystemState::Fault},
    {SystemState::LowPower, Trigger::FatalError, SystemState::Fault},
    {SystemState::Running, Trigger::Shutdown, SystemState::ShuttingDown},
    {SystemState::Degraded, Trigger::Shutdown, SystemState::ShuttingDown},
    {SystemState::LowPower, Trigger::Shutdown, SystemState::ShuttingDown},
    {SystemState::Fault, Trigger::Shutdown, SystemState::ShuttingDown},
    {SystemState::ShuttingDown, Trigger::ShutdownDone, SystemState::Off},
};

System::System(IClock &clock, EventBus &bus)
    : clock_(clock), bus_(bus), machine_(kTransitions, SystemState::Off) {
  machine_.setObserver(&System::onTransition, this);
}

void System::onTransition(SystemState from, SystemState to, void *context) {
  auto *self = static_cast<System *>(context);
  DBMO_LOGI(kTag, "estado %s -> %s", toString(from), toString(to));
  (void)self->bus_.publish(system_event::kStateChanged, static_cast<uint32_t>(from),
                           static_cast<uint32_t>(to));
}

void System::fire(Trigger trigger) {
  if (!machine_.fire(trigger))
    DBMO_LOGD(kTag, "transicao %u ignorada em %s", static_cast<unsigned>(trigger),
              toString(state()));
}

bool System::inOperation(SubsystemState state) {
  return state == SubsystemState::Ready || state == SubsystemState::Degraded;
}

bool System::allReady() const {
  for (size_t i = 0; i < count_; ++i)
    if (entries_[i].state != SubsystemState::Ready) return false;
  return true;
}

int System::indexOf(const ISubsystem &subsystem) const {
  for (size_t i = 0; i < count_; ++i)
    if (entries_[i].subsystem == &subsystem) return static_cast<int>(i);
  return -1;
}

const SubsystemInfo *System::find(const char *name) const {
  if (name == nullptr) return nullptr;
  for (size_t i = 0; i < count_; ++i)
    if (strcmp(entries_[i].subsystem->name(), name) == 0) return &entries_[i];
  return nullptr;
}

const SubsystemInfo *System::find(const ISubsystem &subsystem) const {
  const int index = indexOf(subsystem);
  return index < 0 ? nullptr : &entries_[index];
}

Status System::add(ISubsystem &subsystem, SubsystemOptions options) {
  if (state() != SystemState::Off) return Status::NotPermitted;
  if (subsystem.name() == nullptr) return Status::InvalidArgument;
  if (indexOf(subsystem) >= 0) return Status::AlreadyInitialized;
  // A dependência precisa estar registrada antes: garante a ordem de inicialização.
  if (options.dependsOn != nullptr && indexOf(*options.dependsOn) < 0)
    return Status::InvalidArgument;
  if (count_ == kMaxSubsystems) return Status::NoSpace;

  entries_[count_] = SubsystemInfo{};
  entries_[count_].subsystem = &subsystem;
  entries_[count_].options = options;
  ++count_;
  return Status::Ok;
}

void System::setSubsystemState(size_t index, SubsystemState state) {
  SubsystemInfo &entry = entries_[index];
  if (entry.state == state) return;
  entry.state = state;
  DBMO_LOGI(kTag, "%s: %s (%s)", entry.subsystem->name(), toString(state),
            toString(entry.lastStatus));
  (void)bus_.publish(system_event::kSubsystemChanged, static_cast<uint32_t>(index),
                     static_cast<uint32_t>(state));
}

void System::recordFault(size_t index, Status status) {
  faults_.pushOverwrite(FaultRecord{clock_.nowMs(), static_cast<uint8_t>(index), status});
  DBMO_LOGE(kTag, "falha em %s: %s", entries_[index].subsystem->name(), toString(status));
  (void)bus_.publish(system_event::kFault, static_cast<uint32_t>(index),
                     static_cast<uint32_t>(status));
}

Status System::initEntry(size_t index) {
  SubsystemInfo &entry = entries_[index];

  if (entry.options.dependsOn != nullptr) {
    const SubsystemInfo *dependency = find(*entry.options.dependsOn);
    if (dependency == nullptr || !inOperation(dependency->state)) {
      entry.lastStatus = Status::NotInitialized;
      const bool dependencyAbsent =
          dependency != nullptr && dependency->state == SubsystemState::Absent;
      DBMO_LOGW(kTag, "%s nao iniciado: depende de %s", entry.subsystem->name(),
                entry.options.dependsOn->name());
      setSubsystemState(index, dependencyAbsent ? SubsystemState::Absent : SubsystemState::Failed);
      return Status::NotInitialized;
    }
  }

  if (entry.initAttempts < UINT8_MAX) ++entry.initAttempts;
  const Status status = entry.subsystem->init();
  entry.lastStatus = status;
  if (isOk(status)) {
    setSubsystemState(index, SubsystemState::Ready);
  } else if (status == Status::HardwareAbsent) {
    DBMO_LOGW(kTag, "%s: hardware ausente", entry.subsystem->name());
    setSubsystemState(index, SubsystemState::Absent);
  } else {
    recordFault(index, status);
    setSubsystemState(index, SubsystemState::Failed);
  }
  return status;
}

Status System::boot() {
  if (state() != SystemState::Off) return Status::AlreadyInitialized;
  fire(Trigger::Boot);

  for (size_t i = 0; i < count_; ++i) {
    entries_[i].state = SubsystemState::Uninitialized;
    entries_[i].lastStatus = Status::NotInitialized;
    entries_[i].initAttempts = 0;
  }

  for (size_t i = 0; i < count_; ++i) {
    const Status status = initEntry(i);
    if (entries_[i].options.required && entries_[i].state != SubsystemState::Ready) {
      DBMO_LOGE(kTag, "subsistema obrigatorio %s nao iniciou", entries_[i].subsystem->name());
      fire(Trigger::BootFailed);
      return isOk(status) ? Status::InternalError : status;
    }
  }

  fire(allReady() ? Trigger::BootOk : Trigger::BootDegraded);
  return Status::Ok;
}

void System::tick() {
  const SystemState current = state();
  if (current == SystemState::Off || current == SystemState::Booting ||
      current == SystemState::ShuttingDown)
    return;

  bus_.dispatch();
  const uint32_t now = clock_.nowMs();
  for (size_t i = 0; i < count_; ++i)
    if (inOperation(entries_[i].state)) entries_[i].subsystem->tick(now);
}

Status System::shutdown() {
  if (!machine_.canFire(Trigger::Shutdown)) return Status::NotInitialized;
  fire(Trigger::Shutdown);

  Status first = Status::Ok;
  for (size_t i = count_; i-- > 0;) {
    if (!inOperation(entries_[i].state)) continue;
    const Status status = entries_[i].subsystem->shutdown();
    entries_[i].lastStatus = status;
    if (!isOk(status)) {
      recordFault(i, status);
      if (isOk(first)) first = status;
    }
    setSubsystemState(i, SubsystemState::Stopped);
  }

  fire(Trigger::ShutdownDone);
  return first;
}

void System::updateHealth() {
  const SystemState current = state();
  if (current == SystemState::Running && !allReady()) fire(Trigger::SubsystemLost);
  if (current == SystemState::Degraded && allReady()) fire(Trigger::AllHealthy);
}

Status System::retry(const ISubsystem &subsystem) {
  const int index = indexOf(subsystem);
  if (index < 0) return Status::NotFound;
  if (state() != SystemState::Running && state() != SystemState::Degraded)
    return Status::NotPermitted;
  if (inOperation(entries_[index].state)) return Status::AlreadyInitialized;

  const Status status = initEntry(static_cast<size_t>(index));
  updateHealth();
  return status;
}

void System::reportFault(const ISubsystem &subsystem, Status status, SubsystemState newState) {
  const int index = indexOf(subsystem);
  if (index < 0) return;
  if (newState != SubsystemState::Degraded && newState != SubsystemState::Absent)
    newState = SubsystemState::Failed;

  SubsystemInfo &entry = entries_[index];
  entry.lastStatus = status;
  recordFault(static_cast<size_t>(index), status);
  setSubsystemState(static_cast<size_t>(index), newState);

  if (entry.options.required && !inOperation(newState)) {
    fire(Trigger::FatalError);
    return;
  }
  updateHealth();
}

void System::reportRecovered(const ISubsystem &subsystem) {
  const int index = indexOf(subsystem);
  if (index < 0 || entries_[index].state != SubsystemState::Degraded) return;
  entries_[index].lastStatus = Status::Ok;
  setSubsystemState(static_cast<size_t>(index), SubsystemState::Ready);
  updateHealth();
}

Status System::enterLowPower() {
  if (!machine_.canFire(Trigger::EnterLowPower)) return Status::NotPermitted;
  fire(Trigger::EnterLowPower);
  return Status::Ok;
}

Status System::exitLowPower() {
  if (!machine_.canFire(Trigger::ExitLowPower)) return Status::NotPermitted;
  fire(Trigger::ExitLowPower);
  updateHealth();
  return Status::Ok;
}

}  // namespace dbmo
