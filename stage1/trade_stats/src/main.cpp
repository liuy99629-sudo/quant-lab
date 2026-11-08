// Command line front end for project A (already written for you).
//   ./build/release/stage1/trade_stats/trade_stats stage1/trade_stats/data/sample_trades.csv
#include <cstdio>
#include <fstream>
#include <iostream>

#include "ql/trade_stats.hpp"

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "usage: trade_stats <trades.csv>\n";
    return 2;
  }
  std::ifstream in(argv[1]);
  if (!in) {
    std::cerr << "cannot open " << argv[1] << "\n";
    return 1;
  }

  const auto trades = ql::read_trades(in);
  const auto vwap = ql::vwap_by_symbol(trades);
  const auto dd = ql::max_drawdown_by_symbol(trades);

  std::printf("%-8s %12s %10s\n", "symbol", "vwap", "max_dd");
  for (const auto& [symbol, v] : vwap) {
    const auto it = dd.find(symbol);
    const double d = (it == dd.end()) ? 0.0 : it->second;
    std::printf("%-8s %12.4f %9.2f%%\n", symbol.c_str(), v, d * 100.0);
  }
  std::printf("trades: %zu\n", trades.size());
  return 0;
}
