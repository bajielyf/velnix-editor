cmake_minimum_required(VERSION 3.16)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(VelnixEditor VERSION 0.1.3 LANGUAGES CXX C)
else()
    project(VelnixEditor LANGUAGES CXX C)
endif()

include(GNUInstallDirs)

set(VELNIX_EDITOR_VERSION "${CMAKE_PROJECT_VERSION}" CACHE STRING "Velnix Editor application version")
set(VELNIX_HOMEPAGE "https://bajielyf.github.io/velnix-editor" CACHE STRING "Velnix Editor homepage URL")

# When built standalone, pull in the editor libraries from sibling directories.
if(NOT TARGET scintilla)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../scintilla ${CMAKE_CURRENT_BINARY_DIR}/scintilla)
endif()

if(NOT TARGET lexilla)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lexilla ${CMAKE_CURRENT_BINARY_DIR}/lexilla)
endif()

# Add executable targets
add_executable(velnix_editor
    src/app/main.cpp
    src/core/EditorTypes.cpp
    src/core/Logger.cpp
    src/ui/EditorWindow.cpp
    src/ui/EventHandler.cpp
    src/ui/Localization.cpp
    src/ui/WindowInitializer.cpp
    src/editor/DocumentManager.cpp
    src/editor/DocumentIO.cpp
    src/editor/SaveNormalization.cpp
    src/editor/DocumentState.cpp
    src/editor/DocumentOperations.cpp
    src/editor/DocumentUI.cpp
    src/editor/DocumentRefresh.cpp
    src/editor/DocumentPrompt.cpp
    src/editor/SearchManager.cpp
    src/editor/MacroManager.cpp
    src/editor/SyntaxHighlighter.cpp
    src/config/SettingsManager.cpp
    src/config/CoreSettingsManager.cpp
    src/config/ConfigPaths.cpp
    src/config/ConfigStore.cpp
    src/config/ConfigSchema.cpp
    src/config/ShortcutManager.cpp
    src/config/RecentFilesManager.cpp
)

set_target_properties(velnix_editor
    PROPERTIES
        OUTPUT_NAME "velnix-editor"
)

# VelnixEditor only declares its own source include directory.
# Third-party include paths such as GTK/Scintilla/Lexilla are inherited from
# the linked targets via their PUBLIC usage requirements.
target_include_directories(velnix_editor
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(velnix_editor
    PRIVATE
        VELNIX_EDITOR_VERSION="${VELNIX_EDITOR_VERSION}"
        VELNIX_HOMEPAGE="${VELNIX_HOMEPAGE}"
        VELNIX_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
        VELNIX_ASSET_DIR="${CMAKE_INSTALL_FULL_DATADIR}/velnix-editor/assets"
)

target_link_libraries(velnix_editor
    PRIVATE
        scintilla
        lexilla
)

add_executable(test_gtk src/test_gtk.cpp)

target_include_directories(test_gtk
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(test_gtk
    PRIVATE
        scintilla
)

# Document State Unit Tests
add_executable(test_document_state
    src/editor/DocumentState.cpp
    src/test/DocumentStateTest.cpp
)

target_include_directories(test_document_state
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${GTK3_INCLUDE_DIRS}
)

target_link_libraries(test_document_state
    PRIVATE
        scintilla
        ${GTK3_LIBRARIES}
)

target_compile_options(test_document_state 
    PRIVATE 
        -Wall -Wextra
        ${GTK3_CFLAGS_OTHER}
)

add_executable(test_config_store
    src/core/Logger.cpp
    src/config/ConfigStore.cpp
    src/config/ConfigSchema.cpp
    src/test/ConfigStoreTest.cpp
)

target_include_directories(test_config_store
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(test_config_store
    PRIVATE
        ${GTK3_LIBRARIES}
)

target_compile_options(test_config_store 
    PRIVATE 
        -Wall -Wextra
        ${GTK3_CFLAGS_OTHER}
)

# Document Operations Unit Tests
add_executable(test_document_operations
    src/editor/DocumentIO.cpp
    src/editor/SaveNormalization.cpp
    src/test/DocumentOperationsTest.cpp
)

target_include_directories(test_document_operations
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${GTK3_INCLUDE_DIRS}
)

target_link_libraries(test_document_operations
    PRIVATE
        scintilla
        ${GTK3_LIBRARIES}
)

target_compile_options(velnix_editor PRIVATE -Wall -Wextra)
target_compile_options(test_gtk PRIVATE -Wall -Wextra)

# Install targets
install(TARGETS velnix_editor
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES ${CMAKE_SOURCE_DIR}/packaging/deb/org.velnix.VelnixEditor.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)

install(FILES
        ${CMAKE_SOURCE_DIR}/README.md
        ${CMAKE_SOURCE_DIR}/CHANGELOG.md
    DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/velnix-editor
)

install(FILES ${CMAKE_SOURCE_DIR}/LICENSE
    DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/velnix-editor
    RENAME copyright
)

install(FILES ${CMAKE_SOURCE_DIR}/docs/assets/logo-about.png
    DESTINATION ${CMAKE_INSTALL_DATADIR}/velnix-editor/assets
)

install(FILES ${CMAKE_SOURCE_DIR}/docs/assets/logo-app.png
    DESTINATION ${CMAKE_INSTALL_DATADIR}/velnix-editor/assets
)

install(FILES ${CMAKE_SOURCE_DIR}/docs/assets/logo-app.png
    DESTINATION ${CMAKE_INSTALL_DATADIR}/pixmaps
    RENAME org.velnix.VelnixEditor.png
)

# Custom clean target
add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/cmake_clean.cmake
)

# Create clean script
file(WRITE ${CMAKE_BINARY_DIR}/cmake_clean.cmake
    "file(GLOB_RECURSE FILES_TO_REMOVE \"${CMAKE_BINARY_DIR}/*\")\n"
    "list(FILTER FILES_TO_REMOVE EXCLUDE REGEX \".*\.(cmake|txt)$\")\n"
    "foreach(FILE \${FILES_TO_REMOVE})\n"
    "    if(NOT IS_DIRECTORY \${FILE})\n"
    "        file(REMOVE \${FILE})\n"
    "    endif()\n"
    "endforeach()\n"
)
