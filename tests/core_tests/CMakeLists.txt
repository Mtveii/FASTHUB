add_executable(core_tests id_utils_test.cpp)
target_link_libraries(core_tests PRIVATE Qt6::Test core)
add_test(NAME core_tests COMMAND core_tests)
