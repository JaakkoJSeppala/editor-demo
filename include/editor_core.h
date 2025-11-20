#include "gap_buffer.h"

class EditorCore {
public:
    EditorCore() : buffer(1024) {}

    void insert_text(const std::string& text) {
        buffer.insert(text);
    }

    void move_cursor(size_t pos) {
        buffer.move_cursor(pos);
    }

    void erase(size_t count) {
        buffer.erase(count);
    }

    std::string get_text() const {
        return buffer.get_text();
    }

    size_t length() const {
        return buffer.length();
    }

private:
    GapBuffer buffer;
};