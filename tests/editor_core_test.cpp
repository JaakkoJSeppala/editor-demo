#include <iostream>
#include "../include/editor_core.h"

int main() {
    EditorCore editor;
    editor.insert_text("Hello, world!");
    editor.move_cursor(5);
    editor.insert_text("X");
    std::cout << "Editor buffer: " << editor.get_text() << std::endl;
    editor.erase(1);
    std::cout << "After erase: " << editor.get_text() << std::endl;
    return 0;
}
