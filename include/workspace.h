#pragma once

#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <fstream>
#include <algorithm>

namespace fs = std::filesystem;

// Represents the state of a single file in the workspace
struct FileState {
    std::string path;
    size_t cursor_pos = 0;
    size_t scroll_offset = 0;
    
    FileState() = default;
    FileState(const std::string& p, size_t cursor = 0, size_t scroll = 0)
        : path(p), cursor_pos(cursor), scroll_offset(scroll) {}
};

// Workspace-specific settings (can override global settings)
struct WorkspaceSettings {
    int tab_size = 4;
    bool use_spaces = true;
    std::string theme = "dark";
    std::map<std::string, std::string> custom_settings;
    
    // Load from JSON-like format
    bool load(const std::string& filepath);
    
    // Save to JSON-like format
    bool save(const std::string& filepath) const;
};

// Complete workspace state
struct WorkspaceState {
    std::string root_directory;
    std::vector<std::string> root_folders;  // Multiple root folders
    std::vector<FileState> open_files;
    int active_tab_index = 0;
    WorkspaceSettings settings;
    
    // Serialize to file
    bool save(const std::string& filepath) const;
    
    // Deserialize from file
    bool load(const std::string& filepath);
    
private:
    // Helper to escape strings for simple JSON-like format
    static std::string escape(const std::string& str);
    static std::string unescape(const std::string& str);
};

// Manages workspace state, recent files, and recent workspaces
class WorkspaceManager {
public:
    WorkspaceManager();
    
    // Workspace operations
    bool save_workspace(const WorkspaceState& state, const std::string& workspace_dir);
    bool load_workspace(const std::string& workspace_dir, WorkspaceState& out_state);
    
    // Get workspace file path for a directory
    std::string get_workspace_file(const std::string& workspace_dir) const;
    
    // Recent files (MRU - Most Recently Used)
    void add_recent_file(const std::string& filepath);
    const std::vector<std::string>& get_recent_files() const { return recent_files_; }
    void clear_recent_files();
    
    // Recent workspaces
    void add_recent_workspace(const std::string& workspace_dir);
    const std::vector<std::string>& get_recent_workspaces() const { return recent_workspaces_; }
    void clear_recent_workspaces();
    
    // Persistence for recent files/workspaces
    bool save_recent_lists();
    bool load_recent_lists();
    
    // Get config directory for storing recent lists
    std::string get_config_dir() const;
    
private:
    static const size_t MAX_RECENT_FILES = 20;
    static const size_t MAX_RECENT_WORKSPACES = 10;
    
    std::vector<std::string> recent_files_;
    std::vector<std::string> recent_workspaces_;
    
    // Add to MRU list (moves to front if exists, adds new if not)
    void add_to_mru(std::vector<std::string>& list, const std::string& item, size_t max_size);
};

// Implementation

inline WorkspaceManager::WorkspaceManager() {
    load_recent_lists();
}

inline bool WorkspaceSettings::load(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) return false;
    
    std::string line;
    while (std::getline(file, line)) {
        // Simple key=value parser
        size_t eq_pos = line.find('=');
        if (eq_pos == std::string::npos) continue;
        
        std::string key = line.substr(0, eq_pos);
        std::string value = line.substr(eq_pos + 1);
        
        // Trim whitespace
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
        
        if (key == "tab_size") {
            tab_size = std::stoi(value);
        } else if (key == "use_spaces") {
            use_spaces = (value == "true" || value == "1");
        } else if (key == "theme") {
            theme = value;
        } else {
            custom_settings[key] = value;
        }
    }
    
    return true;
}

inline bool WorkspaceSettings::save(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (!file.is_open()) return false;
    
    file << "tab_size=" << tab_size << "\n";
    file << "use_spaces=" << (use_spaces ? "true" : "false") << "\n";
    file << "theme=" << theme << "\n";
    
    for (const auto& [key, value] : custom_settings) {
        file << key << "=" << value << "\n";
    }
    
    return true;
}

