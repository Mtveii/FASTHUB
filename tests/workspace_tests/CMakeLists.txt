add_executable(workspace_tests local_file_provider_test.cpp)
target_link_libraries(workspace_tests PRIVATE Qt6::Test workspace infrastructure)
add_test(NAME workspace_tests COMMAND workspace_tests)

add_executable(file_locator_tests file_locator_test.cpp)
target_link_libraries(file_locator_tests PRIVATE Qt6::Test workspace infrastructure)
add_test(NAME file_locator_tests COMMAND file_locator_tests)

add_executable(link_parser_test link_parser_test.cpp)
target_link_libraries(link_parser_test PRIVATE Qt6::Test workspace infrastructure)
add_test(NAME link_parser_test COMMAND link_parser_test)

add_executable(ui_layout_tests ui_layout_test.cpp)
target_link_libraries(ui_layout_tests PRIVATE Qt6::Test Qt6::Widgets ui workspace infrastructure)
add_test(NAME ui_layout_tests COMMAND ui_layout_tests)
