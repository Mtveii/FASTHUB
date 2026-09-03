add_library(search STATIC
    search_provider.cpp
    search_provider.h
)
target_include_directories(search PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(search PRIVATE Qt6::Core core database)
