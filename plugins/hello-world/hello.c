// Hello World Plugin - demonstrates basic WASM plugin API
// Compile to WASM with: clang --target=wasm32-wasi -o hello.wasm hello.c

// Simple functions that can be called from the editor

int plugin_init() {
    // Return version number
    return 1;
}

int add_numbers(int a, int b) {
    return a + b;
}

int multiply(int x, int y) {
    return x * y;
}

// Fibonacci example - demonstrates computation
int fibonacci(int n) {
    if (n <= 1) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}
