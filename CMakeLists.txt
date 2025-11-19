cmake_minimum_required(VERSION 3.15)
project(HighPerformanceEditor VERSION 1.0)
option(ENABLE_TREESITTER "Enable Tree-sitter parsing (requires vendored libs)" OFF)


# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Console demo (original)
add_executable(editor_demo
    src/main.cpp
    src/piece_table.cpp
    src/viewport.cpp
    src/indexer.cpp
)

target_include_directories(editor_demo PRIVATE include)

# Comprehensive test suite
add_executable(editor_tests
    src/test_main.cpp
    src/piece_table.cpp
    src/viewport.cpp
    src/undo_manager.cpp
    src/find_dialog.cpp
)

target_include_directories(editor_tests PRIVATE include)

# Plugin system test
if(WIN32)
    add_executable(plugin_test
        src/plugin_test.cpp
        src/plugin_manager.cpp
        src/wasm_runtime.cpp
    )

    target_include_directories(plugin_test PRIVATE include)
    target_link_libraries(plugin_test PRIVATE wasm3)

    if(MSVC)
        target_compile_options(plugin_test PRIVATE /W4)
    else()
        target_compile_options(plugin_test PRIVATE -Wall -Wextra)
    endif()
endif()

# wasm3 library (cross-platform)
set(WASM3_DIR "${CMAKE_SOURCE_DIR}/third_party/wasm3")
if(EXISTS "${WASM3_DIR}/source")
    file(GLOB WASM3_SOURCES "${WASM3_DIR}/source/*.c")
    add_library(wasm3 STATIC ${WASM3_SOURCES})
    target_include_directories(wasm3 PUBLIC "${WASM3_DIR}/source")
    # Disable warnings for third-party code
    if(MSVC)
        target_compile_options(wasm3 PRIVATE /W0)
    else()
        target_compile_options(wasm3 PRIVATE -w)
    endif()
endif()

