#include <iostream>
#include <string>
#include <chrono>
#include "../include/gap_buffer.h"

int main() {
    std::cout << "Editor-demo gap buffer suorituskykytesti\n";
    std::cout << "=======================================\n";

    // GapBuffer suorituskykytesti
    {
        GapBuffer gb;
        const size_t N = 100000;
        std::string sample = "abc";
        auto start = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < N; ++i) {
            gb.insert(sample);
        }
        auto mid = std::chrono::high_resolution_clock::now();
        std::string text = gb.get_text();
        auto end = std::chrono::high_resolution_clock::now();
        auto insert_ms = std::chrono::duration_cast<std::chrono::milliseconds>(mid - start).count();
        auto read_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - mid).count();
        std::cout << "GapBuffer: Inserted " << N << " x 'abc' in " << insert_ms << " ms\n";
        std::cout << "GapBuffer: Read " << text.length() << " chars in " << read_ms << " ms\n";
    }

    std::cout << "GapBuffer suorituskykytesti suoritettu.\n";
    return 0;
}
