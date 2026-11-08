#pragma once
// Stage 0 timing tool.  Measures how long a callable takes, in nanoseconds,
// and reports min / median / mean / p99 / max.
//
// Everything in later stages is benchmarked with this, so read it once and
// then extend it (see docs/stage0.md for ideas).

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <numeric>
#include <string>
#include <vector>

namespace ql {

struct Stats {
  std::size_t n = 0;
  double min_ns = 0;
  double median_ns = 0;
  double mean_ns = 0;
  double p99_ns = 0;
  double p999_ns = 0;
  double max_ns = 0;
};

// Nearest-rank percentile on an already sorted vector.  p is in (0, 100].
inline double percentile_sorted(const std::vector<double>& sorted, double p) {
  if (sorted.empty()) return 0.0;
  // Multiply before dividing: 99.9 / 100.0 rounds up slightly, and
  // ceil(0.999000...01 * 1000) would give rank 1000 instead of 999.
  auto rank = static_cast<std::size_t>(std::ceil(p * static_cast<double>(sorted.size()) / 100.0));
  rank = std::clamp<std::size_t>(rank, 1, sorted.size());
  return sorted[rank - 1];
}

inline Stats compute_stats(std::vector<double> samples) {
  Stats s;
  if (samples.empty()) return s;
  std::sort(samples.begin(), samples.end());
  s.n = samples.size();
  s.min_ns = samples.front();
  s.max_ns = samples.back();
  s.mean_ns = std::accumulate(samples.begin(), samples.end(), 0.0) / static_cast<double>(s.n);
  const std::size_t mid = s.n / 2;
  s.median_ns = (s.n % 2 == 1) ? samples[mid] : (samples[mid - 1] + samples[mid]) / 2.0;
  s.p99_ns = percentile_sorted(samples, 99.0);
  s.p999_ns = percentile_sorted(samples, 99.9);
  return s;
}

// Stops the compiler from deleting the work you are trying to measure.
template <class T>
inline void do_not_optimize(const T& value) {
  asm volatile("" : : "r,m"(value) : "memory");
}

inline void clobber_memory() { asm volatile("" : : : "memory"); }

// Calls f() `warmup` times (not recorded), then `iters` times, timing each
// call separately.  Returns the raw samples in call order.
template <class F>
std::vector<double> measure_samples_ns(F&& f, std::size_t iters, std::size_t warmup = 100) {
  using clock = std::chrono::steady_clock;
  for (std::size_t i = 0; i < warmup; ++i) f();

  std::vector<double> samples;
  samples.reserve(iters);
  for (std::size_t i = 0; i < iters; ++i) {
    const auto t0 = clock::now();
    f();
    const auto t1 = clock::now();
    samples.push_back(std::chrono::duration<double, std::nano>(t1 - t0).count());
  }
  return samples;
}

// Same as measure_samples_ns, but returns statistics over the recorded calls.
template <class F>
Stats measure_ns(F&& f, std::size_t iters, std::size_t warmup = 100) {
  return compute_stats(measure_samples_ns(f, iters, warmup));
}

// Cost of reading the clock twice with nothing in between.
inline double clock_overhead_ns(std::size_t iters = 100000) {
  return measure_ns([] {}, iters, 1000).median_ns;
}

// Like measure_ns, but subtracts the median clock overhead from every sample
// (clamped at 0, since a duration can't be negative).
template <class F>
Stats measure_ns_adjusted(F&& f, std::size_t iters, std::size_t warmup = 100) {
  const double overhead = clock_overhead_ns();
  std::vector<double> samples = measure_samples_ns(f, iters, warmup);
  for (double& x : samples) x = std::max(0.0, x - overhead);
  return compute_stats(std::move(samples));
}

// For very short operations: each sample times `batch` back-to-back calls and
// divides by `batch`, so the clock overhead is spread over many calls.
// batch == 0 is treated as 1.
template <class F>
Stats measure_batched_ns(F&& f, std::size_t iters, std::size_t batch, std::size_t warmup = 100) {
  using clock = std::chrono::steady_clock;
  if (batch == 0) batch = 1;
  for (std::size_t i = 0; i < warmup; ++i) f();

  std::vector<double> samples;
  samples.reserve(iters);
  for (std::size_t i = 0; i < iters; ++i) {
    const auto t0 = clock::now();
    for (std::size_t b = 0; b < batch; ++b) f();
    const auto t1 = clock::now();
    samples.push_back(std::chrono::duration<double, std::nano>(t1 - t0).count() / static_cast<double>(batch));
  }
  return compute_stats(std::move(samples));
}

// Counts samples per decade: bucket 0 is [0, 10) ns, bucket 1 is [10, 100) ns,
// bucket 2 is [100 ns, 1 us), ...  The result is as long as the highest
// non-empty bucket + 1 (empty input gives an empty vector).
inline std::vector<std::size_t> histogram_log10(const std::vector<double>& samples) {
  std::vector<std::size_t> counts;
  for (double x : samples) {
    const std::size_t b = (x < 10.0) ? 0 : static_cast<std::size_t>(std::floor(std::log10(x)));
    if (b >= counts.size()) counts.resize(b + 1, 0);
    ++counts[b];
  }
  return counts;
}

// "1ns", "10ns", "100ns", "1us", ... for 10^e nanoseconds.
inline std::string pow10_ns_label(std::size_t e) {
  static const char* const units[] = {"ns", "us", "ms", "s"};
  const std::size_t u = std::min<std::size_t>(e / 3, 3);
  std::string label = "1";
  for (std::size_t i = 0; i < e - 3 * u; ++i) label += '0';
  return label + units[u];
}

// Prints one line per decade bucket with a bar scaled to the largest bucket,
// so the long tail is visible at a glance.
inline void print_histogram(const std::vector<double>& samples, std::FILE* out = stdout) {
  const std::vector<std::size_t> counts = histogram_log10(samples);
  if (counts.empty()) return;
  const std::size_t biggest = *std::max_element(counts.begin(), counts.end());
  constexpr std::size_t kBarWidth = 50;
  for (std::size_t b = 0; b < counts.size(); ++b) {
    const std::string range = pow10_ns_label(b) + " - " + pow10_ns_label(b + 1);
    std::size_t bar = counts[b] * kBarWidth / biggest;
    if (counts[b] > 0 && bar == 0) bar = 1;  // keep rare tail buckets visible
    std::fprintf(out, "%16s | %-*s %zu\n", range.c_str(), static_cast<int>(kBarWidth), std::string(bar, '#').c_str(),
                 counts[b]);
  }
}

}  // namespace ql
