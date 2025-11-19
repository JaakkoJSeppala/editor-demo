// editor_test.cpp
// Comprehensive test program for Velocity Editor core features

#include "platform_file.h"
#include "platform_process.h"
#include "gpu_renderer.h"
#include "rope_table.h"
#include "lsp_client.h"
#include "refactor_api.h"
#include <iostream>
#include <vector>
#include <string>

using namespace editor;

int main() {
    std::cout << "Velocity Editor Test Program\n";
    std::cout << "===========================\n";

    // RefactorAPI test
    std::cout << "\nRefactorAPI Test\n----------------\n";
    LSPClient lsp;
    RefactorAPI refactor(&lsp);
    std::string test_uri = "file:///test_file.cpp";
    int test_line = 10, test_char = 5;
    refactor.rename_symbol(test_uri, test_line, test_char, "newSymbolName", [](RefactorResult result) {
        std::cout << "Rename result: " << (result.success ? "OK" : "FAIL") << ", " << result.message << std::endl;
    });
    refactor.code_cleanup(test_uri, [](RefactorResult result) {
        std::cout << "Code cleanup result: " << (result.success ? "OK" : "FAIL") << ", " << result.message << std::endl;
    });

    // RopeTable benchmark
    std::cout << "\nRopeTable Benchmark\n-------------------\n";
    RopeTable rope;
    const size_t N = 100000;
    std::string sample = "abc";
    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < N; ++i) {
        rope.insert(rope.get_total_length(), sample);
    }
    auto mid = std::chrono::high_resolution_clock::now();
    std::string text = rope.get_text(0, rope.get_total_length());
    auto end = std::chrono::high_resolution_clock::now();
    auto insert_ms = std::chrono::duration_cast<std::chrono::milliseconds>(mid - start).count();
    auto read_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - mid).count();
    std::cout << "Inserted " << N << " x 'abc' in " << insert_ms << " ms\n";
    std::cout << "Read " << text.length() << " chars in " << read_ms << " ms\n";
    std::cout << "First 15 chars: " << text.substr(0, 15) << "\n";
    std::cout << "Last 15 chars: " << text.substr(text.length() - 15) << "\n";


    // 1. File I/O test
    std::string test_path = "test_file.txt";
    std::string test_content = "Hello, Velocity Editor!\nLine 2.\nLine 3.";
    bool write_ok = PlatformFile::write_file(test_path, test_content, LineEnding::LF);
    std::cout << "File write: " << (write_ok ? "OK" : "FAIL") << std::endl;
    std::string read_content;
    bool read_ok = PlatformFile::read_file(test_path, read_content, LineEnding::Auto);
    std::cout << "File read: " << (read_ok ? "OK" : "FAIL") << std::endl;
    std::cout << "Read content:\n" << read_content << std::endl;

    // 2. Process spawning test
    std::string output;
    int exit_code = 0;
    bool proc_ok = ProcessUtils::execute(
#ifdef _WIN32
        "cmd /C echo Hello from process!",
#else
        "echo Hello from process!",
#endif
        output, exit_code, 2000);
    std::cout << "Process spawn: " << (proc_ok ? "OK" : "FAIL") << ", exit code: " << exit_code << std::endl;
    std::cout << "Process output: " << output << std::endl;

    // 3. GPU renderer stub test
    GpuRendererConfig cfg;
    cfg.backend = GpuBackend::Auto;
    cfg.width = 640;
    cfg.height = 480;
    cfg.enable_vsync = false;
    cfg.debug = true;
    GpuRenderer* renderer = GpuRenderer::create(cfg);
    if (renderer && renderer->initialize(cfg)) {
        renderer->begin_frame();
        renderer->draw_rect(50, 50, 200, 100, 0xFF00FF00);
        renderer->draw_text("GPU Test", 60, 90, 0xFFFFFFFF);
        renderer->draw_line(50, 50, 250, 150, 0xFFFF0000);
        renderer->end_frame();
        renderer->present();
        renderer->shutdown();
        std::cout << "GPU renderer test: OK" << std::endl;
    } else {
        std::cout << "GPU renderer test: FAIL" << std::endl;
    }
    delete renderer;

    // 4. Event simulation (basic)
    std::cout << "Simulating basic events...\n";
    // Simulate file open, process run, and render events
    std::cout << "[Event] File opened: " << test_path << std::endl;
    std::cout << "[Event] Process completed: " << output << std::endl;
    std::cout << "[Event] Render frame complete." << std::endl;

    std::cout << "All tests complete.\n";
    return 0;
}
