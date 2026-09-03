add_executable(search_tests fts_search_test.cpp)
target_link_libraries(search_tests PRIVATE Qt6::Test Qt6::Sql search database infrastructure)
add_test(NAME search_tests COMMAND search_tests)
