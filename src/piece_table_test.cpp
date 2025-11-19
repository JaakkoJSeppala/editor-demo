#include "piece_table.h"
#include <iostream>
#include <cassert>

int main() {
    PieceTable pt;
    // Insert test
    pt.insert(0, "Hello");
    pt.insert(5, ", world!");
    assert(pt.get_text(0, pt.get_total_length()) == "Hello, world!");
    // Delete test
    pt.delete_range(5, 2); // Remove ', '
    assert(pt.get_text(0, pt.get_total_length()) == "Helloworld!");
    // Undo/redo test
    pt.undo();
    assert(pt.get_text(0, pt.get_total_length()) == "Hello, world!");
    pt.redo();
    assert(pt.get_text(0, pt.get_total_length()) == "Helloworld!");
    // Edge case: insert at end
    pt.insert(pt.get_total_length(), " Test");
    assert(pt.get_text(0, pt.get_total_length()) == "Helloworld! Test");
    std::cout << "PieceTable tests passed.\n";
    return 0;
}
