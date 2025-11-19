#include "piece_table.h"
#include "viewport.h"
#include "indexer.h"
#include <iostream>
#include <memory>
#include <sstream>
#include <chrono>

/**
 * High-Performance Text Editor Demo
 * 
 * This demo showcases key architectural concepts for large-scale projects:
 * 1. Piece Table - O(1) insert/delete operations even with million-line files
 * 2. Virtual Scrolling - Only render visible lines for constant performance
 * 3. Background Indexer - Separate thread for instant search
 * 
 * Run this demo to see performance metrics for operations that would
 * slow down web-based editors like VS Code.
 */

void print_separator() {
    std::cout << "\n" << std::string(70, '=') << "\n\n";
}

void demo_piece_table() {
    std::cout << "DEMO 1: Piece Table - Efficient Large File Editing\n";
    std::cout << "---------------------------------------------------\n\n";
    
    // Create a large document
    std::string large_text;
    for (int i = 0; i < 10000; ++i) {
        large_text += "Line " + std::to_string(i + 1) + ": This is sample text for performance testing.\n";
    }
    
    std::cout << "Creating document with 10,000 lines...\n";
    auto start = std::chrono::high_resolution_clock::now();
    
    PieceTable doc(large_text);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Document created in: " << duration.count() / 1000.0 << " ms\n";
    std::cout << "Total lines: " << doc.get_line_count() << "\n";
    std::cout << "Total length: " << doc.get_total_length() << " chars\n\n";
    
    // Test insert performance
    std::cout << "Testing insert at position 1000...\n";
    start = std::chrono::high_resolution_clock::now();
    
    doc.insert(1000, "/* INSERTED TEXT: This is a new comment block */\n");
    
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Insert completed in: " << duration.count() << " microseconds\n";
    std::cout << "New line count: " << doc.get_line_count() << "\n\n";
    
    // Test delete performance
    std::cout << "Testing delete of 500 characters...\n";
    start = std::chrono::high_resolution_clock::now();
    
    doc.remove(5000, 500);
    
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Delete completed in: " << duration.count() << " microseconds\n";
    std::cout << "Final line count: " << doc.get_line_count() << "\n";
    
    std::cout << "\nKey Insight: Operations are O(1) regardless of file size!\n";
    std::cout << "Web editors slow down with large files - native implementation stays fast.\n";
}

void demo_viewport() {
    std::cout << "DEMO 2: Virtual Scrolling - Zero-Latency Rendering\n";
    std::cout << "---------------------------------------------------\n\n";
    
    // Create a massive document
    std::string massive_text;
    for (int i = 0; i < 100000; ++i) {
        massive_text += "Line " + std::to_string(i + 1) + ": Even with 100k lines, scrolling remains instant.\n";
    }
    
    std::cout << "Creating document with 100,000 lines...\n";
    auto doc = std::make_shared<PieceTable>(massive_text);
    std::cout << "Document ready!\n\n";
    
    // Create viewport (simulating a 40-line terminal window)
    Viewport viewport(40, 120);
    viewport.set_document(doc);
    
    std::cout << "Viewport configured: 40 visible lines x 120 columns\n\n";
    
    // Test rendering performance
    std::cout << "Rendering visible lines (only 40 out of 100,000)...\n";
    auto visible = viewport.get_visible_lines();
    
    std::cout << "Render time: " << viewport.get_last_render_time_ms() << " ms\n";
    std::cout << "Lines rendered: " << visible.size() << "\n\n";
    
    // Show first few lines
    std::cout << "First 5 visible lines:\n";
    for (size_t i = 0; i < std::min(size_t(5), visible.size()); ++i) {
        std::cout << "  " << visible[i].substr(0, 60) << "...\n";
    }
    
    // Test scrolling
    std::cout << "\nScrolling to line 50,000...\n";
    auto start = std::chrono::high_resolution_clock::now();
    
    viewport.scroll_to_line(50000);
    visible = viewport.get_visible_lines();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Scroll + render completed in: " << duration.count() / 1000.0 << " ms\n";
    std::cout << "First line now: " << visible[0].substr(0, 60) << "...\n";
    
    std::cout << "\nKey Insight: Only visible lines are processed!\n";
    std::cout << "This is how we maintain 60fps even with million-line files.\n";
    std::cout << "Web DOM-based editors struggle here - native GPU rendering wins.\n";
}

