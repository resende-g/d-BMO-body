// Fila circular de capacidade fixa, sem alocação dinâmica.
#pragma once

#include <cstddef>

namespace dbmo {

template <typename T, size_t Capacity>
class RingBuffer {
 public:
  static constexpr size_t kCapacity = Capacity;

  size_t size() const { return size_; }
  size_t capacity() const { return Capacity; }
  size_t free() const { return Capacity - size_; }
  bool empty() const { return size_ == 0; }
  bool full() const { return size_ == Capacity; }

  void clear() {
    head_ = 0;
    size_ = 0;
  }

  // Falso (e nada muda) quando a fila está cheia.
  bool push(const T &item) {
    if (full()) return false;
    items_[(head_ + size_) % Capacity] = item;
    ++size_;
    return true;
  }

  // Grava sempre: com a fila cheia, descarta o item mais antigo. Verdadeiro se descartou.
  bool pushOverwrite(const T &item) {
    const bool dropped = full();
    if (dropped) {
      head_ = (head_ + 1) % Capacity;
      --size_;
    }
    items_[(head_ + size_) % Capacity] = item;
    ++size_;
    return dropped;
  }

  bool pop(T &out) {
    if (empty()) return false;
    out = items_[head_];
    head_ = (head_ + 1) % Capacity;
    --size_;
    return true;
  }

  // Item na posição index a partir do mais antigo (0). Requer index < size().
  const T &at(size_t index) const { return items_[(head_ + index) % Capacity]; }

  // Grava até count itens; devolve quantos couberam.
  size_t write(const T *items, size_t count) {
    size_t written = 0;
    while (written < count && push(items[written])) ++written;
    return written;
  }

  // Lê até count itens; devolve quantos havia.
  size_t read(T *out, size_t count) {
    size_t done = 0;
    while (done < count && pop(out[done])) ++done;
    return done;
  }

 private:
  T items_[Capacity] = {};
  size_t head_ = 0;
  size_t size_ = 0;
};

}  // namespace dbmo
