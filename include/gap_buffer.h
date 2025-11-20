#ifndef GAP_BUFFER_H
#define GAP_BUFFER_H

#include <vector>
#include <string>

class GapBuffer {
public:
    GapBuffer(size_t initial_capacity = 1024)
        : buffer(initial_capacity, '\0'), gap_start(0), gap_end(initial_capacity) {}

    void insert(char c) {
        if (gap_start == gap_end) expand();
        buffer[gap_start++] = c;
    }

    void insert(const std::string& str) {
        for (char c : str) insert(c);
    }

    void move_cursor(size_t pos) {
        if (pos < gap_start) {
            // Move gap left
            while (gap_start > pos) {
                buffer[--gap_end] = buffer[--gap_start];
            }
        } else if (pos > gap_start) {
            // Move gap right
            while (gap_start < pos) {
                buffer[gap_start++] = buffer[gap_end++];
            }
        }
    }

    void erase(size_t count) {
        gap_end += count;
        if (gap_end > buffer.size()) gap_end = buffer.size();
    }

    std::string get_text() const {
        std::string result;
        result.reserve(buffer.size() - (gap_end - gap_start));
        result.append(buffer.begin(), buffer.begin() + gap_start);
        result.append(buffer.begin() + gap_end, buffer.end());
        return result;
    }

    size_t length() const {
        return buffer.size() - (gap_end - gap_start);
    }

private:
    std::vector<char> buffer;
    size_t gap_start, gap_end;

    void expand() {
        size_t old_size = buffer.size();
        size_t new_size = old_size * 2;
        std::vector<char> new_buffer(new_size, '\0');
        // Copy before gap
        std::copy(buffer.begin(), buffer.begin() + gap_start, new_buffer.begin());
        // Copy after gap
        size_t after_gap = old_size - gap_end;
        std::copy(buffer.begin() + gap_end, buffer.end(), new_buffer.begin() + new_size - after_gap);
        gap_end = new_size - after_gap;
        buffer = std::move(new_buffer);
    }
};

#endif // GAP_BUFFER_H
