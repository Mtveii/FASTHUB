add_library(discord_provider STATIC
    discord_provider.cpp
    discord_provider.h
    discord_oauth.cpp
    discord_oauth.h
)
target_include_directories(discord_provider PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# Public header uses QTcpServer -> Network must propagate to consumers.
target_link_libraries(discord_provider PUBLIC Qt6::Core Qt6::Network)
target_link_libraries(discord_provider PRIVATE Qt6::Gui core database infrastructure)
