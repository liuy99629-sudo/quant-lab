# 阶段 0（第 1–2 周）：环境与计时工具

目标：一个能一键编译、跑测试、跑基准的仓库，以及一个你读得懂并能扩展的计时工具。

## 第 1 周：环境

- [ ] `./ql doctor` 全部通过（`perf` 只在 Linux 上需要）。
- [ ] `./ql test stage0` 全绿（7 个测试）。
- [ ] 读懂 `CMakeLists.txt`：`find_package` 和 `FetchContent` 的先后关系、`ql_add_test` 做了什么、`QL_ASAN` 如何给所有目标加上 `-fsanitize=address,undefined`。
- [ ] 故意写一个越界读写或空指针解引用的小程序，用 `./ql build` 编译后运行，看 ASan 的报告怎么读（看栈回溯里第一行属于你自己代码的位置）。
- [ ] 故意写一个有符号整数溢出，看 UBSan 的报告。
- [ ] 用 Release 模式运行 `./ql demo`，解释为什么 `push_back 1000 (reserve)` 比不 reserve 快。
- [ ] 用 `./ql perf ./build/release/stage0/timing_demo` 运行一次，找到 `cycles`、`instructions`、`cache-misses`、`branch-misses` 四行并写下它们的含义。

## 第 2 周：扩展计时工具

`common/include/ql/timing.hpp` 已经写好，你的任务是扩展它。每做一项就加一个测试。

1. **去掉时钟开销**：增加 `measure_ns_adjusted(f, iters)`，把 `clock_overhead_ns()` 的中位数从每个样本里减掉（结果不能小于 0）。
2. **批量测量**：对极短的操作（几十纳秒），单次计时会被时钟开销淹没。增加 `measure_batched_ns(f, iters, batch)`，一次计时里调用 `f` batch 次，再除以 batch。
3. **直方图输出**：增加 `print_histogram(samples)`，按对数分桶（1–10 ns、10–100 ns、100 ns–1 µs……）打印每个桶的数量，直观看到尾部延迟。
4. **p99.9**：在 `Stats` 里加 `p999_ns`，补测试。
5. **CPU 时间戳（选做）**：x86 上用 `__rdtsc()` 做一个更便宜的计时器，和 `steady_clock` 对比开销。写下为什么直接用 `rdtsc` 做跨核比较有风险。

## 验收标准

- 一条命令编译、测试、跑基准：`./ql test stage0 && ./ql bench`。
- 你能口头解释：为什么要 warmup、为什么中位数比平均值更适合描述典型延迟、p99 反映什么。
- `docs/progress.md` 里有第 1、2 周的三行总结。
