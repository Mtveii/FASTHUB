add_executable(steam_tests steam_presence_test.cpp)
target_link_libraries(steam_tests PRIVATE Qt6::Test Qt6::Network steam_provider core infrastructure)
add_test(NAME steam_tests COMMAND steam_tests)
