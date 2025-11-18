#include "piece_table.h"
#include <algorithm>
#include <sstream>

PieceTable::PieceTable() : line_cache_valid_(false) {
}

PieceTable::PieceTable(const std::string& initial_text) 
    : original_buffer_(initial_text), line_cache_valid_(false) {
    if (!initial_text.empty()) {
        pieces_.emplace_back(Piece::Source::ORIGINAL, 0, initial_text.length());
    }
}

void PieceTable::insert(size_t position, const std::string& text) {
    if (text.empty()) return;
    
    line_cache_valid_ = false;
    
    // Find the piece containing the position
    size_t current_pos = 0;
    for (size_t i = 0; i < pieces_.size(); ++i) {
        size_t piece_end = current_pos + pieces_[i].length;
        
        if (position >= current_pos && position <= piece_end) {
            // Split the piece at the insertion point
            size_t offset_in_piece = position - current_pos;
            
            // Add new text to add buffer
            size_t add_offset = add_buffer_.length();
            add_buffer_ += text;
            
            // Create new pieces
            std::vector<Piece> new_pieces;
            
            // Keep pieces before insertion point
            for (size_t j = 0; j < i; ++j) {
                new_pieces.push_back(pieces_[j]);
            }
            
            // Split current piece if needed
            if (offset_in_piece > 0) {
                new_pieces.emplace_back(pieces_[i].source, pieces_[i].offset, offset_in_piece);
            }
            
            // Insert new text
            new_pieces.emplace_back(Piece::Source::ADD, add_offset, text.length());
            
            // Add remaining part of split piece
            if (offset_in_piece < pieces_[i].length) {
                new_pieces.emplace_back(pieces_[i].source, 
                                       pieces_[i].offset + offset_in_piece, 
                                       pieces_[i].length - offset_in_piece);
            }
            
            // Add remaining pieces
            for (size_t j = i + 1; j < pieces_.size(); ++j) {
                new_pieces.push_back(pieces_[j]);
            }
            
            pieces_ = std::move(new_pieces);
            return;
        }
        
        current_pos = piece_end;
    }
    
    // If position is at the end, simply append
    if (position == current_pos) {
        size_t add_offset = add_buffer_.length();
        add_buffer_ += text;
        pieces_.emplace_back(Piece::Source::ADD, add_offset, text.length());
    }
}

void PieceTable::remove(size_t position, size_t length) {
    if (length == 0) return;
    
    line_cache_valid_ = false;
    
    size_t end_position = position + length;
    size_t current_pos = 0;
    std::vector<Piece> new_pieces;
    
    for (const auto& piece : pieces_) {
        size_t piece_end = current_pos + piece.length;
        
        // Piece is completely before deletion range
        if (piece_end <= position) {
            new_pieces.push_back(piece);
        }
        // Piece is completely after deletion range
        else if (current_pos >= end_position) {
            new_pieces.push_back(piece);
        }
        // Piece overlaps with deletion range
        else {
            // Keep part before deletion
            if (current_pos < position) {
                size_t keep_length = position - current_pos;
                new_pieces.emplace_back(piece.source, piece.offset, keep_length);
            }
            
            // Keep part after deletion
            if (piece_end > end_position) {
                size_t skip_length = end_position - current_pos;
                size_t keep_length = piece_end - end_position;
                new_pieces.emplace_back(piece.source, piece.offset + skip_length, keep_length);
            }
        }
        
        current_pos = piece_end;
    }
    
    pieces_ = std::move(new_pieces);
}

std::string PieceTable::get_text(size_t start, size_t length) const {
    std::string result;
    result.reserve(length);
    
    size_t current_pos = 0;
    size_t remaining = length;
    
    for (const auto& piece : pieces_) {
        if (remaining == 0) break;
        
        size_t piece_end = current_pos + piece.length;
        
        if (piece_end > start && current_pos < start + length) {
            size_t offset_in_piece = (start > current_pos) ? (start - current_pos) : 0;
            size_t copy_length = std::min(piece.length - offset_in_piece, remaining);
            
            const std::string& source = (piece.source == Piece::Source::ORIGINAL) 
                                       ? original_buffer_ : add_buffer_;
            
            result += source.substr(piece.offset + offset_in_piece, copy_length);
            remaining -= copy_length;
        }
        
        current_pos = piece_end;
    }
    
    return result;
}

void PieceTable::rebuild_line_cache() const {
    line_cache_.clear();
    line_cache_.push_back(0);
    
    size_t pos = 0;
    for (const auto& piece : pieces_) {
        const std::string& source = (piece.source == Piece::Source::ORIGINAL) 
                                   ? original_buffer_ : add_buffer_;
        
        for (size_t i = piece.offset; i < piece.offset + piece.length; ++i) {
            pos++;
            if (source[i] == '\n') {
                line_cache_.push_back(pos);
            }
        }
    }
    
    line_cache_valid_ = true;
}

size_t PieceTable::get_line_count() const {
    if (!line_cache_valid_) {
        rebuild_line_cache();
    }
    return line_cache_.size();
}

size_t PieceTable::get_total_length() const {
    size_t total = 0;
    for (const auto& piece : pieces_) {
        total += piece.length;
    }
    return total;
}

std::string PieceTable::get_line(size_t line_number) const {
    if (!line_cache_valid_) {
        rebuild_line_cache();
    }
    
    if (line_number >= line_cache_.size()) {
        return "";
    }
    
    size_t start = line_cache_[line_number];
    size_t end = (line_number + 1 < line_cache_.size()) 
                 ? line_cache_[line_number + 1] - 1  // Exclude newline
                 : get_total_length();
    
    if (end > start && get_text(end - 1, 1) == "\n") {
        end--;
    }
    
    return get_text(start, end - start);
}

std::vector<std::string> PieceTable::get_lines_range(size_t start_line, size_t count) const {
    std::vector<std::string> lines;
    lines.reserve(count);
    
    for (size_t i = start_line; i < start_line + count; ++i) {
        std::string line = get_line(i);
        if (line.empty() && i >= get_line_count()) {
            break;
        }
        lines.push_back(line);
    }
    
    return lines;
}