# GUI editor (cross-platform)
if(WIN32)
    # Platform abstraction test (Windows)
    add_executable(platform_test
        src/platform_test.cpp
        src/platform_types.cpp
        src/platform_window_win32.cpp
    )
    target_include_directories(platform_test PRIVATE include)
    target_link_libraries(platform_test PRIVATE gdi32 user32)
    if(MSVC)
        # Avoid std::atomic conflicts with Windows SDK
        target_compile_definitions(platform_test PRIVATE _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR)
    endif()

    add_executable(editor_gui WIN32
        src/gui_main.cpp
        src/piece_table.cpp
        src/viewport.cpp
        src/indexer.cpp
        src/undo_manager.cpp
        src/find_dialog.cpp
        src/treesitter_bridge.cpp
        src/lsp_client.cpp
        src/git_integration.cpp
        src/terminal.cpp
        src/theme.cpp
        src/wasm_runtime.cpp
        src/plugin_manager.cpp
    )

    target_include_directories(editor_gui PRIVATE include external/json)

    # Link common controls for TreeView and wasm3
    target_link_libraries(editor_gui PRIVATE comctl32 wasm3)

    if(MSVC)
        target_compile_options(editor_gui PRIVATE /W4 /O2)
    else()
        target_compile_options(editor_gui PRIVATE -Wall -Wextra -O3) 
    endif()

    if(ENABLE_TREESITTER)
        target_compile_definitions(editor_gui PRIVATE ENABLE_TREESITTER=1)
        # Optional: auto-detect vendored Tree-sitter and grammars if present under external/
        set(TS_ROOT ${CMAKE_SOURCE_DIR}/external/tree-sitter)

        if(EXISTS ${TS_ROOT}/lib/src/lib.c)
            add_library(tree-sitter STATIC ${TS_ROOT}/lib/src/lib.c)
            # Use the public headers in lib/include
            target_include_directories(tree-sitter PUBLIC ${TS_ROOT}/lib/include)
            if(MSVC)
                target_compile_definitions(tree-sitter PRIVATE _CRT_SECURE_NO_WARNINGS)
            endif()
            message(STATUS "Found tree-sitter core: ${TS_ROOT}")
        else()
            message(STATUS "tree-sitter core not found in external/tree-sitter (skipping)")
        endif()

        # Helper macro to build grammar libraries
        macro(add_ts_grammar LANG_NAME LANG_PATH)
            if(EXISTS ${LANG_PATH}/src/parser.c)
                set(SRCS ${LANG_PATH}/src/parser.c)
                if(EXISTS ${LANG_PATH}/src/scanner.cc)
                    list(APPEND SRCS ${LANG_PATH}/src/scanner.cc)
                elseif(EXISTS ${LANG_PATH}/src/scanner.c)
                    list(APPEND SRCS ${LANG_PATH}/src/scanner.c)
                endif()
                add_library(tree-sitter-${LANG_NAME} STATIC ${SRCS})
                target_include_directories(tree-sitter-${LANG_NAME} PUBLIC ${LANG_PATH}/src ${TS_ROOT}/lib/include)
                if(MSVC)
                    target_compile_definitions(tree-sitter-${LANG_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
                endif()
                message(STATUS "Found tree-sitter-${LANG_NAME} grammar: ${LANG_PATH}")
            else()
                message(STATUS "tree-sitter-${LANG_NAME} grammar not found (skipping)")
            endif()
        endmacro()

        # Build all available grammars
        add_ts_grammar(c ${CMAKE_SOURCE_DIR}/external/tree-sitter-c)
        add_ts_grammar(cpp ${CMAKE_SOURCE_DIR}/external/tree-sitter-cpp)
        add_ts_grammar(python ${CMAKE_SOURCE_DIR}/external/tree-sitter-python)
        add_ts_grammar(javascript ${CMAKE_SOURCE_DIR}/external/tree-sitter-javascript)
        add_ts_grammar(json ${CMAKE_SOURCE_DIR}/external/tree-sitter-json)
        add_ts_grammar(yaml ${CMAKE_SOURCE_DIR}/external/tree-sitter-yaml)

        # TypeScript has two grammars (typescript and tsx) in subdirectories
        if(EXISTS ${CMAKE_SOURCE_DIR}/external/tree-sitter-typescript/typescript/src/parser.c)
            set(TS_TS_SRCS ${CMAKE_SOURCE_DIR}/external/tree-sitter-typescript/typescript/src/parser.c)
            if(EXISTS ${CMAKE_SOURCE_DIR}/external/tree-sitter-typescript/typescript/src/scanner.c)
                list(APPEND TS_TS_SRCS ${CMAKE_SOURCE_DIR}/external/tree-sitter-typescript/typescript/src/scanner.c)
            endif()
            add_library(tree-sitter-typescript STATIC ${TS_TS_SRCS})
            target_include_directories(tree-sitter-typescript PUBLIC 
                ${CMAKE_SOURCE_DIR}/external/tree-sitter-typescript/typescript/src 
                ${TS_ROOT}/lib/include)
            if(MSVC)
                target_compile_definitions(tree-sitter-typescript PRIVATE _CRT_SECURE_NO_WARNINGS)
            endif()
            message(STATUS "Found tree-sitter-typescript grammar")
        endif()

        # Markdown also has tree-inline grammar
        if(EXISTS ${CMAKE_SOURCE_DIR}/external/tree-sitter-markdown/tree-sitter-markdown/src/parser.c)
            set(MD_SRCS ${CMAKE_SOURCE_DIR}/external/tree-sitter-markdown/tree-sitter-markdown/src/parser.c)
            if(EXISTS ${CMAKE_SOURCE_DIR}/external/tree-sitter-markdown/tree-sitter-markdown/src/scanner.c)
                list(APPEND MD_SRCS ${CMAKE_SOURCE_DIR}/external/tree-sitter-markdown/tree-sitter-markdown/src/scanner.c)
            endif()
            add_library(tree-sitter-markdown STATIC ${MD_SRCS})
            target_include_directories(tree-sitter-markdown PUBLIC 
                ${CMAKE_SOURCE_DIR}/external/tree-sitter-markdown/tree-sitter-markdown/src 
                ${TS_ROOT}/lib/include)
            if(MSVC)
                target_compile_definitions(tree-sitter-markdown PRIVATE _CRT_SECURE_NO_WARNINGS)
            endif()
            message(STATUS "Found tree-sitter-markdown grammar")
        endif()

        # Link all available grammars
        set(TS_GRAMMARS c cpp python javascript typescript json yaml markdown)
        if(TARGET tree-sitter)
            target_link_libraries(editor_gui PRIVATE tree-sitter)
        endif()
        foreach(GRAMMAR ${TS_GRAMMARS})
            if(TARGET tree-sitter-${GRAMMAR})
                target_link_libraries(editor_gui PRIVATE tree-sitter-${GRAMMAR})
            endif()
        endforeach()

        # Ensure editor_gui can include tree_sitter/api.h when TS is enabled
        if(EXISTS ${TS_ROOT}/lib/include/tree_sitter/api.h)
            target_include_directories(editor_gui PRIVATE ${TS_ROOT}/lib/include)
        endif()

        message(STATUS "Tree-sitter enabled: will use vendored libs if present")
    else()
        message(STATUS "Tree-sitter disabled (ENABLE_TREESITTER=OFF)")
    endif()

    message(STATUS "Building Win32 GUI editor")
elseif(UNIX AND NOT APPLE)
    # Linux platform with GTK4
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK4 REQUIRED gtk4)
    pkg_check_modules(CAIRO REQUIRED cairo)
    pkg_check_modules(PANGO REQUIRED pango)

    # Platform abstraction test (Linux)
    add_executable(platform_test
        src/platform_test.cpp
        src/platform_types.cpp
        src/platform_window_gtk.cpp
    )
    target_include_directories(platform_test PRIVATE include ${GTK4_INCLUDE_DIRS})
    target_link_libraries(platform_test PRIVATE ${GTK4_LIBRARIES} ${CAIRO_LIBRARIES} ${PANGO_LIBRARIES})
    target_compile_options(platform_test PRIVATE ${GTK4_CFLAGS_OTHER})

    add_executable(editor_gui
        src/gui_main.cpp
        src/piece_table.cpp
        src/viewport.cpp
        src/indexer.cpp
        src/undo_manager.cpp
        src/find_dialog.cpp
        src/treesitter_bridge.cpp
        src/lsp_client.cpp
        src/git_integration.cpp
        src/terminal.cpp
        src/theme.cpp
        src/wasm_runtime.cpp
        src/plugin_manager.cpp
    )

    target_include_directories(editor_gui PRIVATE include external/json ${GTK4_INCLUDE_DIRS})
    target_link_libraries(editor_gui PRIVATE ${GTK4_LIBRARIES} ${CAIRO_LIBRARIES} ${PANGO_LIBRARIES})
    if(TARGET wasm3)
        target_link_libraries(editor_gui PRIVATE wasm3)
    endif()
    target_compile_options(editor_gui PRIVATE -Wall -Wextra -O3 ${GTK4_CFLAGS_OTHER})

    if(ENABLE_TREESITTER)
        target_compile_definitions(editor_gui PRIVATE ENABLE_TREESITTER=1)
        set(TS_ROOT ${CMAKE_SOURCE_DIR}/external/tree-sitter)

        if(EXISTS ${TS_ROOT}/lib/src/lib.c)
            add_library(tree-sitter STATIC ${TS_ROOT}/lib/src/lib.c)
            target_include_directories(tree-sitter PUBLIC ${TS_ROOT}/lib/include)
            message(STATUS "Found tree-sitter core: ${TS_ROOT}")
        endif()

        # Helper macro for grammars
        macro(add_ts_grammar LANG_NAME LANG_PATH)
            if(EXISTS ${LANG_PATH}/src/parser.c)
                set(SRCS ${LANG_PATH}/src/parser.c)
                if(EXISTS ${LANG_PATH}/src/scanner.cc)
                    list(APPEND SRCS ${LANG_PATH}/src/scanner.cc)
                elseif(EXISTS ${LANG_PATH}/src/scanner.c)
                    list(APPEND SRCS ${LANG_PATH}/src/scanner.c)
                endif()
                add_library(tree-sitter-${LANG_NAME} STATIC ${SRCS})
                target_include_directories(tree-sitter-${LANG_NAME} PUBLIC ${LANG_PATH}/src ${TS_ROOT}/lib/include)
                message(STATUS "Found tree-sitter-${LANG_NAME} grammar")
            endif()
        endmacro()

        add_ts_grammar(c ${CMAKE_SOURCE_DIR}/external/tree-sitter-c)
        add_ts_grammar(cpp ${CMAKE_SOURCE_DIR}/external/tree-sitter-cpp)
        add_ts_grammar(python ${CMAKE_SOURCE_DIR}/external/tree-sitter-python)
        add_ts_grammar(javascript ${CMAKE_SOURCE_DIR}/external/tree-sitter-javascript)
        add_ts_grammar(json ${CMAKE_SOURCE_DIR}/external/tree-sitter-json)
        add_ts_grammar(yaml ${CMAKE_SOURCE_DIR}/external/tree-sitter-yaml)

        if(TARGET tree-sitter)
            target_link_libraries(editor_gui PRIVATE tree-sitter)
        endif()
        set(TS_GRAMMARS c cpp python javascript json yaml)
        foreach(GRAMMAR ${TS_GRAMMARS})
            if(TARGET tree-sitter-${GRAMMAR})
                target_link_libraries(editor_gui PRIVATE tree-sitter-${GRAMMAR})
            endif()
        endforeach()

        if(EXISTS ${TS_ROOT}/lib/include/tree_sitter/api.h)
            target_include_directories(editor_gui PRIVATE ${TS_ROOT}/lib/include)
        endif()

        message(STATUS "Tree-sitter enabled for Linux build")
    endif()

    message(STATUS "Building GTK4 GUI editor for Linux")
elseif(APPLE)
    # macOS platform with Cocoa
    # Platform abstraction test (macOS)
    add_executable(platform_test
        src/platform_test.cpp
        src/platform_types.cpp
        src/platform_window_cocoa.mm
    )
    target_include_directories(platform_test PRIVATE include)
    target_link_libraries(platform_test PRIVATE "-framework Cocoa" "-framework CoreGraphics" "-framework CoreText")
    target_compile_options(platform_test PRIVATE -Wall -Wextra -O3)

    add_executable(editor_gui
        src/gui_main.cpp
        src/piece_table.cpp
        src/viewport.cpp
        src/indexer.cpp
        src/undo_manager.cpp
        src/find_dialog.cpp
        src/treesitter_bridge.cpp
        src/lsp_client.cpp
        src/git_integration.cpp
        src/terminal.cpp
        src/theme.cpp
        src/wasm_runtime.cpp
        src/plugin_manager.cpp
    )

    target_include_directories(editor_gui PRIVATE include external/json)
    target_link_libraries(editor_gui PRIVATE "-framework Cocoa" "-framework CoreGraphics" "-framework CoreText")
    if(TARGET wasm3)
        target_link_libraries(editor_gui PRIVATE wasm3)
    endif()
    target_compile_options(editor_gui PRIVATE -Wall -Wextra -O3)

    if(ENABLE_TREESITTER)
        target_compile_definitions(editor_gui PRIVATE ENABLE_TREESITTER=1)
        set(TS_ROOT ${CMAKE_SOURCE_DIR}/external/tree-sitter)

        if(EXISTS ${TS_ROOT}/lib/src/lib.c)
            add_library(tree-sitter STATIC ${TS_ROOT}/lib/src/lib.c)
            target_include_directories(tree-sitter PUBLIC ${TS_ROOT}/lib/include)
            message(STATUS "Found tree-sitter core: ${TS_ROOT}")
        endif()

        # Helper macro for grammars
        macro(add_ts_grammar LANG_NAME LANG_PATH)
            if(EXISTS ${LANG_PATH}/src/parser.c)
                set(SRCS ${LANG_PATH}/src/parser.c)
                if(EXISTS ${LANG_PATH}/src/scanner.cc)
                    list(APPEND SRCS ${LANG_PATH}/src/scanner.cc)
                elseif(EXISTS ${LANG_PATH}/src/scanner.c)
                    list(APPEND SRCS ${LANG_PATH}/src/scanner.c)
                endif()
                add_library(tree-sitter-${LANG_NAME} STATIC ${SRCS})
                target_include_directories(tree-sitter-${LANG_NAME} PUBLIC ${LANG_PATH}/src ${TS_ROOT}/lib/include)
                message(STATUS "Found tree-sitter-${LANG_NAME} grammar")
            endif()
        endmacro()

        add_ts_grammar(c ${CMAKE_SOURCE_DIR}/external/tree-sitter-c)
        add_ts_grammar(cpp ${CMAKE_SOURCE_DIR}/external/tree-sitter-cpp)
        add_ts_grammar(python ${CMAKE_SOURCE_DIR}/external/tree-sitter-python)
        add_ts_grammar(javascript ${CMAKE_SOURCE_DIR}/external/tree-sitter-javascript)
        add_ts_grammar(json ${CMAKE_SOURCE_DIR}/external/tree-sitter-json)
        add_ts_grammar(yaml ${CMAKE_SOURCE_DIR}/external/tree-sitter-yaml)

        if(TARGET tree-sitter)
            target_link_libraries(editor_gui PRIVATE tree-sitter)
        endif()
        set(TS_GRAMMARS c cpp python javascript json yaml)
        foreach(GRAMMAR ${TS_GRAMMARS})
            if(TARGET tree-sitter-${GRAMMAR})
                target_link_libraries(editor_gui PRIVATE tree-sitter-${GRAMMAR})
            endif()
        endforeach()

        if(EXISTS ${TS_ROOT}/lib/include/tree_sitter/api.h)
            target_include_directories(editor_gui PRIVATE ${TS_ROOT}/lib/include)
        endif()

        message(STATUS "Tree-sitter enabled for macOS build")
    endif()

    message(STATUS "Building Cocoa GUI editor for macOS")
endif()

# Compiler flags for optimization
if(MSVC)
    target_compile_options(editor_demo PRIVATE /W4 /O2)
    target_compile_options(editor_tests PRIVATE /W4 /O2)
else()
    target_compile_options(editor_demo PRIVATE -Wall -Wextra -O3)
    target_compile_options(editor_tests PRIVATE -Wall -Wextra -O3)
endif()
