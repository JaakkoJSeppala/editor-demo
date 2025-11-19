#include "test_framework.h"
#include "piece_table.h"
#include "undo_manager.h"
#include "find_dialog.h"
#include "viewport.h"
#include <iostream>
#include <cassert>
#include <algorithm> // For std::min

// Undefine Windows macros that conflict
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

/**
 * Comprehensive Test Suite for Velocity Editor
 * 
 * Test Categories:
 * 1. Unit Tests - Individual component testing
 * 2. Integration Tests - Component interaction
 * 3. Property Tests - Invariant checking with random inputs
 * 4. Fuzz Tests - Robustness with random operations
 * 5. Performance Tests - Benchmarking critical paths
 */

// ============================================================================
// UNIT TESTS - PieceTable
// ============================================================================

void test_piece_table_empty() {
    PieceTable doc;
    TestFramework::assert_equal(size_t(0), doc.get_total_length(), "Empty document length");
    TestFramework::assert_equal(size_t(1), doc.get_line_count(), "Empty document lines");
}

void test_piece_table_initialization() {
    PieceTable doc("Hello\nWorld\n");
    TestFramework::assert_equal(size_t(12), doc.get_total_length(), "Initial length");
    TestFramework::assert_equal(size_t(3), doc.get_line_count(), "Initial line count");
}

void test_piece_table_insert_start() {
    PieceTable doc("World");
    doc.insert(0, "Hello ");
    TestFramework::assert_equal(std::string("Hello World"), 
                                doc.get_text(0, doc.get_total_length()),
                                "Insert at start");
}

void test_piece_table_insert_middle() {
    PieceTable doc("HelloWorld");
    doc.insert(5, " ");
    TestFramework::assert_equal(std::string("Hello World"), 
                                doc.get_text(0, doc.get_total_length()),
                                "Insert in middle");
}

void test_piece_table_insert_end() {
    PieceTable doc("Hello");
    doc.insert(5, " World");
    TestFramework::assert_equal(std::string("Hello World"), 
                                doc.get_text(0, doc.get_total_length()),
                                "Insert at end");
}

void test_piece_table_delete_start() {
    PieceTable doc("Hello World");
    doc.remove(0, 6);
    TestFramework::assert_equal(std::string("World"), 
                                doc.get_text(0, doc.get_total_length()),
                                "Delete from start");
}

void test_piece_table_delete_middle() {
    PieceTable doc("Hello World");
    doc.remove(5, 1);
    TestFramework::assert_equal(std::string("HelloWorld"), 
                                doc.get_text(0, doc.get_total_length()),
                                "Delete from middle");
}

void test_piece_table_delete_end() {
    PieceTable doc("Hello World");
    doc.remove(5, 6);
    TestFramework::assert_equal(std::string("Hello"), 
                                doc.get_text(0, doc.get_total_length()),
                                "Delete from end");
}

void test_piece_table_multiple_operations() {
    PieceTable doc("The quick brown fox");
    doc.insert(10, "very ");  // "The quick very brown fox"
    doc.remove(4, 12);        // "The brown fox"
    doc.insert(4, "lazy ");   // "The lazy brown fox"
    
    std::string result = doc.get_text(0, doc.get_total_length());
    TestFramework::assert_equal(std::string("The lazy rown fox"), result,
                                "Multiple operations");
}

void test_piece_table_get_line() {
    PieceTable doc("Line 1\nLine 2\nLine 3\n");
    TestFramework::assert_equal(std::string("Line 1"), doc.get_line(0), "Get line 0");
    TestFramework::assert_equal(std::string("Line 2"), doc.get_line(1), "Get line 1");
    TestFramework::assert_equal(std::string("Line 3"), doc.get_line(2), "Get line 2");
}

// ============================================================================
// UNIT TESTS - UndoManager
// ============================================================================

void test_undo_manager_single_insert() {
    PieceTable doc("Hello");
    UndoManager undo;
    
    auto cmd = std::make_unique<InsertCommand>(&doc, 5, " World");
    undo.execute(std::move(cmd));
    
    TestFramework::assert_equal(std::string("Hello World"), 
                                doc.get_text(0, doc.get_total_length()),
                                "After insert");
    
    undo.undo();
    TestFramework::assert_equal(std::string("Hello"), 
                                doc.get_text(0, doc.get_total_length()),
                                "After undo");
}