inline std::string WorkspaceState::escape(const std::string& str) {
    std::string result;
    for (char c : str) {
        if (c == '\\') result += "\\\\";
        else if (c == '"') result += "\\\"";
        else if (c == '\n') result += "\\n";
        else if (c == '\r') result += "\\r";
        else if (c == '\t') result += "\\t";
        else result += c;
    }
    return result;
}

inline std::string WorkspaceState::unescape(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '\\' && i + 1 < str.size()) {
            char next = str[i + 1];
            if (next == '\\') result += '\\';
            else if (next == '"') result += '"';
            else if (next == 'n') result += '\n';
            else if (next == 'r') result += '\r';
            else if (next == 't') result += '\t';
            else result += next;
            ++i;
        } else {
            result += str[i];
        }
    }
    return result;
}

inline bool WorkspaceState::save(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (!file.is_open()) return false;
    
    // Simple JSON-like format
    file << "{\n";
    file << "  \"root_directory\": \"" << escape(root_directory) << "\",\n";
    
    // Root folders
    file << "  \"root_folders\": [\n";
    for (size_t i = 0; i < root_folders.size(); ++i) {
        file << "    \"" << escape(root_folders[i]) << "\"";
        if (i < root_folders.size() - 1) file << ",";
        file << "\n";
    }
    file << "  ],\n";
    
    // Open files
    file << "  \"open_files\": [\n";
    for (size_t i = 0; i < open_files.size(); ++i) {
        const auto& f = open_files[i];
        file << "    {\n";
        file << "      \"path\": \"" << escape(f.path) << "\",\n";
        file << "      \"cursor_pos\": " << f.cursor_pos << ",\n";
        file << "      \"scroll_offset\": " << f.scroll_offset << "\n";
        file << "    }";
        if (i < open_files.size() - 1) file << ",";
        file << "\n";
    }
    file << "  ],\n";
    
    file << "  \"active_tab_index\": " << active_tab_index << "\n";
    file << "}\n";
    
    return true;
}

inline bool WorkspaceState::load(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) return false;
    
    // Simple parser (not a full JSON parser, but handles our format)
    std::string line;
    bool in_root_folders = false;
    bool in_open_files = false;
    FileState current_file;
    
    while (std::getline(file, line)) {
        // Trim
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t,") + 1);
        
        if (line.find("\"root_directory\":") != std::string::npos) {
            size_t start = line.find('"', line.find(':')) + 1;
            size_t end = line.rfind('"');
            if (start != std::string::npos && end != std::string::npos) {
                root_directory = unescape(line.substr(start, end - start));
            }
        }
        else if (line.find("\"root_folders\":") != std::string::npos) {
            in_root_folders = true;
            in_open_files = false;
        }
        else if (line.find("\"open_files\":") != std::string::npos) {
            in_open_files = true;
            in_root_folders = false;
        }
        else if (line.find("\"active_tab_index\":") != std::string::npos) {
            size_t pos = line.find(':');
            if (pos != std::string::npos) {
                active_tab_index = std::stoi(line.substr(pos + 1));
            }
            in_open_files = false;
            in_root_folders = false;
        }
        else if (in_root_folders && line.find('"') != std::string::npos) {
            size_t start = line.find('"');
            size_t end = line.rfind('"');
            if (start != std::string::npos && end != std::string::npos && start < end) {
                root_folders.push_back(unescape(line.substr(start + 1, end - start - 1)));
            }
        }
        else if (in_open_files) {
            if (line.find("\"path\":") != std::string::npos) {
                size_t start = line.find('"', line.find(':')) + 1;
                size_t end = line.rfind('"');
                if (start != std::string::npos && end != std::string::npos) {
                    current_file.path = unescape(line.substr(start, end - start));
                }
            }
            else if (line.find("\"cursor_pos\":") != std::string::npos) {
                size_t pos = line.find(':');
                if (pos != std::string::npos) {
                    current_file.cursor_pos = std::stoull(line.substr(pos + 1));
                }
            }
            else if (line.find("\"scroll_offset\":") != std::string::npos) {
                size_t pos = line.find(':');
                if (pos != std::string::npos) {
                    current_file.scroll_offset = std::stoull(line.substr(pos + 1));
                }
            }
            else if (line.find('}') != std::string::npos && !current_file.path.empty()) {
                open_files.push_back(current_file);
                current_file = FileState();
            }
        }
    }
    
    return true;
}

