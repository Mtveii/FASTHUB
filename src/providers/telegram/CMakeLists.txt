add_library(telegram_provider STATIC
    telegram_provider.cpp
    telegram_provider.h
    td_client.cpp
    td_client.h
)
target_include_directories(telegram_provider PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(telegram_provider PRIVATE Qt6::Core core database infrastructure)
