#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

// Include platform_window before other headers
#include "platform_window.h"

#include <cstdio>
#include <thread>
#include <chrono>

using namespace editor;

int main() {
    printf("=== Platform Abstraction Test ===\n");
    
    // Create platform window
    auto window = create_platform_window();
    if (!window) {
        printf("[FAIL] Failed to create platform window\n");
        return 1;
    }
    
    printf("[PASS] Platform window created\n");
    
    // Create window
    if (!window->create("Platform Test Window", 800, 600)) {
        printf("[FAIL] Failed to create window\n");
        return 1;
    }
    
    printf("[PASS] Window created (800x600)\n");
    
    // Set up event handlers
    bool running = true;
    
    window->on_close = [&]() {
        printf("Window closed\n");
        running = false;
    };
    
    window->on_key_event = [&](const KeyEvent& evt) {
        if (evt.pressed) {
            printf("Key pressed: %d", static_cast<int>(evt.key));
            if (evt.character) {
                printf(" ('%c')", evt.character);
            }
            printf("\n");
            
            // Escape to quit
            if (evt.key == Key::Escape) {
                running = false;
            }
        }
    };
    
    window->on_mouse_event = [](const MouseEvent& evt) {
        if (evt.pressed) {
            printf("Mouse button pressed at (%d, %d)\n", evt.position.x, evt.position.y);
        }
        if (evt.wheel_delta != 0) {
            printf("Mouse wheel: %d\n", evt.wheel_delta);
        }
    };
    
    window->on_resize = [](const ResizeEvent& evt) {
        printf("Window resized to %dx%d\n", evt.new_size.width, evt.new_size.height);
    };
    
    window->on_paint = [&](const PaintEvent&) {
        // Clear to dark gray
        window->clear(Color(30, 30, 35));
        
        // Create font
        auto font = window->create_font("Consolas", 20, false, false);
        window->set_font(font);
        
        // Draw some text
        window->draw_text("Platform Abstraction Layer Test", 50, 50, Color(255, 255, 255));
        window->draw_text("Press ESC to quit", 50, 80, Color(200, 200, 200));
        window->draw_text("Try keyboard input and mouse clicks", 50, 110, Color(200, 200, 200));
        
        // Draw rectangle
        window->draw_rectangle(Rect(50, 150, 200, 100), Color(86, 156, 214), true);
        window->draw_text("Rectangle", 90, 185, Color(255, 255, 255));
        
        // Draw line
        window->draw_line(50, 270, 250, 270, Color(255, 0, 0));
        window->draw_text("Red Line", 50, 280, Color(255, 255, 255));
        
        // Color test
        Color test_color = Color::from_hex("#FF6B6B");
        window->draw_rectangle(Rect(50, 320, 50, 50), test_color, true);
        window->draw_text("From Hex: " + test_color.to_hex(), 110, 335, Color(255, 255, 255));
        
        window->destroy_font(font);
    };
    
    printf("[INFO] Showing window (press ESC to quit)\n");
    window->show();
    window->request_redraw();
    
    // Event loop
    while (running) {
        window->process_events();
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
    }
    
    printf("=== Test Complete ===\n");
    return 0;
}
