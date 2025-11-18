#ifndef TAB_MANAGER_H
#define TAB_MANAGER_H

#include "piece_table.h"
#include <memory>
#include <vector>
#include <string>

/**
 * EditorTab - Represents a single open document/file
 */
struct EditorTab {
    std::shared_ptr<PieceTable> document;
    std::string file_path;
    std::string display_name;
    bool is_modified;
    size_t cursor_pos;
    
    EditorTab(std::shared_ptr<PieceTable> doc, const std::string& path = "")
        : document(doc)
        , file_path(path)
        , display_name(path.empty() ? "Untitled" : extract_filename(path))
        , is_modified(false)
        , cursor_pos(0)
    {}
    
private:
    static std::string extract_filename(const std::string& path) {
        size_t last_slash = path.find_last_of("/\\");
        if (last_slash != std::string::npos) {
            return path.substr(last_slash + 1);
        }
        return path;
    }
};

/**
 * TabManager - Manages multiple open documents/tabs
 */
class TabManager {
public:
    TabManager() : active_tab_index_(0) {
        // Start with one empty tab
        new_tab();
    }
    
    // Tab operations
    size_t new_tab(const std::string& content = "", const std::string& file_path = "") {
        auto doc = std::make_shared<PieceTable>(content);
        tabs_.emplace_back(doc, file_path);
        active_tab_index_ = tabs_.size() - 1;
        return active_tab_index_;
    }
    
    bool close_tab(size_t index) {
        if (tabs_.size() <= 1) {
            // Always keep at least one tab
            return false;
        }
        
        if (index >= tabs_.size()) {
            return false;
        }
        
        tabs_.erase(tabs_.begin() + index);
        
        // Adjust active tab index
        if (active_tab_index_ >= tabs_.size()) {
            active_tab_index_ = tabs_.size() - 1;
        }
        
        return true;
    }
    
    void close_active_tab() {
        close_tab(active_tab_index_);
    }
    
    // Navigation
    void next_tab() {
        if (tabs_.empty()) return;
        active_tab_index_ = (active_tab_index_ + 1) % tabs_.size();
    }
    
    void previous_tab() {
        if (tabs_.empty()) return;
        if (active_tab_index_ == 0) {
            active_tab_index_ = tabs_.size() - 1;
        } else {
            active_tab_index_--;
        }
    }
    
    void set_active_tab(size_t index) {
        if (index < tabs_.size()) {
            active_tab_index_ = index;
        }
    }
    
    // Getters
    EditorTab* get_active_tab() {
        if (active_tab_index_ < tabs_.size()) {
            return &tabs_[active_tab_index_];
        }
        return nullptr;
    }
    
    const EditorTab* get_active_tab() const {
        if (active_tab_index_ < tabs_.size()) {
            return &tabs_[active_tab_index_];
        }
        return nullptr;
    }
    
    EditorTab* get_tab(size_t index) {
        if (index < tabs_.size()) {
            return &tabs_[index];
        }
        return nullptr;
    }
    
    size_t get_active_tab_index() const { return active_tab_index_; }
    size_t get_tab_count() const { return tabs_.size(); }
    
    const std::vector<EditorTab>& get_all_tabs() const { return tabs_; }
    
private:
    std::vector<EditorTab> tabs_;
    size_t active_tab_index_;
};

#endif // TAB_MANAGER_H
