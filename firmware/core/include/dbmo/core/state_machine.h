// Máquina de estados dirigida por tabela. Só as transições listadas são aceitas.
#pragma once

#include <cstddef>

namespace dbmo {

template <typename State, typename Trigger>
class StateMachine {
 public:
  struct Transition {
    State from;
    Trigger trigger;
    State to;
  };
  using Observer = void (*)(State from, State to, void *context);

  template <size_t N>
  constexpr StateMachine(const Transition (&table)[N], State initial)
      : table_(table), count_(N), state_(initial) {}

  State state() const { return state_; }

  bool canFire(Trigger trigger) const { return find(trigger) != nullptr; }

  // Aplica a transição se ela existir a partir do estado atual. Falso se não existe.
  bool fire(Trigger trigger) {
    const Transition *transition = find(trigger);
    if (transition == nullptr) return false;
    const State previous = state_;
    state_ = transition->to;
    if (observer_ != nullptr) observer_(previous, state_, observerContext_);
    return true;
  }

  // Chamado depois de cada transição aplicada (inclusive para o mesmo estado).
  void setObserver(Observer observer, void *context) {
    observer_ = observer;
    observerContext_ = context;
  }

  // Força um estado sem passar pela tabela (uso: restauração e testes).
  void reset(State state) { state_ = state; }

 private:
  const Transition *find(Trigger trigger) const {
    for (size_t i = 0; i < count_; ++i)
      if (table_[i].from == state_ && table_[i].trigger == trigger) return &table_[i];
    return nullptr;
  }

  const Transition *table_;
  size_t count_;
  State state_;
  Observer observer_ = nullptr;
  void *observerContext_ = nullptr;
};

}  // namespace dbmo
