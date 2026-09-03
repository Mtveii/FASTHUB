add_library(ai_provider STATIC
    ai_provider.cpp
    ai_provider.h
)
target_include_directories(ai_provider PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ai_provider PRIVATE Qt6::Core Qt6::Network core infrastructure)
