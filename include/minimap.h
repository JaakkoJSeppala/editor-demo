#pragma once

#include <windows.h>
#include <vector>
#include <string>
#include <algorithm>

/**
 * Minimap - Document Overview
 * 
 * Provides a bird's-eye view of the entire document on the right side
 * with syntax color preview, visible region indicator, and click-to-scroll.
 */

class Minimap {
public:
    Minimap() 
        : visible_(true)
        , width_(120)
        , char_width_(1)
        , char_height_(2)
        , max_chars_per_line_(80)
    {}
    
    void set_visible(bool visible) { visible_ = visible; }
    bool is_visible() const { return visible_; }
    
    void set_width(int width) { width_ = width; }
    int get_width() const { return visible_ ? width_ : 0; }
    
    void set_char_dimensions(int width, int height) {
        char_width_ = width;
        char_height_ = height;
    }
    
    // Render the minimap
    void render(HDC hdc, const RECT& area, const std::vector<std::string>& lines, 
                size_t top_line, size_t visible_line_count, 
                const std::vector<COLORREF>& syntax_colors) {
        if (!visible_) return;
        
        // Draw background
        HBRUSH bgBrush = CreateSolidBrush(RGB(25, 25, 30));
        FillRect(hdc, &area, bgBrush);
        DeleteObject(bgBrush);
        
        // Calculate minimap metrics
        int map_height = area.bottom - area.top;
        int total_lines = lines.size();
        if (total_lines == 0) return;
        
        // Scale factor for fitting all lines
        float line_scale = (float)map_height / (float)total_lines;
        
        // Render document preview
        for (size_t i = 0; i < lines.size(); ++i) {
            int y = area.top + (int)(i * line_scale);
            if (y >= area.bottom) break;
            
            render_line_preview(hdc, area.left, y, area.right - area.left, 
                              lines[i], i < syntax_colors.size() ? syntax_colors[i] : RGB(180, 180, 180));
        }
        
        // Draw visible region indicator
        int visible_start_y = area.top + (int)(top_line * line_scale);
        int visible_height = (int)(visible_line_count * line_scale);
        if (visible_height < 3) visible_height = 3; // Minimum height for visibility
        
        RECT visible_rect = {
            area.left,
            visible_start_y,
            area.right,
            visible_start_y + visible_height
        };
        
        // Semi-transparent overlay
        HBRUSH visibleBrush = CreateSolidBrush(RGB(80, 120, 200));
        HPEN visiblePen = CreatePen(PS_SOLID, 1, RGB(100, 150, 255));
        HPEN oldPen = (HPEN)SelectObject(hdc, visiblePen);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        
        // Draw border
        Rectangle(hdc, visible_rect.left, visible_rect.top, visible_rect.right, visible_rect.bottom);
        
        // Draw semi-transparent fill (simulate with dithered pattern)
        for (int y = visible_rect.top; y < visible_rect.bottom; y += 2) {
            for (int x = visible_rect.left; x < visible_rect.right; x += 2) {
                SetPixel(hdc, x, y, RGB(80, 120, 200));
            }
        }
        
        SelectObject(hdc, oldPen);
        SelectObject(hdc, oldBrush);
        DeleteObject(visiblePen);
        DeleteObject(visibleBrush);
        
        // Draw border around minimap
        HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(60, 60, 70));
        oldPen = (HPEN)SelectObject(hdc, borderPen);
        MoveToEx(hdc, area.left, area.top, nullptr);
        LineTo(hdc, area.left, area.bottom);
        SelectObject(hdc, oldPen);
        DeleteObject(borderPen);
    }
    
    // Handle click on minimap - returns line number to scroll to
    size_t handle_click(int y, const RECT& area, size_t total_lines) {
        if (!visible_ || total_lines == 0) return 0;
        
        int map_height = area.bottom - area.top;
        int relative_y = y - area.top;
        if (relative_y < 0) relative_y = 0;
        if (relative_y >= map_height) relative_y = map_height - 1;
        
        // Calculate line from click position
        float line_scale = (float)map_height / (float)total_lines;
        size_t clicked_line = (size_t)(relative_y / line_scale);
        
        if (clicked_line >= total_lines) {
            clicked_line = total_lines > 0 ? total_lines - 1 : 0;
        }
        
        return clicked_line;
    }
    
    // Check if point is in minimap area
    bool is_point_in_minimap(int x, int y, const RECT& area) const {
        if (!visible_) return false;
        return x >= area.left && x <= area.right && y >= area.top && y <= area.bottom;
    }
    
private:
    bool visible_;
    int width_;
    int char_width_;
    int char_height_;
    int max_chars_per_line_;
    
    void render_line_preview(HDC hdc, int x, int y, int width, 
                            const std::string& line, COLORREF color) {
        if (line.empty()) return;
        
        // Simplified rendering - just draw a colored horizontal line representing code
        int line_length = (std::min)((int)line.size(), max_chars_per_line_);
        int preview_width = (line_length * width) / max_chars_per_line_;
        if (preview_width < 1) preview_width = 1;
        
        // Determine color intensity based on line content
        int non_space_count = 0;
        for (char c : line) {
            if (c != ' ' && c != '\t') non_space_count++;
        }
        
        if (non_space_count > 0) {
            HPEN linePen = CreatePen(PS_SOLID, 1, color);
            HPEN oldPen = (HPEN)SelectObject(hdc, linePen);
            
            MoveToEx(hdc, x + 2, y, nullptr);
            LineTo(hdc, x + 2 + preview_width, y);
            
            SelectObject(hdc, oldPen);
            DeleteObject(linePen);
        }
    }
};
