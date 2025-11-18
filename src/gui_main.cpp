#include "piece_table.h"
#include "viewport.h"
#include "undo_manager.h"
#include "find_dialog.h"
#include "workspace.h"
#include "syntax_highlighter.h"
#include "tab_manager.h"
#include "file_tree.h"
#include "workspace.h"
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <commctrl.h>
#include <memory>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <future>
#include <mutex>
#include <atomic>
#include <chrono>
#include <fstream>
#include <algorithm>
#include <vector>

/**
 * High-Performance Text Editor - Native Win32 GUI
 * 
 * Features:
 * - Native Win32 rendering (no external dependencies)
 * - Real-time editing with piece table
 * - Virtual scrolling for million-line files
 * - Hardware-accelerated text rendering
 * - File open/save with Win32 dialogs
 * - Full mouse support with cursor positioning
 * 
 * Controls:
 * - Type to insert text
 * - Backspace/Delete to remove text
 * - Arrow keys to scroll
 * - Mouse click to position cursor
 * - Mouse wheel to scroll
 * - Ctrl+O to open file
 * - Ctrl+S to save file
 * - Ctrl+L to load 50,000 line demo file
 * - F1 to toggle performance stats
 */

class Win32TextEditor {
public:
    Win32TextEditor(HINSTANCE hInstance) 
        : hInstance_(hInstance)
        , document_(std::make_shared<PieceTable>())
        , viewport_(35, 100)
        , undo_manager_(std::make_unique<UndoManager>(1000))
        , find_dialog_(std::make_unique<FindDialog>())
        , highlighter_(std::make_unique<SyntaxHighlighter>())
        , show_file_tree_(true)
        , tree_panel_width_(260)
        , show_find_(false)
        , show_replace_(false)
        , find_text_("")
        , replace_text_("")
        , show_stats_(true)
        , show_line_numbers_(true)
        , relative_line_numbers_(false)
        , replace_edit_find_(true)
        , last_frame_time_(0)
        , current_file_("")
        , is_modified_(false)
        , cursor_visible_(true)
        , cursor_blink_time_(0)
        , has_selection_(false)
        , selection_start_(0)
        , selection_end_(0)
        , char_width_(9)
        , char_height_(22)
        , dragging_tab_(false)
        , drag_tab_index_(static_cast<size_t>(-1))
        , hover_tab_index_(static_cast<size_t>(-1))
        , tab_scroll_offset_(0)
    {
        // Initialize with welcome text
        std::string welcome = 
            "HIGH-PERFORMANCE TEXT EDITOR - Win32 Native GUI\n"
            "===============================================\n\n"
            "Architecture:\n"
            "- Piece Table: O(1) insert/delete operations\n"
            "- Virtual Scrolling: Only renders visible lines\n"
            "- Native Win32: No web tech overhead\n"
            "- C++ Syntax Highlighting with token coloring\n\n"
            "Try typing - notice zero latency even with large files!\n\n"
            "Controls:\n"
            "  Type          - Insert text\n"
            "  Mouse drag    - Select text\n"
            "  Ctrl+A        - Select all\n"
            "  Ctrl+C/X/V    - Copy/Cut/Paste\n"
            "  Ctrl+Z/Ctrl+Y - Undo/Redo\n"
            "  Ctrl+F        - Find text\n"
            "  Ctrl+H        - Replace text\n"
            "  F3/Shift+F3   - Find next/previous\n"
            "  Ctrl+O        - Open file\n"
            "  Ctrl+S        - Save file\n"
            "  F1            - Toggle stats\n"
            "  F2            - Toggle line numbers\n"
            "  ESC           - Quit\n\n"
            "Performance test:\n"
            "Press Ctrl+L to load a massive file and watch it stay at 60fps!\n"
            "Or press Ctrl+O to open any C++ file to see syntax highlighting.\n\n";
        
        // Initialize tab manager and first tab
        show_tabs_ = true;
        tab_bar_height_ = 28;
        tab_manager_ = std::make_unique<TabManager>();
        tab_manager_->new_tab(welcome, "");
        if (auto* tab = tab_manager_->get_active_tab()) {
            document_ = tab->document;
            current_file_ = tab->file_path;
        } else {
            document_ = std::make_shared<PieceTable>(welcome);
        }
        viewport_.set_document(document_);
    }
    
    bool create_window() {
        // Init common controls (for TreeView)
        INITCOMMONCONTROLSEX icc{ sizeof(INITCOMMONCONTROLSEX), ICC_TREEVIEW_CLASSES };
        InitCommonControlsEx(&icc);

        // Register window class
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = window_proc_static;
        wc.hInstance = hInstance_;
        wc.hCursor = LoadCursor(nullptr, IDC_IBEAM);
        wc.hbrBackground = CreateSolidBrush(RGB(30, 30, 35));
        wc.lpszClassName = L"HighPerfEditor";
        
        if (!RegisterClassExW(&wc)) {
            return false;
        }
        
        // Create window
        hwnd_ = CreateWindowExW(
            0,
            L"HighPerfEditor",
            L"High-Performance Text Editor - C++ Demo",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            1200, 800,
            nullptr, nullptr, hInstance_, this
        );
        
        if (!hwnd_) {
            return false;
        }
        
        // Create monospace font
        hFont_ = CreateFontW(
            18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN,
            L"Consolas"
        );
        
        // Calculate actual character dimensions
        HDC hdc = GetDC(hwnd_);
        SelectObject(hdc, hFont_);
        SIZE size;
        GetTextExtentPoint32W(hdc, L"M", 1, &size);
        char_width_ = size.cx;
        char_height_ = size.cy;
        ReleaseDC(hwnd_, hdc);
        
        ShowWindow(hwnd_, SW_SHOW);
        UpdateWindow(hwnd_);

        // Create file tree view control
        DWORD treeStyle = WS_CHILD | WS_VISIBLE | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS;
        RECT cr{}; GetClientRect(hwnd_, &cr);
        int treeTop = 10 + (show_tabs_ ? tab_bar_height_ : 0);
        tree_hwnd_ = CreateWindowExW(WS_EX_CLIENTEDGE, WC_TREEVIEWW, L"",
            treeStyle,
            10, treeTop, tree_panel_width_, cr.bottom - treeTop - 10,
            hwnd_, nullptr, hInstance_, nullptr);
        if (tree_hwnd_) {
            SendMessageW(tree_hwnd_, WM_SETFONT, (WPARAM)hFont_, TRUE);
            setup_tree_image_list();
            // Load current working directory by default
            char cwd[MAX_PATH] = {0};
            GetCurrentDirectoryA(MAX_PATH, cwd);
            current_workspace_dir_ = cwd;
            file_tree_.set_tree_control(tree_hwnd_);
            file_tree_.load_directory(std::string(cwd));
            file_tree_.populate_tree_view();
        }
        
        // Try to load workspace state from previous session
        load_workspace_state();
        
        return true;
    }
    
    int run() {
        MSG msg = {};
        LARGE_INTEGER frequency, last_time, current_time;
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&last_time);
        
        // Start cursor blink timer
        SetTimer(hwnd_, 1, 500, nullptr);
        
        // Start FPS counter timer
        SetTimer(hwnd_, 2, 100, nullptr);
        
        while (GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            
            // Calculate FPS only periodically
            QueryPerformanceCounter(&current_time);
            double delta = static_cast<double>(current_time.QuadPart - last_time.QuadPart) / frequency.QuadPart;
            if (delta > 0.001) { // Update every 1ms minimum
                last_frame_time_ = delta * 1000.0;
                fps_ = 1.0 / delta;
                last_time = current_time;
            }
        }
        
        KillTimer(hwnd_, 1);
        KillTimer(hwnd_, 2);
        return static_cast<int>(msg.wParam);
    }

