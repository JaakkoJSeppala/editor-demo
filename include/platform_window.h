#ifndef PLATFORM_WINDOW_H
#define PLATFORM_WINDOW_H

#include "platform_types.h"
#include <string>
#include <functional>
#include <memory>

namespace editor {

// Event types
struct KeyEvent {
    Key key;
    KeyModifier modifiers;
    bool pressed;  // true = pressed, false = released
    char character;  // For text input
};

struct MouseEvent {
    MouseButton button;
    Point position;
    KeyModifier modifiers;
    bool pressed;  // true = pressed, false = released
    int wheel_delta;  // For scroll events
};

struct ResizeEvent {
    Size new_size;
};

struct PaintEvent {
    Rect damaged_rect;
};

// Abstract platform window interface
class IPlatformWindow {
public:
    virtual ~IPlatformWindow() = default;
    
    // Window management
    virtual bool create(const std::string& title, int width, int height) = 0;
    virtual void destroy() = 0;
    virtual void show() = 0;
    virtual void hide() = 0;
    virtual void set_title(const std::string& title) = 0;
    virtual void set_size(int width, int height) = 0;
    virtual Size get_size() const = 0;
    
    // Event loop
    virtual void run_event_loop() = 0;
    virtual void process_events() = 0;  // Process pending events without blocking
    virtual void request_redraw() = 0;
    
    // Drawing
    virtual PlatformGraphicsContext get_graphics_context() = 0;
    virtual void begin_paint() = 0;
    virtual void end_paint() = 0;
    
    // Font handling
    virtual PlatformFont create_font(const std::string& family, int size, bool bold = false, bool italic = false) = 0;
    virtual void destroy_font(PlatformFont font) = 0;
    virtual void set_font(PlatformFont font) = 0;
    virtual Size measure_text(const std::string& text) = 0;
    
    // Drawing primitives
    virtual void clear(const Color& color) = 0;
    virtual void draw_rectangle(const Rect& rect, const Color& color, bool filled = true) = 0;
    virtual void draw_text(const std::string& text, int x, int y, const Color& color) = 0;
    virtual void draw_line(int x1, int y1, int x2, int y2, const Color& color) = 0;
    
    // Clipboard
    virtual std::string get_clipboard_text() = 0;
    virtual void set_clipboard_text(const std::string& text) = 0;
    
    // Cursor
    virtual void set_cursor_visible(bool visible) = 0;
    
    // Event callbacks
    std::function<void(const KeyEvent&)> on_key_event;
    std::function<void(const MouseEvent&)> on_mouse_event;
    std::function<void(const ResizeEvent&)> on_resize;
    std::function<void(const PaintEvent&)> on_paint;
    std::function<void()> on_close;
    
    // Get native platform handle
    virtual PlatformWindow get_native_handle() const = 0;
};

// Factory function to create platform-specific window
std::unique_ptr<IPlatformWindow> create_platform_window();

} // namespace editor

#endif // PLATFORM_WINDOW_H
