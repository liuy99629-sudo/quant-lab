#include <gtest/gtest.h>

#include <algorithm>
#include <random>
#include <sstream>

#include "ql/trade_stats.hpp"

using ql::Trade;

namespace {
Trade T(std::int64_t ts, const char* sym, double price, std::int64_t qty) { return Trade{ts, sym, price, qty}; }
}  // namespace

// ---- parse_line -------------------------------------------------------------

TEST(ParseLine, ValidLine) {
  auto t = ql::parse_line("1000,AAA,10.5,100");
  ASSERT_TRUE(t.has_value());
  EXPECT_EQ(t->ts_ns, 1000);
  EXPECT_EQ(t->symbol, "AAA");
  EXPECT_DOUBLE_EQ(t->price, 10.5);
  EXPECT_EQ(t->qty, 100);
}

TEST(ParseLine, IntegerPriceIsFine) {
  auto t = ql::parse_line("7,BHP,40,3");
  ASSERT_TRUE(t.has_value());
  EXPECT_DOUBLE_EQ(t->price, 40.0);
}

TEST(ParseLine, RejectsHeader) { EXPECT_FALSE(ql::parse_line("timestamp_ns,symbol,price,qty").has_value()); }

TEST(ParseLine, RejectsEmptyAndWrongFieldCount) {
  EXPECT_FALSE(ql::parse_line("").has_value());
  EXPECT_FALSE(ql::parse_line("1,AAA,1.0").has_value());
  EXPECT_FALSE(ql::parse_line("1,AAA,1.0,5,extra").has_value());
}

TEST(ParseLine, RejectsBadNumbers) {
  EXPECT_FALSE(ql::parse_line("abc,AAA,1.0,5").has_value());
  EXPECT_FALSE(ql::parse_line("1,AAA,xyz,5").has_value());
  EXPECT_FALSE(ql::parse_line("1,AAA,1.0,5.5").has_value());   // qty must be an integer
  EXPECT_FALSE(ql::parse_line("1,AAA,1.0,12abc").has_value()); // trailing garbage
}

TEST(ParseLine, RejectsNonPositiveValuesAndEmptySymbol) {
  EXPECT_FALSE(ql::parse_line("1,AAA,0,5").has_value());
  EXPECT_FALSE(ql::parse_line("1,AAA,-1.5,5").has_value());
  EXPECT_FALSE(ql::parse_line("1,AAA,1.5,0").has_value());
  EXPECT_FALSE(ql::parse_line("1,AAA,1.5,-3").has_value());
  EXPECT_FALSE(ql::parse_line("1,,1.5,3").has_value());
}

// ---- read_trades ------------------------------------------------------------

TEST(ReadTrades, SkipsHeaderAndBadLinesKeepsOrder) {
  std::istringstream in(
      "timestamp_ns,symbol,price,qty\n"
      "2,BBB,5.0,10\n"
      "garbage line\n"
      "1,AAA,10.0,100\n"
      "\n"
      "3,AAA,11.0,50\n");
  auto trades = ql::read_trades(in);
  ASSERT_EQ(trades.size(), 3u);
  EXPECT_EQ(trades[0].symbol, "BBB");
  EXPECT_EQ(trades[1].symbol, "AAA");
  EXPECT_EQ(trades[2].ts_ns, 3);
}

TEST(ReadTrades, ToleratesWindowsLineEndings) {
  std::istringstream in("timestamp_ns,symbol,price,qty\r\n1,AAA,10.0,100\r\n2,AAA,11.0,50\r\n");
  auto trades = ql::read_trades(in);
  ASSERT_EQ(trades.size(), 2u);
  EXPECT_EQ(trades[1].qty, 50);
}

TEST(ReadTrades, EmptyInput) {
  std::istringstream in("");
  EXPECT_TRUE(ql::read_trades(in).empty());
}

// ---- vwap -------------------------------------------------------------------

TEST(Vwap, WeightsByQuantity) {
  std::vector<Trade> trades = {T(1, "AAA", 10, 100), T(2, "AAA", 20, 300), T(3, "BBB", 5, 10)};
  auto v = ql::vwap_by_symbol(trades);
  ASSERT_EQ(v.size(), 2u);
  EXPECT_NEAR(v["AAA"], 17.5, 1e-9);  // (10*100 + 20*300) / 400
  EXPECT_NEAR(v["BBB"], 5.0, 1e-9);
}

TEST(Vwap, EmptyInputGivesEmptyMap) { EXPECT_TRUE(ql::vwap_by_symbol({}).empty()); }

// ---- max drawdown -----------------------------------------------------------

TEST(MaxDrawdown, ClassicSeries) {
  // prices in time order: 100 120 90 110 60 80  -> peak 120, trough 60 => 50%
  std::vector<Trade> trades;
  const double prices[] = {100, 120, 90, 110, 60, 80};
  for (int i = 0; i < 6; ++i) trades.push_back(T(i + 1, "AAA", prices[i], 1));
  auto d = ql::max_drawdown_by_symbol(trades);
  EXPECT_NEAR(d["AAA"], 0.5, 1e-12);
}

TEST(MaxDrawdown, InputIsSortedByTimestampFirst) {
  std::vector<Trade> trades;
  const double prices[] = {100, 120, 90, 110, 60, 80};
  for (int i = 0; i < 6; ++i) trades.push_back(T(i + 1, "AAA", prices[i], 1));
  std::mt19937 rng(7);
  std::shuffle(trades.begin(), trades.end(), rng);
  auto d = ql::max_drawdown_by_symbol(trades);
  EXPECT_NEAR(d["AAA"], 0.5, 1e-12);
}

TEST(MaxDrawdown, SymbolsAreIndependent) {
  std::vector<Trade> trades = {T(1, "AAA", 100, 1), T(2, "BBB", 10, 1), T(3, "AAA", 50, 1), T(4, "BBB", 12, 1)};
  auto d = ql::max_drawdown_by_symbol(trades);
  EXPECT_NEAR(d["AAA"], 0.5, 1e-12);
  EXPECT_NEAR(d["BBB"], 0.0, 1e-12);
}

TEST(MaxDrawdown, MonotonicRiseOrSingleTradeIsZero) {
  std::vector<Trade> up = {T(1, "AAA", 1, 1), T(2, "AAA", 2, 1), T(3, "AAA", 3, 1)};
  EXPECT_NEAR(ql::max_drawdown_by_symbol(up)["AAA"], 0.0, 1e-12);
  std::vector<Trade> one = {T(1, "AAA", 5, 1)};
  EXPECT_NEAR(ql::max_drawdown_by_symbol(one)["AAA"], 0.0, 1e-12);
}

TEST(MaxDrawdown, EqualTimestampsKeepInputOrder) {
  // Same ts: input order is the order. 100 then 50 => 50% drawdown (not 0).
  std::vector<Trade> trades = {T(5, "AAA", 100, 1), T(5, "AAA", 50, 1)};
  EXPECT_NEAR(ql::max_drawdown_by_symbol(trades)["AAA"], 0.5, 1e-12);
}
