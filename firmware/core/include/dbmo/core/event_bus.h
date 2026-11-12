// Barramento de eventos assíncrono: publish() só enfileira; dispatch() entrega na tarefa
// principal. É o único canal de comunicação entre subsistemas.
#pragma once

#include <cstddef>
#include <cstdint>

#include "dbmo/core/event.h"
#include "dbmo/core/hal/clock.h"
#include "dbmo/core/hal/critical_section.h"
#include "dbmo/core/ring_buffer.h"
#include "dbmo/core/status.h"

namespace dbmo {

class EventBus {
 public:
  using Handler = void (*)(const Event &event, void *context);

  static constexpr size_t kQueueCapacity = 32;
  static constexpr size_t kMaxSubscribers = 32;

  // clock carimba o evento na publicação; lock protege a fila quando há publicação a
  // partir de interrupção ou de outra tarefa. Ambos opcionais.
  explicit EventBus(IClock *clock = nullptr, ICriticalSection *lock = nullptr);

  Status subscribe(EventId id, Handler handler, void *context);
  Status subscribeDomain(EventDomain domain, Handler handler, void *context);
  Status subscribeAll(Handler handler, void *context);
  // Remove todas as inscrições desse par (handler, context).
  Status unsubscribe(Handler handler, void *context);

  // Enfileira. NoSpace se a fila está cheia (o evento é descartado e contado).
  Status publish(const Event &event);
  Status publish(EventId id, uint32_t a = 0, uint32_t b = 0) { return publish(Event(id, a, b)); }

  // Entrega até maxEvents eventos pendentes aos inscritos; devolve quantos entregou.
  // Eventos publicados por um handler entram na fila e respeitam o mesmo limite.
  // Um handler pode se inscrever ou cancelar inscrições durante a entrega: quem entra
  // só recebe os eventos seguintes; quem sai deixa de receber na hora. Um handler não
  // pode chamar dispatch() (a chamada devolve 0).
  size_t dispatch(size_t maxEvents = kQueueCapacity);

  size_t pending() const { return queue_.size(); }
  uint32_t droppedCount() const { return dropped_; }
  size_t subscriberCount() const { return slots_ - removed_; }

 private:
  enum class Filter : uint8_t { Exact, Domain, All };
  struct Subscription {
    Filter filter;
    EventId id;  // Exact: id; Domain: domínio no byte alto
    Handler handler;  // nullptr: inscrição cancelada, aguardando compactação
    void *context;
  };

  Status add(Filter filter, EventId id, Handler handler, void *context);
  void compact();
  static bool matches(const Subscription &subscription, EventId id);

  IClock *clock_;
  ICriticalSection *lock_;
  NullCriticalSection nullLock_;
  RingBuffer<Event, kQueueCapacity> queue_;
  Subscription subscriptions_[kMaxSubscribers] = {};
  size_t slots_ = 0;    // posições ocupadas da tabela, incluindo as canceladas
  size_t removed_ = 0;  // canceladas durante uma entrega, ainda na tabela
  bool dispatching_ = false;
  uint32_t dropped_ = 0;
};

}  // namespace dbmo
