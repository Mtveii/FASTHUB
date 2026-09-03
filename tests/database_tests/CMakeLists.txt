add_executable(database_tests database_manager_test.cpp)
target_link_libraries(database_tests PRIVATE Qt6::Test Qt6::Sql database infrastructure)
add_test(NAME database_tests COMMAND database_tests)
