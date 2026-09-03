add_executable(sync_tests sync_service_test.cpp)
target_link_libraries(sync_tests PRIVATE Qt6::Test Qt6::Sql Qt6::Concurrent sync database core infrastructure)
add_test(NAME sync_tests COMMAND sync_tests)
