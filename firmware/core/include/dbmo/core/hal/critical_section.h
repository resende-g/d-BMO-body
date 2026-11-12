// Seção crítica curta, para dados tocados por interrupção ou por outra tarefa.
// No computador (testes) e em código de tarefa única, usar NullCriticalSection.
#pragma once

namespace dbmo {

class ICriticalSection {
 public:
  virtual ~ICriticalSection() = default;
  virtual void enter() = 0;
  virtual void exit() = 0;
};

class NullCriticalSection final : public ICriticalSection {
 public:
  void enter() override {}
  void exit() override {}
};

class ScopedCritical {
 public:
  explicit ScopedCritical(ICriticalSection &section) : section_(section) { section_.enter(); }
  ~ScopedCritical() { section_.exit(); }
  ScopedCritical(const ScopedCritical &) = delete;
  ScopedCritical &operator=(const ScopedCritical &) = delete;

 private:
  ICriticalSection &section_;
};

}  // namespace dbmo
