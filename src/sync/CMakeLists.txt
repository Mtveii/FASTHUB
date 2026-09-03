add_library(sync STATIC
    sync_service.cpp
    sync_service.h
)
target_include_directories(sync PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(sync PRIVATE Qt6::Core Qt6::Concurrent core database)
