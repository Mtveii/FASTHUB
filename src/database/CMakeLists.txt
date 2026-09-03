add_library(database STATIC
    database_manager.cpp
    database_manager.h
)
target_include_directories(database PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# Public headers include QSqlDatabase/QString -> Sql must propagate to consumers.
target_link_libraries(database PUBLIC Qt6::Core Qt6::Sql)
target_link_libraries(database PRIVATE core)
