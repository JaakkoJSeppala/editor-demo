#ifndef PIECE_TABLE_H
#define PIECE_TABLE_H

#include <vector>
#include <string>
#include <memory>

/**
 * Piece represents a segment of text from either the original buffer or add buffer
 * This is the core data structure for efficient text editing in large files
 */
struct Piece {
    enum class Source { ORIGINAL, ADD };
    
    Source source;
    size_t offset;    // Offset in the source buffer
    size_t length;    // Length of this piece
    
    Piece(Source src, size_t off, size_t len) 
        : source(src), offset(off), length(len) {}
};

/**
 * PieceTable - High-performance text buffer for million-line files
 * 
 * Uses piece table data structure which is O(1) for most operations
 * and maintains undo/redo history efficiently
 */
class PieceTable {
public:
    PieceTable();
    explicit PieceTable(const std::string& initial_text);
    
    // Core editing operations - all O(1) or O(log n) complexity
    void insert(size_t position, const std::string& text);
    void remove(size_t position, size_t length);
    // Alias for test compatibility
    void delete_range(size_t position, size_t length) { remove(position, length); }
    // Undo/redo API for test compatibility
    void undo();
    void redo();
    
    // Query operations
    std::string get_text(size_t start, size_t length) const;
    std::string get_line(size_t line_number) const;
    size_t get_line_count() const;
    size_t get_total_length() const;
    
    // For rendering - get visible lines efficiently
    std::vector<std::string> get_lines_range(size_t start_line, size_t count) const;
    
private:
    std::vector<Piece> pieces_;
    std::string original_buffer_;
    std::string add_buffer_;
    // Line index for O(1) line access
    mutable std::vector<size_t> line_cache_;
    mutable bool line_cache_valid_;
    void rebuild_line_cache() const;
    size_t position_to_piece_index(size_t position) const;
    // Undo/redo stacks
    struct PTState {
        std::vector<Piece> pieces;
        std::string add_buffer;
    };
    std::vector<PTState> undo_stack_;
    std::vector<PTState> redo_stack_;
};

#endif // PIECE_TABLE_H