void test_undo_manager_single_delete() {
    PieceTable doc("Hello World");
    UndoManager undo;
    
    auto cmd = std::make_unique<DeleteCommand>(&doc, 5, 6);
    undo.execute(std::move(cmd));
    
    TestFramework::assert_equal(std::string("Hello"), 
                                doc.get_text(0, doc.get_total_length()),
                                "After delete");
    
    undo.undo();
    TestFramework::assert_equal(std::string("Hello World"), 
                                doc.get_text(0, doc.get_total_length()),
                                "After undo");
}

void test_undo_manager_multiple_operations() {
    PieceTable doc("A");
    UndoManager undo;
    
    undo.execute(std::make_unique<InsertCommand>(&doc, 1, "B"));
    undo.execute(std::make_unique<InsertCommand>(&doc, 2, "C"));
    undo.execute(std::make_unique<InsertCommand>(&doc, 3, "D"));
    
    TestFramework::assert_equal(std::string("ABCD"), 
                                doc.get_text(0, doc.get_total_length()),
                                "After inserts");
    
    undo.undo();
    TestFramework::assert_equal(std::string("ABC"), 
                                doc.get_text(0, doc.get_total_length()),
                                "After 1 undo");
    
    undo.undo();
    TestFramework::assert_equal(std::string("AB"), 
                                doc.get_text(0, doc.get_total_length()),
                                "After 2 undos");
}

void test_undo_manager_redo() {
    PieceTable doc("Hello");
    UndoManager undo;
    
    undo.execute(std::make_unique<InsertCommand>(&doc, 5, " World"));
    undo.undo();
    undo.redo();
    
    TestFramework::assert_equal(std::string("Hello World"), 
                                doc.get_text(0, doc.get_total_length()),
                                "After redo");
}

// ============================================================================
// UNIT TESTS - FindDialog
// ============================================================================

void test_find_simple() {
    FindDialog finder;
    std::string text = "Hello World Hello";
    
    auto matches = finder.find_all(text, "Hello");
    TestFramework::assert_equal(size_t(2), matches.size(), "Find 'Hello' count");
    TestFramework::assert_equal(size_t(0), matches[0].position, "First match position");
    TestFramework::assert_equal(size_t(12), matches[1].position, "Second match position");
}

void test_find_case_insensitive() {
    FindDialog finder;
    finder.set_case_sensitive(false);
    std::string text = "Hello hello HELLO";
    
    auto matches = finder.find_all(text, "hello");
    TestFramework::assert_equal(size_t(3), matches.size(), "Case insensitive count");
}

void test_find_case_sensitive() {
    FindDialog finder;
    finder.set_case_sensitive(true);
    std::string text = "Hello hello HELLO";
    
    auto matches = finder.find_all(text, "hello");
    TestFramework::assert_equal(size_t(1), matches.size(), "Case sensitive count");
}

void test_find_no_match() {
    FindDialog finder;
    std::string text = "Hello World";
    
    auto matches = finder.find_all(text, "xyz");
    TestFramework::assert_equal(size_t(0), matches.size(), "No matches");
}

// ============================================================================
// PROPERTY-BASED TESTS
// ============================================================================

void test_property_insert_increases_length() {
    PropertyTester tester;
    
    tester.check_property<std::string>(
        "Insert increases document length",
        [&tester]() { return tester.random_string(50); },
        [](const std::string& text) {
            PieceTable doc("Hello");
            size_t before = doc.get_total_length();
            doc.insert(before, text);
            size_t after = doc.get_total_length();
            return after == before + text.length();
        },
        50
    );
}

void test_property_delete_decreases_length() {
    PropertyTester tester;
    
    tester.check_property<size_t>(
        "Delete decreases document length",
        [&tester]() { return tester.random_size_t(10); },
        [](size_t len) {
            if (len == 0) return true;
            std::string initial(len, 'x');
            PieceTable doc(initial);
            size_t before = doc.get_total_length();
            doc.remove(0, len / 2);
            size_t after = doc.get_total_length();
            return after == before - len / 2;
        },
        50
    );
}

void test_property_undo_inverts_insert() {
    PropertyTester tester;
    
    tester.check_property<std::string>(
        "Undo inverts insert",
        [&tester]() { return tester.random_string(30); },
        [](const std::string& text) {
            PieceTable doc("Base");
            UndoManager undo;
            std::string before = doc.get_text(0, doc.get_total_length());
            
            undo.execute(std::make_unique<InsertCommand>(&doc, doc.get_total_length(), text));
            undo.undo();
            
            std::string after = doc.get_text(0, doc.get_total_length());
            return before == after;
        },
        50
    );
}

