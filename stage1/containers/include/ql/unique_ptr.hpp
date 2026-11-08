#pragma once
// Project B1: your own std::unique_ptr (single object, default delete only).
//
// Learn: RAII, ownership, move semantics, why copy is deleted.
// Every member below is declared so the tests compile; the bodies marked TODO
// are yours.  Remove the todo(...) call when you implement a function.
//
// Stretch goals (after the tests pass):
//   * mark the move operations, get(), operator*, operator->, release() noexcept
//   * add a custom Deleter template parameter (default: std::default_delete<T>)
//   * support T[] with operator[]

#include <utility>

#include "ql/todo.hpp"

namespace ql {

template <class T>
class UniquePtr {
 public:
  UniquePtr() = default;  // holds nothing

  explicit UniquePtr(T* p) {  // takes ownership of p
    (void)p;
    // TODO
  }

  ~UniquePtr() {
    // TODO: destroy what we own (delete of a null pointer is fine)
  }

  UniquePtr(const UniquePtr&) = delete;
  UniquePtr& operator=(const UniquePtr&) = delete;

  UniquePtr(UniquePtr&& other) {
    (void)other;
    todo("UniquePtr move constructor");
  }

  UniquePtr& operator=(UniquePtr&& other) {
    (void)other;
    todo("UniquePtr move assignment (watch out for self-assignment)");
  }

  T* get() const { todo("UniquePtr::get"); }
  T& operator*() const { todo("UniquePtr::operator*"); }
  T* operator->() const { todo("UniquePtr::operator->"); }
  explicit operator bool() const { todo("UniquePtr::operator bool"); }

  // Gives up ownership WITHOUT destroying; caller becomes responsible.
  T* release() { todo("UniquePtr::release"); }

  // Destroys the old object (if any) and takes ownership of p.
  void reset(T* p = nullptr) {
    (void)p;
    todo("UniquePtr::reset");
  }

  void swap(UniquePtr& other) {
    (void)other;
    todo("UniquePtr::swap");
  }

 private:
  T* ptr_ = nullptr;
};

// Constructs a T from args and wraps it.
template <class T, class... Args>
UniquePtr<T> make_unique(Args&&... args) {
  ((void)args, ...);
  todo("ql::make_unique (hint: std::forward)");
}

}  // namespace ql
