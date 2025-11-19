#include "plugin_manager.h"
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

namespace editor {

// Plugin implementation
Plugin::Plugin(const std::string& id, const std::string& path)
    : path_(path)
    , runtime_(nullptr)
    , activated_(false)
{
    metadata_.id = id;
}

Plugin::~Plugin() {
    if (activated_) {
        deactivate();
    }
    if (runtime_) {
        unload();
    }
}

bool Plugin::load() {
    if (runtime_ && runtime_->is_initialized()) {
        set_error("Plugin already loaded");
        return false;
    }

    // Create WASM runtime
    runtime_ = std::make_unique<WasmRuntime>();
    
    // Initialize with 64KB stack
    if (!runtime_->initialize(64 * 1024)) {
        set_error("Failed to initialize WASM runtime: " + runtime_->get_error());
        runtime_.reset();
        return false;
    }

    // Load WASM module
    if (!runtime_->load_module(path_)) {
        set_error("Failed to load WASM module: " + runtime_->get_error());
        runtime_.reset();
        return false;
    }

    // Call plugin_init to get version
    int64_t version = 0;
    if (call_function("plugin_init", {}, &version)) {
        metadata_.version = std::to_string(version);
    }

    // Try to get plugin name (if exported)
    // Note: This would require string support in wasm3, which is complex
    // For now, we'll use the ID as the name
    metadata_.name = metadata_.id;
    
    // Default capabilities
    metadata_.capabilities = PluginCapability::DocumentManipulation | 
                            PluginCapability::UIContributions |
                            PluginCapability::CommandRegistration;

    error_message_.clear();
    return true;
}

bool Plugin::activate() {
    if (!runtime_ || !runtime_->is_initialized()) {
        set_error("Plugin not loaded");
        return false;
    }

    if (activated_) {
        set_error("Plugin already activated");
        return false;
    }

    // Call plugin_activate if it exists (optional)
    call_function("plugin_activate", {});

    activated_ = true;
    error_message_.clear();
    return true;
}

bool Plugin::deactivate() {
    if (!activated_) {
        return true;
    }

    // Call plugin_deactivate if it exists (optional)
    call_function("plugin_deactivate", {});

    activated_ = false;
    error_message_.clear();
    return true;
}

bool Plugin::unload() {
    if (activated_) {
        deactivate();
    }

    if (runtime_) {
        runtime_->reset();
        runtime_.reset();
    }

    error_message_.clear();
    return true;
}

bool Plugin::call_function(const std::string& func_name, const std::vector<int64_t>& args, int64_t* result) {
    if (!runtime_ || !runtime_->is_initialized()) {
        set_error("Plugin not loaded");
        return false;
    }

    bool success = runtime_->call_function(func_name, args, result);
    if (!success) {
        // Not all functions need to exist, so don't set error for missing functions
        if (runtime_->get_error().find("Failed to find function") == std::string::npos) {
            set_error(runtime_->get_error());
        }
    }
    
    return success;
}

const std::string& Plugin::get_error() const {
    if (!error_message_.empty()) {
        return error_message_;
    }
    if (runtime_) {
        return runtime_->get_error();
    }
    static const std::string empty;
    return empty;
}

void Plugin::set_error(const std::string& error) {
    error_message_ = error;
}

// PluginManager implementation
PluginManager::PluginManager()
    : api_(nullptr)
    , initialized_(false)
{
}

PluginManager::~PluginManager() {
    // Unload all plugins
    for (auto& pair : plugins_) {
        pair.second->unload();
    }
    plugins_.clear();
}

bool PluginManager::initialize() {
    if (initialized_) {
        return true;
    }

    initialized_ = true;
    error_message_.clear();
    return true;
}

std::vector<std::string> PluginManager::scan_plugins(const std::string& directory) {
    std::vector<std::string> found_plugins;

    if (!fs::exists(directory) || !fs::is_directory(directory)) {
        set_error("Plugin directory does not exist: " + directory);
        return found_plugins;
    }

    try {
        for (const auto& entry : fs::recursive_directory_iterator(directory)) {
            if (entry.is_regular_file() && entry.path().extension() == ".wasm") {
                found_plugins.push_back(entry.path().string());
            }
        }
    } catch (const fs::filesystem_error& e) {
        set_error(std::string("Failed to scan plugin directory: ") + e.what());
    }

    return found_plugins;
}

bool PluginManager::load_plugin(const std::string& path) {
    if (!initialized_) {
        set_error("PluginManager not initialized");
        return false;
    }

    // Extract plugin ID from filename (without .wasm extension)
    fs::path p(path);
    std::string plugin_id = p.stem().string();

    // Check if already loaded
    if (plugins_.find(plugin_id) != plugins_.end()) {
        set_error("Plugin already loaded: " + plugin_id);
        return false;
    }

    // Create and load plugin
    auto plugin = std::make_unique<Plugin>(plugin_id, path);
    if (!plugin->load()) {
        set_error("Failed to load plugin: " + plugin->get_error());
        return false;
    }

    // Validate plugin
    if (!validate_plugin(plugin.get())) {
        set_error("Plugin validation failed");
        return false;
    }

    // Check dependencies
    if (!check_dependencies(plugin->metadata())) {
        set_error("Plugin dependencies not satisfied");
        return false;
    }

    plugins_[plugin_id] = std::move(plugin);
    error_message_.clear();
    return true;
}

bool PluginManager::unload_plugin(const std::string& plugin_id) {
    auto it = plugins_.find(plugin_id);
    if (it == plugins_.end()) {
        set_error("Plugin not found: " + plugin_id);
        return false;
    }

    // Deactivate and unload
    it->second->unload();
    plugins_.erase(it);

    error_message_.clear();
    return true;
}

bool PluginManager::activate_plugin(const std::string& plugin_id) {
    auto it = plugins_.find(plugin_id);
    if (it == plugins_.end()) {
        set_error("Plugin not found: " + plugin_id);
        return false;
    }

    if (!it->second->activate()) {
        set_error("Failed to activate plugin: " + it->second->get_error());
        return false;
    }

    error_message_.clear();
    return true;
}

bool PluginManager::deactivate_plugin(const std::string& plugin_id) {
    auto it = plugins_.find(plugin_id);
    if (it == plugins_.end()) {
        set_error("Plugin not found: " + plugin_id);
        return false;
    }

    if (!it->second->deactivate()) {
        set_error("Failed to deactivate plugin: " + it->second->get_error());
        return false;
    }

    error_message_.clear();
    return true;
}

Plugin* PluginManager::get_plugin(const std::string& plugin_id) {
    auto it = plugins_.find(plugin_id);
    if (it != plugins_.end()) {
        return it->second.get();
    }
    return nullptr;
}

std::vector<Plugin*> PluginManager::get_loaded_plugins() {
    std::vector<Plugin*> result;
    for (auto& pair : plugins_) {
        if (pair.second->is_loaded()) {
            result.push_back(pair.second.get());
        }
    }
    return result;
}

std::vector<Plugin*> PluginManager::get_activated_plugins() {
    std::vector<Plugin*> result;
    for (auto& pair : plugins_) {
        if (pair.second->is_activated()) {
            result.push_back(pair.second.get());
        }
    }
    return result;
}

bool PluginManager::is_plugin_loaded(const std::string& plugin_id) const {
    auto it = plugins_.find(plugin_id);
    return it != plugins_.end() && it->second->is_loaded();
}

bool PluginManager::is_plugin_activated(const std::string& plugin_id) const {
    auto it = plugins_.find(plugin_id);
    return it != plugins_.end() && it->second->is_activated();
}

void PluginManager::set_error(const std::string& error) {
    error_message_ = error;
}

bool PluginManager::validate_plugin(Plugin* plugin) {
    if (!plugin || !plugin->is_loaded()) {
        set_error("Plugin is null or not loaded");
        return false;
    }

    // Basic validation - check that plugin_init exists and returns valid version
    int64_t version = 0;
    if (!plugin->call_function("plugin_init", {}, &version)) {
        // For now, be lenient - plugin_init is optional
        // set_error("Plugin missing required plugin_init function");
        // return false;
        return true;  // Allow plugins without plugin_init
    }

    // Version should be > 0 if plugin_init exists
    if (version <= 0) {
        set_error("plugin_init returned invalid version: " + std::to_string(version));
        return false;
    }

    return true;
}

bool PluginManager::check_dependencies(const PluginMetadata& metadata) {
    // Check if all dependencies are loaded and activated
    for (const auto& dep_id : metadata.dependencies) {
        if (!is_plugin_activated(dep_id)) {
            set_error("Missing dependency: " + dep_id);
            return false;
        }
    }
    return true;
}

} // namespace editor
