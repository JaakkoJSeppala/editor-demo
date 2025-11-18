#include "undo_manager.h"
#include "piece_table.h"

// InsertCommand implementation
void InsertCommand::execute() {
    document_->insert(position_, text_);
}

void InsertCommand::undo() {
    document_->remove(position_, text_.length());
}

// DeleteCommand implementation
void DeleteCommand::execute() {
    // Save the text being deleted so we can restore it
    deleted_text_ = document_->get_text(position_, length_);
    document_->remove(position_, length_);
}

void DeleteCommand::undo() {
    // Restore the deleted text
    document_->insert(position_, deleted_text_);
}

// UndoManager implementation
void UndoManager::execute(std::unique_ptr<Command> cmd) {
    // If we're not at the end of the undo stack, discard any "future" commands
    if (current_index_ < commands_.size()) {
        commands_.erase(commands_.begin() + current_index_, commands_.end());
    }
    
    // Execute the command
    cmd->execute();
    
    // Add to history
    commands_.push_back(std::move(cmd));
    current_index_ = commands_.size();
    
    // Trim if we exceeded max depth
    trim_to_depth();
}

void UndoManager::undo() {
    if (!can_undo()) {
        return;
    }
    
    // Move back one step
    current_index_--;
    
    // Undo the command at current index
    commands_[current_index_]->undo();
}

void UndoManager::redo() {
    if (!can_redo()) {
        return;
    }
    
    // Redo the command at current index
    commands_[current_index_]->redo();
    
    // Move forward one step
    current_index_++;
}

void UndoManager::trim_to_depth() {
    if (commands_.size() > max_depth_) {
        size_t to_remove = commands_.size() - max_depth_;
        commands_.erase(commands_.begin(), commands_.begin() + to_remove);
        current_index_ -= to_remove;
    }
}
