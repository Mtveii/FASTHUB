cmake_minimum_required(VERSION 3.22)
project(FastHub VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Reject 32-bit (x86) builds as per VISION.md
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "FastHub does not support 32-bit (x86) builds. Target x64 or ARM64.")
endif()

# Find Qt 6
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Svg Sql Network Concurrent WebEngineWidgets Test)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Cross-module headers are included as "core/...", "infrastructure/...", etc.
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

add_subdirectory(third_party/qrcodegen)
add_subdirectory(third_party/md4c)
add_subdirectory(tools)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

add_subdirectory(src/core)
add_subdirectory(src/database)
add_subdirectory(src/network)
add_subdirectory(src/infrastructure)
add_subdirectory(src/workspace)
add_subdirectory(src/providers/telegram)
add_subdirectory(src/sync)
add_subdirectory(src/search)
add_subdirectory(src/providers/ai)
add_subdirectory(src/providers/steam)
add_subdirectory(src/providers/discord)
add_subdirectory(src/plugins)
add_subdirectory(src/ui)
add_subdirectory(src/app)
