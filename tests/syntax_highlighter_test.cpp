#include "../include/syntax_highlighter.h"
#include <iostream>

void test_cpp_highlight() {
    SyntaxHighlighter highlighter;
    highlighter.set_language(SyntaxHighlighter::Language::Cpp);
    std::string line = "int main() { // entry point";
    SyntaxHighlighter::LineState state;
    SyntaxHighlighter::LineState out_state;
    auto tokens = highlighter.tokenize_line(line, state, out_state);
    std::cout << "C++ Highlight: ";
    for (const auto& t : tokens) {
        std::cout << "[" << line.substr(t.start, t.length) << ":" << t.type << "] ";
    }
    std::cout << std::endl;
}

void test_python_highlight() {
    SyntaxHighlighter highlighter;
    highlighter.set_language(SyntaxHighlighter::Language::Python);
    std::string line = "def foo(): # function";
    SyntaxHighlighter::LineState state;
    SyntaxHighlighter::LineState out_state;
    auto tokens = highlighter.tokenize_line(line, state, out_state);
    std::cout << "Python Highlight: ";
    for (const auto& t : tokens) {
        std::cout << "[" << line.substr(t.start, t.length) << ":" << t.type << "] ";
    }
    std::cout << std::endl;
}

int main() {
    test_cpp_highlight();
    test_python_highlight();
    return 0;
}
