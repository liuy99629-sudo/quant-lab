add_library(trade_stats_lib src/trade_stats.cpp)
target_include_directories(trade_stats_lib PUBLIC include)
target_link_libraries(trade_stats_lib PUBLIC ql_common)

add_executable(trade_stats src/main.cpp)
target_link_libraries(trade_stats PRIVATE trade_stats_lib)

ql_add_test(trade_stats_test stage1a tests/trade_stats_test.cpp)
target_link_libraries(trade_stats_test PRIVATE trade_stats_lib)
