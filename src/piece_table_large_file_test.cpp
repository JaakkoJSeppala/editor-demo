#include "piece_table.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <string>

int main() {
    std::ifstream file("test_file_large_gen.txt");
    std::string line;
    PieceTable pt;
    size_t line_count = 0;
    auto start = std::chrono::high_resolution_clock::now();
    while (std::getline(file, line)) {
        pt.insert(pt.get_total_length(), line + "\n");
        ++line_count;
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Inserted " << line_count << " lines in " << ms << " ms\n";
    // Test reading first and last line
    std::cout << "First line: " << pt.get_line(0) << "\n";
    std::cout << "Last line: " << pt.get_line(pt.get_line_count() - 1) << "\n";
    return 0;
}
