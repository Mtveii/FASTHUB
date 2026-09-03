add_executable(theme_tests theme_manager_test.cpp)
target_link_libraries(theme_tests PRIVATE Qt6::Test Qt6::Widgets Qt6::Sql ui database core)
add_test(NAME theme_tests COMMAND theme_tests)
