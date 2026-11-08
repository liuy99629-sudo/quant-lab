#include <gtest/gtest.h>

#include <numeric>
#include <stdexcept>
#include <string>
#include <utility>

#include "ql/vector.hpp"

namespace {
struct Counted {
  static inline int alive = 0, copies = 0, moves = 0;
  int v;
  explicit Counted(int x = 0) : v(x) { ++alive; }
  Counted(const Counted& o) : v(o.v) { ++alive; ++copies; }
  Counted(Counted&& o) noexcept : v(o.v) { ++alive; ++moves; }
  Counted& operator=(const Counted& o) { v = o.v; ++copies; return *this; }
  Counted& operator=(Counted&& o) noexcept { v = o.v; ++moves; return *this; }
  ~Counted() { --alive; }
  static void reset() { alive = copies = moves = 0; }
};
}  // namespace

class VectorTest : public ::testing::Test {
 protected:
  void SetUp() override { Counted::reset(); }
};

TEST_F(VectorTest, StartsEmpty) {
  ql::Vector<int> v;
  EXPECT_EQ(v.size(), 0u);
  EXPECT_EQ(v.capacity(), 0u);
  EXPECT_TRUE(v.empty());
  EXPECT_EQ(v.begin(), v.end());
}

TEST_F(VectorTest, PushBackAndIndex) {
  ql::Vector<int> v;
  for (int i = 0; i < 10; ++i) v.push_back(i * i);
  ASSERT_EQ(v.size(), 10u);
  for (int i = 0; i < 10; ++i) EXPECT_EQ(v[static_cast<std::size_t>(i)], i * i);
  EXPECT_EQ(v.front(), 0);
  EXPECT_EQ(v.back(), 81);
}

TEST_F(VectorTest, GrowthIsGeometric) {
  ql::Vector<int> v;
  std::size_t reallocations = 0, last_cap = v.capacity();
  for (int i = 0; i < 1000; ++i) {
    v.push_back(i);
    EXPECT_GE(v.capacity(), v.size());
    if (v.capacity() != last_cap) {
      ++reallocations;
      last_cap = v.capacity();
    }
  }
  EXPECT_LE(reallocations, 12u) << "capacity should multiply, not grow by a constant";
}

TEST_F(VectorTest, ReserveAvoidsReallocation) {
  ql::Vector<int> v;
  v.reserve(100);
  EXPECT_GE(v.capacity(), 100u);
  EXPECT_EQ(v.size(), 0u);
  const int* before = v.data();
  for (int i = 0; i < 100; ++i) v.push_back(i);
  EXPECT_EQ(v.data(), before);
}

TEST_F(VectorTest, ReserveSmallerDoesNothing) {
  ql::Vector<int> v;
  v.reserve(50);
  v.reserve(10);
  EXPECT_GE(v.capacity(), 50u);
}

TEST_F(VectorTest, ElementsAreContiguousAndRangeForWorks) {
  ql::Vector<int> v;
  for (int i = 1; i <= 5; ++i) v.push_back(i);
  int sum = 0;
  for (int x : v) sum += x;
  EXPECT_EQ(sum, 15);
  EXPECT_EQ(v.data() + 3, &v[3]);
}

TEST_F(VectorTest, AtThrowsWhenOutOfRange) {
  ql::Vector<int> v;
  v.push_back(1);
  EXPECT_EQ(v.at(0), 1);
  EXPECT_THROW(v.at(1), std::out_of_range);
}

TEST_F(VectorTest, PopBackDestroysTheElement) {
  ql::Vector<Counted> v;
  for (int i = 0; i < 3; ++i) v.emplace_back(i);
  EXPECT_EQ(Counted::alive, 3);
  v.pop_back();
  EXPECT_EQ(Counted::alive, 2);
  EXPECT_EQ(v.size(), 2u);
  EXPECT_EQ(v.back().v, 1);
}

