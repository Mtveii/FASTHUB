add_library(qrcodegen STATIC
    qrcodegen.cpp
    qrcodegen.hpp
)
target_include_directories(qrcodegen PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
