#include <gtest/gtest.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <new>
#include <string>

#include "ql/object_pool.hpp"

// Count every heap allocation made through operator new in this executable.
static std::atomic<std::size_t> g_allocs{0};

// Every allocation form must be replaced together, otherwise a pointer from one
// allocator can reach another allocator's free() (AddressSanitizer reports that).
static void* counted_alloc(std::size_t n) {
  g_allocs.fetch_add(1, std::memory_order_relaxed);
  return std::malloc(n ? n : 1);
}
static void* counted_aligned_alloc(std::size_t n, std::size_t align) {
  g_allocs.fetch_add(1, std::memory_order_relaxed);
  if (n == 0) n = 1;
  return std::aligned_alloc(align, (n + align - 1) / align * align);
}

void* operator new(std::size_t n) {
  if (void* p = counted_alloc(n)) return p;
  throw std::bad_alloc();
}
void* operator new[](std::size_t n) {
  if (void* p = counted_alloc(n)) return p;
  throw std::bad_alloc();
}
void* operator new(std::size_t n, const std::nothrow_t&) noexcept { return counted_alloc(n); }
void* operator new[](std::size_t n, const std::nothrow_t&) noexcept { return counted_alloc(n); }
void* operator new(std::size_t n, std::align_val_t a) {
  if (void* p = counted_aligned_alloc(n, static_cast<std::size_t>(a))) return p;
  throw std::bad_alloc();
}
void* operator new[](std::size_t n, std::align_val_t a) {
  if (void* p = counted_aligned_alloc(n, static_cast<std::size_t>(a))) return p;
  throw std::bad_alloc();
}

void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }
void operator delete(void* p, const std::nothrow_t&) noexcept { std::free(p); }
void operator delete[](void* p, const std::nothrow_t&) noexcept { std::free(p); }
void operator delete(void* p, std::align_val_t) noexcept { std::free(p); }
void operator delete[](void* p, std::align_val_t) noexcept { std::free(p); }
void operator delete(void* p, std::size_t, std::align_val_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t, std::align_val_t) noexcept { std::free(p); }

namespace {
struct Tracker {
  static inline int alive = 0;
  int a;
  std::string b;
  Tracker(int x, std::string y) : a(x), b(std::move(y)) { ++alive; }
  explicit Tracker(int x = 0) : a(x) { ++alive; }
  ~Tracker() { --alive; }
};
struct alignas(32) Wide {
  char c = 0;
};
}  // namespace

class PoolTest : public ::testing::Test {
 protected:
  void SetUp() override { Tracker::alive = 0; }
};

TEST_F(PoolTest, AcquireUpToCapacityThenNull) {
  ql::ObjectPool<Tracker, 4> pool;
  EXPECT_EQ(pool.capacity(), 4u);
  EXPECT_EQ(pool.size(), 0u);
  std::array<Tracker*, 4> p{};
  for (std::size_t i = 0; i < 4; ++i) {
    p[i] = pool.acquire(static_cast<int>(i));
    ASSERT_NE(p[i], nullptr);
  }
  EXPECT_EQ(pool.size(), 4u);
  EXPECT_EQ(pool.acquire(99), nullptr);
  for (std::size_t i = 0; i < 4; ++i)
    for (std::size_t j = i + 1; j < 4; ++j) EXPECT_NE(p[i], p[j]);
}

TEST_F(PoolTest, ArgumentsAreForwardedToTheConstructor) {
  ql::ObjectPool<Tracker, 2> pool;
  Tracker* t = pool.acquire(7, std::string("hello"));
  ASSERT_NE(t, nullptr);
  EXPECT_EQ(t->a, 7);
  EXPECT_EQ(t->b, "hello");
}

TEST_F(PoolTest, ReleaseDestroysTheObjectAndFreesTheSlot) {
  ql::ObjectPool<Tracker, 2> pool;
  Tracker* t = pool.acquire(1);
  EXPECT_EQ(Tracker::alive, 1);
  EXPECT_TRUE(pool.release(t));
  EXPECT_EQ(Tracker::alive, 0);
  EXPECT_EQ(pool.size(), 0u);
}

TEST_F(PoolTest, MostRecentlyReleasedSlotIsReusedFirst) {
  ql::ObjectPool<Tracker, 4> pool;
  Tracker* a = pool.acquire(1);
  Tracker* b = pool.acquire(2);
  (void)a;
  ASSERT_TRUE(pool.release(b));
  Tracker* c = pool.acquire(3);
  EXPECT_EQ(c, b);
}

TEST_F(PoolTest, DoubleReleaseAndForeignPointersAreRejected) {
  ql::ObjectPool<Tracker, 2> pool;
  Tracker* t = pool.acquire(1);
  EXPECT_TRUE(pool.release(t));
  EXPECT_FALSE(pool.release(t));  // already free
  Tracker outsider(5);
  EXPECT_FALSE(pool.release(&outsider));
  EXPECT_FALSE(pool.release(nullptr));
  EXPECT_EQ(Tracker::alive, 1);  // only `outsider` is alive
}

TEST_F(PoolTest, DestroyingThePoolDestroysLiveObjects) {
  {
    ql::ObjectPool<Tracker, 8> pool;
    for (int i = 0; i < 5; ++i) pool.acquire(i);
    EXPECT_EQ(Tracker::alive, 5);
  }
  EXPECT_EQ(Tracker::alive, 0);
}

TEST_F(PoolTest, SlotsRespectTheAlignmentOfT) {
  ql::ObjectPool<Wide, 8> pool;
  for (int i = 0; i < 8; ++i) {
    Wide* w = pool.acquire();
    ASSERT_NE(w, nullptr);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(w) % alignof(Wide), 0u);
  }
}

TEST_F(PoolTest, NeverTouchesTheHeapOnTheHotPath) {
  ql::ObjectPool<Tracker, 64> pool;
  std::array<Tracker*, 64> slots{};
  const std::size_t before = g_allocs.load();
  for (int round = 0; round < 1000; ++round) {
    for (std::size_t i = 0; i < 64; ++i) slots[i] = pool.acquire(static_cast<int>(i));
    for (std::size_t i = 0; i < 64; ++i) pool.release(slots[i]);
  }
  const std::size_t after = g_allocs.load();
  EXPECT_EQ(after - before, 0u) << "acquire/release must not allocate";
}
