#include <gtest/gtest.h>

#include <string>
#include <type_traits>
#include <utility>

#include "ql/unique_ptr.hpp"

namespace {
struct Tracker {
  static inline int alive = 0;
  int value;
  explicit Tracker(int v = 0) : value(v) { ++alive; }
  ~Tracker() { --alive; }
  Tracker(const Tracker&) = delete;
};
}  // namespace

class UniquePtrTest : public ::testing::Test {
 protected:
  void SetUp() override { Tracker::alive = 0; }
};

TEST_F(UniquePtrTest, DefaultIsNull) {
  ql::UniquePtr<Tracker> p;
  EXPECT_EQ(p.get(), nullptr);
  EXPECT_FALSE(static_cast<bool>(p));
}

TEST_F(UniquePtrTest, OwnsAndDestroysAtScopeExit) {
  {
    ql::UniquePtr<Tracker> p(new Tracker(5));
    EXPECT_EQ(Tracker::alive, 1);
    EXPECT_TRUE(static_cast<bool>(p));
  }
  EXPECT_EQ(Tracker::alive, 0);
}

TEST_F(UniquePtrTest, DereferenceAndArrow) {
  ql::UniquePtr<Tracker> p(new Tracker(42));
  EXPECT_EQ(p->value, 42);
  EXPECT_EQ((*p).value, 42);
  p->value = 7;
  EXPECT_EQ(p.get()->value, 7);
}

TEST_F(UniquePtrTest, MoveConstructorTransfersOwnership) {
  ql::UniquePtr<Tracker> a(new Tracker(1));
  Tracker* raw = a.get();
  ql::UniquePtr<Tracker> b(std::move(a));
  EXPECT_EQ(a.get(), nullptr);
  EXPECT_EQ(b.get(), raw);
  EXPECT_EQ(Tracker::alive, 1);
}

TEST_F(UniquePtrTest, MoveAssignmentDestroysPreviousObject) {
  ql::UniquePtr<Tracker> a(new Tracker(1));
  ql::UniquePtr<Tracker> b(new Tracker(2));
  EXPECT_EQ(Tracker::alive, 2);
  Tracker* raw_b = b.get();
  a = std::move(b);
  EXPECT_EQ(Tracker::alive, 1);
  EXPECT_EQ(a.get(), raw_b);
  EXPECT_EQ(b.get(), nullptr);
}

TEST_F(UniquePtrTest, SelfMoveAssignmentIsSafe) {
  ql::UniquePtr<Tracker> a(new Tracker(1));
  auto& alias = a;
  a = std::move(alias);
  EXPECT_NE(a.get(), nullptr);
  EXPECT_EQ(Tracker::alive, 1);
}

TEST_F(UniquePtrTest, ResetReplacesAndDestroysOld) {
  ql::UniquePtr<Tracker> p(new Tracker(1));
  p.reset(new Tracker(2));
  EXPECT_EQ(Tracker::alive, 1);
  EXPECT_EQ(p->value, 2);
  p.reset();
  EXPECT_EQ(Tracker::alive, 0);
  EXPECT_EQ(p.get(), nullptr);
}

TEST_F(UniquePtrTest, ReleaseGivesUpOwnershipWithoutDestroying) {
  ql::UniquePtr<Tracker> p(new Tracker(9));
  Tracker* raw = p.release();
  EXPECT_EQ(p.get(), nullptr);
  EXPECT_EQ(Tracker::alive, 1);
  delete raw;
  EXPECT_EQ(Tracker::alive, 0);
}

TEST_F(UniquePtrTest, SwapExchangesPointers) {
  ql::UniquePtr<Tracker> a(new Tracker(1));
  ql::UniquePtr<Tracker> b(new Tracker(2));
  a.swap(b);
  EXPECT_EQ(a->value, 2);
  EXPECT_EQ(b->value, 1);
  EXPECT_EQ(Tracker::alive, 2);
}

TEST_F(UniquePtrTest, MakeUniqueForwardsArguments) {
  auto p = ql::make_unique<std::string>(5, 'x');
  EXPECT_EQ(*p, "xxxxx");
  auto t = ql::make_unique<Tracker>(11);
  EXPECT_EQ(t->value, 11);
  EXPECT_EQ(Tracker::alive, 1);
}

TEST(UniquePtrTraits, CopyIsForbidden) {
  static_assert(!std::is_copy_constructible_v<ql::UniquePtr<int>>);
  static_assert(!std::is_copy_assignable_v<ql::UniquePtr<int>>);
  static_assert(std::is_move_constructible_v<ql::UniquePtr<int>>);
  static_assert(std::is_move_assignable_v<ql::UniquePtr<int>>);
  SUCCEED();
}
