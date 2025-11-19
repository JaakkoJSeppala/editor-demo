@echo off
REM Build hello world plugin to WASM with explicit exports

clang --target=wasm32 ^
    -nostdlib ^
    -O0 ^
    -Wl,--no-entry ^
    -Wl,--export=plugin_init ^
    -Wl,--export=add_numbers ^
    -Wl,--export=multiply ^
    -Wl,--export=fibonacci ^
    -Wl,--strip-debug ^
    -o hello.wasm ^
    hello.c

if %ERRORLEVEL% EQU 0 (
    echo Successfully compiled hello.wasm
    echo File size:
    dir hello.wasm | findstr hello.wasm
    echo.
    echo Exports should include: plugin_init, add_numbers, multiply, fibonacci
) else (
    echo Compilation failed
)

