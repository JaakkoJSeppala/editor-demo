#include "gpu_renderer.h"
#include <iostream>

namespace editor {

class StubGpuRenderer : public GpuRenderer {
public:
    bool initialize(const GpuRendererConfig& config) override {
        std::cout << "StubGpuRenderer initialized with backend: " << (int)config.backend << std::endl;
        return true;
    }
    void resize(int width, int height) override {
        std::cout << "StubGpuRenderer resize: " << width << "x" << height << std::endl;
    }
    void begin_frame() override {
        std::cout << "StubGpuRenderer begin_frame" << std::endl;
    }
    void end_frame() override {
        std::cout << "StubGpuRenderer end_frame" << std::endl;
    }
    void draw_text(const std::string& text, int x, int y, uint32_t color) override {
        std::cout << "StubGpuRenderer draw_text: '" << text << "' at (" << x << "," << y << ")" << std::endl;
    }
    void draw_rect(int x, int y, int w, int h, uint32_t color) override {
        std::cout << "StubGpuRenderer draw_rect: (" << x << "," << y << "," << w << "," << h << ")" << std::endl;
    }
    void draw_line(int x1, int y1, int x2, int y2, uint32_t color) override {
        std::cout << "StubGpuRenderer draw_line: (" << x1 << "," << y1 << ") to (" << x2 << "," << y2 << ")" << std::endl;
    }
    void present() override {
        std::cout << "StubGpuRenderer present" << std::endl;
    }
    void shutdown() override {
        std::cout << "StubGpuRenderer shutdown" << std::endl;
    }
};

GpuRenderer* GpuRenderer::create(const GpuRendererConfig& config) {
    return new StubGpuRenderer();
}

} // namespace editor
