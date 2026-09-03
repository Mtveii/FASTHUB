add_library(workspace STATIC
    local_file_provider.cpp
    local_file_provider.h
    file_locator.cpp
    file_locator.h
    parsers/link_parser.cpp
    parsers/link_parser.h
)
target_include_directories(workspace PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(workspace PRIVATE Qt6::Core Qt6::Gui core database)
