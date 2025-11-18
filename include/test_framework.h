#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <string>
#include <vector>
#include <functional>
#include <iostream>
#include <chrono>
#include <random>

/**
 * Lightweight test framework for Velocity Editor
 * 
 * Supports:
 * - Unit tests with assertions
 * - Property-based testing (QuickCheck style)
 * - Fuzzing for robustness
 * - Performance benchmarks
 */

class TestFramework {
public:
    struct TestResult {
        std::string name;
        bool passed;
        std::string error_message;
        double duration_ms;
    };
    
    using TestFunction = std::function<void()>;
    
    // Register a test
    void add_test(const std::string& name, TestFunction test) {
        tests_.push_back({name, test});
    }
    
    // Run all tests
    std::vector<TestResult> run_all() {
        std::vector<TestResult> results;
        
        std::cout << "Running " << tests_.size() << " tests...\n";
        std::cout << "========================================\n";
        
        for (const auto& test : tests_) {
            auto start = std::chrono::high_resolution_clock::now();
            TestResult result;
            result.name = test.name;
            
            try {
                test.func();
                result.passed = true;
                result.error_message = "";
            } catch (const std::exception& e) {
                result.passed = false;
                result.error_message = e.what();
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            result.duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
            
            results.push_back(result);
            
            // Print result
            std::cout << (result.passed ? "[PASS]" : "[FAIL]") 
                      << " " << result.name 
                      << " (" << result.duration_ms << "ms)\n";
            
            if (!result.passed) {
                std::cout << "       Error: " << result.error_message << "\n";
            }
        }
        
        // Summary
        int passed = 0;
        for (const auto& r : results) {
            if (r.passed) passed++;
        }
        
        std::cout << "========================================\n";
        std::cout << "Results: " << passed << "/" << results.size() << " passed\n";
        
        return results;
    }
    
    // Assertion helpers
    static void assert_true(bool condition, const std::string& message = "Assertion failed") {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }
    
    static void assert_equal(size_t expected, size_t actual, const std::string& message = "") {
        if (expected != actual) {
            throw std::runtime_error(message + " - Expected: " + std::to_string(expected) 
                                   + ", Got: " + std::to_string(actual));
        }
    }
    
    static void assert_equal(const std::string& expected, const std::string& actual, 
                            const std::string& message = "") {
        if (expected != actual) {
            throw std::runtime_error(message + " - Expected: '" + expected 
                                   + "', Got: '" + actual + "'");
        }
    }
    
private:
    struct Test {
        std::string name;
        TestFunction func;
    };
    
    std::vector<Test> tests_;
};

/**
 * Property-based testing helper
 * Generates random inputs to test properties
 */
class PropertyTester {
public:
    PropertyTester() : rng_(std::random_device{}()) {}
    
    // Test a property with random inputs
    template<typename T>
    void check_property(const std::string& name, 
                       std::function<T()> generator,
                       std::function<bool(T)> property,
                       int iterations = 100) {
        std::cout << "Property test: " << name << " (" << iterations << " cases)\n";
        
        for (int i = 0; i < iterations; ++i) {
            T input = generator();
            if (!property(input)) {
                throw std::runtime_error("Property violated at iteration " + std::to_string(i));
            }
        }
        
        std::cout << "  All cases passed!\n";
    }
    
    // Random generators
    std::string random_string(size_t max_length = 100) {
        std::uniform_int_distribution<size_t> len_dist(0, max_length);
        std::uniform_int_distribution<int> char_dist(32, 126); // Printable ASCII
        
        size_t length = len_dist(rng_);
        std::string result;
        result.reserve(length);
        
        for (size_t i = 0; i < length; ++i) {
            result += static_cast<char>(char_dist(rng_));
        }
        
        return result;
    }
    
    size_t random_size_t(size_t max = 10000) {
        std::uniform_int_distribution<size_t> dist(0, max);
        return dist(rng_);
    }
    
    int random_int(int min = 0, int max = 1000) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(rng_);
    }
    
private:
    std::mt19937 rng_;
};

/**
 * Fuzzer for robustness testing
 */
class Fuzzer {
public:
    Fuzzer() : rng_(std::random_device{}()) {}
    
    // Generate random text edits
    struct Edit {
        enum Type { INSERT, REMOVE, REPLACE };
        Type type;
        size_t position;
        std::string text;
        size_t length; // For delete/replace
    };
    
    Edit random_edit(size_t doc_length, size_t max_edit_size = 100) {
        std::uniform_int_distribution<int> type_dist(0, 2);
        std::uniform_int_distribution<size_t> pos_dist(0, doc_length);
        std::uniform_int_distribution<size_t> len_dist(1, (std::max)(max_edit_size, doc_length + 1));
        
        Edit edit;
        edit.type = static_cast<Edit::Type>(type_dist(rng_));
        edit.position = doc_length > 0 ? pos_dist(rng_) : 0;
        
        switch (edit.type) {
            case Edit::INSERT:
                edit.text = random_printable_string(len_dist(rng_));
                break;
            case Edit::REMOVE:
                edit.length = (std::min)(len_dist(rng_), doc_length - edit.position);
                break;
            case Edit::REPLACE:
                edit.length = (std::min)(len_dist(rng_), doc_length - edit.position);
                edit.text = random_printable_string(len_dist(rng_));
                break;
        }
        
        return edit;
    }
    
    std::string random_printable_string(size_t length) {
        std::uniform_int_distribution<int> char_dist(32, 126);
        std::string result;
        result.reserve(length);
        
        for (size_t i = 0; i < length; ++i) {
            result += static_cast<char>(char_dist(rng_));
        }
        
        return result;
    }
    
private:
    std::mt19937 rng_;
};

#endif // TEST_FRAMEWORK_H
