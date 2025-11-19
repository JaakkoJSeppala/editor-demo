#ifndef WASM_RUNTIME_H
#define WASM_RUNTIME_H

#include <string>
#include <memory>
#include <vector>
#include <functional>

// Forward declarations for wasm3 types
struct M3Environment;
struct M3Runtime;
struct M3Module;
struct M3Function;

namespace editor {

// WASM runtime wrapper using wasm3
class WasmRuntime {
public:
    WasmRuntime();
    ~WasmRuntime();

    // Disable copy
    WasmRuntime(const WasmRuntime&) = delete;
    WasmRuntime& operator=(const WasmRuntime&) = delete;

    // Initialize runtime with memory size
    bool initialize(size_t stack_size_bytes = 64 * 1024);
    
    // Load WASM module from file
    bool load_module(const std::string& path);
    
    // Load WASM module from memory
    bool load_module_from_memory(const uint8_t* wasm_bytes, size_t size);
    
    // Find and call exported function
    bool call_function(const std::string& func_name, const std::vector<int64_t>& args, int64_t* result = nullptr);
    
    // Link host function (C++ function callable from WASM)
    using HostFunction = std::function<int64_t(const std::vector<int64_t>&)>;
    bool link_host_function(const std::string& module_name, const std::string& func_name, HostFunction func);
    
    // Get last error message
    const std::string& get_error() const { return error_message_; }
    
    // Check if runtime is initialized
    bool is_initialized() const { return runtime_ != nullptr; }
    
    // Reset runtime (unload all modules)
    void reset();
    
private:
    M3Environment* env_;
    M3Runtime* runtime_;
    M3Module* module_;
    std::string error_message_;
    size_t stack_size_;
    
    void set_error(const std::string& error);
};

} // namespace editor

#endif // WASM_RUNTIME_H
