#include "editor_core.h"
#include "gap_buffer.h"
#include <fstream>
#include <string>

EditorCore::EditorCore() : buffer_() {}

bool EditorCore::open(const std::string& filename) {
    std::ifstream file(filename, std::ios::in | std::ios::binary);
    if (!file.is_open()) return false;
    buffer_.clear();
    std::string line;
    while (std::getline(file, line)) {
        buffer_.insert(buffer_.size(), line + "\n");
    }
    file.close();
    filename_ = filename;
    return true;
}

bool EditorCore::save(const std::string& filename) {
    std::ofstream file(filename, std::ios::out | std::ios::binary);
    if (!file.is_open()) return false;
    file << buffer_.str();
    file.close();
    filename_ = filename;
    return true;
}

void EditorCore::insert(size_t pos, const std::string& text) {
    buffer_.insert(pos, text);
}

void EditorCore::erase(size_t pos, size_t len) {
    buffer_.erase(pos, len);
}

std::string EditorCore::get_text() const {
    return buffer_.str();
}

void EditorCore::clear() {
    buffer_.clear();
}

// TODO: Implement undo/redo logic
