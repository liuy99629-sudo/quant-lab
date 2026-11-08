#include "ql/trade_stats.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <istream>
#include <system_error>
#include <utility>

#include "ql/todo.hpp"

namespace ql {

// Hints (read after you have tried for a while):
//   * std::from_chars works on a (const char*, const char*) range, does no
//     allocation, and tells you via its result whether the WHOLE field parsed.
//   * Split with std::string_view::find(',') and substr; avoid std::string and
//     std::stringstream per line, they allocate.
//   * std::getline(in, line) reuses the same std::string buffer if you declare
//     it outside the loop.

namespace {

// Parses the WHOLE field as a number; returns false on empty input,
// non-numeric text, trailing garbage ("12abc") or overflow.
template <typename T>
bool parse_number(std::string_view field, T& out) {
  const char* first = field.data();
  const char* last = field.data() + field.size();
  auto [ptr, ec] = std::from_chars(first, last, out);
  return ec == std::errc{} && ptr == last;
}

}  // namespace

std::optional<Trade> parse_line(std::string_view line) {
  // 1. Split into exactly 4 fields on ','.
  std::string_view fields[4];
  std::size_t count = 0;
  while (true) {
    const std::size_t comma = line.find(',');
    if (count == 4) return std::nullopt;  // a 5th field exists -> malformed
    fields[count++] = line.substr(0, comma);
    if (comma == std::string_view::npos) break;
    line.remove_prefix(comma + 1);
  }
  if (count != 4) return std::nullopt;

  // 2. Convert and validate each field.
  Trade t;
  if (!parse_number(fields[0], t.ts_ns)) return std::nullopt;
  if (fields[1].empty()) return std::nullopt;
  if (!parse_number(fields[2], t.price) || !std::isfinite(t.price) || t.price <= 0) return std::nullopt;
  if (!parse_number(fields[3], t.qty) || t.qty <= 0) return std::nullopt;
  t.symbol = std::string(fields[1]);  // allocate only once the line is known to be good
  return t;
}

std::vector<Trade> read_trades(std::istream& in) {
  std::vector<Trade> trades;
  std::string line;  // reused across iterations, so getline can keep its buffer
  while (std::getline(in, line)) {
    std::string_view view = line;
    if (!view.empty() && view.back() == '\r') view.remove_suffix(1);
    if (auto t = parse_line(view)) trades.push_back(std::move(*t));
  }
  return trades;
}

std::map<std::string, double> vwap_by_symbol(const std::vector<Trade>& trades) {
  (void)trades;
  todo("vwap_by_symbol");
}

std::map<std::string, double> max_drawdown_by_symbol(const std::vector<Trade>& trades) {
  (void)trades;
  todo("max_drawdown_by_symbol");
}

}  // namespace ql
