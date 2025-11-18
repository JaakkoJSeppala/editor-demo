#ifndef VIEWPORT_H
#define VIEWPORT_H

#include "piece_table.h"
#include <vector>
#include <string>

/**
 * Viewport - Virtual scrolling renderer
 * 
 * Only renders visible lines to maintain 60fps even with million-line files
 * This is the key to zero-latency scrolling in large projects
 */
class Viewport {
public:
    Viewport(size_t visible_lines, size_t visible_columns);
    
    // Set the document to display
    void set_document(std::shared_ptr<PieceTable> document);
    
    // Scrolling operations - O(1) complexity
    void scroll_up(size_t lines = 1);
    void scroll_down(size_t lines = 1);
    void scroll_to_line(size_t line);
    
    // Get current visible content
    std::vector<std::string> get_visible_lines() const;
    
    // Viewport properties
    size_t get_top_line() const { return top_line_; }
    size_t get_visible_line_count() const { return visible_lines_; }
    
    // Performance metrics
    double get_last_render_time_ms() const { return last_render_time_ms_; }
    
private:
    std::shared_ptr<PieceTable> document_;
    size_t top_line_;
    size_t visible_lines_;
    size_t visible_columns_;
    
    double last_render_time_ms_;
    
    void clamp_scroll_position();
};

#endif // VIEWPORT_H
