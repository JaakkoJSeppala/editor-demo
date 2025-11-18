# File Tree Implementation Guide

## Overview
This document describes the file tree view implementation for the Velocity Editor, enabling project navigation with a Win32 TreeView control.

## Files Created
- `include/file_tree.h` - File tree data structure and TreeView integration

## Implementation Steps

### 1. Add File Tree to GUI (src/gui_main.cpp)

#### Add includes:
```cpp
#include "file_tree.h"
#include <commctrl.h>  // For TreeView
```

#### Add to Win32TextEditor class private members:
```cpp
private:
    // File tree
    std::unique_ptr<FileTree> file_tree_;
    HWND tree_hwnd_ = nullptr;
    int tree_width_ = 250;
    bool show_file_tree_ = true;
```

#### In constructor, initialize file tree:
```cpp
Win32TextEditor(HINSTANCE hInstance) 
    : hInstance_(hInstance)
    , file_tree_(std::make_unique<FileTree>())
    // ... existing initialization
{
    // ... existing code
}
```

#### In create_window(), after main window creation:
```cpp
bool create_window() {
    // ... existing window creation code ...
    
    // Create TreeView control for file tree
    if (show_file_tree_) {
        tree_hwnd_ = CreateWindowExA(
            WS_EX_CLIENTEDGE,
            WC_TREEVIEWA,
            "",
            WS_CHILD | WS_VISIBLE | WS_BORDER | 
            TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_SHOWSELALWAYS,
            0, get_content_top(),  // x, y
            tree_width_, 600,      // width, height (will resize)
            hwnd_,
            (HMENU)IDC_TREE_VIEW,  // Define IDC_TREE_VIEW = 1001
            hInstance_,
            nullptr
        );
        
        if (tree_hwnd_) {
            file_tree_->set_tree_control(tree_hwnd_);
        }
    }
    
    return true;
}
```

### 2. Handle Window Resizing (WM_SIZE)

```cpp
case WM_SIZE:
    if (tree_hwnd_ && show_file_tree_) {
        RECT client_rect;
        GetClientRect(hwnd_, &client_rect);
        
        int content_top = get_content_top();
        int content_height = client_rect.bottom - content_top;
        
        // Resize tree view
        MoveWindow(tree_hwnd_, 
                   0, content_top,
                   tree_width_, content_height,
                   TRUE);
    }
    InvalidateRect(hwnd_, nullptr, FALSE);
    return 0;
```

### 3. Adjust Editor Area

Update rendering and mouse coordinates to account for tree width:

```cpp
int get_editor_left() const {
    return (show_file_tree_ ? tree_width_ + 5 : 0);
}

// In on_paint(), adjust x-offset:
// ... render tabs and text with x-offset from get_editor_left()

// In on_mouse_click(), adjust x coordinate:
void on_mouse_click(int x, int y) {
    x -= get_editor_left();  // Adjust for tree
    // ... rest of click handling
}
```

### 4. Handle Tree Selection (WM_NOTIFY)

```cpp
case WM_NOTIFY: {
    LPNMHDR nmhdr = (LPNMHDR)lParam;
    
    if (nmhdr->hwndFrom == tree_hwnd_ && nmhdr->code == TVN_SELCHANGED) {
        LPNMTREEVIEWA pnmtv = (LPNMTREEVIEWA)lParam;
        
        TreeNode* node = file_tree_->find_node_by_item(pnmtv->itemNew.hItem);
        if (node && !node->is_directory) {
            // Open file in tab
            open_file_from_path(node->full_path);
        }
    }
    else if (nmhdr->hwndFrom == tree_hwnd_ && nmhdr->code == NM_DBLCLK) {
        // Double-click to expand/collapse
        TVHITTESTINFO ht = {};
        GetCursorPos(&ht.pt);
        ScreenToClient(tree_hwnd_, &ht.pt);
        TreeView_HitTest(tree_hwnd_, &ht);
        
        if (ht.hItem) {
            TreeNode* node = file_tree_->find_node_by_item(ht.hItem);
            if (node && node->is_directory) {
                TreeView_Expand(tree_hwnd_, ht.hItem, TVE_TOGGLE);
            }
        }
    }
    
    return 0;
}
```

