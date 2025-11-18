#pragma once

#include <vector>
#include <map>
#include <string>
#include <algorithm>

/**
 * Code Folding Manager
 * 
 * Detects foldable regions based on braces and indentation,
 * manages fold/unfold state, and provides visual controls.
 */

struct FoldRegion {
    size_t start_line = 0;      // Starting line (inclusive)
    size_t end_line = 0;        // Ending line (inclusive)
    bool is_folded = false;     // Current fold state
    int indent_level = 0;       // Indentation level
    
    FoldRegion() = default;
    FoldRegion(size_t start, size_t end, int indent = 0) 
        : start_line(start), end_line(end), indent_level(indent) {}
    
    bool contains_line(size_t line) const {
        return line >= start_line && line <= end_line;
    }
    
    size_t line_count() const {
        return end_line >= start_line ? (end_line - start_line + 1) : 0;
    }
};

class CodeFoldingManager {
public:
    CodeFoldingManager() = default;
    
    // Analyze document and detect foldable regions
    void analyze_document(const std::vector<std::string>& lines) {
        regions_.clear();
        
        // Detect brace-based regions (C++, JavaScript, etc.)
        detect_brace_regions(lines);
        
        // Detect indentation-based regions (Python, YAML, etc.)
        // detect_indent_regions(lines);
    }
    
    // Toggle fold state for region at line
    bool toggle_fold(size_t line) {
        for (auto& region : regions_) {
            if (region.start_line == line) {
                region.is_folded = !region.is_folded;
                return true;
            }
        }
        return false;
    }
    
    // Fold region at line
    void fold(size_t line) {
        for (auto& region : regions_) {
            if (region.start_line == line) {
                region.is_folded = true;
                return;
            }
        }
    }
    
    // Unfold region at line
    void unfold(size_t line) {
        for (auto& region : regions_) {
            if (region.start_line == line) {
                region.is_folded = false;
                return;
            }
        }
    }
    
    // Fold all regions
    void fold_all() {
        for (auto& region : regions_) {
            region.is_folded = true;
        }
    }
    
    // Unfold all regions
    void unfold_all() {
        for (auto& region : regions_) {
            region.is_folded = false;
        }
    }
    
    // Check if a line should be visible (not folded)
    bool is_line_visible(size_t line) const {
        for (const auto& region : regions_) {
            if (region.is_folded && line > region.start_line && line <= region.end_line) {
                return false;
            }
        }
        return true;
    }
    
    // Get foldable region at line (if exists)
    const FoldRegion* get_region_at_line(size_t line) const {
        for (const auto& region : regions_) {
            if (region.start_line == line) {
                return &region;
            }
        }
        return nullptr;
    }
    
    // Get all regions
    const std::vector<FoldRegion>& get_regions() const {
        return regions_;
    }
    
    // Get visible line indices (after folding)
    std::vector<size_t> get_visible_lines(size_t total_lines) const {
        std::vector<size_t> visible;
        for (size_t i = 0; i < total_lines; ++i) {
            if (is_line_visible(i)) {
                visible.push_back(i);
            }
        }
        return visible;
    }
    
    // Save fold state
    std::map<size_t, bool> get_fold_state() const {
        std::map<size_t, bool> state;
        for (const auto& region : regions_) {
            state[region.start_line] = region.is_folded;
        }
        return state;
    }
    
    // Restore fold state
    void restore_fold_state(const std::map<size_t, bool>& state) {
        for (auto& region : regions_) {
            auto it = state.find(region.start_line);
            if (it != state.end()) {
                region.is_folded = it->second;
            }
        }
    }
    
private:
    std::vector<FoldRegion> regions_;
    
    void detect_brace_regions(const std::vector<std::string>& lines) {
        std::vector<size_t> brace_stack;  // Stack of opening brace line numbers
        std::vector<int> indent_stack;    // Stack of indentation levels
        
        for (size_t i = 0; i < lines.size(); ++i) {
            const std::string& line = lines[i];
            
            // Calculate indentation
            int indent = 0;
            for (char c : line) {
                if (c == ' ') indent++;
                else if (c == '\t') indent += 4;
                else break;
            }
            
            // Skip empty lines and comments
            std::string trimmed = trim(line);
            if (trimmed.empty() || (trimmed.length() >= 2 && trimmed[0] == '/' && trimmed[1] == '/')) {
                continue;
            }
            
            // Look for opening braces
            size_t open_pos = trimmed.find('{');
            if (open_pos != std::string::npos) {
                brace_stack.push_back(i);
                indent_stack.push_back(indent);
            }
            
            // Look for closing braces
            size_t close_pos = trimmed.find('}');
            if (close_pos != std::string::npos && !brace_stack.empty()) {
                size_t start_line = brace_stack.back();
                size_t end_line = i;
                int region_indent = indent_stack.back();
                
                brace_stack.pop_back();
                indent_stack.pop_back();
                
                // Only create region if it spans multiple lines
                if (end_line > start_line + 1) {
                    regions_.emplace_back(start_line, end_line, region_indent);
                }
            }
        }
    }
    
    static std::string trim(const std::string& str) {
        size_t start = 0;
        while (start < str.length() && (str[start] == ' ' || str[start] == '\t')) {
            start++;
        }
        size_t end = str.length();
        while (end > start && (str[end-1] == ' ' || str[end-1] == '\t' || str[end-1] == '\r' || str[end-1] == '\n')) {
            end--;
        }
        return str.substr(start, end - start);
    }
};
