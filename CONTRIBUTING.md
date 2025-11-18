# Contributing to Velocity Editor

First off, thank you for considering contributing to Velocity Editor! 🎉

## How Can I Contribute?

### Reporting Bugs

Before creating bug reports, please check existing issues. When you create a bug report, include as many details as possible:

**Bug Report Template:**
```markdown
**Description:**
A clear description of the bug.

**Steps to Reproduce:**
1. Open editor
2. Press Ctrl+L
3. Scroll to line 40000
4. Click on text...

**Expected Behavior:**
What you expected to happen.

**Actual Behavior:**
What actually happened.

**Environment:**
- OS: Windows 11 / Ubuntu 22.04 / macOS 14
- Compiler: MSVC 2022 / GCC 11 / Clang 15
- Build Type: Release / Debug
- File size: 50,000 lines

**Screenshots/Logs:**
If applicable, add screenshots or console output.
```

### Suggesting Features

Feature suggestions are welcome! Please:

1. **Check existing issues** to avoid duplicates
2. **Describe the use case** - why is this feature needed?
3. **Provide examples** - how would it work?
4. **Consider performance** - how does it affect large files?

**Feature Request Template:**
```markdown
**Feature Name:**
Brief title for the feature

**Problem:**
What problem does this solve?

**Proposed Solution:**
How would this feature work?

**Alternatives Considered:**
What other approaches did you think of?

**Performance Impact:**
How might this affect performance on large files?
```

### Pull Requests

1. **Fork the repository**
2. **Create a feature branch** (`git checkout -b feature/amazing-feature`)
3. **Make your changes**
4. **Add tests** if applicable
5. **Ensure code compiles** without warnings
6. **Commit with clear messages**
7. **Push to your fork**
8. **Open a Pull Request**

## Coding Style

### C++ Style Guide

We follow modern C++17 best practices:

#### Naming Conventions
```cpp
// Classes: PascalCase
class PieceTable { };

// Functions/methods: snake_case
void insert_text(size_t position, const std::string& text);

// Variables: snake_case
size_t cursor_position_;
std::string current_file_;

// Constants: UPPER_SNAKE_CASE or kPascalCase
constexpr size_t MAX_FILE_SIZE = 100'000'000;
const int kDefaultFontSize = 16;

// Private members: trailing underscore
private:
    int member_variable_;
```

#### Code Structure
```cpp
// Header guards
#ifndef PIECE_TABLE_H
#define PIECE_TABLE_H

// Includes: standard library first, then project headers
#include <string>
#include <vector>
#include "viewport.h"

// Brief documentation for classes
/**
 * PieceTable - Efficient text buffer for large files
 * 
 * Uses piece table data structure for O(1) insert/delete.
 * Maintains separate buffers for original and added text.
 */
class PieceTable {
public:
    // Public interface first
    PieceTable();
    void insert(size_t position, const std::string& text);
    
private:
    // Private implementation
    void rebuild_cache();
    std::vector<Piece> pieces_;
};

#endif // PIECE_TABLE_H
```

#### Best Practices

**DO:**
- ✅ Use `const` and `constexpr` where possible
- ✅ Use smart pointers (`std::unique_ptr`, `std::shared_ptr`)
- ✅ Prefer STL containers over raw arrays
- ✅ Use RAII for resource management
- ✅ Add comments explaining *why*, not *what*
- ✅ Keep functions short and focused
- ✅ Use meaningful variable names

**DON'T:**
- ❌ Use raw `new`/`delete` (use smart pointers)
- ❌ Use C-style casts (use `static_cast`, etc.)
- ❌ Ignore compiler warnings
- ❌ Use `using namespace std;` in headers
- ❌ Commit commented-out code
- ❌ Leave TODO comments without issue numbers

### Performance Guidelines

Velocity is all about performance. Consider these when contributing:

1. **Big-O Complexity**: Prefer O(1) or O(log n) operations
2. **Memory Allocation**: Minimize allocations in hot paths
3. **Cache Locality**: Keep related data together
4. **Avoid Copies**: Use references and move semantics
5. **Profile First**: Don't optimize without measuring

**Example - Efficient Text Insertion:**
```cpp
// GOOD: O(1) insertion with piece table
void PieceTable::insert(size_t position, const std::string& text) {
    add_buffer_ += text;  // Append to buffer
    pieces_.insert(find_piece(position), 
                   Piece(Source::ADD, add_buffer_.size() - text.size(), text.size()));
}

// BAD: O(n) insertion with string copy
void NaiveBuffer::insert(size_t position, const std::string& text) {
    buffer_ = buffer_.substr(0, position) + text + buffer_.substr(position);
}
```

## Testing

### Running Tests
```bash
cd build
./editor_demo  # Runs performance benchmarks
./editor_gui   # Manual testing
```

### Performance Testing Checklist
- [ ] Test with 10k line file
- [ ] Test with 100k line file
- [ ] Test with 1M line file
- [ ] Monitor FPS (should stay at 60)
- [ ] Check memory usage (should be minimal)
- [ ] Test scrolling performance
- [ ] Test insert/delete speed

### Adding New Tests
```cpp
// Example test in main.cpp
void test_large_file_performance() {
    std::string large_text;
    for (int i = 0; i < 100000; ++i) {
        large_text += "Line " + std::to_string(i) + "\n";
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    PieceTable doc(large_text);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    assert(duration.count() < 1000); // Should load in under 1 second
}
```

## Building

### Debug Build
```bash
mkdir build-debug && cd build-debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
```

### Release Build
```bash
mkdir build-release && cd build-release
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

### With Address Sanitizer (Linux/Mac)
```bash
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON ..
cmake --build .
```

## Documentation

### Code Comments
```cpp
/**
 * Brief description of what the function does.
 * 
 * Detailed explanation if needed, including:
 * - Algorithm complexity
 * - Performance characteristics
 * - Thread safety
 * 
 * @param position The character position in the document
 * @param text The text to insert
 * @return true if insertion succeeded
 * 
 * @note This operation is O(log n) where n is the number of pieces
 */
bool insert(size_t position, const std::string& text);
```

### README Updates
When adding features, update:
- Feature list in README.md
- Keyboard shortcuts table
- Performance benchmarks if relevant

## Review Process

1. **Automated Checks**: CI will run builds and tests
2. **Code Review**: Maintainer will review your code
3. **Feedback**: Address any requested changes
4. **Merge**: Once approved, we'll merge your PR!

## Community

- Be respectful and constructive
- Help others in discussions
- Share your use cases and ideas
- Celebrate successes together! 🎉

## Questions?

Don't hesitate to ask! Open a discussion on GitHub or ping maintainers.

---

Thank you for contributing to Velocity Editor! 🚀
