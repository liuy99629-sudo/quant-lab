#pragma once
// Project A: read a CSV of trades and compute VWAP and max drawdown per symbol.
//
// CSV format (first line is a header and must be skipped):
//     timestamp_ns,symbol,price,qty
//     1000,AAA,10.50,100
//
// Rules the tests check:
//   * timestamp_ns : integer
//   * symbol       : non-empty text, no spaces trimming (the text between commas, as is)
//   * price        : floating point, must be > 0
//   * qty          : integer, must be > 0
//   * exactly 4 fields; anything else is malformed -> parse_line returns nullopt

#include <cstdint>
#include <iosfwd>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ql {

struct Trade {
  std::int64_t ts_ns{};
  std::string symbol;
  double price{};
  std::int64_t qty{};
};

// Parses one CSV line (no trailing newline).  Returns nullopt if malformed.
// The header line is "malformed" too (its timestamp is not a number), which
// is how read_trades gets rid of it.
std::optional<Trade> parse_line(std::string_view line);

// Reads every line from `in`, keeps the well-formed trades in file order,
// silently skips the rest.  Tolerate a trailing '\r' (Windows line endings).
std::vector<Trade> read_trades(std::istream& in);

// Volume weighted average price per symbol:  sum(price*qty) / sum(qty)
std::map<std::string, double> vwap_by_symbol(const std::vector<Trade>& trades);

// Maximum drawdown per symbol as a fraction in [0, 1].
// Take that symbol's trade prices ordered by ts_ns (the input may NOT be
// sorted; trades with equal ts_ns keep their input order).  Walk the series
// keeping the highest price so far (the peak); the drawdown at each point is
// (peak - price) / peak.  Return the largest value seen.
std::map<std::string, double> max_drawdown_by_symbol(const std::vector<Trade>& trades);

}  // namespace ql
