#include "theme.h"
#include <fstream>
#include <sstream>
#include <algorithm>

Theme::Theme() : current_theme_name_("Dark+") {
    initialize_built_in_themes();
    current_scheme_ = built_in_themes_["Dark+"];
}

void Theme::initialize_built_in_themes() {
    built_in_themes_["Dark+"] = create_dark_plus();
    built_in_themes_["Light+"] = create_light_plus();
    built_in_themes_["Monokai"] = create_monokai();
}

void Theme::load_theme(const std::string& name) {
    auto it = built_in_themes_.find(name);
    if (it != built_in_themes_.end()) {
        current_scheme_ = it->second;
        current_theme_name_ = name;
    }
}

std::vector<std::string> Theme::get_available_themes() const {
    std::vector<std::string> themes;
    for (const auto& pair : built_in_themes_) {
        themes.push_back(pair.first);
    }
    return themes;
}

Theme::ColorScheme Theme::create_dark_plus() {
    ColorScheme scheme;
    
    // Editor colors
    scheme.background = RGB(30, 30, 35);
    scheme.foreground = RGB(212, 212, 212);
    scheme.selection = RGB(38, 79, 120);
    scheme.line_highlight = RGB(45, 45, 60);
    scheme.cursor = RGB(255, 255, 255);
    
    // Gutter colors
    scheme.gutter_background = RGB(35, 35, 45);
    scheme.line_number = RGB(100, 100, 120);
    scheme.line_number_active = RGB(180, 180, 200);
    
    // Syntax colors
    scheme.keyword = RGB(86, 156, 214);          // Blue
    scheme.string = RGB(206, 145, 120);          // Orange
    scheme.comment = RGB(106, 153, 85);          // Green
    scheme.number = RGB(181, 206, 168);          // Light green
    scheme.function = RGB(220, 220, 170);        // Yellow
    scheme.type = RGB(78, 201, 176);             // Cyan
    scheme.variable = RGB(156, 220, 254);        // Light blue
    scheme.operator_color = RGB(212, 212, 212);  // White
    
    // UI colors
    scheme.tab_active = RGB(50, 50, 60);
    scheme.tab_inactive = RGB(35, 35, 40);
    scheme.tab_border = RGB(60, 60, 70);
    scheme.status_bar = RGB(20, 20, 25);
    scheme.minimap_background = RGB(25, 25, 30);
    
    // Terminal colors
    scheme.terminal_background = RGB(20, 20, 25);
    scheme.terminal_foreground = RGB(204, 204, 204);
    scheme.terminal_cursor = RGB(255, 255, 255);
    
    // Git colors
    scheme.git_added = RGB(100, 255, 100);
    scheme.git_modified = RGB(255, 200, 0);
    scheme.git_deleted = RGB(255, 100, 100);
    scheme.git_untracked = RGB(150, 150, 255);
    
    // Diagnostic colors
    scheme.error = RGB(255, 80, 80);
    scheme.warning = RGB(255, 180, 0);
    scheme.info = RGB(100, 150, 255);
    
    return scheme;
}

Theme::ColorScheme Theme::create_light_plus() {
    ColorScheme scheme;
    
    // Editor colors
    scheme.background = RGB(255, 255, 255);
    scheme.foreground = RGB(0, 0, 0);
    scheme.selection = RGB(173, 214, 255);
    scheme.line_highlight = RGB(245, 245, 245);
    scheme.cursor = RGB(0, 0, 0);
    
    // Gutter colors
    scheme.gutter_background = RGB(248, 248, 248);
    scheme.line_number = RGB(150, 150, 150);
    scheme.line_number_active = RGB(80, 80, 80);
    
    // Syntax colors
    scheme.keyword = RGB(0, 0, 255);             // Blue
    scheme.string = RGB(163, 21, 21);            // Dark red
    scheme.comment = RGB(0, 128, 0);             // Green
    scheme.number = RGB(9, 134, 88);             // Teal
    scheme.function = RGB(121, 94, 38);          // Brown
    scheme.type = RGB(38, 127, 153);             // Cyan
    scheme.variable = RGB(1, 1, 129);            // Dark blue
    scheme.operator_color = RGB(0, 0, 0);        // Black
    
    // UI colors
    scheme.tab_active = RGB(240, 240, 240);
    scheme.tab_inactive = RGB(250, 250, 250);
    scheme.tab_border = RGB(200, 200, 200);
    scheme.status_bar = RGB(230, 230, 230);
    scheme.minimap_background = RGB(245, 245, 245);
    
    // Terminal colors
    scheme.terminal_background = RGB(250, 250, 250);
    scheme.terminal_foreground = RGB(0, 0, 0);
    scheme.terminal_cursor = RGB(0, 0, 0);
    
    // Git colors
    scheme.git_added = RGB(0, 180, 0);
    scheme.git_modified = RGB(200, 140, 0);
    scheme.git_deleted = RGB(200, 0, 0);
    scheme.git_untracked = RGB(80, 80, 200);
    
    // Diagnostic colors
    scheme.error = RGB(255, 0, 0);
    scheme.warning = RGB(255, 140, 0);
    scheme.info = RGB(0, 100, 255);
    
    return scheme;
}

