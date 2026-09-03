add_executable(qr_tests qr_login_dialog_test.cpp)
target_link_libraries(qr_tests PRIVATE Qt6::Test Qt6::Gui Qt6::Widgets ui qrcodegen)
add_test(NAME qr_tests COMMAND qr_tests)
