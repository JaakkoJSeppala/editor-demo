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

# GUI editor (Windows native)
if(WIN32)
    add_executable(editor_gui WIN32
        src/gui_main.cpp
        src/piece_table.cpp
        src/viewport.cpp
        src/indexer.cpp
        src/undo_manager.cpp
        src/find_dialog.cpp
        src/treesitter_bridge.cpp
    )

    target_include_directories(editor_gui PRIVATE include)

    # Link common controls for TreeView
    target_link_libraries(editor_gui PRIVATE comctl32)

    if(MSVC)
        target_compile_options(editor_gui PRIVATE /W4 /O2)
    else()
        target_compile_options(editor_gui PRIVATE -Wall -Wextra -O3) 
    endif()

    if(ENABLE_TREESITTER)
        target_compile_definitions(editor_gui PRIVATE ENABLE_TREESITTER=1)
        # Optional: auto-detect vendored Tree-sitter and grammars if present under external/
        set(TS_ROOT ${CMAKE_SOURCE_DIR}/external/tree-sitter)
        set(TS_C ${CMAKE_SOURCE_DIR}/external/tree-sitter-c)
        set(TS_CPP ${CMAKE_SOURCE_DIR}/external/tree-sitter-cpp)

        if(EXISTS ${TS_ROOT}/lib/src/lib.c)
            add_library(tree-sitter STATIC ${TS_ROOT}/lib/src/lib.c)
            target_include_directories(tree-sitter PUBLIC ${TS_ROOT}/lib/src)
            if(MSVC)
                target_compile_definitions(tree-sitter PRIVATE _CRT_SECURE_NO_WARNINGS)
            endif()
            message(STATUS "Found tree-sitter core: ${TS_ROOT}")
        else()
            message(STATUS "tree-sitter core not found in external/tree-sitter (skipping)")
        endif()

        if(EXISTS ${TS_C}/src/parser.c)
            add_library(tree-sitter-c STATIC ${TS_C}/src/parser.c)
            target_include_directories(tree-sitter-c PUBLIC ${TS_C}/src)
            message(STATUS "Found tree-sitter-c grammar: ${TS_C}")
        else()
            message(STATUS "tree-sitter-c grammar not found (skipping)")
        endif()

        if(EXISTS ${TS_CPP}/src/parser.c)
            set(TS_CPP_SRCS ${TS_CPP}/src/parser.c)
            if(EXISTS ${TS_CPP}/src/scanner.cc)
                list(APPEND TS_CPP_SRCS ${TS_CPP}/src/scanner.cc)
            endif()
            add_library(tree-sitter-cpp STATIC ${TS_CPP_SRCS})
            target_include_directories(tree-sitter-cpp PUBLIC ${TS_CPP}/src)
            message(STATUS "Found tree-sitter-cpp grammar: ${TS_CPP}")
        else()
            message(STATUS "tree-sitter-cpp grammar not found (skipping)")
        endif()

        if(TARGET tree-sitter)
            target_link_libraries(editor_gui PRIVATE tree-sitter)
        endif()
        if(TARGET tree-sitter-c)
            target_link_libraries(editor_gui PRIVATE tree-sitter-c)
        endif()
        if(TARGET tree-sitter-cpp)
            target_link_libraries(editor_gui PRIVATE tree-sitter-cpp)
        endif()

        message(STATUS "Tree-sitter enabled: will use vendored libs if present")
    else()
        message(STATUS "Tree-sitter disabled (ENABLE_TREESITTER=OFF)")
    endif()

    message(STATUS "Building Win32 GUI editor")
endif()

# Compiler flags for optimization
if(MSVC)
    target_compile_options(editor_demo PRIVATE /W4 /O2)
    target_compile_options(editor_tests PRIVATE /W4 /O2)
else()
    target_compile_options(editor_demo PRIVATE -Wall -Wextra -O3)
    target_compile_options(editor_tests PRIVATE -Wall -Wextra -O3)
endif()
