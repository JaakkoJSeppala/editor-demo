#ifndef UNDO_MANAGER_H
#define UNDO_MANAGER_H

#include <string>
#include <vector>
#include <memory>

/**
 * Command represents a single text editing operation that can be undone/redone
 */
class Command {
public:
    virtual ~Command() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
    virtual void redo() { execute(); }
};

/**
 * InsertCommand - Represents text insertion that can be undone
 */
class InsertCommand : public Command {
public:
    InsertCommand(class PieceTable* doc, size_t pos, const std::string& text)
        : document_(doc), position_(pos), text_(text) {}
    
    void execute() override;
    void undo() override;
    
private:
    PieceTable* document_;
    size_t position_;
    std::string text_;
};

/**
 * DeleteCommand - Represents text deletion that can be undone
 */
class DeleteCommand : public Command {
public:
    DeleteCommand(class PieceTable* doc, size_t pos, size_t len)
        : document_(doc), position_(pos), length_(len), deleted_text_() {}
    
    void execute() override;
    void undo() override;
    
private:
    PieceTable* document_;
    size_t position_;
    size_t length_;
    std::string deleted_text_;
};

/**
 * UndoManager - Manages undo/redo stack with configurable depth
 */
class UndoManager {
public:
    explicit UndoManager(size_t max_depth = 1000) 
        : current_index_(0), max_depth_(max_depth) {}
    
    // Execute and add command to history
    void execute(std::unique_ptr<Command> cmd);
    
    // Undo/redo operations
    bool can_undo() const { return current_index_ > 0; }
    bool can_redo() const { return current_index_ < commands_.size(); }
    
    void undo();
    void redo();
    
    // Clear history
    void clear() {
        commands_.clear();
        current_index_ = 0;
    }
    
    size_t get_undo_count() const { return current_index_; }
    size_t get_redo_count() const { return commands_.size() - current_index_; }
    
private:
    std::vector<std::unique_ptr<Command>> commands_;
    size_t current_index_;
    size_t max_depth_;
    
    void trim_to_depth();
};

#endif // UNDO_MANAGER_H
