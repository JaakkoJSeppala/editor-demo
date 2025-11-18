cmake_minimum_required(VERSION 3.15)
project(HighPerformanceEditor VERSION 1.0)

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
    )

    target_include_directories(editor_gui PRIVATE include)

    if(MSVC)
        target_compile_options(editor_gui PRIVATE /W4 /O2)
    else()
        target_compile_options(editor_gui PRIVATE -Wall -Wextra -O3) 
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
