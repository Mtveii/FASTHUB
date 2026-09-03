add_executable(canvas_tests canvas_document_test.cpp)
target_link_libraries(canvas_tests PRIVATE Qt6::Test core)
add_test(NAME canvas_tests COMMAND canvas_tests)