### 5. Add File Opening Helper

```cpp
void open_file_from_path(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        MessageBoxA(hwnd_, "Failed to open file", "Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    file.close();
    
    // Create new tab or reuse existing
    document_ = std::make_shared<PieceTable>(content);
    if (auto* tab = tab_manager_->get_active_tab()) {
        tab->document = document_;
        tab->file_path = path;
        tab->display_name = path;
        tab->is_modified = false;
    }
    
    current_file_ = path;
    cursor_pos_ = 0;
    is_modified_ = false;
    viewport_.set_document(document_);
    update_title();
    InvalidateRect(hwnd_, nullptr, FALSE);
}
```

### 6. Add Menu Item to Open Folder

In window message loop, add handler for Ctrl+Shift+O:

```cpp
else if (key == L'O' && (GetKeyState(VK_CONTROL) & 0x8000) && (GetKeyState(VK_SHIFT) & 0x8000)) {
    // Ctrl+Shift+O - Open folder
    open_folder();
}

// Implementation:
void open_folder() {
    // Use folder picker dialog
    BROWSEINFOA bi = {};
    bi.hwndOwner = hwnd_;
    bi.lpszTitle = "Select Folder to Open";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    
    LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
    if (pidl) {
        char path[MAX_PATH];
        if (SHGetPathFromIDListA(pidl, path)) {
            file_tree_->load_directory(path);
            file_tree_->populate_tree_view();
        }
        CoTaskMemFree(pidl);
    }
}
```

### 7. Add Toggle File Tree (F12)

```cpp
else if (key == VK_F12) {
    show_file_tree_ = !show_file_tree_;
    if (tree_hwnd_) {
        ShowWindow(tree_hwnd_, show_file_tree_ ? SW_SHOW : SW_HIDE);
    }
    // Trigger resize to adjust layout
    RECT rect;
    GetClientRect(hwnd_, &rect);
    SendMessage(hwnd_, WM_SIZE, 0, MAKELPARAM(rect.right, rect.bottom));
}
```

### 8. Link with Common Controls

In CMakeLists.txt, ensure comctl32 is linked:

```cmake
target_link_libraries(editor_gui PRIVATE comctl32)
```

Add to WinMain before creating window:

```cpp
// Initialize common controls for TreeView
INITCOMMONCONTROLSEX icex;
icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
icex.dwICC = ICC_TREEVIEW_CLASSES;
InitCommonControlsEx(&icex);
```

## Keyboard Shortcuts

- **Ctrl+Shift+O**: Open folder in tree view
- **F12**: Toggle file tree visibility
- **Arrow keys in tree**: Navigate tree
- **Enter in tree**: Open selected file
- **Double-click**: Open file or expand/collapse folder

## Visual Layout

```
┌──────────────────────────────────────────┐
│  Tab 1  │  Tab 2  │  Tab 3              │ Tab Bar
├──────────┬───────────────────────────────┤
│          │                               │
│  Tree    │   Editor Content              │
│  View    │                               │
│          │                               │
│ ▼ src    │                               │
│   ├ main │                               │
│   └ util │                               │
│          │                               │
│ ▼ incl   │                               │
│   ├ head │                               │
│          │                               │
└──────────┴───────────────────────────────┘
    250px        Rest of width
```

## Next Steps

1. Apply these changes to `src/gui_main.cpp`
2. Update CMakeLists.txt to link comctl32
3. Build and test
4. Add context menu (right-click) for file operations
5. Add file icons using ImageList
6. Add filtering (show only certain file types)

## Testing

1. Launch editor
2. Press Ctrl+Shift+O and select a folder
3. Navigate tree with arrow keys
4. Double-click or press Enter on a file to open
5. Press F12 to toggle tree visibility
6. Verify multiple files can be opened in tabs
