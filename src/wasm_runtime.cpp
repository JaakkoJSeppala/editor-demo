#include "wasm_runtime.h"
#include <fstream>
#include <sstream>

// wasm3 headers
#include "../third_party/wasm3/source/wasm3.h"
#include "../third_party/wasm3/source/m3_env.h"

namespace editor {

WasmRuntime::WasmRuntime()
    : env_(nullptr)
    , runtime_(nullptr)
    , module_(nullptr)
    , stack_size_(0)
{
}

WasmRuntime::~WasmRuntime() {
    reset();
}

bool WasmRuntime::initialize(size_t stack_size_bytes) {
    if (runtime_) {
        set_error("Runtime already initialized");
        return false;
    }

    stack_size_ = stack_size_bytes;
    
    // Create environment
    env_ = m3_NewEnvironment();
    if (!env_) {
        set_error("Failed to create wasm3 environment");
        return false;
    }

    // Create runtime with stack size
    runtime_ = m3_NewRuntime(env_, stack_size_bytes, nullptr);
    if (!runtime_) {
        set_error("Failed to create wasm3 runtime");
        m3_FreeEnvironment(env_);
        env_ = nullptr;
        return false;
    }

    error_message_.clear();
    return true;
}

bool WasmRuntime::load_module(const std::string& path) {
    if (!runtime_) {
        set_error("Runtime not initialized");
        return false;
    }

    // Read file
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        set_error("Failed to open WASM file: " + path);
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        set_error("Failed to read WASM file: " + path);
        return false;
    }

    return load_module_from_memory(buffer.data(), buffer.size());
}

bool WasmRuntime::load_module_from_memory(const uint8_t* wasm_bytes, size_t size) {
    if (!runtime_) {
        set_error("Runtime not initialized");
        return false;
    }

    // Parse module
    M3Result result = m3_ParseModule(env_, &module_, wasm_bytes, static_cast<uint32_t>(size));
    if (result) {
        set_error(std::string("Failed to parse WASM module: ") + result);
        return false;
    }

    // Load module into runtime
    result = m3_LoadModule(runtime_, module_);
    if (result) {
        set_error(std::string("Failed to load WASM module: ") + result);
        m3_FreeModule(module_);
        module_ = nullptr;
        return false;
    }

    error_message_.clear();
    return true;
}

bool WasmRuntime::call_function(const std::string& func_name, const std::vector<int64_t>& args, int64_t* result) {
    if (!runtime_ || !module_) {
        set_error("Runtime or module not initialized");
        return false;
    }

    // Find function
    M3Function* func = nullptr;
    M3Result m3_result = m3_FindFunction(&func, runtime_, func_name.c_str());
    if (m3_result) {
        set_error(std::string("Failed to find function '") + func_name + "': " + m3_result);
        return false;
    }

    // wasm3 expects arguments as string array
    // For now, we'll use m3_Call for simple no-arg functions
    if (args.empty()) {
        m3_result = m3_Call(func, 0, nullptr);
    } else {
        // Convert args to string format that wasm3 expects
        std::vector<std::string> arg_strings;
        std::vector<const char*> arg_ptrs;
        
        for (const auto& arg : args) {
            arg_strings.push_back(std::to_string(arg));
        }
        
        for (const auto& arg_str : arg_strings) {
            arg_ptrs.push_back(arg_str.c_str());
        }
        
        m3_result = m3_CallArgv(func, static_cast<uint32_t>(args.size()), arg_ptrs.data());
    }
    
    if (m3_result) {
        set_error(std::string("Failed to call function '") + func_name + "': " + m3_result);
        return false;
    }

    // Get result if requested
    if (result) {
        uint64_t ret_val = 0;
        m3_result = m3_GetResultsV(func, &ret_val);
        if (m3_result == nullptr) {
            *result = static_cast<int64_t>(ret_val);
        } else {
            *result = 0;
        }
    }

    error_message_.clear();
    return true;
}

bool WasmRuntime::link_host_function(const std::string& /*module_name*/, const std::string& /*func_name*/, HostFunction /*func*/) {
    if (!runtime_) {
        set_error("Runtime not initialized");
        return false;
    }

    // This is a placeholder - full implementation would use m3_LinkRawFunction
    // and create a trampoline to call the C++ HostFunction
    set_error("Host function linking not yet implemented");
    return false;
}

void WasmRuntime::reset() {
    if (module_) {
        // Module is owned by runtime, will be freed with it
        module_ = nullptr;
    }
    
    if (runtime_) {
        m3_FreeRuntime(runtime_);
        runtime_ = nullptr;
    }
    
    if (env_) {
        m3_FreeEnvironment(env_);
        env_ = nullptr;
    }
    
    error_message_.clear();
}

void WasmRuntime::set_error(const std::string& error) {
    error_message_ = error;
}

} // namespace editor