inline bool WorkspaceManager::save_workspace(const WorkspaceState& state, const std::string& workspace_dir) {
    std::string workspace_file = get_workspace_file(workspace_dir);
    
    // Create .velocity directory if needed
    fs::path dir = fs::path(workspace_file).parent_path();
    if (!fs::exists(dir)) {
        fs::create_directories(dir);
    }
    
    if (state.save(workspace_file)) {
        add_recent_workspace(workspace_dir);
        save_recent_lists();
        return true;
    }
    
    return false;
}

inline bool WorkspaceManager::load_workspace(const std::string& workspace_dir, WorkspaceState& out_state) {
    std::string workspace_file = get_workspace_file(workspace_dir);
    
    if (out_state.load(workspace_file)) {
        add_recent_workspace(workspace_dir);
        save_recent_lists();
        return true;
    }
    
    return false;
}

inline std::string WorkspaceManager::get_workspace_file(const std::string& workspace_dir) const {
    fs::path dir(workspace_dir);
    return (dir / ".velocity" / "workspace.vel").string();
}

inline void WorkspaceManager::add_recent_file(const std::string& filepath) {
    // Normalize path
    std::string normalized = fs::absolute(filepath).string();
    add_to_mru(recent_files_, normalized, MAX_RECENT_FILES);
    save_recent_lists();
}

inline void WorkspaceManager::add_recent_workspace(const std::string& workspace_dir) {
    // Normalize path
    std::string normalized = fs::absolute(workspace_dir).string();
    add_to_mru(recent_workspaces_, normalized, MAX_RECENT_WORKSPACES);
}

inline void WorkspaceManager::clear_recent_files() {
    recent_files_.clear();
    save_recent_lists();
}

inline void WorkspaceManager::clear_recent_workspaces() {
    recent_workspaces_.clear();
    save_recent_lists();
}

inline std::string WorkspaceManager::get_config_dir() const {
#ifdef _WIN32
    const char* appdata = getenv("APPDATA");
    if (appdata) {
        return std::string(appdata) + "\\Velocity";
    }
    return ".";
#else
    const char* home = getenv("HOME");
    if (home) {
        return std::string(home) + "/.config/velocity";
    }
    return ".";
#endif
}

inline bool WorkspaceManager::save_recent_lists() {
    std::string config_dir = get_config_dir();
    
    // Create config directory
    if (!fs::exists(config_dir)) {
        fs::create_directories(config_dir);
    }
    
    // Save recent files
    {
        std::ofstream file(config_dir + "/recent_files.txt");
        if (file.is_open()) {
            for (const auto& path : recent_files_) {
                file << path << "\n";
            }
        }
    }
    
    // Save recent workspaces
    {
        std::ofstream file(config_dir + "/recent_workspaces.txt");
        if (file.is_open()) {
            for (const auto& path : recent_workspaces_) {
                file << path << "\n";
            }
        }
    }
    
    return true;
}

inline bool WorkspaceManager::load_recent_lists() {
    std::string config_dir = get_config_dir();
    
    // Load recent files
    {
        std::ifstream file(config_dir + "/recent_files.txt");
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line) && recent_files_.size() < MAX_RECENT_FILES) {
                if (!line.empty() && fs::exists(line)) {
                    recent_files_.push_back(line);
                }
            }
        }
    }
    
    // Load recent workspaces
    {
        std::ifstream file(config_dir + "/recent_workspaces.txt");
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line) && recent_workspaces_.size() < MAX_RECENT_WORKSPACES) {
                if (!line.empty() && fs::exists(line)) {
                    recent_workspaces_.push_back(line);
                }
            }
        }
    }
    
    return true;
}

inline void WorkspaceManager::add_to_mru(std::vector<std::string>& list, const std::string& item, size_t max_size) {
    // Remove if already exists
    auto it = std::find(list.begin(), list.end(), item);
    if (it != list.end()) {
        list.erase(it);
    }
    
    // Add to front
    list.insert(list.begin(), item);
    
    // Trim to max size
    if (list.size() > max_size) {
        list.resize(max_size);
    }
}
