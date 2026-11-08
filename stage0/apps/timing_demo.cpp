// Prints timing statistics for a few small operations.
// Run in Release mode:  ./ql demo
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <random>
#include <vector>

#include "ql/timing.hpp"

static void print(const char* name, const ql::Stats& s) {
  std::printf("%-28s n=%-7zu min=%8.1f  median=%8.1f  mean=%8.1f  p99=%8.0f  p99.9=%8.0f  max=%10.0f  (ns)\n",
              name, s.n, s.min_ns, s.median_ns, s.mean_ns, s.p99_ns, s.p999_ns, s.max_ns);
}

int main() {
  std::printf("clock overhead (median): %.0f ns\n\n", ql::clock_overhead_ns());

  std::mt19937 rng(42);
  std::vector<int> base(1000);
  for (auto& x : base) x = static_cast<int>(rng());

  print("sort 1000 ints", ql::measure_ns(
                              [&] {
                                auto v = base;
                                std::sort(v.begin(), v.end());
                                ql::do_not_optimize(v);
                              },
                              5000));

  print("push_back 1000 (no reserve)", ql::measure_ns(
                                           [] {
                                             std::vector<int> v;
                                             for (int i = 0; i < 1000; ++i) v.push_back(i);
                                             ql::do_not_optimize(v);
                                           },
                                           5000));

  print("push_back 1000 (reserve)", ql::measure_ns(
                                        [] {
                                          std::vector<int> v;
                                          v.reserve(1000);
                                          for (int i = 0; i < 1000; ++i) v.push_back(i);
                                          ql::do_not_optimize(v);
                                        },
                                        5000));

  // Week 2: a ~1 ns operation is drowned by the clock overhead unless batched.
  std::uint64_t x = 1;
  const auto tiny = [&] {
    x = x * 6364136223846793005ULL + 1;
    ql::do_not_optimize(x);
  };
  std::printf("\n");
  print("tiny op (single)", ql::measure_ns(tiny, 100000));
  print("tiny op (adjusted)", ql::measure_ns_adjusted(tiny, 100000));
  print("tiny op (batch 1000)", ql::measure_batched_ns(tiny, 1000, 1000));

  std::printf("\nhistogram: sort 1000 ints\n");
  ql::print_histogram(ql::measure_samples_ns(
      [&] {
        auto v = base;
        std::sort(v.begin(), v.end());
        ql::do_not_optimize(v);
      },
      5000));
  return 0;
}
