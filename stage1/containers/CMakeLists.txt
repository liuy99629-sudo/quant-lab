add_library(containers INTERFACE)
target_include_directories(containers INTERFACE include)
target_link_libraries(containers INTERFACE ql_common)

ql_add_test(unique_ptr_test stage1b tests/unique_ptr_test.cpp)
target_link_libraries(unique_ptr_test PRIVATE containers)

ql_add_test(vector_test stage1b tests/vector_test.cpp)
target_link_libraries(vector_test PRIVATE containers)

# pool_test replaces global operator new to prove the pool never touches the heap,
# so it lives in its own executable.
ql_add_test(pool_test stage1b tests/pool_test.cpp)
target_link_libraries(pool_test PRIVATE containers)
