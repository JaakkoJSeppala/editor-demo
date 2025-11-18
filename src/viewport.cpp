#include "viewport.h"
#include <chrono>
#include <algorithm>

Viewport::Viewport(size_t visible_lines, size_t visible_columns)
    : top_line_(0)
    , visible_lines_(visible_lines)
    , visible_columns_(visible_columns)
    , last_render_time_ms_(0.0) {
}

void Viewport::set_document(std::shared_ptr<PieceTable> document) {
    document_ = document;
    top_line_ = 0;
}

void Viewport::scroll_up(size_t lines) {
    if (top_line_ >= lines) {
        top_line_ -= lines;
    } else {
        top_line_ = 0;
    }
}

void Viewport::scroll_down(size_t lines) {
    top_line_ += lines;
    clamp_scroll_position();
}

void Viewport::scroll_to_line(size_t line) {
    top_line_ = line;
    clamp_scroll_position();
}

void Viewport::clamp_scroll_position() {
    if (!document_) {
        top_line_ = 0;
        return;
    }
    
    size_t max_line = document_->get_line_count();
    if (max_line > visible_lines_) {
        max_line -= visible_lines_;
    } else {
        max_line = 0;
    }
    
    if (top_line_ > max_line) {
        top_line_ = max_line;
    }
}

std::vector<std::string> Viewport::get_visible_lines() const {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::vector<std::string> result;
    
    if (!document_) {
        return result;
    }
    
    // Only fetch visible lines - this is the key to performance
    // Even with 1M lines in document, we only process ~50 lines
    result = document_->get_lines_range(top_line_, visible_lines_);
    
    // Truncate lines that are too long for the viewport
    for (auto& line : result) {
        if (line.length() > visible_columns_) {
            line = line.substr(0, visible_columns_);
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    // Store render time for performance monitoring
    const_cast<Viewport*>(this)->last_render_time_ms_ = duration.count() / 1000.0;
    
    return result;
}