// ============================================================================
// FUZZ TESTS
// ============================================================================

void test_fuzz_random_operations() {
    std::cout << "Fuzz test: Random operations (1000 iterations)\n";
    auto fuzz_start = std::chrono::high_resolution_clock::now();
    Fuzzer fuzzer;
    PieceTable doc("Initial content");
    double total_edit_time = 0.0;
    double total_verify_time = 0.0;
    for (int i = 0; i < 1000; ++i) {
        size_t doc_len = doc.get_total_length();
        auto edit = fuzzer.random_edit(doc_len, 20);
        try {
            auto edit_start = std::chrono::high_resolution_clock::now();
            // Jos PieceTable on liian iso, tehdään vain poistoja
            if (doc_len > 5000 && (edit.type == Fuzzer::Edit::INSERT || edit.type == Fuzzer::Edit::REPLACE)) {
                // Muutetaan edit.type REMOVEksi
                edit.type = Fuzzer::Edit::REMOVE;
                edit.length = std::min<size_t>(20, doc_len);
            }
            switch (edit.type) {
                case Fuzzer::Edit::INSERT:
                    if (edit.position <= doc_len) {
                        doc.insert(edit.position, edit.text);
                    }
                    break;
                case Fuzzer::Edit::REMOVE:
                    if (edit.position < doc_len && edit.length > 0) {
                        doc.remove(edit.position, edit.length);
                    }
                    break;
                case Fuzzer::Edit::REPLACE:
                    if (edit.position < doc_len && edit.length > 0) {
                        doc.remove(edit.position, edit.length);
                        doc.insert(edit.position, edit.text);
                    }
                    break;
            }
            auto edit_end = std::chrono::high_resolution_clock::now();
            total_edit_time += std::chrono::duration<double, std::milli>(edit_end - edit_start).count();
            // Verify document is still valid
            auto verify_start = std::chrono::high_resolution_clock::now();
            size_t len = doc.get_total_length();
            if (len > 0) {
                doc.get_text(0, (std::min)(len, size_t(10)));
            }
            auto verify_end = std::chrono::high_resolution_clock::now();
            total_verify_time += std::chrono::duration<double, std::milli>(verify_end - verify_start).count();
        } catch (const std::exception& e) {
            throw std::runtime_error("Fuzz test failed at iteration " + 
                                   std::to_string(i) + ": " + e.what());
        }
    }
    auto fuzz_end = std::chrono::high_resolution_clock::now();
    double total_time = std::chrono::duration<double, std::milli>(fuzz_end - fuzz_start).count();
    std::cout << "  Survived 1000 random operations!\n";
    std::cout << "  Total fuzz time: " << total_time << " ms\n";
    std::cout << "  Total edit time: " << total_edit_time << " ms\n";
    std::cout << "  Total verify time: " << total_verify_time << " ms\n";
}

void test_fuzz_undo_redo_chaos() {
    std::cout << "Fuzz test: Undo/Redo chaos (500 iterations)\n";
    
    Fuzzer fuzzer;
    PieceTable doc("Start");
    UndoManager undo;
    std::mt19937 rng(std::random_device{}());
    
    for (int i = 0; i < 500; ++i) {
        std::uniform_int_distribution<int> action_dist(0, 3);
        int action = action_dist(rng);
        
        try {
            switch (action) {
                case 0: // Insert
                    if (doc.get_total_length() < 10000) {
                        std::string text = fuzzer.random_printable_string(5);
                        size_t pos = std::uniform_int_distribution<size_t>(
                            0, doc.get_total_length())(rng);
                        undo.execute(std::make_unique<InsertCommand>(&doc, pos, text));
                    }
                    break;
                    
                case 1: // Delete
                    if (doc.get_total_length() > 0) {
                        size_t pos = std::uniform_int_distribution<size_t>(
                            0, doc.get_total_length() - 1)(rng);
                        size_t len = (std::min)(size_t(5), doc.get_total_length() - pos);
                        undo.execute(std::make_unique<DeleteCommand>(&doc, pos, len));
                    }
                    break;
                    
                case 2: // Undo
                    if (undo.can_undo()) {
                        undo.undo();
                    }
                    break;
                    
                case 3: // Redo
                    if (undo.can_redo()) {
                        undo.redo();
                    }
                    break;
            }
            
            // Verify document is valid
            doc.get_total_length();
            
        } catch (const std::exception& e) {
            throw std::runtime_error("Undo/Redo fuzz failed at iteration " + 
                                   std::to_string(i) + ": " + e.what());
        }
    }
    
    std::cout << "  Survived 500 random undo/redo operations!\n";
}

