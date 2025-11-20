#include "rope_table.h"
#include <iostream>
#include <cassert>

int main() {
    RopeTable rope;
    // Insert test
    rope.insert(0, "abc");
    rope.insert(3, "def");
    assert(rope.get_text(0, rope.get_total_length()) == "abcdef");
    // Delete test
    rope.delete_range(2, 2); // Remove 'cd'
    assert(rope.get_text(0, rope.get_total_length()) == "abef");
    // Undo/redo test
    rope.undo();
    assert(rope.get_text(0, rope.get_total_length()) == "abcdef");
    rope.redo();
    assert(rope.get_text(0, rope.get_total_length()) == "abef");
    // Edge case: insert at end
    rope.insert(rope.get_total_length(), "xyz");
    assert(rope.get_text(0, rope.get_total_length()) == "abefxyz");
    std::cout << "RopeTable tests passed.\n";
    return 0;
}
