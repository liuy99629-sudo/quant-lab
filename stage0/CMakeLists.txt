ql_add_test(timing_test stage0 tests/timing_test.cpp)

add_executable(timing_demo apps/timing_demo.cpp)
target_link_libraries(timing_demo PRIVATE ql_common)

add_executable(vector_bench bench/vector_bench.cpp)
target_link_libraries(vector_bench PRIVATE ql_common benchmark::benchmark_main)
