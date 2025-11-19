#ifndef PLUGIN_API_H
#define PLUGIN_API_H

#include <string>
#include <vector>
#include <functional>
#include <cstdint>

namespace editor {

// Forward declarations
class PieceTable;
class EditorWindow;

// Plugin capability flags
enum class PluginCapability : uint32_t {
    None = 0,
    DocumentManipulation = 1 << 0,  // Can modify documents
    UIContributions = 1 << 1,       // Can add UI elements
    CommandRegistration = 1 << 2,   // Can register commands
    EventListeners = 1 << 3,        // Can listen to editor events
    SettingsAccess = 1 << 4,        // Can read/write settings
    FileSystemAccess = 1 << 5,      // Can access file system (restricted)
    NetworkAccess = 1 << 6,         // Can make network requests (restricted)
    All = 0xFFFFFFFF
};

inline PluginCapability operator|(PluginCapability a, PluginCapability b) {
    return static_cast<PluginCapability>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline bool operator&(PluginCapability a, PluginCapability b) {
    return (static_cast<uint32_t>(a) & static_cast<uint32_t>(b)) != 0;
}

// Plugin metadata
struct PluginMetadata {
    std::string id;              // Unique identifier
    std::string name;            // Display name
    std::string version;         // Semantic version (e.g., "1.0.0")
    std::string author;          // Author name
    std::string description;     // Short description
    PluginCapability capabilities;  // Required capabilities
    std::vector<std::string> dependencies;  // Other plugin IDs this depends on
};

// Document manipulation API (host functions callable from WASM)
struct DocumentAPI {
    // Get document content
    std::function<std::string(int doc_id)> get_text;
    
    // Insert text at position
    std::function<bool(int doc_id, size_t pos, const std::string& text)> insert_text;
    
    // Delete range
    std::function<bool(int doc_id, size_t start, size_t length)> delete_text;
    
    // Replace range
    std::function<bool(int doc_id, size_t start, size_t length, const std::string& text)> replace_text;
    
    // Get selection
    std::function<std::pair<size_t, size_t>(int doc_id)> get_selection;
    
    // Set selection
    std::function<bool(int doc_id, size_t start, size_t end)> set_selection;
    
    // Get cursor position
    std::function<size_t(int doc_id)> get_cursor;
    
    // Set cursor position
    std::function<bool(int doc_id, size_t pos)> set_cursor;
};

// UI API (host functions for UI contributions)
struct UIAPI {
    // Show message
    std::function<void(const std::string& message)> show_message;
    
    // Show error
    std::function<void(const std::string& error)> show_error;
    
    // Show input dialog
    std::function<std::string(const std::string& prompt, const std::string& default_value)> show_input;
    
    // Create output panel
    std::function<int(const std::string& title)> create_output_panel;
    
    // Write to output panel
    std::function<void(int panel_id, const std::string& text)> write_output;
};

// Command definition
struct Command {
    std::string id;              // Unique command ID
    std::string title;           // Display title
    std::string category;        // Category for grouping
    std::string keybinding;      // Optional keyboard shortcut (e.g., "Ctrl+Shift+P")
    std::function<void()> handler;  // Command handler
};

// Event types
enum class EditorEvent {
    DocumentOpened,
    DocumentClosed,
    DocumentChanged,
    DocumentSaved,
    SelectionChanged,
    CursorMoved,
    ThemeChanged,
    SettingsChanged
};

// Event listener callback
using EventCallback = std::function<void(EditorEvent event, void* data)>;

// Plugin API - interface exposed to WASM plugins
class PluginAPI {
public:
    virtual ~PluginAPI() = default;
    
    // Document manipulation
    virtual DocumentAPI& documents() = 0;
    
    // UI API
    virtual UIAPI& ui() = 0;
    
    // Register command
    virtual bool register_command(const Command& command) = 0;
    
    // Unregister command
    virtual bool unregister_command(const std::string& command_id) = 0;
    
    // Add event listener
    virtual int add_event_listener(EditorEvent event, EventCallback callback) = 0;
    
    // Remove event listener
    virtual bool remove_event_listener(int listener_id) = 0;
    
    // Get/set settings
    virtual std::string get_setting(const std::string& key, const std::string& default_value = "") = 0;
    virtual bool set_setting(const std::string& key, const std::string& value) = 0;
};

} // namespace editor

#endif // PLUGIN_API_H
