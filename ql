#!/usr/bin/env bash
# One entry point for the whole lab.  Run ./ql help
set -euo pipefail
cd "$(dirname "$0")"

JOBS="$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"

need() { command -v "$1" >/dev/null 2>&1; }

configure_and_build() {
  local preset="$1"
  cmake --preset "$preset" >/dev/null
  cmake --build --preset "$preset" -j "$JOBS"
}

usage() {
  cat <<'EOF'
Usage: ./ql <command> [args]

  doctor              check that the toolchain is installed
  build [preset]      configure + build (default preset: debug-asan)
  test  [label]       build with ASan/UBSan and run tests
                      labels: stage0 | stage1a | stage1b   (default: all)
  bench               build Release and run the Google Benchmark demo
  demo                build Release and run the timing demo
  tsan [label]        build with ThreadSanitizer and run tests (stage 4 onwards)
  perf <cmd...>       run a command under `perf stat` (Linux only)
  clean               delete build/

Presets: debug-asan | release | tsan
EOF
}

cmd="${1:-help}"
shift || true

case "$cmd" in
  doctor)
    ok=1
    for t in g++ cmake git make; do
      if need "$t"; then printf '  [ok]   %s\n' "$t"; else printf '  [MISS] %s\n' "$t"; ok=0; fi
    done
    need clang++ && echo "  [ok]   clang++ (optional)" || echo "  [skip] clang++ (optional)"
    need ninja   && echo "  [ok]   ninja (optional)"   || echo "  [skip] ninja (optional)"
    need perf    && echo "  [ok]   perf"              || echo "  [skip] perf (Linux only; install linux-tools for your kernel)"
    need python3 && echo "  [ok]   python3 (for data generator)" || echo "  [MISS] python3"
    if need g++; then echo "  g++ version: $(g++ --version | head -1)"; fi
    if need cmake; then echo "  cmake version: $(cmake --version | head -1)"; fi
    [ "$ok" = 1 ] && echo "doctor: toolchain looks good" || { echo "doctor: install the missing tools above"; exit 1; }
    ;;
  build)
    configure_and_build "${1:-debug-asan}"
    ;;
  test)
    configure_and_build debug-asan
    if [ -n "${1:-}" ]; then
      ctest --test-dir build/debug-asan --output-on-failure -L "$1"
    else
      ctest --test-dir build/debug-asan --output-on-failure
    fi
    ;;
  tsan)
    configure_and_build tsan
    if [ -n "${1:-}" ]; then
      ctest --test-dir build/tsan --output-on-failure -L "$1"
    else
      ctest --test-dir build/tsan --output-on-failure
    fi
    ;;
  bench)
    configure_and_build release
    ./build/release/stage0/vector_bench
    ;;
  demo)
    configure_and_build release
    ./build/release/stage0/timing_demo
    ;;
  perf)
    if ! need perf; then echo "perf not found (Linux only)."; exit 1; fi
    [ $# -ge 1 ] || { echo "usage: ./ql perf <command...>"; exit 1; }
    perf stat -d "$@"
    ;;
  clean)
    rm -rf build
    echo "removed build/"
    ;;
  help|-h|--help|*)
    usage
    ;;
esac