// ============================================================================
// PERFORMANCE TESTS
// ============================================================================

void test_performance_large_file_insert() {
    std::cout << "Performance test: Large file inserts\n";
    
    PieceTable doc;
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 10000; ++i) {
        doc.insert(doc.get_total_length(), "Line " + std::to_string(i) + "\n");
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double, std::milli>(end - start).count();
    
    std::cout << "  10,000 inserts: " << duration << "ms\n";
    std::cout << "  Average per insert: " << (duration / 10000.0) << "ms\n";
    
    TestFramework::assert_true(duration < 1000.0, "Inserts should be fast (< 1s for 10k)");
}

void test_performance_search() {
    std::cout << "Performance test: Search in large document\n";
    
    std::string large_doc;
    for (int i = 0; i < 10000; ++i) {
        large_doc += "This is line " + std::to_string(i) + " with searchable content\n";
    }
    
    FindDialog finder;
    auto start = std::chrono::high_resolution_clock::now();
    
    auto matches = finder.find_all(large_doc, "searchable");
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double, std::milli>(end - start).count();
    
    std::cout << "  Found " << matches.size() << " matches in " << duration << "ms\n";
    
    TestFramework::assert_equal(size_t(10000), matches.size(), "Should find all matches");
    TestFramework::assert_true(duration < 100.0, "Search should be fast (< 100ms)");
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main() {
    std::cout << "==============================================\n";
    std::cout << "VELOCITY EDITOR - COMPREHENSIVE TEST SUITE\n";
    std::cout << "==============================================\n\n";
    
    TestFramework tests;
    
    // PieceTable unit tests
    tests.add_test("PieceTable: Empty document", test_piece_table_empty);
    tests.add_test("PieceTable: Initialization", test_piece_table_initialization);
    tests.add_test("PieceTable: Insert at start", test_piece_table_insert_start);
    tests.add_test("PieceTable: Insert in middle", test_piece_table_insert_middle);
    tests.add_test("PieceTable: Insert at end", test_piece_table_insert_end);
    tests.add_test("PieceTable: Delete from start", test_piece_table_delete_start);
    tests.add_test("PieceTable: Delete from middle", test_piece_table_delete_middle);
    tests.add_test("PieceTable: Delete from end", test_piece_table_delete_end);
    tests.add_test("PieceTable: Multiple operations", test_piece_table_multiple_operations);
    tests.add_test("PieceTable: Get line", test_piece_table_get_line);
    
    // UndoManager unit tests
    tests.add_test("UndoManager: Single insert", test_undo_manager_single_insert);
    tests.add_test("UndoManager: Single delete", test_undo_manager_single_delete);
    tests.add_test("UndoManager: Multiple operations", test_undo_manager_multiple_operations);
    tests.add_test("UndoManager: Redo", test_undo_manager_redo);
    
    // FindDialog unit tests
    tests.add_test("FindDialog: Simple find", test_find_simple);
    tests.add_test("FindDialog: Case insensitive", test_find_case_insensitive);
    tests.add_test("FindDialog: Case sensitive", test_find_case_sensitive);
    tests.add_test("FindDialog: No match", test_find_no_match);
    
    // Property-based tests
    tests.add_test("Property: Insert increases length", test_property_insert_increases_length);
    tests.add_test("Property: Delete decreases length", test_property_delete_decreases_length);
    tests.add_test("Property: Undo inverts insert", test_property_undo_inverts_insert);
    
    // Fuzz tests
    tests.add_test("Fuzz: Random operations", test_fuzz_random_operations);
    tests.add_test("Fuzz: Undo/Redo chaos", test_fuzz_undo_redo_chaos);
    
    // Performance tests
    tests.add_test("Performance: Large file inserts", test_performance_large_file_insert);
    tests.add_test("Performance: Search", test_performance_search);
    
    // Run all tests
    auto results = tests.run_all();
    
    // Exit with error if any test failed
    for (const auto& result : results) {
        if (!result.passed) {
            return 1;
        }
    }
    
    std::cout << "\n✓ All tests passed!\n";
    return 0;
}