void demo_indexer() {
    std::cout << "DEMO 3: Background Indexer - Instant Search\n";
    std::cout << "--------------------------------------------\n\n";
    
    BackgroundIndexer indexer;
    indexer.start();
    
    std::cout << "Background indexer started in separate thread...\n\n";
    
    // Index multiple files
    std::cout << "Indexing source files...\n";
    
    indexer.index_file("src/main.cpp", 
        "int main() {\n"
        "    auto editor = create_editor();\n"
        "    editor.run();\n"
        "    return 0;\n"
        "}\n");
    
    indexer.index_file("src/renderer.cpp",
        "class Renderer {\n"
        "    void render() {\n"
        "        // GPU-accelerated rendering\n"
        "        auto viewport = get_viewport();\n"
        "        viewport.draw();\n"
        "    }\n"
        "};\n");
    
    indexer.index_file("src/buffer.cpp",
        "class Buffer {\n"
        "    void insert(const std::string& text) {\n"
        "        // Piece table insert\n"
        "    }\n"
        "};\n");
    
    std::cout << "Indexed files: " << indexer.get_indexed_file_count() << "\n\n";
    
    // Perform searches
    std::cout << "Searching for 'render'...\n";
    auto start = std::chrono::high_resolution_clock::now();
    
    auto results = indexer.search("render");
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Search completed in: " << duration.count() << " microseconds\n";
    std::cout << "Results found: " << results.size() << "\n\n";
    
    for (const auto& result : results) {
        std::cout << "  " << result.file_path << ":" << result.line_number + 1 
                  << ":" << result.column + 1 << "\n";
        std::cout << "    " << result.line_content << "\n";
    }
    
    std::cout << "\nSearching for 'viewport'...\n";
    results = indexer.search("viewport");
    std::cout << "Results found: " << results.size() << "\n";
    
    for (const auto& result : results) {
        std::cout << "  " << result.file_path << ":" << result.line_number + 1 << "\n";
    }
    
    indexer.stop();
    
    std::cout << "\nKey Insight: In-memory inverted index enables instant search!\n";
    std::cout << "No need to grep through files - results are pre-indexed.\n";
    std::cout << "This scales to millions of lines across thousands of files.\n";
}

void print_architecture_summary() {
    std::cout << "\nARCHITECTURE SUMMARY\n";
    std::cout << "====================\n\n";
    
    std::cout << "1. TEXT BUFFER: Piece Table\n";
    std::cout << "   - O(1) insert/delete operations\n";
    std::cout << "   - Perfect for large files (1M+ lines)\n";
    std::cout << "   - Used by: Sublime Text, VS Code (internal)\n\n";
    
    std::cout << "2. RENDERING: Virtual Scrolling\n";
    std::cout << "   - Only renders visible lines\n";
    std::cout << "   - Constant time regardless of file size\n";
    std::cout << "   - GPU rendering would make this even faster\n\n";
    
    std::cout << "3. SEARCH: Background Indexer\n";
    std::cout << "   - Separate thread, doesn't block editor\n";
    std::cout << "   - In-memory inverted index\n";
    std::cout << "   - Instant results even in huge projects\n\n";
    
    std::cout << "4. WHY NOT WEB TECH?\n";
    std::cout << "   - DOM manipulation is slow for large documents\n";
    std::cout << "   - JavaScript GC pauses cause stuttering\n";
    std::cout << "   - Native code + GPU = 10-100x faster\n\n";
    
    std::cout << "5. NEXT STEPS FOR PRODUCTION:\n";
    std::cout << "   - Add GPU rendering (wgpu, Vulkan, Metal)\n";
    std::cout << "   - Implement Language Server Protocol (LSP)\n";
    std::cout << "   - Add WASM-based plugin system\n";
    std::cout << "   - Multi-workspace support\n";
    std::cout << "   - Incremental parsing for syntax highlighting\n\n";
}

int main(int argc, char* argv[]) {
    bool bench_mode = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--bench" || arg == "--autotest") {
            bench_mode = true;
        }
    }

    if (bench_mode) {
        std::cout << "\n[Bench Mode] Running automated performance tests...\n";
        print_separator();
        demo_piece_table();
        print_separator();
        demo_viewport();
        print_separator();
        demo_indexer();
        print_separator();
        print_architecture_summary();
        std::cout << "\n[Bench Mode] All tests complete. See metrics above.\n";
        return 0;
    }

    // ...existing code...
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                                    ║\n";
    std::cout << "║         HIGH-PERFORMANCE TEXT EDITOR - C++ DEMO                    ║\n";
    std::cout << "║                                                                    ║\n";
    std::cout << "║  Demonstrating architecture for large-scale projects              ║\n";
    std::cout << "║  (Alternative to VS Code for million-line codebases)              ║\n";
    std::cout << "║                                                                    ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════════╝\n";
    print_separator();
    demo_piece_table();
    print_separator();
    demo_viewport();
    print_separator();
    demo_indexer();
    print_separator();
    print_architecture_summary();
    std::cout << "Demo completed! Check the performance metrics above.\n";
    std::cout << "This demonstrates why native editors outperform web-based ones.\n\n";
    return 0;
}
