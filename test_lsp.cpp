// LSP Feature Test File
// This file tests Language Server Protocol integration with clangd

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

// Test 1: Go to Definition (F12 on function call)
void test_function() {
    std::cout << "Testing LSP features" << std::endl;
}

// Test 2: Diagnostics (should show error)
void test_diagnostics() {
    // This should show a red squiggle - type mismatch
    int x = "this is a string"; // ERROR: cannot convert string to int
}

// Test 3: Code Completion (type "std::vec" and see suggestions)
void test_completion() {
    // Type: std::vec
    // Should suggest: std::vector
    std::vector<int> numbers = {1, 2, 3, 4, 5};
    
    // Type: std::cou
    // Should suggest: std::cout
    std::cout << "Size: " << numbers.size() << std::endl;
}

// Test 4: Semantic Highlighting via Tree-sitter
class TestClass {
private:
    int member_variable;
    std::string name;
    
public:
    TestClass(const std::string& n) : member_variable(0), name(n) {}
    
    int get_value() const { return member_variable; }
    void set_value(int v) { member_variable = v; }
};

// Test 5: Cross-file Navigation
void use_algorithm() {
    std::vector<int> data = {5, 2, 8, 1, 9};
    // Press F12 on "sort" to jump to algorithm header
    std::sort(data.begin(), data.end());
}

int main() {
    // Call test function - press F12 to jump to definition
    test_function();
    
    test_completion();
    
    TestClass obj("example");
    obj.set_value(42);
    std::cout << "Value: " << obj.get_value() << std::endl;
    
    use_algorithm();
    
    return 0;
}
