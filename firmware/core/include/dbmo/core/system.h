// Núcleo do firmware: inicialização controlada dos subsistemas, estado global, laço
// principal e registro de falhas.
#pragma once

#include <cstddef>
#include <cstdint>

#include "dbmo/core/event_bus.h"
#include "dbmo/core/hal/clock.h"
#include "dbmo/core/ring_buffer.h"
#include "dbmo/core/state_machine.h"
#include "dbmo/core/status.h"
#include "dbmo/core/subsystem.h"

namespace dbmo {

enum class SystemState : uint8_t {
  Off,
  Booting,
  Running,   // todos os subsistemas prontos
  Degraded,  // algum subsistema opcional ausente, degradado ou com falha
  LowPower,
  ShuttingDown,
  Fault,  // um subsistema obrigatório falhou
};

const char *toString(SystemState state);

// Eventos do domínio System.
namespace system_event {
// a = estado anterior, b = estado novo (SystemState).
inline constexpr EventId kStateChanged = makeEventId(EventDomain::System, 1);
// a = índice do subsistema, b = estado novo (SubsystemState).
inline constexpr EventId kSubsystemChanged = makeEventId(EventDomain::System, 2);
// a = índice do subsistema, b = Status da falha.
inline constexpr EventId kFault = makeEventId(EventDomain::System, 3);
}  // namespace system_event

struct SubsystemOptions {
  // Obrigatório: se não ficar Ready, o sistema vai para Fault.
  bool required = false;
  // Só inicializa se este outro subsistema estiver Ready ou Degraded.
  const ISubsystem *dependsOn = nullptr;
};

struct SubsystemInfo {
  ISubsystem *subsystem = nullptr;
  SubsystemOptions options;
  SubsystemState state = SubsystemState::Uninitialized;
  Status lastStatus = Status::NotInitialized;
  uint8_t initAttempts = 0;
};

struct FaultRecord {
  uint32_t timestampMs = 0;
  uint8_t subsystemIndex = 0;
  Status status = Status::Ok;
};

class System {
 public:
  static constexpr size_t kMaxSubsystems = 16;
  static constexpr size_t kFaultHistory = 8;

  System(IClock &clock, EventBus &bus);

  // Registra um subsistema. A ordem de registro é a ordem de inicialização; o
  // desligamento ocorre na ordem inversa. Só é aceito com o sistema em Off.
  Status add(ISubsystem &subsystem, SubsystemOptions options = {});

  // Inicializa os subsistemas em ordem. Um opcional ausente ou com falha não interrompe
  // o boot (o sistema termina em Degraded). Um obrigatório que falha interrompe: o
  // sistema vai para Fault e o Status da falha é devolvido.
  Status boot();

  // Uma volta do laço principal: entrega os eventos pendentes e chama tick() dos
  // subsistemas em Ready ou Degraded.
  void tick();

  // Desliga os subsistemas na ordem inversa. Devolve o primeiro erro, mas desliga todos.
  Status shutdown();

  // Tenta inicializar de novo um subsistema que não está em operação (bancada, hot-plug).
  Status retry(const ISubsystem &subsystem);

  // Um subsistema avisa que perdeu função em operação. newState: Degraded, Failed ou
  // Absent. Se ele é obrigatório e saiu de operação, o sistema vai para Fault.
  void reportFault(const ISubsystem &subsystem, Status status,
                   SubsystemState newState = SubsystemState::Degraded);
  // Um subsistema Degraded avisa que voltou ao normal.
  void reportRecovered(const ISubsystem &subsystem);

  Status enterLowPower();
  Status exitLowPower();

  SystemState state() const { return machine_.state(); }
  size_t subsystemCount() const { return count_; }
  const SubsystemInfo &info(size_t index) const { return entries_[index]; }
  const SubsystemInfo *find(const char *name) const;
  const SubsystemInfo *find(const ISubsystem &subsystem) const;

  size_t faultCount() const { return faults_.size(); }
  // Falha index a partir da mais antiga (0). Requer index < faultCount().
  const FaultRecord &fault(size_t index) const { return faults_.at(index); }

 private:
  enum class Trigger : uint8_t {
    Boot,
    BootOk,
    BootDegraded,
    BootFailed,
    SubsystemLost,
    AllHealthy,
    EnterLowPower,
    ExitLowPower,
    FatalError,
    Shutdown,
    ShutdownDone,
  };
  using Machine = StateMachine<SystemState, Trigger>;
  static const Machine::Transition kTransitions[];

  int indexOf(const ISubsystem &subsystem) const;
  static bool inOperation(SubsystemState state);
  bool allReady() const;
  Status initEntry(size_t index);
  void setSubsystemState(size_t index, SubsystemState state);
  void recordFault(size_t index, Status status);
  void fire(Trigger trigger);
  void updateHealth();
  static void onTransition(SystemState from, SystemState to, void *context);

  IClock &clock_;
  EventBus &bus_;
  Machine machine_;
  SubsystemInfo entries_[kMaxSubsystems];
  size_t count_ = 0;
  RingBuffer<FaultRecord, kFaultHistory> faults_;
};

}  // namespace dbmo
