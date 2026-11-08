#pragma once
// Project B3: fixed-capacity object pool.  No heap allocation, ever.
//
// This is the allocation strategy low-latency systems use on the hot path:
// reserve every object up front, hand them out in O(1), take them back in O(1).
//
// Contract the tests check:
//   * ObjectPool<T, N> owns N slots of inline storage (an aligned byte array).
//   * acquire(args...)  constructs a T in a free slot and returns its address,
//                       or nullptr when all N slots are in use.
//   * release(p)        destroys *p and frees its slot.  Returns true on
//                       success.  Returns false (and does nothing) when p does
//                       not belong to this pool, or its slot is not in use
//                       (double release).
//   * The most recently released slot is the next one handed out (LIFO).
//   * Destroying the pool destroys any objects still alive in it.
//   * size() = number of slots in use.
//
// Suggested layout: alignas(T) std::byte storage_[N * sizeof(T)];
//                   bool live_[N];  std::size_t free_[N];  std::size_t free_count_;
// Compare addresses with std::uintptr_t, not with < on unrelated pointers.
//
// Stretch goals: make it thread-safe for one producer / one consumer, or build
// a version whose free list lives inside the free slots themselves.

#include <cstddef>
#include <cstdint>
#include <new>
#include <utility>

#include "ql/todo.hpp"

namespace ql {

template <class T, std::size_t N>
class ObjectPool {
 public:
  ObjectPool() = default;
  ObjectPool(const ObjectPool&) = delete;
  ObjectPool& operator=(const ObjectPool&) = delete;

  ~ObjectPool() {
    // TODO: destroy every object that is still alive
  }

  static constexpr std::size_t capacity() noexcept { return N; }

  std::size_t size() const { todo("ObjectPool::size"); }

  template <class... Args>
  T* acquire(Args&&... args) {
    ((void)args, ...);
    todo("ObjectPool::acquire");
  }

  bool release(T* p) {
    (void)p;
    todo("ObjectPool::release");
  }

 private:
  // TODO: add your storage and bookkeeping here
};

}  // namespace ql
