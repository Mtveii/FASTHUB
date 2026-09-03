add_executable(markdown_tests markdown_renderer_test.cpp)
target_link_libraries(markdown_tests PRIVATE Qt6::Test Qt6::Gui Qt6::Widgets ui md4c)
add_test(NAME markdown_tests COMMAND markdown_tests)
