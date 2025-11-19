#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H

#include "plugin_api.h"
#include "wasm_runtime.h"
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>

namespace editor {

// Represents a loaded plugin instance
class Plugin {
public:
    Plugin(const std::string& id, const std::string& path);
    ~Plugin();
    
    // Load and initialize the plugin
    bool load();
    
    // Activate the plugin (start listening to events, etc.)
    bool activate();
    
    // Deactivate the plugin (stop listeners, etc.)
    bool deactivate();
    
    // Unload the plugin
    bool unload();
    
    // Get metadata
    const PluginMetadata& metadata() const { return metadata_; }
    
    // Check if loaded
    bool is_loaded() const { return runtime_ != nullptr && runtime_->is_initialized(); }
    
    // Check if activated
    bool is_activated() const { return activated_; }
    
    // Get plugin ID
    const std::string& id() const { return metadata_.id; }
    
    // Get plugin path
    const std::string& path() const { return path_; }
    
    // Call a plugin function
    bool call_function(const std::string& func_name, const std::vector<int64_t>& args = {}, int64_t* result = nullptr);
    
    // Get last error
    const std::string& get_error() const;
    
private:
    PluginMetadata metadata_;
    std::string path_;
    std::unique_ptr<WasmRuntime> runtime_;
    bool activated_;
    std::string error_message_;
    
    void set_error(const std::string& error);
};

// Manages all plugins
class PluginManager {
public:
    PluginManager();
    ~PluginManager();
    
    // Disable copy
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;
    
    // Initialize plugin system
    bool initialize();
    
    // Scan directory for plugins
    std::vector<std::string> scan_plugins(const std::string& directory);
    
    // Load plugin from file
    bool load_plugin(const std::string& path);
    
    // Unload plugin by ID
    bool unload_plugin(const std::string& plugin_id);
    
    // Activate plugin
    bool activate_plugin(const std::string& plugin_id);
    
    // Deactivate plugin
    bool deactivate_plugin(const std::string& plugin_id);
    
    // Get plugin by ID
    Plugin* get_plugin(const std::string& plugin_id);
    
    // Get all loaded plugins
    std::vector<Plugin*> get_loaded_plugins();
    
    // Get all activated plugins
    std::vector<Plugin*> get_activated_plugins();
    
    // Check if plugin is loaded
    bool is_plugin_loaded(const std::string& plugin_id) const;
    
    // Check if plugin is activated
    bool is_plugin_activated(const std::string& plugin_id) const;
    
    // Get last error
    const std::string& get_error() const { return error_message_; }
    
    // Set plugin API (used by host to provide API to plugins)
    void set_plugin_api(PluginAPI* api) { api_ = api; }
    
private:
    std::unordered_map<std::string, std::unique_ptr<Plugin>> plugins_;
    PluginAPI* api_;
    std::string error_message_;
    bool initialized_;
    
    void set_error(const std::string& error);
    bool validate_plugin(Plugin* plugin);
    bool check_dependencies(const PluginMetadata& metadata);
};

} // namespace editor

#endif // PLUGIN_MANAGER_H
