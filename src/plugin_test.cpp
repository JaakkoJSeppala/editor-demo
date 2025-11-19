#include "plugin_manager.h"
#include <iostream>
#include <cassert>

using namespace editor;

int main() {
    std::cout << "=== Plugin System Test ===" << std::endl << std::endl;

    // Initialize plugin manager
    PluginManager manager;
    if (!manager.initialize()) {
        std::cerr << "Failed to initialize plugin manager: " << manager.get_error() << std::endl;
        return 1;
    }
    std::cout << "[PASS] Plugin manager initialized" << std::endl;

    // Load hello world plugin
    const char* plugin_path = "../plugins/hello-world/hello.wasm";
    std::cout << "\nLoading plugin: " << plugin_path << std::endl;
    
    if (!manager.load_plugin(plugin_path)) {
        std::cerr << "[FAIL] Failed to load plugin: " << manager.get_error() << std::endl;
        return 1;
    }
    std::cout << "[PASS] Plugin loaded successfully" << std::endl;

    // Get plugin
    Plugin* plugin = manager.get_plugin("hello");
    if (!plugin) {
        std::cerr << "[FAIL] Plugin not found after loading" << std::endl;
        return 1;
    }

    // Activate plugin
    std::cout << "\nActivating plugin..." << std::endl;
    if (!manager.activate_plugin("hello")) {
        std::cerr << "[FAIL] Failed to activate plugin: " << manager.get_error() << std::endl;
        return 1;
    }
    std::cout << "[PASS] Plugin activated" << std::endl;

    // Test plugin_init
    std::cout << "\nTesting plugin_init()..." << std::endl;
    int64_t version = 0;
    if (!plugin->call_function("plugin_init", {}, &version)) {
        std::cerr << "[FAIL] Failed to call plugin_init: " << plugin->get_error() << std::endl;
        return 1;
    }
    std::cout << "[PASS] plugin_init() returned version: " << version << std::endl;
    assert(version == 1);

    // Test add_numbers
    std::cout << "\nTesting add_numbers(5, 3)..." << std::endl;
    int64_t result = 0;
    if (!plugin->call_function("add_numbers", {5, 3}, &result)) {
        std::cerr << "[FAIL] Failed to call add_numbers: " << plugin->get_error() << std::endl;
        return 1;
    }
    std::cout << "[PASS] add_numbers(5, 3) = " << result << std::endl;
    assert(result == 8);

    // Test multiply
    std::cout << "\nTesting multiply(4, 7)..." << std::endl;
    result = 0;
    if (!plugin->call_function("multiply", {4, 7}, &result)) {
        std::cerr << "[FAIL] Failed to call multiply: " << plugin->get_error() << std::endl;
        return 1;
    }
    std::cout << "[PASS] multiply(4, 7) = " << result << std::endl;
    assert(result == 28);

    // Test fibonacci
    std::cout << "\nTesting fibonacci(10)..." << std::endl;
    result = 0;
    if (!plugin->call_function("fibonacci", {10}, &result)) {
        std::cerr << "[FAIL] Failed to call fibonacci: " << plugin->get_error() << std::endl;
        return 1;
    }
    std::cout << "[PASS] fibonacci(10) = " << result << std::endl;
    assert(result == 55);

    // Test larger fibonacci
    std::cout << "\nTesting fibonacci(15)..." << std::endl;
    result = 0;
    if (!plugin->call_function("fibonacci", {15}, &result)) {
        std::cerr << "[FAIL] Failed to call fibonacci: " << plugin->get_error() << std::endl;
        return 1;
    }
    std::cout << "[PASS] fibonacci(15) = " << result << std::endl;
    assert(result == 610);

    // Deactivate plugin
    std::cout << "\nDeactivating plugin..." << std::endl;
    if (!manager.deactivate_plugin("hello")) {
        std::cerr << "[FAIL] Failed to deactivate plugin: " << manager.get_error() << std::endl;
        return 1;
    }
    std::cout << "[PASS] Plugin deactivated" << std::endl;

    // Unload plugin
    std::cout << "\nUnloading plugin..." << std::endl;
    if (!manager.unload_plugin("hello")) {
        std::cerr << "[FAIL] Failed to unload plugin: " << manager.get_error() << std::endl;
        return 1;
    }
    std::cout << "[PASS] Plugin unloaded" << std::endl;

    std::cout << "\n=== All Tests Passed! ===" << std::endl;
    return 0;
}
