# One-shot brand asset generator (лого.png + fasthub.ico).
add_executable(logogen logogen.cpp)
target_link_libraries(logogen PRIVATE Qt6::Core Qt6::Gui Qt6::Svg ui)
set_target_properties(logogen PROPERTIES WIN32_EXECUTABLE OFF)

# One-shot DPAPI secret writer: secretsaver <key> <value>.
add_executable(secretsaver secretsaver.cpp)
target_link_libraries(secretsaver PRIVATE Qt6::Core infrastructure)
