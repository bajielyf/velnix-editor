cmake_minimum_required(VERSION 3.16)

project(lexilla LANGUAGES CXX)

if(NOT TARGET scintilla)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../scintilla ${CMAKE_CURRENT_BINARY_DIR}/scintilla)
endif()

file(GLOB LEXILLA_LEXLIB_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/lexlib/*.cxx"
)

file(GLOB LEXILLA_LEXER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/lexers/*.cxx"
)

if(NOT WIN32)
    list(REMOVE_ITEM LEXILLA_LEXER_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/lexers/LexUser.cxx
    )
endif()

add_library(lexilla STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Lexilla.cxx
    ${LEXILLA_LEXLIB_SOURCES}
    ${LEXILLA_LEXER_SOURCES}
)

target_include_directories(lexilla
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/lexlib
        ${CMAKE_CURRENT_SOURCE_DIR}/lexers
)

target_link_libraries(lexilla
    PUBLIC
        scintilla
)

target_compile_options(lexilla
    PRIVATE
        -Wall
        -Wextra
)
