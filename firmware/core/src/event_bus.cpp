#include "dbmo/core/event_bus.h"

namespace dbmo {

EventBus::EventBus(IClock *clock, ICriticalSection *lock)
    : clock_(clock), lock_(lock != nullptr ? lock : &nullLock_) {}

Status EventBus::add(Filter filter, EventId id, Handler handler, void *context) {
  if (handler == nullptr) return Status::InvalidArgument;
  for (size_t i = 0; i < slots_; ++i) {
    const Subscription &existing = subscriptions_[i];
    if (existing.filter == filter && existing.id == id && existing.handler == handler &&
        existing.context == context)
      return Status::AlreadyInitialized;
  }
  if (slots_ == kMaxSubscribers) return Status::NoSpace;
  subscriptions_[slots_++] = Subscription{filter, id, handler, context};
  return Status::Ok;
}

Status EventBus::subscribe(EventId id, Handler handler, void *context) {
  return add(Filter::Exact, id, handler, context);
}

Status EventBus::subscribeDomain(EventDomain domain, Handler handler, void *context) {
  return add(Filter::Domain, makeEventId(domain, 0), handler, context);
}

Status EventBus::subscribeAll(Handler handler, void *context) {
  return add(Filter::All, 0, handler, context);
}

Status EventBus::unsubscribe(Handler handler, void *context) {
  if (handler == nullptr) return Status::InvalidArgument;
  size_t cancelled = 0;
  for (size_t i = 0; i < slots_; ++i) {
    Subscription &subscription = subscriptions_[i];
    if (subscription.handler == handler && subscription.context == context) {
      subscription.handler = nullptr;
      ++cancelled;
    }
  }
  if (cancelled == 0) return Status::NotFound;
  removed_ += cancelled;
  // Durante a entrega a tabela não pode ser reordenada: quem está sendo percorrido
  // perderia eventos. A compactação fica para o fim do dispatch().
  if (!dispatching_) compact();
  return Status::Ok;
}

void EventBus::compact() {
  size_t kept = 0;
  for (size_t i = 0; i < slots_; ++i)
    if (subscriptions_[i].handler != nullptr) subscriptions_[kept++] = subscriptions_[i];
  slots_ = kept;
  removed_ = 0;
}

Status EventBus::publish(const Event &event) {
  Event stamped = event;
  if (clock_ != nullptr && stamped.timestampMs == 0) stamped.timestampMs = clock_->nowMs();

  ScopedCritical guard(*lock_);
  if (!queue_.push(stamped)) {
    ++dropped_;
    return Status::NoSpace;
  }
  return Status::Ok;
}

bool EventBus::matches(const Subscription &subscription, EventId id) {
  switch (subscription.filter) {
    case Filter::Exact: return subscription.id == id;
    case Filter::Domain: return domainOf(subscription.id) == domainOf(id);
    case Filter::All: return true;
  }
  return false;
}

size_t EventBus::dispatch(size_t maxEvents) {
  if (dispatching_) return 0;
  dispatching_ = true;

  size_t delivered = 0;
  while (delivered < maxEvents) {
    Event event;
    {
      ScopedCritical guard(*lock_);
      if (!queue_.pop(event)) break;
    }
    // Limite fixado antes da entrega: quem se inscreve agora só recebe os próximos.
    const size_t count = slots_;
    for (size_t i = 0; i < count; ++i) {
      const Subscription subscription = subscriptions_[i];
      if (subscription.handler != nullptr && matches(subscription, event.id))
        subscription.handler(event, subscription.context);
    }
    ++delivered;
  }

  dispatching_ = false;
  if (removed_ > 0) compact();
  return delivered;
}

}  // namespace dbmo
