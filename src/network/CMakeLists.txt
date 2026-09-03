add_library(network STATIC)
target_include_directories(network PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(network PRIVATE Qt6::Core Qt6::Network core)
