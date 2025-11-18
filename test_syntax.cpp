#include <iostream>
#include <string>
#include <vector>

// This is a test file for syntax highlighting
/* Multi-line comment
   with multiple lines */

class Example {
private:
    int value_;
    std::string name_;
    
public:
    Example(int v, const std::string& n) 
        : value_(v), name_(n) {}
    
    int get_value() const { return value_; }
    void set_value(int v) { value_ = v; }
    
    static void test_function() {
        // Numbers and strings
        int x = 42;
        float pi = 3.14159f;
        double big = 1e10;
        std::string msg = "Hello, World!";
        
        // Keywords
        if (x > 0) {
            for (int i = 0; i < 10; ++i) {
                std::cout << i << std::endl;
            }
        } else {
            while (true) {
                break;
            }
        }
        
        // Preprocessor
        #ifdef DEBUG
        std::cout << "Debug mode" << std::endl;
        #endif
    }
};

int main() {
    auto ptr = std::make_unique<Example>(100, "test");
    ptr->test_function();
    return 0;
}
