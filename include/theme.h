#pragma once

#include <string>
#include <unordered_map>
#include <windows.h>

/**
 * Theme - Color scheme manager for editor UI
 * 
 * Supports JSON-based theme definitions with color token mapping.
 * Provides built-in themes: Dark+, Light+, Monokai
 */
class Theme {
public:
    struct ColorScheme {
        // Editor colors
        COLORREF background;
        COLORREF foreground;
        COLORREF selection;
        COLORREF line_highlight;
        COLORREF cursor;
        
        // Gutter colors
        COLORREF gutter_background;
        COLORREF line_number;
        COLORREF line_number_active;
        
        // Syntax colors
        COLORREF keyword;
        COLORREF string;
        COLORREF comment;
        COLORREF number;
        COLORREF function;
        COLORREF type;
        COLORREF variable;
        COLORREF operator_color;
        
        // UI colors
        COLORREF tab_active;
        COLORREF tab_inactive;
        COLORREF tab_border;
        COLORREF status_bar;
        COLORREF minimap_background;
        
        // Terminal colors
        COLORREF terminal_background;
        COLORREF terminal_foreground;
        COLORREF terminal_cursor;
        
        // Git colors
        COLORREF git_added;
        COLORREF git_modified;
        COLORREF git_deleted;
        COLORREF git_untracked;
        
        // Diagnostic colors
        COLORREF error;
        COLORREF warning;
        COLORREF info;
    };
    
    Theme();
    ~Theme() = default;
    
    // Theme management
    void load_theme(const std::string& name);
    void load_from_json(const std::string& json_path);
    bool save_theme(const std::string& name, const std::string& json_path);
    
    // Getters
    const ColorScheme& get_colors() const { return current_scheme_; }
    std::string get_current_theme() const { return current_theme_name_; }
    std::vector<std::string> get_available_themes() const;
    
    // Built-in themes
    static ColorScheme create_dark_plus();
    static ColorScheme create_light_plus();
    static ColorScheme create_monokai();
    
private:
    ColorScheme current_scheme_;
    std::string current_theme_name_;
    std::unordered_map<std::string, ColorScheme> built_in_themes_;
    
    void initialize_built_in_themes();
    COLORREF parse_hex_color(const std::string& hex);
    std::string color_to_hex(COLORREF color);
};
