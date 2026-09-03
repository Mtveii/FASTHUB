add_library(md4c STATIC
    md4c.c
    md4c.h
    md4c-html.c
    md4c-html.h
    entity.c
    entity.h
)
target_include_directories(md4c PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
