#include "piece_table.h"
#include <iostream>
#include <chrono>
#include <string>

int main() {
    PieceTable pt;
    const size_t N = 1000000;
    std::string sample = "abcdefghij\n";
    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < N; ++i) {
        pt.insert(pt.get_total_length(), sample);
    }
    auto mid = std::chrono::high_resolution_clock::now();
    std::string text = pt.get_text(0, pt.get_total_length());
    auto end = std::chrono::high_resolution_clock::now();
    auto insert_ms = std::chrono::duration_cast<std::chrono::milliseconds>(mid - start).count();
    auto read_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - mid).count();
    std::cout << "Inserted " << N << " lines in " << insert_ms << " ms\n";
    std::cout << "Read " << text.length() << " chars in " << read_ms << " ms\n";
    // Profile undo/redo for 1 minute
    auto undo_start = std::chrono::high_resolution_clock::now();
    size_t undo_count = 0;
    while (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - undo_start).count() < 60) {
        pt.undo();
        ++undo_count;
    }
    auto undo_end = std::chrono::high_resolution_clock::now();
    auto undo_ms = std::chrono::duration_cast<std::chrono::milliseconds>(undo_end - undo_start).count();
    std::cout << "Undo operations in 1 min: " << undo_count << " (" << undo_ms << " ms)\n";
    return 0;
}
