#include "gpu_renderer.h"
#include <iostream>
#include <cassert>

int main() {
    GpuRendererConfig cfg;
    cfg.backend = GpuBackend::Auto;
    cfg.width = 320;
    cfg.height = 240;
    cfg.enable_vsync = false;
    cfg.debug = true;
    GpuRenderer* renderer = GpuRenderer::create(cfg);
    assert(renderer);
    assert(renderer->initialize(cfg));
    renderer->begin_frame();
    renderer->draw_rect(10, 10, 100, 50, 0xFF00FF00);
    renderer->draw_text("Test", 20, 30, 0xFFFFFFFF);
    renderer->draw_line(10, 10, 110, 60, 0xFFFF0000);
    renderer->end_frame();
    renderer->present();
    renderer->shutdown();
    delete renderer;
    std::cout << "GpuRenderer tests passed.\n";
    return 0;
}
