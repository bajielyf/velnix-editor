cmake_minimum_required(VERSION 3.16)

project(scintilla LANGUAGES C CXX)

if(NOT GTK3_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0 gmodule-no-export-2.0)
endif()

file(GLOB SCINTILLA_CORE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
)

file(GLOB SCINTILLA_GTK_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/gtk/*.cxx"
)

add_library(scintilla STATIC
    ${SCINTILLA_CORE_SOURCES}
    ${SCINTILLA_GTK_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/gtk/scintilla-marshal.c
)

target_compile_definitions(scintilla PUBLIC GTK)

# Scintilla exports both its own headers and GTK include directories so
# applications linking it can include <gtk/gtk.h>, <Scintilla.h>, and
# <ScintillaWidget.h> without repeating paths.
target_include_directories(scintilla
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/gtk
        ${GTK3_INCLUDE_DIRS}
)

target_link_libraries(scintilla
    PUBLIC
        ${GTK3_LIBRARIES}
)

target_compile_options(scintilla
    PRIVATE
        ${GTK3_CFLAGS_OTHER}
        -Wall
        -Wextra
)
