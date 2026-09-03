add_library(plugins STATIC
    plugin_manager.cpp
    plugin_manager.h
)
target_include_directories(plugins PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(plugins PRIVATE Qt6::Core core infrastructure)
