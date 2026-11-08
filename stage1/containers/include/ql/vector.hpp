#pragma once
// Project B2: your own std::vector.
//
// Learn: raw storage vs live objects, placement new, explicit destructor calls,
// geometric growth, the rule of five, move_if_noexcept.
//
// Plan:
//   1. Hold three things: T* data_, size_, capacity_.
//   2. Get raw bytes with ::operator new(n * sizeof(T)); free with ::operator delete.
//      Never use `new T[n]`: that would construct n objects you do not have yet.
//   3. Construct elements with placement new:  ::new (ptr) T(args...)
//      Destroy them by hand:                    ptr->~T()
//   4. When full, allocate a bigger block (double it), move the elements across
//      (std::move_if_noexcept), destroy the old ones, free the old block.
//
// Stretch goals: insert/erase, shrink_to_fit, strong exception guarantee for
// push_back, a Vector<bool>-free life.

#include <cstddef>
#include <stdexcept>
#include <utility>

#include "ql/todo.hpp"

namespace ql {

template <class T>
class Vector {
 public:
  using value_type = T;
  using iterator = T*;
  using const_iterator = const T*;

  Vector() = default;

  ~Vector() {
    // TODO: destroy every live element, then release the memory block
  }

  Vector(const Vector& other) {
    (void)other;
    todo("Vector copy constructor (deep copy)");
  }

  Vector& operator=(const Vector& other) {
    (void)other;
    todo("Vector copy assignment (handle self-assignment)");
  }

  Vector(Vector&& other) {
    (void)other;
    todo("Vector move constructor (steal the buffer, leave other empty)");
  }

  Vector& operator=(Vector&& other) {
    (void)other;
    todo("Vector move assignment");
  }

  std::size_t size() const { todo("Vector::size"); }
  std::size_t capacity() const { todo("Vector::capacity"); }
  bool empty() const { todo("Vector::empty"); }

  T* data() { todo("Vector::data"); }
  const T* data() const { todo("Vector::data const"); }

  iterator begin() { todo("Vector::begin"); }
  iterator end() { todo("Vector::end"); }
  const_iterator begin() const { todo("Vector::begin const"); }
  const_iterator end() const { todo("Vector::end const"); }

  T& operator[](std::size_t i) {
    (void)i;
    todo("Vector::operator[]");
  }
  const T& operator[](std::size_t i) const {
    (void)i;
    todo("Vector::operator[] const");
  }

  // Throws std::out_of_range if i >= size().
  T& at(std::size_t i) {
    (void)i;
    todo("Vector::at");
  }

  T& front() { todo("Vector::front"); }
  T& back() { todo("Vector::back"); }

  void reserve(std::size_t n) {
    (void)n;
    todo("Vector::reserve (never shrinks; never changes size)");
  }

  void push_back(const T& v) {
    (void)v;
    todo("Vector::push_back(const T&)");
  }

  void push_back(T&& v) {
    (void)v;
    todo("Vector::push_back(T&&)");
  }

  template <class... Args>
  T& emplace_back(Args&&... args) {
    ((void)args, ...);
    todo("Vector::emplace_back (construct in place; push_back can call this)");
  }

  void pop_back() { todo("Vector::pop_back (destroy the last element)"); }

  // Destroys all elements, keeps the allocated capacity.
  void clear() { todo("Vector::clear"); }
};

}  // namespace ql
