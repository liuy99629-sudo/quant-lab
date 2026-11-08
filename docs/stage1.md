# 阶段 1（第 3–10 周）：把 C++ 写顺手

目标：能独立写出资源安全的 C++ 代码；理解 RAII、移动语义和对象生命周期；知道什么时候会分配内存。

两个项目：

- 项目 A（`stage1/trade_stats`）：读成交 CSV，算 VWAP 和最大回撤。练文件读写、`vector`、`string_view`、标准算法。
- 项目 B（`stage1/containers`）：自己实现 `UniquePtr`、`Vector`、`ObjectPool`。练 RAII、移动语义、模板基础。

配套：Exercism C++ 路线前 20 题 + 每周 2–3 道 LeetCode（清单见 `docs/leetcode.md`）。

每周投入按 5–8 小时算：项目 4–5 小时，刷题约 2 小时。

## 第 3 周：项目 A，解析

- 读 `trade_stats.hpp` 里的规则和 `trade_stats_test.cpp` 里的 `ParseLine`、`ReadTrades` 测试。
- 实现 `parse_line` 和 `read_trades`。
- 目标：`./ql test stage1a` 里这两组测试全绿。
- 提示：`std::from_chars` 不分配内存，并且能告诉你整个字段是否解析成功；检查 `ptr` 是否等于字段末尾。
- 本周 C++ 知识点：`string_view`、`optional`、`from_chars`、`getline`。

## 第 4 周：项目 A，VWAP 和命令行

- 实现 `vwap_by_symbol`。
- 用 `gen_trades.py` 生成数据：`python3 stage1/trade_stats/data/gen_trades.py --rows 100000 --out /tmp/t.csv`，再用 `./build/release/stage1/trade_stats/trade_stats /tmp/t.csv` 跑。
- 手算一个小例子，对照输出。
- 本周知识点：`std::map` 与 `std::unordered_map` 的区别；浮点数累加的精度问题（试试把数量改成很大的数）。

## 第 5 周：项目 A，最大回撤与性能

- 实现 `max_drawdown_by_symbol`（注意输入不一定有序，相同时间戳保持输入顺序，用 `std::stable_sort`）。
- 用 Release 构建在 100 万行数据上跑，用 `ql::measure_ns` 或 `time` 记录耗时。
- 做三个优化并每次记录前后数据：`reserve` 预留空间、避免每行构造临时 `std::string`、把 `std::map` 换成 `unordered_map`。
- 用 `./ql perf` 看 `cache-misses`。
- 在 `docs/progress.md` 记录优化表：改动、耗时、你对结果的解释。

## 第 6 周：项目 B1，UniquePtr

- 实现 `stage1/containers/include/ql/unique_ptr.hpp`。
- 目标：`./ql test stage1b` 里 `UniquePtrTest.*` 和 `UniquePtrTraits.*` 全绿。
- 重点想清楚：移动赋值先释放旧对象还是先接管新对象；自移动赋值为什么要保护。
- 加分：加 `noexcept`，加自定义 Deleter 模板参数，支持 `T[]`。

## 第 7 周：项目 B2，Vector（上）

- 实现存储、`size/capacity`、`push_back`、`emplace_back`、`reserve`、`pop_back`、`clear`、析构。
- 只用 `::operator new` 申请原始内存，用 placement new 构造，用 `~T()` 销毁。
- 目标：`VectorTest` 中除拷贝和移动外的测试全绿。
- 重点：增长时用 `std::move_if_noexcept` 搬运元素，测试会检查 noexcept 移动的类型不被拷贝。

## 第 8 周：项目 B2，Vector（下）

- 实现拷贝构造、拷贝赋值、移动构造、移动赋值（Rule of Five）。
- 目标：`VectorTest.*` 全部绿，并且 ASan 无报告。
- 思考题：`push_back` 时如果元素构造抛异常，容器处于什么状态？这叫哪种异常安全保证？
- 加分：实现 `insert`/`erase`，写一个有时会抛异常的元素类型来测试。

## 第 9 周：项目 B3，ObjectPool

- 实现 `object_pool.hpp`。
- 目标：`PoolTest.*` 全绿，尤其是 `NeverTouchesTheHeapOnTheHotPath`。
- 重点：对齐（`alignas(T)`）、空闲链表（LIFO）、用 `std::uintptr_t` 判断指针是否属于池子、重复释放返回 `false`。
- 思考题：为什么低延迟系统要用对象池而不是每次 `new`？

## 第 10 周：收尾与对比

1. 给 `stage1/containers` 增加 `bench/`，用 Google Benchmark 比较：
   - `ql::Vector<int>` 和 `std::vector<int>` 的 `push_back`；
   - `ObjectPool` 的 `acquire/release` 和 `new/delete`；
   - 在 Release 下报告 p50 和 p99（Benchmark 默认给平均值，用 `--benchmark_repetitions` 并看统计）。
2. 在 `./ql tsan` 和 `./ql test` 下确认两种 sanitizer 都干净。
3. 补一份 `docs/stage1-review.md`：每个项目最难的 bug、你学到的三件事、还没搞懂的两个问题。

## 阶段 1 验收标准

- `./ql test stage1a` 与 `./ql test stage1b` 全部通过，且无 sanitizer 报告。
- 项目 A 在 100 万行数据上的耗时有记录，并有优化前后对比表。
- 你能不看代码，在白板上讲清楚：Vector 的扩容过程、unique_ptr 的移动语义、ObjectPool 的空闲链表。
- 完成 LeetCode 清单中本阶段的 24 题（至少 18 题）。