Theme::ColorScheme Theme::create_monokai() {
    ColorScheme scheme;
    
    // Editor colors
    scheme.background = RGB(39, 40, 34);
    scheme.foreground = RGB(248, 248, 242);
    scheme.selection = RGB(73, 72, 62);
    scheme.line_highlight = RGB(58, 61, 50);
    scheme.cursor = RGB(248, 248, 240);
    
    // Gutter colors
    scheme.gutter_background = RGB(46, 47, 41);
    scheme.line_number = RGB(144, 145, 148);
    scheme.line_number_active = RGB(200, 200, 200);
    
    // Syntax colors
    scheme.keyword = RGB(249, 38, 114);          // Pink
    scheme.string = RGB(230, 219, 116);          // Yellow
    scheme.comment = RGB(117, 113, 94);          // Gray
    scheme.number = RGB(174, 129, 255);          // Purple
    scheme.function = RGB(166, 226, 46);         // Green
    scheme.type = RGB(102, 217, 239);            // Cyan
    scheme.variable = RGB(253, 151, 31);         // Orange
    scheme.operator_color = RGB(249, 38, 114);   // Pink
    
    // UI colors
    scheme.tab_active = RGB(58, 61, 50);
    scheme.tab_inactive = RGB(46, 47, 41);
    scheme.tab_border = RGB(80, 80, 70);
    scheme.status_bar = RGB(35, 36, 31);
    scheme.minimap_background = RGB(35, 36, 31);
    
    // Terminal colors
    scheme.terminal_background = RGB(35, 36, 31);
    scheme.terminal_foreground = RGB(248, 248, 242);
    scheme.terminal_cursor = RGB(248, 248, 240);
    
    // Git colors
    scheme.git_added = RGB(166, 226, 46);
    scheme.git_modified = RGB(230, 219, 116);
    scheme.git_deleted = RGB(249, 38, 114);
    scheme.git_untracked = RGB(102, 217, 239);
    
    // Diagnostic colors
    scheme.error = RGB(249, 38, 114);
    scheme.warning = RGB(253, 151, 31);
    scheme.info = RGB(102, 217, 239);
    
    return scheme;
}

COLORREF Theme::parse_hex_color(const std::string& hex) {
    // Parse #RRGGBB format
    if (hex.length() != 7 || hex[0] != '#') {
        return RGB(0, 0, 0);
    }
    
    unsigned int r, g, b;
    sscanf_s(hex.c_str() + 1, "%02x%02x%02x", &r, &g, &b);
    return RGB(r, g, b);
}

std::string Theme::color_to_hex(COLORREF color) {
    char buf[8];
    sprintf_s(buf, sizeof(buf), "#%02X%02X%02X", 
              GetRValue(color), GetGValue(color), GetBValue(color));
    return std::string(buf);
}

void Theme::load_from_json(const std::string& json_path) {
    // TODO: Implement JSON parsing
    // For now, just load Dark+ as default
    (void)json_path;
    load_theme("Dark+");
}

bool Theme::save_theme(const std::string& name, const std::string& json_path) {
    // TODO: Implement JSON serialization
    (void)name;
    (void)json_path;
    return false;
}
