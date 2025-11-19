#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <functional>

namespace editor {

// Supported GPU backends
enum class GpuBackend {
    Auto,
    Vulkan,
    Metal,
    DirectX12,
    OpenGL,
    WGPU
};

struct GpuRendererConfig {
    GpuBackend backend = GpuBackend::Auto;
    int width = 0;
    int height = 0;
    bool enable_vsync = true;
    bool enable_hdr = false;
    bool debug = false;
};

class GpuRenderer {
public:
    virtual ~GpuRenderer() {}
    virtual bool initialize(const GpuRendererConfig& config) = 0;
    virtual void resize(int width, int height) = 0;
    virtual void begin_frame() = 0;
    virtual void end_frame() = 0;
    virtual void draw_text(const std::string& text, int x, int y, uint32_t color) = 0;
    virtual void draw_rect(int x, int y, int w, int h, uint32_t color) = 0;
    virtual void draw_line(int x1, int y1, int x2, int y2, uint32_t color) = 0;
    virtual void present() = 0;
    virtual void shutdown() = 0;

    // Factory
    static GpuRenderer* create(const GpuRendererConfig& config);
};

} // namespace editor
