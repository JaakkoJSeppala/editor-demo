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
        # NOTE: Link your vendored Tree-sitter libs here when available, e.g.:
        # target_include_directories(editor_gui PRIVATE external/treesitter/include)
        # target_link_libraries(editor_gui PRIVATE tree-sitter tree-sitter-c tree-sitter-cpp ...)
        message(STATUS "Tree-sitter enabled: remember to add grammars and link libs")
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
