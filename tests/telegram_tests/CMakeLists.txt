add_executable(telegram_tests tg_embed_tests.cpp)
target_link_libraries(telegram_tests PRIVATE Qt6::Test Qt6::Widgets ui core infrastructure)
set_property(TARGET telegram_tests PROPERTY WIN32_EXECUTABLE FALSE)
add_test(NAME telegram_tests COMMAND telegram_tests)
