# quant-lab：做市商后台技术的动手训练仓库

这是一个"边做边学"的仓库。每个阶段只有一个主项目，配一组会失败的测试（红灯），你的任务是让它们全部变绿，并且在 ASan/UBSan 下零报错。

当前包含：

- 阶段 0：环境和计时工具（已完成，测试全绿，你只需要读懂并扩展它）
- 阶段 1：把 C++ 写顺手（项目 A、项目 B，测试是红的，等你来写）

后面的阶段（行情接入、订单簿、无锁队列、定价与风控）会沿用同样的结构，一个阶段一个目录，等你完成这里再加。

## 快速开始

```bash
./ql doctor        # 检查 g++ / cmake / git / python3 / perf 是否齐全
./ql test stage0   # 阶段 0：应该全绿（7 个测试）
./ql test stage1a  # 项目 A：现在全红，你要让它变绿
./ql test stage1b  # 项目 B：现在全红，你要让它变绿
./ql demo          # Release 模式跑计时工具示例
./ql bench         # Release 模式跑 Google Benchmark 示例
```

第一次 `./ql test` 会配置并编译整个仓库。GoogleTest 和 Google Benchmark 如果系统里已经装了就直接用，没装就在配置时从 GitHub 下载（需要联网）。

安装依赖的参考命令：

- Ubuntu / Debian / WSL：`sudo apt install build-essential cmake git python3 libgtest-dev libbenchmark-dev linux-tools-generic`
- macOS：`brew install cmake googletest google-benchmark`（macOS 没有 `perf`，用 Instruments 代替）

## 目录

```
common/include/ql/     todo.hpp（占位标记）、timing.hpp（阶段 0 计时工具）
stage0/                计时工具的测试、demo 和 Google Benchmark 示例
stage1/trade_stats/    项目 A：读成交 CSV，算 VWAP 和最大回撤
stage1/containers/     项目 B：UniquePtr、Vector、ObjectPool
docs/                  每个阶段的逐周任务、刷题清单、进度表
ql                     统一入口脚本
```

## 怎么用这个仓库学习

1. 打开一个红灯的测试文件，先读测试，再读头文件里的注释。测试就是规格书。
2. 在对应头文件或 `.cpp` 里把 `todo(...)` 换成真正的实现，一次只让一个测试变绿：
   `ctest --test-dir build/debug-asan -R UniquePtrTest.DefaultIsNull --output-on-failure`
3. 全部变绿后，再看 sanitizer 有没有报告（`./ql test` 默认就开着）。
4. 每周日在 `docs/progress.md` 里写三行：学到了什么、卡在哪里、下周计划。

红灯阶段你会看到很多 LeakSanitizer 的泄漏报告，这是正常的：占位的析构函数还没释放内存。实现完成后它们会自然消失。

## 规则

- 不要改测试来让它通过。如果你认为测试本身有问题，把原因写进 `docs/progress.md`，再决定改不改。
- 先写对，再写快。每个阶段结束前才允许做性能优化，并且要用 `ql::measure_ns` 或 Google Benchmark 拿数据。
- 卡住超过 45 分钟就去看头文件里的 Hints 或 `docs/` 里对应周的提示，再卡住就把现象发给我。
