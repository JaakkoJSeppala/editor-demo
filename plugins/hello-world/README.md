# Hello World Plugin

A simple demonstration plugin for the Velocity Editor extension system.

## Features

This plugin demonstrates:
- **plugin_init()** - Initialize and return plugin version
- **add_numbers(a, b)** - Basic arithmetic
- **multiply(x, y)** - Integer multiplication
- **fibonacci(n)** - Recursive computation example

## Building

### Requirements
- LLVM/Clang with WebAssembly support
- WASI SDK (optional, for WASI features)

### Compile to WASM

Using clang:
```bash
clang --target=wasm32 -nostdlib -Wl,--no-entry -Wl,--export-all -o hello.wasm hello.c
```

Or using WASI SDK:
```bash
clang --target=wasm32-wasi -o hello.wasm hello.c
```

### Pre-built Binary

The `hello.wasm` file is included pre-compiled for convenience.

## Testing

From the editor, you can load and test this plugin:

```cpp
WasmRuntime runtime;
runtime.initialize();
runtime.load_module("plugins/hello-world/hello.wasm");

int64_t result;
runtime.call_function("plugin_init", {}, &result);  // Returns 1
runtime.call_function("add_numbers", {5, 3}, &result);  // Returns 8
runtime.call_function("multiply", {4, 7}, &result);  // Returns 28
runtime.call_function("fibonacci", {10}, &result);  // Returns 55
```

## Plugin API

Future versions will support:
- Document manipulation (insert, delete, replace text)
- UI contributions (commands, menus, panels)
- Event listeners (on save, on change, etc.)
- Settings and configuration
- Host function calls (log, alert, etc.)
