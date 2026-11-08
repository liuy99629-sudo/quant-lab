#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "ql/timing.hpp"

TEST(Stats, OddCount) {
  auto s = ql::compute_stats({5, 1, 3, 2, 4});
  EXPECT_EQ(s.n, 5u);
  EXPECT_DOUBLE_EQ(s.min_ns, 1);
  EXPECT_DOUBLE_EQ(s.max_ns, 5);
  EXPECT_DOUBLE_EQ(s.median_ns, 3);
  EXPECT_DOUBLE_EQ(s.mean_ns, 3);
  EXPECT_DOUBLE_EQ(s.p99_ns, 5);
}

TEST(Stats, EvenCountMedianIsAverageOfMiddleTwo) {
  auto s = ql::compute_stats({4, 1, 3, 2});
  EXPECT_DOUBLE_EQ(s.median_ns, 2.5);
}

TEST(Stats, P99UsesNearestRank) {
  std::vector<double> v;
  for (int i = 1; i <= 100; ++i) v.push_back(i);
  auto s = ql::compute_stats(v);
  EXPECT_DOUBLE_EQ(s.p99_ns, 99);
}

TEST(Stats, EmptyInputGivesZeros) {
  auto s = ql::compute_stats({});
  EXPECT_EQ(s.n, 0u);
  EXPECT_DOUBLE_EQ(s.median_ns, 0);
}

TEST(Measure, RecordsRequestedNumberOfIterations) {
  int counter = 0;
  auto s = ql::measure_ns([&] { ++counter; }, 500, 10);
  EXPECT_EQ(s.n, 500u);
  EXPECT_EQ(counter, 510);  // warmup calls run but are not recorded
}

TEST(Measure, StatsAreOrdered) {
  auto s = ql::measure_ns(
      [] {
        std::string str(64, 'x');
        ql::do_not_optimize(str);
      },
      2000);
  EXPECT_LE(s.min_ns, s.median_ns);
  EXPECT_LE(s.median_ns, s.p99_ns);
  EXPECT_LE(s.p99_ns, s.max_ns);
  EXPECT_GT(s.max_ns, 0);
}

TEST(Measure, ClockOverheadIsSmallAndPositive) {
  double o = ql::clock_overhead_ns(20000);
  EXPECT_GE(o, 0);
  EXPECT_LT(o, 10000);  // 10 microseconds; a clock read should be far cheaper
}

// ---- week 2 extensions ------------------------------------------------------

TEST(Stats, P999UsesNearestRank) {
  std::vector<double> v;
  for (int i = 1; i <= 1000; ++i) v.push_back(i);
  auto s = ql::compute_stats(v);
  EXPECT_DOUBLE_EQ(s.p999_ns, 999);
  EXPECT_LE(s.p99_ns, s.p999_ns);
  EXPECT_LE(s.p999_ns, s.max_ns);
}

TEST(Stats, P999WithFewSamplesIsMax) {
  auto s = ql::compute_stats({3, 1, 2});
  EXPECT_DOUBLE_EQ(s.p999_ns, 3);
}

TEST(MeasureAdjusted, NeverNegativeAndCountsCalls) {
  int counter = 0;
  auto s = ql::measure_ns_adjusted([&] { ++counter; }, 500, 10);
  EXPECT_EQ(s.n, 500u);
  EXPECT_EQ(counter, 510);
  EXPECT_GE(s.min_ns, 0);
}

TEST(MeasureBatched, CallsFBatchTimesPerSample) {
  int counter = 0;
  auto s = ql::measure_batched_ns([&] { ++counter; }, 100, 50, 10);
  EXPECT_EQ(s.n, 100u);
  EXPECT_EQ(counter, 10 + 100 * 50);
  EXPECT_GE(s.min_ns, 0);
}

TEST(MeasureBatched, ZeroBatchActsLikeOne) {
  int counter = 0;
  auto s = ql::measure_batched_ns([&] { ++counter; }, 20, 0, 0);
  EXPECT_EQ(s.n, 20u);
  EXPECT_EQ(counter, 20);
}

TEST(Histogram, BucketsByDecade) {
  auto h = ql::histogram_log10({0.5, 5, 15, 150, 150, 2000});
  ASSERT_EQ(h.size(), 4u);
  EXPECT_EQ(h[0], 2u);  // [0, 10) ns
  EXPECT_EQ(h[1], 1u);  // [10, 100) ns
  EXPECT_EQ(h[2], 2u);  // [100 ns, 1 us)
  EXPECT_EQ(h[3], 1u);  // [1 us, 10 us)
}

TEST(Histogram, EmptyInputGivesEmptyHistogram) { EXPECT_TRUE(ql::histogram_log10({}).empty()); }

TEST(Histogram, Labels) {
  EXPECT_EQ(ql::pow10_ns_label(0), "1ns");
  EXPECT_EQ(ql::pow10_ns_label(2), "100ns");
  EXPECT_EQ(ql::pow10_ns_label(3), "1us");
  EXPECT_EQ(ql::pow10_ns_label(7), "10ms");
}