TEST_F(VectorTest, ClearDestroysElementsButKeepsCapacity) {
  ql::Vector<Counted> v;
  for (int i = 0; i < 10; ++i) v.emplace_back(i);
  const auto cap = v.capacity();
  v.clear();
  EXPECT_EQ(Counted::alive, 0);
  EXPECT_EQ(v.size(), 0u);
  EXPECT_EQ(v.capacity(), cap);
}

TEST_F(VectorTest, DestructorDestroysEverything) {
  {
    ql::Vector<Counted> v;
    for (int i = 0; i < 20; ++i) v.emplace_back(i);
    EXPECT_EQ(Counted::alive, 20);
  }
  EXPECT_EQ(Counted::alive, 0);
}

TEST_F(VectorTest, GrowthMovesInsteadOfCopyingWhenMoveIsNoexcept) {
  ql::Vector<Counted> v;
  for (int i = 0; i < 100; ++i) v.emplace_back(i);
  EXPECT_EQ(Counted::copies, 0);
  for (int i = 0; i < 100; ++i) v.push_back(Counted(i));
  EXPECT_EQ(Counted::copies, 0);
  EXPECT_EQ(v.size(), 200u);
  EXPECT_EQ(Counted::alive, 200);
}

TEST_F(VectorTest, EmplaceBackConstructsInPlaceAndReturnsReference) {
  ql::Vector<std::string> v;
  std::string& s = v.emplace_back(3, 'a');
  EXPECT_EQ(s, "aaa");
  EXPECT_EQ(&s, &v.back());
}

TEST_F(VectorTest, CopyConstructorMakesDeepCopy) {
  ql::Vector<std::string> a;
  for (int i = 0; i < 5; ++i) a.push_back(std::string(40, static_cast<char>('a' + i)));  // long: heap strings
  ql::Vector<std::string> b(a);
  ASSERT_EQ(b.size(), 5u);
  b[0] = "changed";
  EXPECT_EQ(a[0], std::string(40, 'a'));
  EXPECT_NE(a.data(), b.data());
}

TEST_F(VectorTest, CopyAssignmentReplacesContentsAndHandlesSelf) {
  ql::Vector<int> a, b;
  for (int i = 0; i < 4; ++i) a.push_back(i);
  for (int i = 0; i < 9; ++i) b.push_back(100 + i);
  b = a;
  ASSERT_EQ(b.size(), 4u);
  EXPECT_EQ(b[3], 3);
  auto& alias = b;
  b = alias;
  EXPECT_EQ(b.size(), 4u);
  EXPECT_EQ(b[2], 2);
}

TEST_F(VectorTest, MoveConstructorStealsTheBuffer) {
  ql::Vector<Counted> a;
  for (int i = 0; i < 8; ++i) a.emplace_back(i);
  const Counted* buf = a.data();
  ql::Vector<Counted> b(std::move(a));
  EXPECT_EQ(b.data(), buf);
  EXPECT_EQ(b.size(), 8u);
  EXPECT_EQ(a.size(), 0u);
  EXPECT_EQ(Counted::copies, 0);
  EXPECT_EQ(Counted::alive, 8);
}

TEST_F(VectorTest, MoveAssignmentReleasesOldContentsAndSteals) {
  ql::Vector<Counted> a, b;
  for (int i = 0; i < 3; ++i) a.emplace_back(i);
  for (int i = 0; i < 5; ++i) b.emplace_back(i);
  const Counted* buf = a.data();
  b = std::move(a);
  EXPECT_EQ(Counted::alive, 3);
  EXPECT_EQ(b.data(), buf);
  EXPECT_EQ(a.size(), 0u);
}

TEST_F(VectorTest, WorksWithStringsUnderSanitizers) {
  ql::Vector<std::string> v;
  for (int i = 0; i < 500; ++i) v.push_back("a string long enough to live on the heap #" + std::to_string(i));
  EXPECT_EQ(v[499], "a string long enough to live on the heap #499");
  v.pop_back();
  v.clear();
  EXPECT_TRUE(v.empty());
}
