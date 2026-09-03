add_library(infrastructure STATIC
    secure_storage.cpp
    secure_storage.h
    logger.cpp
    logger.h
    performance_manager.cpp
    performance_manager.h
    crash_handler.cpp
    crash_handler.h
)
target_include_directories(infrastructure PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(infrastructure PRIVATE Qt6::Core core)
if(WIN32)
    target_link_libraries(infrastructure PRIVATE crypt32 dbghelp)
endif()
