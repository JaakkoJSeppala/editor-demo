#pragma once
#include <string>
#include <vector>
#include <memory>
#include <windows.h>
#include <commctrl.h>
#include <filesystem>

namespace fs = std::filesystem;

struct TreeNode {
    std::string name;
    std::string full_path;
    bool is_directory;
    bool is_expanded;
    HTREEITEM tree_item;  // Win32 TreeView handle
    std::vector<std::unique_ptr<TreeNode>> children;
    
    TreeNode(const std::string& name, const std::string& path, bool is_dir)
        : name(name)
        , full_path(path)
        , is_directory(is_dir)
        , is_expanded(false)
        , tree_item(nullptr) {}
};

class FileTree {
public:
    FileTree() : root_(nullptr), tree_hwnd_(nullptr) {}
    
    // Load directory structure
    void load_directory(const std::string& path) {
        root_path_ = path;
        root_ = scan_directory(path);
    }
    
    // Get root node
    TreeNode* get_root() const { return root_.get(); }
    
    // Set the Win32 TreeView control handle
    void set_tree_control(HWND hwnd) { tree_hwnd_ = hwnd; }
    
    // Populate Win32 TreeView control
    void populate_tree_view() {
        if (!tree_hwnd_ || !root_) return;
        
        // Clear existing items
        TreeView_DeleteAllItems(tree_hwnd_);
        
        // Add root and children
        populate_node(TVI_ROOT, root_.get());
    }

    void reload() {
        if (root_path_.empty()) return;
        root_ = scan_directory(root_path_);
        populate_tree_view();
    }
    
    // Find node by TreeView item handle
    TreeNode* find_node_by_item(HTREEITEM item) {
        if (!root_) return nullptr;
        return find_node_recursive(root_.get(), item);
    }
    
    // Get file icon index based on extension
    static int get_icon_index(const std::string& filename, bool is_directory) {
        if (is_directory) return 0; // Folder icon
        
        // Extract extension
        size_t dot = filename.find_last_of('.');
        if (dot == std::string::npos) return 1; // Default file icon
        
        std::string ext = filename.substr(dot);
        
        // Map extensions to icon indices
        if (ext == ".cpp" || ext == ".cc" || ext == ".cxx") return 2;
        if (ext == ".h" || ext == ".hpp" || ext == ".hxx") return 3;
        if (ext == ".txt" || ext == ".md") return 4;
        if (ext == ".json" || ext == ".xml") return 5;
        
        return 1; // Default
    }
    
private:
    std::string root_path_;
    std::unique_ptr<TreeNode> root_;
    HWND tree_hwnd_;
    
    // Recursively scan directory
    std::unique_ptr<TreeNode> scan_directory(const std::string& path) {
        fs::path p(path);
        
        if (!fs::exists(p)) return nullptr;
        
        auto node = std::make_unique<TreeNode>(
            p.filename().string(),
            path,
            fs::is_directory(p)
        );
        
        if (fs::is_directory(p)) {
            try {
                std::vector<fs::path> entries;
                for (const auto& entry : fs::directory_iterator(p)) {
                    entries.push_back(entry.path());
                }
                
                // Sort: directories first, then alphabetically
                std::sort(entries.begin(), entries.end(), [](const fs::path& a, const fs::path& b) {
                    bool a_is_dir = fs::is_directory(a);
                    bool b_is_dir = fs::is_directory(b);
                    if (a_is_dir != b_is_dir) return a_is_dir > b_is_dir;
                    return a.filename().string() < b.filename().string();
                });
                
                // Recursively add children
                for (const auto& entry : entries) {
                    auto child = scan_directory(entry.string());
                    if (child) {
                        node->children.push_back(std::move(child));
                    }
                }
            } catch (const fs::filesystem_error&) {
                // Skip directories we can't read
            }
        }
        
        return node;
    }
    
    // Populate TreeView with nodes
    void populate_node(HTREEITEM parent, TreeNode* node) {
        if (!node) return;
        
        TVINSERTSTRUCTA tvis = {};
        tvis.hParent = parent;
        tvis.hInsertAfter = TVI_LAST;
        tvis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        tvis.item.pszText = const_cast<char*>(node->name.c_str());
        tvis.item.lParam = reinterpret_cast<LPARAM>(node);
        tvis.item.cChildren = node->is_directory && !node->children.empty() ? 1 : 0;
        int icon_idx = get_icon_index(node->name, node->is_directory);
        tvis.item.iImage = icon_idx;
        tvis.item.iSelectedImage = icon_idx;
        
        node->tree_item = TreeView_InsertItem(tree_hwnd_, &tvis);
        
        // Add children
        for (auto& child : node->children) {
            populate_node(node->tree_item, child.get());
        }
    }
    
    // Find node by TreeView item handle recursively
    TreeNode* find_node_recursive(TreeNode* node, HTREEITEM item) {
        if (!node) return nullptr;
        if (node->tree_item == item) return node;
        
        for (auto& child : node->children) {
            TreeNode* found = find_node_recursive(child.get(), item);
            if (found) return found;
        }
        
        return nullptr;
    }
};
