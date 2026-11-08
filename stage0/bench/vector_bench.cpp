// Google Benchmark example.  Run in Release mode:  ./ql bench
#include <benchmark/benchmark.h>

#include <vector>

static void BM_PushBack(benchmark::State& state) {
  const auto n = static_cast<int>(state.range(0));
  for (auto _ : state) {
    std::vector<int> v;
    for (int i = 0; i < n; ++i) v.push_back(i);
    benchmark::DoNotOptimize(v.data());
  }
}
BENCHMARK(BM_PushBack)->Arg(1000)->Arg(100000);

static void BM_PushBackReserved(benchmark::State& state) {
  const auto n = static_cast<int>(state.range(0));
  for (auto _ : state) {
    std::vector<int> v;
    v.reserve(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) v.push_back(i);
    benchmark::DoNotOptimize(v.data());
  }
}
BENCHMARK(BM_PushBackReserved)->Arg(1000)->Arg(100000);