private:
    // Tabs
    std::unique_ptr<TabManager> tab_manager_;
    bool show_tabs_ = false;
    int tab_bar_height_ = 28;
    std::vector<RECT> tab_rects_;

    // Transient status message (e.g., replace-all count)
    std::wstring status_message_;
    std::chrono::steady_clock::time_point status_message_until_{};
    void show_status_message(const std::wstring& msg, int duration_ms) {
        status_message_ = msg;
        status_message_until_ = std::chrono::steady_clock::now() + std::chrono::milliseconds(duration_ms);
    }

    static LRESULT CALLBACK window_proc_static(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        Win32TextEditor* editor = nullptr;
        
        if (msg == WM_CREATE) {
            CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
            editor = reinterpret_cast<Win32TextEditor*>(cs->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(editor));
        } else {
            editor = reinterpret_cast<Win32TextEditor*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        }
        
        if (editor) {
            return editor->window_proc(hwnd, msg, wParam, lParam);
        }
        
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    
    LRESULT window_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
            case WM_PAINT:
                on_paint();
                return 0;
            case WM_NOTIFY: {
                LPNMHDR hdr = reinterpret_cast<LPNMHDR>(lParam);
                if (hdr && hdr->hwndFrom == tree_hwnd_) {
                    if (hdr->code == NM_DBLCLK) {
                        // Open file on double click
                        HTREEITEM sel = TreeView_GetSelection(tree_hwnd_);
                        if (sel) {
                            TreeNode* node = file_tree_.find_node_by_item(sel);
                            if (node && !node->is_directory) {
                                open_file_from_path(node->full_path);
                            }
                        }
                    } else if (hdr->code == TVN_BEGINDRAGW || hdr->code == TVN_BEGINDRAGA) {
                        // Start drag operation in the file tree
                        LPNMTREEVIEWW tv = reinterpret_cast<LPNMTREEVIEWW>(lParam);
                        HTREEITEM item = tv->itemNew.hItem;
                        if (item) {
                            dragging_tree_ = true;
                            tree_drag_item_ = item;
                            TreeView_SelectItem(tree_hwnd_, item);
                            tree_drag_img_ = TreeView_CreateDragImage(tree_hwnd_, item);
                            POINT scr{}; GetCursorPos(&scr);
                            ImageList_BeginDrag(tree_drag_img_, 0, 0, 0);
                            ImageList_DragEnter(nullptr, scr.x, scr.y);
                            SetCapture(hwnd_);
                        }
                    }
                }
                break;
            }
            case WM_CONTEXTMENU: {
                HWND src = (HWND)wParam;
                if (src == tree_hwnd_) {
                    POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                    if (pt.x == -1 && pt.y == -1) {
                        GetCursorPos(&pt);
                    }
                    // Hit test to select item under cursor
                    ScreenToClient(tree_hwnd_, &pt);
                    TVHITTESTINFO hti{}; hti.pt = pt;
                    HTREEITEM hit = TreeView_HitTest(tree_hwnd_, &hti);
                    if (hit) TreeView_SelectItem(tree_hwnd_, hit);

                    HMENU menu = CreatePopupMenu();
                    AppendMenuW(menu, MF_STRING, 1, L"New File");
                    AppendMenuW(menu, MF_STRING, 2, L"New Folder");
                    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                    AppendMenuW(menu, MF_STRING, 3, L"Delete");

                    POINT spt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                    if (spt.x == -1 && spt.y == -1) GetCursorPos(&spt);
                    int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN, spt.x, spt.y, 0, hwnd_, nullptr);
                    DestroyMenu(menu);

                    if (cmd != 0) {
                        HTREEITEM sel = TreeView_GetSelection(tree_hwnd_);
                        TreeNode* node = sel ? file_tree_.find_node_by_item(sel) : nullptr;
                        if (node) handle_tree_context_command(cmd, node);
                    }
                    return 0;
                }
                break;
            }
                
            case WM_CHAR:
                on_char(static_cast<wchar_t>(wParam));
                is_modified_ = true;
                update_title();
                InvalidateRect(hwnd_, nullptr, FALSE);
                return 0;
                
            case WM_KEYDOWN:
                on_key_down(wParam);
                InvalidateRect(hwnd_, nullptr, FALSE);
                return 0;
                
            case WM_LBUTTONDOWN: {
                int mx = LOWORD(lParam), my = HIWORD(lParam);
                int tabs_top = 10;
                int tabs_bottom = tabs_top + (show_tabs_ ? tab_bar_height_ : 0);
                if (show_tabs_ && my >= tabs_top && my <= tabs_bottom) {
                    // Check for tab under cursor to start drag
                    for (size_t i = 0; i < tab_rects_.size(); ++i) {
                        const RECT& r = tab_rects_[i];
                        if (mx >= r.left && mx <= r.right && my >= r.top && my <= r.bottom) {
                            dragging_tab_ = true;
                            drag_tab_index_ = i;
                            SetCapture(hwnd_);
                            InvalidateRect(hwnd_, nullptr, FALSE);
                            return 0;
                        }
                    }
                }
                on_mouse_click(mx, my);
                SetCapture(hwnd_); // Capture mouse for text dragging
                InvalidateRect(hwnd_, nullptr, FALSE);
                return 0;
            }
                
            case WM_LBUTTONUP:
                if (dragging_tab_) {
                    int mx = LOWORD(lParam), my = HIWORD(lParam);
                    size_t target = drag_tab_index_;
                    for (size_t i = 0; i < tab_rects_.size(); ++i) {
                        const RECT& r = tab_rects_[i];
                        if (mx >= r.left && mx <= r.right && my >= r.top && my <= r.bottom) { target = i; break; }
                    }
                    if (target != drag_tab_index_ && tab_manager_) {
                        tab_manager_->move_tab(drag_tab_index_, target);
                        switch_to_tab(target);
                    }
                    dragging_tab_ = false;
                    drag_tab_index_ = static_cast<size_t>(-1);
                    ReleaseCapture();
                    InvalidateRect(hwnd_, nullptr, FALSE);
                    return 0;
                } else if (dragging_tree_) {
                    // Finish tree drag and drop
                    dragging_tree_ = false;
                    ImageList_DragLeave(nullptr);
                    ImageList_EndDrag();
                    if (tree_drag_img_) { ImageList_Destroy(tree_drag_img_); tree_drag_img_ = nullptr; }
                    ReleaseCapture();

                    POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                    ClientToScreen(hwnd_, &pt);
                    POINT tpt = pt; ScreenToClient(tree_hwnd_, &tpt);
                    TVHITTESTINFO hti{}; hti.pt = tpt;
                    HTREEITEM drop_item = TreeView_HitTest(tree_hwnd_, &hti);
                    if (drop_item && tree_drag_item_ && drop_item != tree_drag_item_) {
                        TreeNode* src = file_tree_.find_node_by_item(tree_drag_item_);
                        TreeNode* tgt = file_tree_.find_node_by_item(drop_item);
                        if (src && tgt) {
                            std::string dest_dir;
                            if (tgt->is_directory) dest_dir = tgt->full_path; else {
                                std::filesystem::path tp(tgt->full_path);
                                dest_dir = tp.parent_path().string();
                            }
                            attempt_move_node(src->full_path, dest_dir);
                        }
                    }
                    if (tree_hover_item_) { TreeView_SelectDropTarget(tree_hwnd_, nullptr); tree_hover_item_ = nullptr; }
                    tree_drag_item_ = nullptr;
                    return 0;
                } else {
                    ReleaseCapture();
                    return 0;
                }
                
            case WM_MOUSEMOVE:
                if (dragging_tree_) {
                    POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                    ClientToScreen(hwnd_, &pt);
                    ImageList_DragMove(pt.x, pt.y);

                    POINT tpt = pt; ScreenToClient(tree_hwnd_, &tpt);
                    TVHITTESTINFO hti{}; hti.pt = tpt;
                    HTREEITEM hit = TreeView_HitTest(tree_hwnd_, &hti);
                    if (hit != tree_hover_item_) {
                        if (tree_hover_item_) TreeView_SelectDropTarget(tree_hwnd_, nullptr);
                        tree_hover_item_ = hit;
                        if (tree_hover_item_) TreeView_SelectDropTarget(tree_hwnd_, tree_hover_item_);
                    }
                    return 0;
                }
                if (wParam & MK_LBUTTON) {
                    // Dragging to select
                    on_mouse_drag(LOWORD(lParam), HIWORD(lParam));
                    InvalidateRect(hwnd_, nullptr, FALSE);
                }
                return 0;
                
            case WM_MOUSEWHEEL: {
                // Scroll tabs when mouse is over the tab bar
                POINT p{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                ScreenToClient(hwnd_, &p);
                int tabs_top = 10;
                int tabs_bottom = tabs_top + (show_tabs_ ? tab_bar_height_ : 0);
                if (show_tabs_ && p.y >= tabs_top && p.y <= tabs_bottom) {
                    int delta = GET_WHEEL_DELTA_WPARAM(wParam);
                    int step = (delta > 0 ? -80 : 80);
                    tab_scroll_offset_ = (std::max)(0, tab_scroll_offset_ + step);
                    InvalidateRect(hwnd_, nullptr, FALSE);
                    return 0;
                }
                on_mouse_wheel(GET_WHEEL_DELTA_WPARAM(wParam));
                InvalidateRect(hwnd_, nullptr, FALSE);
                return 0;
            }
                
            case WM_TIMER:
                if (wParam == 1) {
                    // Cursor blink timer
                    cursor_visible_ = !cursor_visible_;
                    cursor_blink_time_++;
                    
                    // Only invalidate the cursor area, not the whole window
                    RECT cursor_rect;
                    GetClientRect(hwnd_, &cursor_rect);
                    cursor_rect.left = get_content_left();
                    cursor_rect.right = cursor_rect.right - 230;
                    InvalidateRect(hwnd_, &cursor_rect, FALSE);
                }
                else if (wParam == 2) {
                    // FPS update timer - only update stats area
                    if (show_stats_) {
                        RECT stats_rect;
                        GetClientRect(hwnd_, &stats_rect);
                        stats_rect.left = stats_rect.right - 220;
                        InvalidateRect(hwnd_, &stats_rect, FALSE);
                    }
                }
                return 0;
                
            case WM_SIZE:
                // Resize tree view
                if (tree_hwnd_) {
                    RECT cr{}; GetClientRect(hwnd_, &cr);
                    int treeTop = 10 + (show_tabs_ ? tab_bar_height_ : 0);
                    MoveWindow(tree_hwnd_, 10, treeTop, show_file_tree_ ? tree_panel_width_ : 0, cr.bottom - treeTop - 10, TRUE);
                    ShowWindow(tree_hwnd_, show_file_tree_ ? SW_SHOW : SW_HIDE);
                }
                InvalidateRect(hwnd_, nullptr, FALSE);
                return 0;
                
            case WM_CLOSE:
                if (is_modified_) {
                    int result = MessageBoxW(hwnd_, 
                        L"File has unsaved changes. Save before closing?",
                        L"Unsaved Changes",
                        MB_YESNOCANCEL | MB_ICONQUESTION);
                    
                    if (result == IDYES) {
                        if (!save_file()) {
                            return 0; // Cancel close if save failed
                        }
                    } else if (result == IDCANCEL) {
                        return 0; // Cancel close
                    }
                }
                // Save workspace state before closing
                save_workspace_state();
                DestroyWindow(hwnd_);
                return 0;
                
            case WM_DESTROY:
                DeleteObject(hFont_);
                PostQuitMessage(0);
                return 0;
        }
        
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    
    void on_paint() {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd_, &ps);
        
        // Set up double buffering
        RECT client_rect;
        GetClientRect(hwnd_, &client_rect);
        
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBitmap = CreateCompatibleBitmap(hdc, client_rect.right, client_rect.bottom);
        HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);
        
        // Clear background
        HBRUSH bgBrush = CreateSolidBrush(RGB(30, 30, 35));
        FillRect(memDC, &client_rect, bgBrush);
        DeleteObject(bgBrush);
        
        // Set text rendering mode for smoother text
        SetBkMode(memDC, TRANSPARENT);
        SetTextRenderingHint(memDC);
        SelectObject(memDC, hFont_);
        
        // Render tab bar (if enabled)
        render_tabs(memDC, client_rect);

        // Render text lines
        auto visible_lines = viewport_.get_visible_lines();
        int y = get_content_top();
        size_t line_num = viewport_.get_top_line();
        size_t current_line = get_cursor_line();
        RECT client_rect_copy = client_rect;
        int base_left = get_content_left();
        
        for (const auto& line : visible_lines) {
            // Line number (gray) - only if enabled
            int text_x_offset = base_left; // Default offset when line numbers are hidden
            if (show_line_numbers_) {
                // Prepare number string: relative or absolute
                std::wstring line_num_str;
                if (relative_line_numbers_) {
                    if (line_num == current_line) {
                        // Show absolute number for current line (like Vim)
                        line_num_str = std::to_wstring(line_num + 1);
                    } else {
                        size_t diff = (line_num > current_line) ? (line_num - current_line) : (current_line - line_num);
                        line_num_str = std::to_wstring(diff);
                    }
                } else {
                    line_num_str = std::to_wstring(line_num + 1);
                }

                // Draw gutter background
                RECT gutter_rect{ base_left, y, base_left + 70, y + char_height_ };
                HBRUSH gutterBrush = CreateSolidBrush(RGB(35, 35, 45));
                FillRect(memDC, &gutter_rect, gutterBrush);
                DeleteObject(gutterBrush);

                // Right-align numbers in gutter
                SIZE num_sz{};
                GetTextExtentPoint32W(memDC, line_num_str.c_str(), (int)line_num_str.length(), &num_sz);
                int gutter_left = base_left;
                int gutter_right = base_left + 70;
                int num_x = gutter_right - num_sz.cx - 4; // small padding
                SetTextColor(memDC, RGB(100, 100, 120));
                TextOutW(memDC, num_x, y, line_num_str.c_str(), (int)line_num_str.length());
                text_x_offset = base_left + 70; // Offset for text when line numbers are shown
            }
            
            // Current line highlighting (draw before text)
            if (line_num == current_line) {
                RECT hl_rect{
                    text_x_offset,
                    y,
                    client_rect_copy.right - (show_stats_ ? 230 : 10),
                    y + char_height_
                };
                HBRUSH hlBrush = CreateSolidBrush(RGB(45, 45, 60));
                FillRect(memDC, &hl_rect, hlBrush);
                DeleteObject(hlBrush);
            }

            // Calculate line position in document
            size_t line_start_pos = 0;
            for (size_t i = 0; i < line_num; ++i) {
                line_start_pos += document_->get_line(i).length() + 1;
            }
            
            // Tokenize line for syntax highlighting
            auto tokens = highlighter_->tokenize_line(line);
            
            // Convert to wide string and render with selection highlighting and syntax colors
            std::wstring wline;
            for (size_t i = 0; i < line.length(); ++i) {
                size_t char_pos = line_start_pos + i;
                
                // Check if character is in selection
                if (has_selection_ && char_pos >= get_selection_start() && char_pos < get_selection_end()) {
                    // Draw selection background
                    RECT sel_rect = {
                        static_cast<LONG>(text_x_offset + i * char_width_),
                        y,
                        static_cast<LONG>(text_x_offset + (i + 1) * char_width_),
                        y + char_height_
                    };
                    HBRUSH selBrush = CreateSolidBrush(RGB(60, 60, 120));
                    FillRect(memDC, &sel_rect, selBrush);
                    DeleteObject(selBrush);
                }
                
                wline += static_cast<wchar_t>(line[i]);
            }
            
            // Render line with syntax highlighting
            size_t last_pos = 0;
            for (const auto& token : tokens) {
                // Render any normal text before this token
                if (token.start > last_pos) {
                    SetTextColor(memDC, RGB(220, 220, 220));
                    std::wstring segment = wline.substr(last_pos, token.start - last_pos);
                    TextOutW(memDC, text_x_offset + last_pos * char_width_, y, segment.c_str(), segment.length());
                }
                
                // Render token with appropriate color
                SetTextColor(memDC, token.get_color());
                std::wstring segment = wline.substr(token.start, token.length);
                TextOutW(memDC, text_x_offset + token.start * char_width_, y, segment.c_str(), segment.length());
                
                last_pos = token.start + token.length;
            }
            
            // Render any remaining text
            if (last_pos < wline.length()) {
                SetTextColor(memDC, RGB(220, 220, 220));
                std::wstring segment = wline.substr(last_pos);
                TextOutW(memDC, text_x_offset + last_pos * char_width_, y, segment.c_str(), segment.length());
            }
            
            // Draw cursor if on this line
            if (cursor_visible_ && line_num == get_cursor_line()) {
                size_t cursor_col = get_cursor_column();
                if (cursor_col <= line.length()) {
                    HPEN cursorPen = CreatePen(PS_SOLID, 2, RGB(255, 255, 0));
                    HPEN oldPen = (HPEN)SelectObject(memDC, cursorPen);
                    
                    int cursor_x = text_x_offset + cursor_col * char_width_;
                    MoveToEx(memDC, cursor_x, y, nullptr);
                    LineTo(memDC, cursor_x, y + char_height_);
                    
                    SelectObject(memDC, oldPen);
                    DeleteObject(cursorPen);
                }
            }
            
            // Draw extra cursors if in multi-cursor mode
            if (cursor_visible_ && multi_cursor_mode_) {
                for (size_t extra_pos : extra_cursors_) {
                    // Skip if it's the main cursor position
                    if (extra_pos == cursor_pos_) continue;
                    
                    // Check if this extra cursor is on current line
                    size_t pos_counter = 0;
                    for (size_t i = 0; i < line_num; ++i) {
                        pos_counter += document_->get_line(i).length() + 1;
                    }
                    size_t line_end = pos_counter + line.length();
                    
                    if (extra_pos >= pos_counter && extra_pos <= line_end) {
                        size_t col = extra_pos - pos_counter;
                        HPEN extraCursorPen = CreatePen(PS_SOLID, 2, RGB(200, 200, 255));
                        HPEN oldPen = (HPEN)SelectObject(memDC, extraCursorPen);
                        
                        int cursor_x = text_x_offset + col * char_width_;
                        MoveToEx(memDC, cursor_x, y, nullptr);
                        LineTo(memDC, cursor_x, y + char_height_);
                        
                        SelectObject(memDC, oldPen);
                        DeleteObject(extraCursorPen);
                    }
                }
            }
            
            y += char_height_;
            line_num++;
        }

        // Render project-wide search panel if visible
        if (show_project_search_) {
            render_project_search_panel(memDC, client_rect);
        }
        
        // Render stats
        if (show_stats_) {
            render_stats(memDC, client_rect);
        }
        
        // Copy to screen with one operation (no flickering)
        BitBlt(hdc, 0, 0, client_rect.right, client_rect.bottom, memDC, 0, 0, SRCCOPY);
        
        // Cleanup
        SelectObject(memDC, oldBitmap);
        DeleteObject(memBitmap);
        DeleteDC(memDC);
        
        EndPaint(hwnd_, &ps);
    }
    
    void SetTextRenderingHint(HDC hdc) {
        // Enable ClearType for smoother text
        SetBkMode(hdc, TRANSPARENT);
    }

    int get_content_top() const {
        return 10 + (show_tabs_ ? tab_bar_height_ : 0);
    }

    int get_content_left() const {
        return (show_file_tree_ ? (10 + tree_panel_width_ + 10) : 10);
    }

    void render_tabs(HDC hdc, const RECT& client_rect) {
        if (!show_tabs_ || !tab_manager_) return;
        tab_rects_.clear();
        const int view_left = 10;
        const int view_right = client_rect.right - 10;
        int x = view_left - tab_scroll_offset_;
        int y = 10;
        int padding_x = 12;
        int padding_y = 6;
        int gap = 6;
        size_t count = tab_manager_->get_tab_count();
        int total_width = 0;
        for (size_t i = 0; i < count; ++i) {
            const EditorTab* tab = tab_manager_->get_tab(i);
            std::string name = tab->display_name;
            size_t slash = name.find_last_of("/\\");
            if (slash != std::string::npos) name = name.substr(slash + 1);
            if (tab->is_modified) name += " *";
            std::wstring wname;
            for (char c : name) wname += static_cast<wchar_t>(c);
            SIZE sz{};
            GetTextExtentPoint32W(hdc, wname.c_str(), (int)wname.length(), &sz);
            int w = sz.cx + padding_x * 2;
            int h = tab_bar_height_;
            RECT r{ x, y, x + w, y + h };
            tab_rects_.push_back(r);
            if (r.right >= view_left && r.left <= view_right) {
                HBRUSH brush = CreateSolidBrush(i == tab_manager_->get_active_tab_index() ? RGB(50,50,60) : RGB(35,35,40));
                FillRect(hdc, &r, brush);
                DeleteObject(brush);
                HPEN pen = CreatePen(PS_SOLID, 1, RGB(80,80,90));
                HPEN oldPen = (HPEN)SelectObject(hdc, pen);
                HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
                Rectangle(hdc, r.left, r.top, r.right, r.bottom);
                SelectObject(hdc, oldBrush);
                SelectObject(hdc, oldPen);
                DeleteObject(pen);
                SetTextColor(hdc, RGB(200,200,210));
                SetBkMode(hdc, TRANSPARENT);
                TextOutW(hdc, x + padding_x, y + (h - sz.cy)/2, wname.c_str(), (int)wname.length());
            }
            x += w + gap;
            total_width = x - view_left;
        }

        bool overflow_left = tab_scroll_offset_ > 0;
        bool overflow_right = (view_left + total_width) > view_right;
        if (overflow_left) {
            HPEN pen = CreatePen(PS_SOLID, 2, RGB(160,160,170));
            HPEN old = (HPEN)SelectObject(hdc, pen);
            MoveToEx(hdc, view_left + 2, y + tab_bar_height_/2, nullptr);
            LineTo(hdc, view_left + 10, y + tab_bar_height_/2 - 6);
            MoveToEx(hdc, view_left + 2, y + tab_bar_height_/2, nullptr);
            LineTo(hdc, view_left + 10, y + tab_bar_height_/2 + 6);
            SelectObject(hdc, old);
            DeleteObject(pen);
        }
        if (overflow_right) {
            HPEN pen = CreatePen(PS_SOLID, 2, RGB(160,160,170));
            HPEN old = (HPEN)SelectObject(hdc, pen);
            int xr = view_right - 2;
            MoveToEx(hdc, xr - 8, y + tab_bar_height_/2 - 6, nullptr);
            LineTo(hdc, xr, y + tab_bar_height_/2);
            MoveToEx(hdc, xr - 8, y + tab_bar_height_/2 + 6, nullptr);
            LineTo(hdc, xr, y + tab_bar_height_/2);
            SelectObject(hdc, old);
            DeleteObject(pen);
        }

        // Clamp scroll offset if content became smaller
        if (!overflow_right && tab_scroll_offset_ > 0) {
            int excess = (view_left + total_width) - view_right;
            if (excess < 0) excess = 0;
            if (tab_scroll_offset_ > excess) tab_scroll_offset_ = excess;
        }
    }

    void switch_to_tab(size_t index) {
        if (!tab_manager_) return;
        // Persist current state to active tab
        if (auto* cur = tab_manager_->get_active_tab()) {
            cur->cursor_pos = cursor_pos_;
            cur->is_modified = is_modified_;
            cur->file_path = current_file_;
        }
        tab_manager_->set_active_tab(index);
        if (auto* tab = tab_manager_->get_active_tab()) {
            document_ = tab->document;
            current_file_ = tab->file_path;
            cursor_pos_ = tab->cursor_pos;
            is_modified_ = tab->is_modified;
            viewport_.set_document(document_);
        }
        update_title();
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
    
    void render_stats(HDC hdc, const RECT& client_rect) {
        std::wostringstream stats;
        stats << L"FPS: " << static_cast<int>(fps_) << L"\n";
        stats << L"Frame: " << std::fixed << std::setprecision(2) << last_frame_time_ << L"ms\n";
        stats << L"Lines: " << document_->get_line_count() << L"\n";
        stats << L"Chars: " << document_->get_total_length() << L"\n";
        stats << L"Cursor: " << get_cursor_line() + 1 << L":" << get_cursor_column() + 1;
        if (multi_cursor_mode_ && !extra_cursors_.empty()) {
            stats << L" (+" << extra_cursors_.size() << L")";
        }
        stats << L"\n";
        stats << L"View: " << viewport_.get_top_line() + 1 << L"\n";
        stats << L"Render: " << std::fixed << std::setprecision(2) 
              << viewport_.get_last_render_time_ms() << L"ms\n";
        
        if (show_find_) {
            std::wstring wfind;
            for (char c : find_text_) {
                wfind += static_cast<wchar_t>(c);
            }
            stats << L"Find: " << wfind << L"\n";
            if (find_dialog_->has_matches()) {
                stats << L"Matches: " << (find_dialog_->get_current_match_index() + 1) 
                      << L"/" << find_dialog_->get_match_count() << L"\n";
            }
        }
        
        if (show_replace_) {
            std::wstring wfind, wrep;
            for (char c : find_text_) wfind += static_cast<wchar_t>(c);
            for (char c : replace_text_) wrep += static_cast<wchar_t>(c);
            stats << L"Find: " << wfind << (replace_edit_find_ ? L"  [editing]" : L"") << L"\n";
            stats << L"Replace: " << wrep << (!replace_edit_find_ ? L"  [editing]" : L"") << L"\n";
            stats << L"Ctrl+R: Replace current   Ctrl+Shift+R: Replace All\n";
            if (find_dialog_->has_matches()) {
                stats << L"Matches: " << (find_dialog_->get_current_match_index() + 1) 
                      << L"/" << find_dialog_->get_match_count() << L"\n";
            }
        }
        
        // Transient status message (if any)
        {
            auto now = std::chrono::steady_clock::now();
            if (now < status_message_until_ && !status_message_.empty()) {
                stats << status_message_ << L"\n";
            }
        }
        
        if (is_modified_) {
            stats << L"[Modified]\n";
        }
        stats << L"\nF1: Toggle stats";
        if (show_find_) {
            stats << L"\nF3: Find next";
        }
        
        // Background
        RECT stats_rect = {
            client_rect.right - 220,
            10,
            client_rect.right - 10,
            180
        };

        HBRUSH statsBrush = CreateSolidBrush(RGB(20, 20, 25));
        FillRect(hdc, &stats_rect, statsBrush);
        DeleteObject(statsBrush);
        
        // Draw border
        HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(60, 200, 60));
        HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, stats_rect.left, stats_rect.top, stats_rect.right, stats_rect.bottom);
        SelectObject(hdc, oldPen);
        SelectObject(hdc, oldBrush);
        DeleteObject(borderPen);
        
        // Stats text
        SetTextColor(hdc, RGB(100, 255, 100));
        RECT text_rect = stats_rect;
        text_rect.left += 10;
        text_rect.top += 10;
        
        HFONT smallFont = CreateFontW(
            14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN,
            L"Consolas"
        );
        HFONT oldFont = (HFONT)SelectObject(hdc, smallFont);
        
        DrawTextW(hdc, stats.str().c_str(), -1, &text_rect, DT_LEFT | DT_TOP);
        
        SelectObject(hdc, oldFont);
        DeleteObject(smallFont);
    }

    // --- Project-wide Search: helpers ---
    static std::vector<std::string> split_patterns(const std::string& s) {
        std::vector<std::string> out;
        std::string cur;
        for (char c : s) {
            if (c == ';' || c == ',') {
                if (!cur.empty()) out.push_back(cur);
                cur.clear();
            } else if (!(out.empty() && (c == ' ' || c == '\t'))) {
                cur += c;
            } else {
                cur += c;
            }
        }
        if (!cur.empty()) out.push_back(cur);
        return out;
    }

    static bool wildcard_match_one(const char* pat, const char* str) {
        // simple '*' and '?' matcher
        const char* s = nullptr; const char* p = nullptr;
        while (*str) {
            if (*pat == '*') { p = ++pat; s = str; continue; }
            if (*pat == '?' || *pat == *str) { ++pat; ++str; continue; }
            if (p) { pat = p; str = ++s; continue; }
            return false;
        }
        while (*pat == '*') ++pat;
        return *pat == 0;
    }

    static bool wildcard_match(const std::string& pattern, const std::string& value) {
        return wildcard_match_one(pattern.c_str(), value.c_str());
    }

    bool path_matches_includes_excludes(const std::string& path) {
        auto includes = split_patterns(project_include_patterns_);
        auto excludes = split_patterns(project_exclude_patterns_);
        std::string lower_path = path;
        std::transform(lower_path.begin(), lower_path.end(), lower_path.begin(), [](unsigned char c){ return (char)tolower(c); });
        std::string filename;
        size_t pos = lower_path.find_last_of("/\\");
        filename = (pos == std::string::npos) ? lower_path : lower_path.substr(pos + 1);
        // Excludes: substring or wildcard on full path or filename
        for (auto& ex : excludes) {
            std::string lex = ex; std::transform(lex.begin(), lex.end(), lex.begin(), ::tolower);
            if (lex.find('*') != std::string::npos || lex.find('?') != std::string::npos) {
                if (wildcard_match(lex, filename) || wildcard_match(lex, lower_path)) return false;
            } else {
                if (lower_path.find(lex) != std::string::npos) return false;
            }
        }
        // Includes: if any include pattern matches filename
        bool inc_ok = includes.empty();
        for (auto& inc : includes) {
            std::string linc = inc; std::transform(linc.begin(), linc.end(), linc.begin(), ::tolower);
            if (wildcard_match(linc, filename) || wildcard_match(linc, lower_path)) { inc_ok = true; break; }
        }
        return inc_ok;
    }

    void start_project_search() {
        if (project_search_in_progress_) return;
        project_search_in_progress_ = true;
        selected_result_index_ = -1;
        {
            std::lock_guard<std::mutex> lock(project_results_mutex_);
            project_results_.clear();
        }
        std::string root = current_workspace_dir_.empty() ? std::string(".") : current_workspace_dir_;
        std::string query = project_search_query_;
        auto includes = project_include_patterns_;
        auto excludes = project_exclude_patterns_;
        project_search_future_ = std::async(std::launch::async, [this, root, query, includes, excludes]() {
            std::vector<ProjectSearchResult> results;
            std::vector<std::string> files;
            try {
                for (auto it = std::filesystem::recursive_directory_iterator(root, std::filesystem::directory_options::skip_permission_denied); it != std::filesystem::recursive_directory_iterator(); ++it) {
                    const auto& p = *it;
                    if (!p.is_regular_file()) continue;
                    std::string path = p.path().string();
                    if (!path_matches_includes_excludes(path)) continue;
                    files.push_back(path);
                }
            } catch (...) {}

            // Parallel scan in batches
            const size_t max_parallel = (std::max)(2u, std::thread::hardware_concurrency());
            std::atomic<size_t> index{0};
            auto worker = [&]() {
                size_t i;
                while ((i = index.fetch_add(1)) < files.size()) {
                    const std::string& f = files[i];
                    std::ifstream in(f, std::ios::binary);
                    if (!in) continue;
                    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
                    in.close();

                    size_t start = 0;
                    while (true) {
                        size_t pos = content.find(query, start);
                        if (pos == std::string::npos) break;
                        // Compute line/column and extract line text
                        size_t line = 0, col = 0, last_nl = 0;
                        for (size_t k = 0; k < pos; ++k) if (content[k] == '\n') { line++; last_nl = k + 1; }
                        col = pos - last_nl;
                        size_t line_end = content.find('\n', pos);
                        std::string line_text = content.substr(last_nl, (line_end == std::string::npos ? content.size() : line_end) - last_nl);
                        ProjectSearchResult r{ f, line, col, line_text };
                        {
                            std::lock_guard<std::mutex> lock(project_results_mutex_);
                            project_results_.push_back(std::move(r));
                        }
                        start = pos + (query.empty() ? 1 : query.size());
                    }
                }
            };
            std::vector<std::thread> threads;
            for (size_t t = 0; t < max_parallel; ++t) threads.emplace_back(worker);
            for (auto& th : threads) th.join();

            project_search_in_progress_ = false;
            InvalidateRect(hwnd_, nullptr, FALSE);
        });
    }

    void open_result_at_index(int idx) {
        std::lock_guard<std::mutex> lock(project_results_mutex_);
        if (idx < 0 || idx >= (int)project_results_.size()) return;
        const auto& r = project_results_[idx];
        open_file_from_path(r.file_path);
        // Move cursor to line/column
        size_t pos = 0;
        for (size_t i = 0; i < r.line; ++i) pos += document_->get_line(i).length() + 1;
        pos += (std::min)(r.column, document_->get_line(r.line).length());
        cursor_pos_ = pos;
        viewport_.scroll_to_line(r.line);
        InvalidateRect(hwnd_, nullptr, FALSE);
    }

    void replace_in_files() {
        std::string find = project_search_query_;
        std::string repl = project_replace_query_;
        if (find.empty()) return;
        size_t replaced_total = 0;
        std::vector<std::string> touched_files;

        // Gather distinct files from results
        {
            std::lock_guard<std::mutex> lock(project_results_mutex_);
            std::vector<std::string> files;
            for (const auto& r : project_results_) files.push_back(r.file_path);
            std::sort(files.begin(), files.end());
            files.erase(std::unique(files.begin(), files.end()), files.end());
            for (const auto& f : files) {
                std::ifstream in(f, std::ios::binary);
                if (!in) continue;
                std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
                in.close();
                size_t count = 0, pos = 0;
                while ((pos = content.find(find, pos)) != std::string::npos) {
                    content.replace(pos, find.size(), repl);
                    pos += repl.size();
                    ++count;
                }
                if (count > 0) {
                    std::ofstream out(f, std::ios::binary | std::ios::trunc);
                    if (out) { out.write(content.data(), content.size()); out.close(); }
                    replaced_total += count;
                    touched_files.push_back(f);
                }
            }
        }
        // Re-run search to refresh results
        start_project_search();
        // Status message
        std::wostringstream msg;
        msg << L"Replaced " << replaced_total << L" occurrences in " << touched_files.size() << L" files";
        show_status_message(msg.str(), 3000);
    }

    void render_project_search_panel(HDC hdc, const RECT& client_rect) {
        RECT panel{ 10, client_rect.bottom - results_panel_height_, client_rect.right - 10, client_rect.bottom - 10 };
        HBRUSH bg = CreateSolidBrush(RGB(24, 24, 30));
        FillRect(hdc, &panel, bg); DeleteObject(bg);
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(80, 80, 100));
        HPEN oldPen = (HPEN)SelectObject(hdc, pen);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, panel.left, panel.top, panel.right, panel.bottom);
        SelectObject(hdc, oldPen); DeleteObject(pen); SelectObject(hdc, oldBrush);

        SetTextColor(hdc, RGB(200, 200, 220));
        HFONT smallFont = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
        HFONT oldFont = (HFONT)SelectObject(hdc, smallFont);

        int x = panel.left + 10; int y = panel.top + 10;
        auto draw_label = [&](const wchar_t* w, int& yy){ RECT r{ x, yy, panel.right - 10, yy + 18 }; DrawTextW(hdc, w, -1, &r, DT_LEFT | DT_TOP); yy += 18; };
        auto draw_input = [&](const std::string& s, bool focused, int& yy){
            RECT r{ x, yy, panel.right - 10, yy + 22 };
            HBRUSH ib = CreateSolidBrush(focused ? RGB(40, 40, 60) : RGB(30, 30, 40));
            FillRect(hdc, &r, ib); DeleteObject(ib);
            std::wstring ws; for (char c : s) ws += (wchar_t)c;
            r.left += 6; DrawTextW(hdc, ws.c_str(), -1, &r, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            yy += 26;
        };

        draw_label(L"Project Search (Ctrl+Shift+F) — Enter: Search, Ctrl+Shift+R: Replace in Files", y);
        draw_label(L"Query:", y);      draw_input(project_search_query_,   project_search_focus_==0, y);
        draw_label(L"Replace:", y);    draw_input(project_replace_query_,  project_search_focus_==1, y);
        draw_label(L"Include:", y);    draw_input(project_include_patterns_, project_search_focus_==2, y);
        draw_label(L"Exclude:", y);    draw_input(project_exclude_patterns_, project_search_focus_==3, y);

        // Results area header
        int results_top = y + 4;
        RECT sep{ panel.left + 4, results_top, panel.right - 4, results_top + 1 };
        HBRUSH sb = CreateSolidBrush(RGB(60, 60, 80)); FillRect(hdc, &sep, sb); DeleteObject(sb);

        // Results info
        std::wstring info;
        if (project_search_in_progress_) info = L"Searching...";
        else {
            std::lock_guard<std::mutex> lock(project_results_mutex_);
            info = L"Results: "; info += std::to_wstring(project_results_.size());
        }
        RECT ir{ panel.left + 8, results_top + 6, panel.right - 10, results_top + 28 };
        DrawTextW(hdc, info.c_str(), -1, &ir, DT_LEFT | DT_TOP);

        // Results list grouped by file
        int list_top = results_top + 28;
        int row_h = char_height_ + 4;
        result_rows_layout_.clear();
        int draw_y = list_top;
        {
            std::lock_guard<std::mutex> lock(project_results_mutex_);
            // Group by file
            std::map<std::string, std::vector<int>> groups;
            for (int i = 0; i < (int)project_results_.size(); ++i) {
                groups[project_results_[i].file_path].push_back(i);
            }
            for (auto& kv : groups) {
                // Header
                std::wstring wfile; for (char c : kv.first) wfile += (wchar_t)c;
                SetTextColor(hdc, RGB(160, 200, 255));
                RECT hr{ panel.left + 8, draw_y, panel.right - 10, draw_y + row_h };
                DrawTextW(hdc, wfile.c_str(), -1, &hr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
                result_rows_layout_.push_back(ResultRow{ true, kv.first, -1 });
                draw_y += row_h;
                // Items
                SetTextColor(hdc, RGB(220, 220, 220));
                for (int idx : kv.second) {
                    const auto& r = project_results_[idx];
                    std::wostringstream line;
                    line << (r.line + 1) << L":" << (r.column + 1) << L"  ";
                    std::wstring wline; for (char c : r.line_text) wline += (wchar_t)c;
                    std::wstring ws = line.str() + wline;
                    RECT rr{ panel.left + 26, draw_y, panel.right - 10, draw_y + row_h };
                    if (idx == selected_result_index_) {
                        HBRUSH sel = CreateSolidBrush(RGB(50, 70, 110)); FillRect(hdc, &rr, sel); DeleteObject(sel);
                    }
                    DrawTextW(hdc, ws.c_str(), -1, &rr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
                    result_rows_layout_.push_back(ResultRow{ false, kv.first, idx });
                    draw_y += row_h;
                    if (draw_y > panel.bottom - 8) break; // clip
                }
                if (draw_y > panel.bottom - 8) break;
            }
        }
        SelectObject(hdc, oldFont); DeleteObject(smallFont);
    }

    void mark_active_tab_modified() {
        if (tab_manager_) {
            if (auto* tab = tab_manager_->get_active_tab()) {
                tab->is_modified = true;
            }
        }
    }
    
    void on_char(wchar_t ch) {
        // Project-wide search text input and field navigation
        if (show_project_search_) {
            if (ch == L'\t') {
                project_search_focus_ = (project_search_focus_ + 1) % 5;
                return;
            }
            if (ch == L'\r') { // Enter triggers search
                if (!project_search_query_.empty() && !project_search_in_progress_) {
                    start_project_search();
                }
                return;
            }
            if (ch >= 32 && ch < 127) {
                char c = static_cast<char>(ch);
                switch (project_search_focus_) {
                    case 0: project_search_query_ += c; break;
                    case 1: project_replace_query_ += c; break;
                    case 2: project_include_patterns_ += c; break;
                    case 3: project_exclude_patterns_ += c; break;
                    default: break;
                }
                return;
            }
        }
        // If find mode is active, add to search string
        if (show_find_ && ch >= 32 && ch < 127) {
            char c = static_cast<char>(ch);
            find_text_ += c;
            perform_find();
            return;
        }
        
        // If replace mode is active, edit find/replace inputs
        if (show_replace_) {
            if (ch == L'\t') {
                replace_edit_find_ = !replace_edit_find_;
                return;
            }
            if (ch >= 32 && ch < 127) {
                char c = static_cast<char>(ch);
                if (replace_edit_find_) {
                    find_text_ += c;
                    perform_find();
                } else {
                    replace_text_ += c;
                }
                return;
            }
        }
        
        if (ch >= 32 || ch == L'\r' || ch == L'\n' || ch == L'\t') {
            char c = static_cast<char>(ch);
            if (ch == L'\r') c = '\n';
            if (ch == L'\t') c = ' ';
            
            std::string str(1, c);
            
            if (multi_cursor_mode_ && !extra_cursors_.empty()) {
                // Multi-cursor insertion - sort cursors and insert from end to start
                std::vector<size_t> all_cursors = extra_cursors_;
                all_cursors.push_back(cursor_pos_);
                std::sort(all_cursors.begin(), all_cursors.end(), std::greater<size_t>());
                
                // Remove duplicates
                all_cursors.erase(std::unique(all_cursors.begin(), all_cursors.end()), all_cursors.end());
                
                // Insert at each cursor from end to start
                for (size_t pos : all_cursors) {
                    auto cmd = std::make_unique<InsertCommand>(document_.get(), pos, str);
                    undo_manager_->execute(std::move(cmd));
                }
                
                // Update all cursor positions
                cursor_pos_++;
                for (size_t& pos : extra_cursors_) {
                    pos++;
                }
            } else {
                // Single cursor insertion
                auto cmd = std::make_unique<InsertCommand>(document_.get(), cursor_pos_, str);
                undo_manager_->execute(std::move(cmd));
                cursor_pos_++;
            }
            
            is_modified_ = true;
            mark_active_tab_modified();
            
            // Reset cursor blink
            cursor_visible_ = true;
            cursor_blink_time_ = 0;
        }
    }
    
    void on_mouse_click(int x, int y) {
        // Clicks inside project search results panel
        if (show_project_search_) {
            RECT client_rect; GetClientRect(hwnd_, &client_rect);
            int panel_top = client_rect.bottom - results_panel_height_;
            if (y >= panel_top) {
                int row_height = char_height_ + 4;
                int content_top = panel_top + 40; // header area has inputs
                if (y >= content_top) {
                    int rel_y = y - content_top;
                    int row_index = rel_y / row_height;
                    if (row_index >= 0 && row_index < (int)result_rows_layout_.size()) {
                        const auto& row = result_rows_layout_[row_index];
                        if (!row.is_header && row.result_index >= 0) {
                            selected_result_index_ = row.result_index;
                            open_result_at_index(selected_result_index_);
                        }
                        InvalidateRect(hwnd_, nullptr, FALSE);
                        return;
                    }
                }
            }
        }
        // Handle tab clicks first
        int tabs_top = 10;
        int tabs_bottom = tabs_top + (show_tabs_ ? tab_bar_height_ : 0);
        if (show_tabs_ && y >= tabs_top && y <= tabs_bottom) {
            for (size_t i = 0; i < tab_rects_.size(); ++i) {
                const RECT& r = tab_rects_[i];
                if (x >= r.left && x <= r.right && y >= r.top && y <= r.bottom) {
                    switch_to_tab(i);
                    return;
                }
            }
        }

        // Convert screen coordinates to line/column
        int text_offset = (show_line_numbers_ ? 70 : 0) + get_content_left();
        if (x < text_offset) return; // Clicked on line numbers area or margin
        
        int line_index = (y - get_content_top()) / char_height_;
        size_t clicked_line = viewport_.get_top_line() + line_index;
        
        if (clicked_line >= document_->get_line_count()) {
            clicked_line = document_->get_line_count() > 0 ? document_->get_line_count() - 1 : 0;
        }
        
        // Calculate column using actual character width
        int col_index = (x - text_offset) / char_width_;
        if (col_index < 0) col_index = 0;
        
        // Get the line content to check bounds
        std::string line = document_->get_line(clicked_line);
        if (col_index > static_cast<int>(line.length())) {
            col_index = line.length();
        }
        
        // Calculate cursor position in document
        size_t clicked_pos = 0;
        for (size_t i = 0; i < clicked_line; ++i) {
            clicked_pos += document_->get_line(i).length() + 1; // +1 for newline
        }
        clicked_pos += col_index;
        
        // Clamp to document bounds
        if (clicked_pos > document_->get_total_length()) {
            clicked_pos = document_->get_total_length();
        }
        
        // Check for Ctrl+Click to add cursor
        bool ctrl_pressed = GetKeyState(VK_CONTROL) & 0x8000;
        if (ctrl_pressed) {
            // Add cursor at clicked position
            multi_cursor_mode_ = true;
            if (std::find(extra_cursors_.begin(), extra_cursors_.end(), clicked_pos) == extra_cursors_.end()) {
                extra_cursors_.push_back(clicked_pos);
            }
            return;
        }
        
        // Normal click - clear multi-cursor mode
        multi_cursor_mode_ = false;
        extra_cursors_.clear();
        cursor_pos_ = clicked_pos;
        
        // Start new selection
        has_selection_ = false;
        selection_start_ = cursor_pos_;
        selection_end_ = cursor_pos_;
        
        // Reset cursor blink
        cursor_visible_ = true;
        cursor_blink_time_ = 0;
        
        std::cout << "Cursor moved to line " << clicked_line + 1 
                  << ", column " << col_index + 1 
                  << " (pos " << cursor_pos_ << ")\n";
    }
    
    void on_mouse_drag(int x, int y) {
        // Convert screen coordinates to position
        int text_offset = (show_line_numbers_ ? 70 : 0) + get_content_left();
        if (x < text_offset) return;
        
        int line_index = (y - get_content_top()) / char_height_;
        size_t clicked_line = viewport_.get_top_line() + line_index;
        
        if (clicked_line >= document_->get_line_count()) {
            clicked_line = document_->get_line_count() > 0 ? document_->get_line_count() - 1 : 0;
        }
        
        int col_index = (x - text_offset) / char_width_;
        if (col_index < 0) col_index = 0;
        
        std::string line = document_->get_line(clicked_line);
        if (col_index > static_cast<int>(line.length())) {
            col_index = line.length();
        }
        
        // Calculate new cursor position
        size_t new_pos = 0;
        for (size_t i = 0; i < clicked_line; ++i) {
            new_pos += document_->get_line(i).length() + 1;
        }
        new_pos += col_index;
        
        // Update selection
        cursor_pos_ = new_pos;
        selection_end_ = new_pos;
        has_selection_ = (selection_start_ != selection_end_);
    }
    
    void on_key_down(WPARAM key) {
        if (key == VK_ESCAPE) {
            if (show_project_search_) {
                show_project_search_ = false;
                return;
            } else if (show_find_) {
                show_find_ = false;
                find_text_ = "";
                find_dialog_->clear_matches();
            } else if (show_replace_) {
                show_replace_ = false;
                find_text_ = "";
                replace_text_ = "";
                find_dialog_->clear_matches();
            } else if (multi_cursor_mode_) {
                // Clear multi-cursor mode
                multi_cursor_mode_ = false;
                extra_cursors_.clear();
            } else {
                SendMessage(hwnd_, WM_CLOSE, 0, 0);
            }
        }
        else if (key == VK_BACK) {
            // If project search is active, backspace edits the active field
            if (show_project_search_) {
                std::string* target = nullptr;
                switch (project_search_focus_) {
                    case 0: target = &project_search_query_; break;
                    case 1: target = &project_replace_query_; break;
                    case 2: target = &project_include_patterns_; break;
                    case 3: target = &project_exclude_patterns_; break;
                    default: break;
                }
                if (target && !target->empty()) target->pop_back();
                return;
            }
            // If in find mode, delete from search string
            if (show_find_) {
                if (!find_text_.empty()) {
                    find_text_.pop_back();
                    if (!find_text_.empty()) {
                        perform_find();
                    } else {
                        find_dialog_->clear_matches();
                    }
                }
                return;
            }
            // If in replace mode, backspace in active field
            if (show_replace_) {
                if (replace_edit_find_) {
                    if (!find_text_.empty()) {
                        find_text_.pop_back();
                        if (!find_text_.empty()) {
                            perform_find();
                        } else {
                            find_dialog_->clear_matches();
                        }
                    }
                } else {
                    if (!replace_text_.empty()) replace_text_.pop_back();
                }
                return;
            }
                        is_modified_ = true;
                        mark_active_tab_modified();
            
            if (cursor_pos_ > 0) {
                // Use undo manager for deletion
                auto cmd = std::make_unique<DeleteCommand>(document_.get(), cursor_pos_ - 1, 1);
                undo_manager_->execute(std::move(cmd));
                cursor_pos_--;
                is_modified_ = true;
                is_modified_ = true;
                mark_active_tab_modified();
                update_title();
            }
        }
        else if (key == VK_DELETE) {
            if (cursor_pos_ < document_->get_total_length()) {
                // Use undo manager for deletion
                auto cmd = std::make_unique<DeleteCommand>(document_.get(), cursor_pos_, 1);
                undo_manager_->execute(std::move(cmd));
                is_modified_ = true;
                mark_active_tab_modified();
                update_title();
            }
        }
        else if (key == L'Z' && (GetKeyState(VK_CONTROL) & 0x8000)) {
            // Ctrl+Z - Undo
            if (undo_manager_->can_undo()) {
                undo_manager_->undo();
                is_modified_ = true;
                mark_active_tab_modified();
                update_title();
            }
        }
        else if (key == L'Y' && (GetKeyState(VK_CONTROL) & 0x8000)) {
            // Ctrl+Y - Redo
            if (undo_manager_->can_redo()) {
                undo_manager_->redo();
                is_modified_ = true;
                mark_active_tab_modified();
                update_title();
            }
        }
        else if (key == VK_LEFT) {
            if (cursor_pos_ > 0) cursor_pos_--;
        }
        else if (key == VK_RIGHT) {
            if (cursor_pos_ < document_->get_total_length()) cursor_pos_++;
        }
        else if (key == VK_UP) {
            viewport_.scroll_up(1);
        }
        else if (key == VK_DOWN) {
            viewport_.scroll_down(1);
        }
        else if (key == VK_PRIOR) { // Page Up
            viewport_.scroll_up(10);
        }
        else if (key == VK_NEXT) { // Page Down
            viewport_.scroll_down(10);
        }
        else if (key == VK_HOME) {
            cursor_pos_ = 0;
            viewport_.scroll_to_line(0);
        }
        else if (key == VK_END) {
            cursor_pos_ = document_->get_total_length();
        }
        else if (key == L'F' && (GetKeyState(VK_CONTROL) & 0x8000) && (GetKeyState(VK_SHIFT) & 0x8000)) {
            // Ctrl+Shift+F - Toggle Project-wide Search panel
            show_project_search_ = !show_project_search_;
            if (show_project_search_) {
                // Hide other overlays
                show_find_ = false;
                show_replace_ = false;
                project_search_focus_ = 0;
                selected_result_index_ = -1;
            }
            InvalidateRect(hwnd_, nullptr, FALSE);
        }
        else if (key == L'O' && (GetKeyState(VK_CONTROL) & 0x8000)) {
            open_file();
        }
        else if (key == L'S' && (GetKeyState(VK_CONTROL) & 0x8000)) {
            save_file();
        }
        else if (key == L'L' && (GetKeyState(VK_CONTROL) & 0x8000)) {
            load_large_demo_file();
        }
        else if (key == L'R' && (GetKeyState(VK_CONTROL) & 0x8000)) {
            // Ctrl+R - Recent files dialog
            show_recent_files_dialog();
        }
        else if (key == L'F' && (GetKeyState(VK_CONTROL) & 0x8000)) {
            // Ctrl+F - Toggle find mode
            if (!show_project_search_) {
                show_find_ = !show_find_;
            }
            show_replace_ = false;
            if (show_find_) {
                find_text_.clear();
                find_dialog_->clear_matches();
            }
        }
        else if (key == L'H' && (GetKeyState(VK_CONTROL) & 0x8000)) {
            // Ctrl+H - Toggle replace mode
            show_replace_ = !show_replace_;
            show_find_ = false;
            if (show_replace_) {
                find_text_.clear();
                replace_text_.clear();
                find_dialog_->clear_matches();
            }
        }
        else if (key == L'A' && (GetKeyState(VK_CONTROL) & 0x8000)) {
            // Ctrl+A - Select all
            selection_start_ = 0;
            selection_end_ = document_->get_total_length();
            has_selection_ = true;
            cursor_pos_ = selection_end_;
            // Clear multi-cursor mode on select all
            multi_cursor_mode_ = false;
            extra_cursors_.clear();
        }
        else if (key == L'D' && (GetKeyState(VK_CONTROL) & 0x8000) && !(GetKeyState(VK_SHIFT) & 0x8000)) {
            // Ctrl+D - Add next occurrence to multi-cursor
            if (has_selection_) {
                std::string selected = get_selected_text();
                if (!selected.empty()) {
                    // Build document text
                    std::string doc_text;
                    for (size_t i = 0; i < document_->get_line_count(); ++i) {
                        doc_text += document_->get_line(i) + "\n";
                    }
                    
                    // Find next occurrence after current selection
                    size_t search_start = get_selection_end();
                    size_t pos = doc_text.find(selected, search_start);
                    
                    if (pos != std::string::npos) {
                        // Enable multi-cursor mode
                        multi_cursor_mode_ = true;
                        
                        // Add current selection end to extra cursors if not already there
                        size_t current_end = get_selection_end();
                        if (std::find(extra_cursors_.begin(), extra_cursors_.end(), current_end) == extra_cursors_.end()) {
                            extra_cursors_.push_back(current_end);
                        }
                        
                        // Add new cursor at found position
                        extra_cursors_.push_back(pos + selected.length());
                        
                        // Update main cursor to the new position
                        cursor_pos_ = pos + selected.length();
                        selection_start_ = pos;
                        selection_end_ = pos + selected.length();
                        has_selection_ = true;
                        
                        // Scroll to new cursor
                        size_t line = 0;
                        size_t char_count = 0;
                        for (size_t i = 0; i < document_->get_line_count(); ++i) {
                            size_t line_len = document_->get_line(i).length() + 1;
                            if (char_count + line_len > pos) {
                                line = i;
                                break;
                            }
                            char_count += line_len;
                        }
                        viewport_.scroll_to_line(line);
                    }
                }
            } else {
                // No selection - select word under cursor and enter multi-cursor mode
                // For simplicity, we'll require a selection first
            }
        }
        else if (key == L'C' && (GetKeyState(VK_CONTROL) & 0x8000)) {
            // Ctrl+C - Copy
            copy_to_clipboard();
        }
        else if (key == L'X' && (GetKeyState(VK_CONTROL) & 0x8000)) {
            // Ctrl+X - Cut
            if (copy_to_clipboard()) {
                delete_selection();
            }
        }
        else if (key == L'V' && (GetKeyState(VK_CONTROL) & 0x8000)) {
            // Ctrl+V - Paste
            paste_from_clipboard();
        }
        else if (key == L'R' && (GetKeyState(VK_CONTROL) & 0x8000) && !(GetKeyState(VK_SHIFT) & 0x8000)) {
            // Ctrl+R - Replace current match (in replace mode)
            if (show_replace_ && !find_text_.empty() && find_dialog_->has_matches()) {
                auto* current_match = find_dialog_->get_current_match();
                if (current_match) {
                    // Delete the matched text
                    auto cmd = std::make_unique<DeleteCommand>(document_.get(), 
                                                               current_match->position, 
                                                               current_match->length);
                    undo_manager_->execute(std::move(cmd));
                    cursor_pos_ = current_match->position;
                    is_modified_ = true;
                    mark_active_tab_modified();
                    update_title();
                    
                    // Re-search and find next match
                    perform_find();
                    if (find_dialog_->has_matches()) {
                        find_next();
                    }
                }
            }
        }
        else if (key == L'R' && (GetKeyState(VK_CONTROL) & 0x8000) && (GetKeyState(VK_SHIFT) & 0x8000)) {
            // Ctrl+Shift+R - Replace All
            if (show_replace_ && !find_text_.empty()) {
                // Build full document text
                std::string doc_text;
                for (size_t i = 0; i < document_->get_line_count(); ++i) {
                    doc_text += document_->get_line(i) + "\n";
                }
                int replaced = find_dialog_->replace_all(doc_text, find_text_, replace_text_);
                if (replaced > 0) {
                    // Replace document content
                    document_ = std::make_shared<PieceTable>(doc_text);
                    viewport_.set_document(document_);
                    cursor_pos_ = 0;
                    is_modified_ = true;
                    mark_active_tab_modified();
                    if (tab_manager_) {
                        if (auto* tab = tab_manager_->get_active_tab()) {
                            tab->document = document_;
                        }
                    }
                    // Refresh matches
                    perform_find();
                    update_title();
                    // Show feedback
                    show_status_message(L"Replaced " + std::to_wstring(replaced) + L" occurrence(s)", 2000);
                } else {
                    // No replacements performed
                    show_status_message(L"No matches to replace", 2000);
                }
            }
        }
        else if (key == VK_F3) {
            // F3 - Find next
            if (!find_text_.empty()) {
                bool shift = GetKeyState(VK_SHIFT) & 0x8000;
                if (shift) {
                    find_previous();
                } else {
                    find_next();
                }
            }
        }
        else if (key == VK_F1) {
            show_stats_ = !show_stats_;
        }
        else if (key == VK_F2) {
            show_line_numbers_ = !show_line_numbers_;
        }
        else if (key == L'B' && (GetKeyState(VK_CONTROL) & 0x8000)) {
            // Ctrl+B - Toggle file tree view
            show_file_tree_ = !show_file_tree_;
            SendMessage(hwnd_, WM_SIZE, 0, 0);
        }
        else if (key == VK_F4) {
            // Toggle relative line numbers
            relative_line_numbers_ = !relative_line_numbers_;
        }
        else if (key == VK_TAB && (GetKeyState(VK_CONTROL) & 0x8000)) {
            // Ctrl+Tab / Ctrl+Shift+Tab - Switch tabs
            if (tab_manager_) {
                size_t count = tab_manager_->get_tab_count();
                if (count > 1) {
                    size_t cur = tab_manager_->get_active_tab_index();
                    bool shift = GetKeyState(VK_SHIFT) & 0x8000;
                    size_t next = shift ? (cur == 0 ? count - 1 : cur - 1) : (cur + 1) % count;
                    switch_to_tab(next);
                }
            }
        }
        else if ((key >= L'1' && key <= L'9') && (GetKeyState(VK_CONTROL) & 0x8000)) {
            // Ctrl+1-9 - Switch to tab by number
            if (tab_manager_) {
                size_t tab_index = key - L'1';  // 0-based index
                if (tab_index < tab_manager_->get_tab_count()) {
                    switch_to_tab(tab_index);
                }
            }
        }
        else if (key == L'T' && (GetKeyState(VK_CONTROL) & 0x8000)) {
            // Ctrl+T - New tab
            if (tab_manager_) {
                size_t idx = tab_manager_->new_tab("", "");
                switch_to_tab(idx);
                is_modified_ = false;
                update_title();
            }
        }
        else if (key == L'W' && (GetKeyState(VK_CONTROL) & 0x8000)) {
            // Ctrl+W - Close tab
            if (tab_manager_) {
                size_t before = tab_manager_->get_tab_count();
                size_t cur = tab_manager_->get_active_tab_index();
                if (tab_manager_->close_tab(cur)) {
                    size_t newIndex = cur;
                    size_t count = tab_manager_->get_tab_count();
                    if (newIndex >= count) newIndex = (count > 0 ? count - 1 : 0);
                    switch_to_tab(newIndex);
                }
            }
        }
        else if (key == L'W' && (GetKeyState(VK_CONTROL) & 0x8000) && (GetKeyState(VK_SHIFT) & 0x8000)) {
            // Ctrl+Shift+W - Close all tabs
            if (tab_manager_) {
                tab_manager_->close_all_tabs();
                switch_to_tab(0);
                is_modified_ = false;
                update_title();
            }
        }
        
        // Reset cursor blink on any key
        cursor_visible_ = true;
        cursor_blink_time_ = 0;
    }
    
    void on_mouse_wheel(int delta) {
        if (delta > 0) {
            viewport_.scroll_up(3);
        } else {
            viewport_.scroll_down(3);
        }
    }
    
    void load_large_demo_file() {
        std::cout << "\n=== Loading large demo file ===\n";
        std::cout << "Generating 50,000 lines...\n";
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::string large_text;
        for (int i = 0; i < 50000; ++i) {
            large_text += "Line " + std::to_string(i + 1) + 
                         ": This is a performance test. Even with 50k lines, this editor maintains 60fps!\n";
        }
        
        document_ = std::make_shared<PieceTable>(large_text);
        viewport_.set_document(document_);
        cursor_pos_ = 0;
        current_file_ = "";
        is_modified_ = false;
        update_title();
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "File loaded in: " << duration.count() << " ms\n";
        std::cout << "Total lines: " << document_->get_line_count() << "\n";
        std::cout << "Total chars: " << document_->get_total_length() << "\n";
        std::cout << "\nTry scrolling - notice it stays at 60fps!\n";
        std::cout << "This is virtual scrolling + piece table in action!\n\n";
    }
    
    bool open_file() {
        OPENFILENAMEW ofn = {};
        wchar_t filename[MAX_PATH] = L"";
        
        ofn.lStructSize = sizeof(OPENFILENAMEW);
        ofn.hwndOwner = hwnd_;
        ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
        ofn.lpstrFile = filename;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
        ofn.lpstrDefExt = L"txt";
        
        if (GetOpenFileNameW(&ofn)) {
            // Convert wide string to narrow string
            char narrow_filename[MAX_PATH];
            WideCharToMultiByte(CP_UTF8, 0, filename, -1, narrow_filename, MAX_PATH, nullptr, nullptr);
            
            std::cout << "\n=== Opening file ===\n";
            std::cout << "File: " << narrow_filename << "\n";
            
            auto start = std::chrono::high_resolution_clock::now();
            
            // Read file
            std::ifstream file(narrow_filename, std::ios::binary);
            if (!file) {
                MessageBoxW(hwnd_, L"Failed to open file", L"Error", MB_OK | MB_ICONERROR);
                return false;
            }
            
            std::string content((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
            file.close();
            
            // Load into current tab's document
            if (tab_manager_) {
                // Replace current document content by creating a new PieceTable
                document_ = std::make_shared<PieceTable>(content);
                if (auto* tab = tab_manager_->get_active_tab()) {
                    tab->document = document_;
                    tab->file_path = narrow_filename;
                    tab->display_name = narrow_filename;
                    tab->cursor_pos = 0;
                    tab->is_modified = false;
                }
            } else {
                document_ = std::make_shared<PieceTable>(content);
            }
            viewport_.set_document(document_);
            cursor_pos_ = 0;
            current_file_ = narrow_filename;
            is_modified_ = false;
            
            // Add to recent files
            workspace_manager_.add_recent_file(narrow_filename);
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            
            std::cout << "Loaded in: " << duration.count() << " ms\n";
            std::cout << "Lines: " << document_->get_line_count() << "\n";
            std::cout << "Size: " << content.length() << " bytes\n\n";
            
            update_title();
            InvalidateRect(hwnd_, nullptr, TRUE);
            return true;
        }
        
        return false;
    }
    
    bool save_file() {
        std::string filename = current_file_;
        
        // If no current file, show save dialog
        if (filename.empty()) {
            OPENFILENAMEW ofn = {};
            wchar_t wfilename[MAX_PATH] = L"";
            
            ofn.lStructSize = sizeof(OPENFILENAMEW);
            ofn.hwndOwner = hwnd_;
            ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
            ofn.lpstrFile = wfilename;
            ofn.nMaxFile = MAX_PATH;
            ofn.Flags = OFN_OVERWRITEPROMPT;
            ofn.lpstrDefExt = L"txt";
            
            if (!GetSaveFileNameW(&ofn)) {
                return false;
            }
            
            // Convert wide string to narrow string
            char narrow_filename[MAX_PATH];
            WideCharToMultiByte(CP_UTF8, 0, wfilename, -1, narrow_filename, MAX_PATH, nullptr, nullptr);
            filename = narrow_filename;
        }
        
        std::cout << "\n=== Saving file ===\n";
        std::cout << "File: " << filename << "\n";
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Get all text from document
        std::string content = document_->get_text(0, document_->get_total_length());
        
        // Write to file
        std::ofstream file(filename, std::ios::binary);
        if (!file) {
            MessageBoxW(hwnd_, L"Failed to save file", L"Error", MB_OK | MB_ICONERROR);
            return false;
        }
        
        file.write(content.c_str(), content.length());
        file.close();
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "Saved in: " << duration.count() << " ms\n";
        std::cout << "Size: " << content.length() << " bytes\n\n";
        
        current_file_ = filename;
        is_modified_ = false;
        if (tab_manager_) {
            if (auto* tab = tab_manager_->get_active_tab()) {
                tab->file_path = filename;
                tab->display_name = filename;
                tab->is_modified = false;
            }
        }
        
        // Add to recent files
        workspace_manager_.add_recent_file(filename);
        
        update_title();
        InvalidateRect(hwnd_, nullptr, FALSE);
        
        return true;
    }
    
    void update_title() {
        std::wstring title = L"High-Performance Text Editor";
        
        if (!current_file_.empty()) {
            // Convert filename to wide string
            wchar_t wfilename[MAX_PATH];
            MultiByteToWideChar(CP_UTF8, 0, current_file_.c_str(), -1, wfilename, MAX_PATH);
            
            // Extract just the filename
            std::wstring fullpath(wfilename);
            size_t last_slash = fullpath.find_last_of(L"\\/");
            std::wstring filename = (last_slash != std::wstring::npos) 
                ? fullpath.substr(last_slash + 1) : fullpath;
            
            title = filename;
        }
        
        if (is_modified_) {
            title += L" *";
        }
        
        title += L" - High-Performance Text Editor";
        SetWindowTextW(hwnd_, title.c_str());
    }
    
    size_t get_cursor_line() const {
        size_t pos = 0;
        size_t line = 0;
        
        for (size_t i = 0; i < document_->get_line_count(); ++i) {
            size_t line_len = document_->get_line(i).length() + 1; // +1 for newline
            if (pos + line_len > cursor_pos_) {
                return i;
            }
            pos += line_len;
            line = i;
        }
        
        return line;
    }
    
    size_t get_selection_start() const {
        return (std::min)(selection_start_, selection_end_);
    }
    
    size_t get_selection_end() const {
        return (std::max)(selection_start_, selection_end_);
    }
    
    std::string get_selected_text() const {
        if (!has_selection_) return "";
        
        size_t start = get_selection_start();
        size_t end = get_selection_end();
        return document_->get_text(start, end - start);
    }
    
    bool copy_to_clipboard() {
        if (!has_selection_) return false;
        
        std::string text = get_selected_text();
        if (text.empty()) return false;
        
        if (!OpenClipboard(hwnd_)) {
            std::cout << "Failed to open clipboard\n";
            return false;
        }
        
        EmptyClipboard();
        
        // Allocate global memory for the text
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
        if (!hMem) {
            CloseClipboard();
            return false;
        }
        
        // Copy text to allocated memory
        char* pMem = static_cast<char*>(GlobalLock(hMem));
        memcpy(pMem, text.c_str(), text.size() + 1);
        GlobalUnlock(hMem);
        
        // Set clipboard data
        SetClipboardData(CF_TEXT, hMem);
        CloseClipboard();
        
        std::cout << "Copied " << text.size() << " characters to clipboard\n";
        return true;
    }
    
    void paste_from_clipboard() {
        if (!OpenClipboard(hwnd_)) {
            std::cout << "Failed to open clipboard\n";
            return;
        }
        
        HANDLE hData = GetClipboardData(CF_TEXT);
        if (!hData) {
            CloseClipboard();
            return;
        }
        
        char* pText = static_cast<char*>(GlobalLock(hData));
        if (pText) {
            std::string text(pText);
            GlobalUnlock(hData);
            
            // Delete selection if any
            if (has_selection_) {
                delete_selection();
            }
            
            // Insert text at cursor
            auto cmd = std::make_unique<InsertCommand>(document_.get(), cursor_pos_, text);
            undo_manager_->execute(std::move(cmd));
            cursor_pos_ += text.length();
            is_modified_ = true;
            update_title();
            
            std::cout << "Pasted " << text.size() << " characters from clipboard\n";
        }
        
        CloseClipboard();
    }
    
    void delete_selection() {
        if (!has_selection_) return;
        
        size_t start = get_selection_start();
        size_t end = get_selection_end();
        size_t length = end - start;
        
        auto cmd = std::make_unique<DeleteCommand>(document_.get(), start, length);
        undo_manager_->execute(std::move(cmd));
        
        cursor_pos_ = start;
        has_selection_ = false;
        is_modified_ = true;
        update_title();
    }
    
    size_t get_cursor_column() const {
        size_t pos = 0;
        
        for (size_t i = 0; i < document_->get_line_count(); ++i) {
            std::string line = document_->get_line(i);
            size_t line_len = line.length() + 1; // +1 for newline
            
            if (pos + line_len > cursor_pos_) {
                return cursor_pos_ - pos;
            }
            pos += line_len;
        }
        
        return 0;
    }
    
    void find_next() {
        if (find_text_.empty()) return;
        
        std::string doc_text;
        for (size_t i = 0; i < document_->get_line_count(); ++i) {
            doc_text += document_->get_line(i) + "\n";
        }
        
        SearchMatch match;
        if (find_dialog_->find_next(doc_text, find_text_, cursor_pos_ + 1, match)) {
            cursor_pos_ = match.position;
            viewport_.scroll_to_line(match.line);
        }
    }
    
    void find_previous() {
        if (find_text_.empty()) return;
        
        std::string doc_text;
        for (size_t i = 0; i < document_->get_line_count(); ++i) {
            doc_text += document_->get_line(i) + "\n";
        }
        
        SearchMatch match;
        if (find_dialog_->find_previous(doc_text, find_text_, cursor_pos_, match)) {
            cursor_pos_ = match.position;
            viewport_.scroll_to_line(match.line);
        }
    }
    
    void perform_find() {
        if (find_text_.empty()) return;
        
        std::string doc_text;
        for (size_t i = 0; i < document_->get_line_count(); ++i) {
            doc_text += document_->get_line(i) + "\n";
        }
        
        auto matches = find_dialog_->find_all(doc_text, find_text_);
        find_dialog_->set_matches(matches);
        
        if (find_dialog_->has_matches()) {
            const SearchMatch* match = find_dialog_->get_current_match();
            if (match) {
                cursor_pos_ = match->position;
                viewport_.scroll_to_line(match->line);
            }
        }
    }
    
    // Workspace management functions
    void save_workspace_state() {
        if (current_workspace_dir_.empty()) {
            // Use current working directory
            char cwd[MAX_PATH] = {0};
            GetCurrentDirectoryA(MAX_PATH, cwd);
            current_workspace_dir_ = cwd;
        }
        
        WorkspaceState state;
        state.root_directory = current_workspace_dir_;
        
        // Save all open tabs
        if (tab_manager_) {
            const auto& tabs = tab_manager_->get_all_tabs();
            for (size_t i = 0; i < tabs.size(); ++i) {
                if (!tabs[i].file_path.empty()) {
                    FileState fs;
                    fs.path = tabs[i].file_path;
                    fs.cursor_pos = tabs[i].cursor_pos;
                    fs.scroll_offset = viewport_.get_top_line();
                    state.open_files.push_back(fs);
                }
            }
            state.active_tab_index = tab_manager_->get_active_tab_index();
        }
        
        workspace_manager_.save_workspace(state, current_workspace_dir_);
    }
    
    void load_workspace_state() {
        if (current_workspace_dir_.empty()) {
            // Use current working directory
            char cwd[MAX_PATH] = {0};
            GetCurrentDirectoryA(MAX_PATH, cwd);
            current_workspace_dir_ = cwd;
        }
        
        WorkspaceState state;
        if (workspace_manager_.load_workspace(current_workspace_dir_, state)) {
            // Close all tabs first
            if (tab_manager_) {
                tab_manager_->close_all_tabs();
            }
            
            // Load each file into a tab
            for (const auto& file_state : state.open_files) {
                if (fs::exists(file_state.path)) {
                    std::ifstream file(file_state.path, std::ios::binary);
                    if (file) {
                        std::string content((std::istreambuf_iterator<char>(file)),
                                          std::istreambuf_iterator<char>());
                        file.close();
                        
                        auto doc = std::make_shared<PieceTable>(content);
                        if (tab_manager_) {
                            tab_manager_->new_tab(content, file_state.path);
                            if (auto* tab = tab_manager_->get_active_tab()) {
                                tab->cursor_pos = file_state.cursor_pos;
                            }
                        }
                    }
                }
            }
            
            // Switch to the previously active tab
            if (tab_manager_ && state.active_tab_index < tab_manager_->get_all_tabs().size()) {
                switch_to_tab(state.active_tab_index);
            }
        }
    }
    
    void show_recent_files_dialog() {
        const auto& recent_files = workspace_manager_.get_recent_files();
        
        if (recent_files.empty()) {
            MessageBoxW(hwnd_, L"No recent files", L"Recent Files", MB_OK | MB_ICONINFORMATION);
            return;
        }
        
        // Create a simple listbox dialog
        HWND dialog = CreateWindowExW(
            WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
            L"STATIC",
            L"Recent Files (Ctrl+R)",
            WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU,
            100, 100, 600, 400,
            hwnd_, nullptr, hInstance_, nullptr
        );
        
        if (!dialog) return;
        
        // Create listbox
        HWND listbox = CreateWindowExW(
            0,
            L"LISTBOX",
            L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_HASSTRINGS,
            10, 10, 570, 340,
            dialog, (HMENU)1001, hInstance_, nullptr
        );
        
        // Set font
        SendMessageW(listbox, WM_SETFONT, (WPARAM)hFont_, TRUE);
        
        // Populate with recent files
        for (const auto& filepath : recent_files) {
            wchar_t wpath[MAX_PATH];
            MultiByteToWideChar(CP_UTF8, 0, filepath.c_str(), -1, wpath, MAX_PATH);
            SendMessageW(listbox, LB_ADDSTRING, 0, (LPARAM)wpath);
        }
        
        // Create OK and Cancel buttons
        HWND btn_ok = CreateWindowExW(
            0, L"BUTTON", L"Open",
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            400, 360, 80, 25,
            dialog, (HMENU)IDOK, hInstance_, nullptr
        );
        
        HWND btn_cancel = CreateWindowExW(
            0, L"BUTTON", L"Cancel",
            WS_CHILD | WS_VISIBLE,
            490, 360, 80, 25,
            dialog, (HMENU)IDCANCEL, hInstance_, nullptr
        );
        
        SendMessageW(btn_ok, WM_SETFONT, (WPARAM)hFont_, TRUE);
        SendMessageW(btn_cancel, WM_SETFONT, (WPARAM)hFont_, TRUE);
        
        // Modal loop
        MSG msg;
        bool done = false;
        int selected_index = -1;
        
        while (!done && GetMessage(&msg, nullptr, 0, 0)) {
            if (msg.message == WM_COMMAND) {
                if (LOWORD(msg.wParam) == IDOK) {
                    selected_index = (int)SendMessageW(listbox, LB_GETCURSEL, 0, 0);
                    done = true;
                } else if (LOWORD(msg.wParam) == IDCANCEL) {
                    done = true;
                } else if (HIWORD(msg.wParam) == LBN_DBLCLK && LOWORD(msg.wParam) == 1001) {
                    selected_index = (int)SendMessageW(listbox, LB_GETCURSEL, 0, 0);
                    done = true;
                }
            } else if (msg.message == WM_CLOSE && msg.hwnd == dialog) {
                done = true;
            }
            
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        DestroyWindow(dialog);
        
        // Open the selected file
        if (selected_index >= 0 && selected_index < (int)recent_files.size()) {
            const std::string& filepath = recent_files[selected_index];
            open_file_from_path(filepath);
        }
    }
    
    HINSTANCE hInstance_;
    HWND hwnd_ = nullptr;
    HFONT hFont_ = nullptr;
    HIMAGELIST tree_images_ = nullptr;
    HWND tree_hwnd_ = nullptr;
    FileTree file_tree_;
    // Tree drag & drop state
    bool dragging_tree_ = false;
    HIMAGELIST tree_drag_img_ = nullptr;
    HTREEITEM tree_drag_item_ = nullptr;
    HTREEITEM tree_hover_item_ = nullptr;
    // Tab bar state
    bool dragging_tab_ = false;
    size_t drag_tab_index_ = static_cast<size_t>(-1);
    size_t hover_tab_index_ = static_cast<size_t>(-1);
    int tab_scroll_offset_ = 0;
    
    std::shared_ptr<PieceTable> document_;
    Viewport viewport_;
    std::unique_ptr<UndoManager> undo_manager_;
    std::unique_ptr<FindDialog> find_dialog_;
    std::unique_ptr<SyntaxHighlighter> highlighter_;
    
    bool show_file_tree_;
    int tree_panel_width_;
    bool show_find_;
    bool show_replace_;
    std::string find_text_;
    std::string replace_text_;
    
    size_t cursor_pos_ = 0;
    bool show_stats_;
    bool show_line_numbers_;
    bool relative_line_numbers_;
    bool replace_edit_find_;
    
    double fps_ = 60.0;
    double last_frame_time_;
    
    std::string current_file_;
    bool is_modified_;
    
    bool cursor_visible_;
    int cursor_blink_time_;
    
    bool has_selection_;
    size_t selection_start_;
    size_t selection_end_;
    
    int char_width_;   // Actual character width from font metrics
    int char_height_;  // Actual character height from font metrics
    
    // Multi-cursor support
    std::vector<size_t> extra_cursors_;  // Additional cursor positions
    bool multi_cursor_mode_ = false;
    
    // Workspace management
    WorkspaceManager workspace_manager_;
    std::string current_workspace_dir_;

    // Project-wide search
    struct ProjectSearchResult {
        std::string file_path;
        size_t line = 0;     // 0-based
        size_t column = 0;   // 0-based
        std::string line_text;
    };

    bool show_project_search_ = false;
    std::string project_search_query_;
    std::string project_replace_query_;
    std::string project_include_patterns_ = "*.cpp;*.hpp;*.h;*.c;*.txt;*.md";
    std::string project_exclude_patterns_ = ".git;build;node_modules;*.exe;*.dll;*.obj;*.pdb";
    int project_search_focus_ = 0; // 0=query,1=replace,2=include,3=exclude,4=results
    int results_panel_height_ = 260;
    std::atomic<bool> project_search_in_progress_{false};
    std::future<void> project_search_future_;
    std::vector<ProjectSearchResult> project_results_;
    std::mutex project_results_mutex_;
    int selected_result_index_ = -1;

    struct ResultRow { // layout helper for click mapping
        bool is_header = false;
        std::string file_path;
        int result_index = -1; // index into project_results_ if not header
    };
    std::vector<ResultRow> result_rows_layout_;

    void setup_tree_image_list() {
        if (!tree_hwnd_) return;
        if (tree_images_) {
            ImageList_Destroy(tree_images_);
            tree_images_ = nullptr;
        }
        tree_images_ = ImageList_Create(16, 16, ILC_COLOR32, 6, 6);
        auto add_swatch = [&](COLORREF color) {
            HDC hdc = GetDC(hwnd_);
            HDC mem = CreateCompatibleDC(hdc);
            HBITMAP bmp = CreateCompatibleBitmap(hdc, 16, 16);
            HBITMAP old = (HBITMAP)SelectObject(mem, bmp);
            HBRUSH br = CreateSolidBrush(color);
            RECT r{0,0,16,16};
            FillRect(mem, &r, br);
            DeleteObject(br);
            SelectObject(mem, old);
            DeleteDC(mem);
            ReleaseDC(hwnd_, hdc);
            int idx = ImageList_Add(tree_images_, bmp, nullptr);
            DeleteObject(bmp);
            return idx;
        };
        // 0: folder, 1: default, 2: cpp, 3: header, 4: txt/md, 5: json/xml
        add_swatch(RGB(240, 200, 100)); // folder
        add_swatch(RGB(160, 160, 170)); // default
        add_swatch(RGB(80, 160, 255));  // cpp
        add_swatch(RGB(120, 200, 255)); // header
        add_swatch(RGB(180, 220, 160)); // text
        add_swatch(RGB(255, 180, 120)); // json/xml
        TreeView_SetImageList(tree_hwnd_, tree_images_, TVSIL_NORMAL);
    }

    void handle_tree_context_command(int cmd, TreeNode* node) {
        namespace fs = std::filesystem;
        auto parent_dir = [&](const std::string& p) {
            fs::path fp(p);
            if (fs::is_directory(fp)) return fp;
            return fp.parent_path();
        };

        if (cmd == 1) { // New File
            fs::path dir = parent_dir(node->full_path);
            fs::path base = dir / "New File.txt";
            fs::path candidate = base;
            int i = 1;
            while (fs::exists(candidate)) {
                candidate = dir / ("New File (" + std::to_string(i++) + ").txt");
            }
            std::ofstream(candidate.string()).close();
            file_tree_.reload();
        } else if (cmd == 2) { // New Folder
            fs::path dir = parent_dir(node->full_path);
            fs::path base = dir / "New Folder";
            fs::path candidate = base;
            int i = 1;
            while (fs::exists(candidate)) {
                candidate = dir / ("New Folder (" + std::to_string(i++) + ")");
            }
            fs::create_directory(candidate);
            file_tree_.reload();
        } else if (cmd == 3) { // Delete
            std::wstring wpath;
            for (char c : node->full_path) wpath += static_cast<wchar_t>(c);
            std::wstring msg = L"Delete \"" + wpath + L"\"?";
            if (MessageBoxW(hwnd_, msg.c_str(), L"Confirm Delete", MB_YESNO | MB_ICONWARNING) == IDYES) {
                try {
                    if (std::filesystem::is_directory(node->full_path)) {
                        std::filesystem::remove_all(node->full_path);
                    } else {
                        std::filesystem::remove(node->full_path);
                    }
                } catch (...) {}
                file_tree_.reload();
            }
        }
    }

    void open_file_from_path(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) return;
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        
        // Add to recent files
        workspace_manager_.add_recent_file(path);
        
        if (tab_manager_) {
            size_t idx = tab_manager_->new_tab(content, path);
            switch_to_tab(idx);
            is_modified_ = false;
            update_title();
        } else {
            document_ = std::make_shared<PieceTable>(content);
            viewport_.set_document(document_);
            current_file_ = path;
            is_modified_ = false;
            update_title();
        }
        InvalidateRect(hwnd_, nullptr, TRUE);
    }
    
    static bool is_descendant_path(const std::string& src, const std::string& dst) {
        std::filesystem::path s(src);
        std::filesystem::path d(dst);
        std::error_code ec;
        auto sp = std::filesystem::weakly_canonical(s, ec);
        auto dp = std::filesystem::weakly_canonical(d, ec);
        if (ec) return false;
        auto spstr = sp.string();
        auto dpstr = dp.string();
        auto lower = [](std::string x){ for (auto& c: x) c = (char)tolower((unsigned char)c); return x; };
        spstr = lower(spstr);
        dpstr = lower(dpstr);
        if (dpstr.size() < spstr.size()) return false;
        if (dpstr.compare(0, spstr.size(), spstr) != 0) return false;
        if (dpstr.size() == spstr.size()) return true;
        char next = dpstr[spstr.size()];
        return next == '\\' || next == '/';
    }

    void attempt_move_node(const std::string& src_path, const std::string& dest_dir) {
        using namespace std::filesystem;
        try {
            path sp(src_path);
            path dd(dest_dir);
            if (!exists(sp) || !exists(dd) || !is_directory(dd)) return;
            if (is_descendant_path(src_path, dest_dir)) return; // prevent moving into self/descendant
            path new_path = dd / sp.filename();
            if (equivalent(sp.parent_path(), dd)) return; // no-op
            if (exists(new_path)) {
                std::string base = sp.stem().string();
                std::string ext = sp.has_extension() ? sp.extension().string() : std::string();
                int i = 1;
                do {
                    new_path = dd / (base + " (" + std::to_string(i++) + ")" + ext);
                } while (exists(new_path));
            }
            rename(sp, new_path);
            file_tree_.reload();
        } catch (...) {
            MessageBoxW(hwnd_, L"Failed to move item.", L"Drag & Drop", MB_OK | MB_ICONERROR);
        }
    }
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    // Allocate console for debug output
    AllocConsole();
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    
    std::cout << "=================================================\n";
    std::cout << "  HIGH-PERFORMANCE TEXT EDITOR - Win32 GUI\n";
    std::cout << "=================================================\n\n";
    std::cout << "Architecture:\n";
    std::cout << "- Piece Table for O(1) edits\n";
    std::cout << "- Virtual scrolling (only visible lines)\n";
    std::cout << "- Native Win32 rendering (no web tech)\n";
    std::cout << "- Hardware-accelerated GDI\n\n";
    std::cout << "Opening editor window...\n\n";
    std::cout << "Press Ctrl+L in the editor to load 50k line demo!\n";
    std::cout << "Watch how it stays at 60fps even with huge files.\n\n";
    
    Win32TextEditor editor(hInstance);
    
    if (!editor.create_window()) {
        MessageBoxW(nullptr, L"Failed to create window", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    return editor.run();
}

