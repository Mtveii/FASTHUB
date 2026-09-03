add_library(steam_provider STATIC
    steam_provider.cpp
    steam_provider.h
    steam_presence.cpp
    steam_presence.h
)
target_include_directories(steam_provider PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# Network is used in the public header surface (QNetworkAccessManager member).
target_link_libraries(steam_provider PUBLIC Qt6::Core Qt6::Network)
target_link_libraries(steam_provider PRIVATE core database infrastructure)
