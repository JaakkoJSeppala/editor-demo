// GTK4 implementation of IPlatformWindow
// Linux platform window using GTK4 and Cairo for rendering

#ifdef PLATFORM_LINUX

#include "platform_window.h"
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <cairo/cairo.h>
#include <pango/pango.h>
#include <memory>
#include <unordered_map>
#include <cstring>

namespace {
    // Map GTK widgets to GtkWindow instances for event callbacks
    std::unordered_map<GtkWidget*, class GtkWindow*> window_map_;
}

class GtkWindow : public IPlatformWindow {
public:
    GtkWindow() 
        : window_(nullptr)
        , drawing_area_(nullptr)
        , cairo_context_(nullptr)
        , pango_layout_(nullptr)
        , current_font_(nullptr)
        , width_(0)
        , height_(0)
        , in_paint_(false) {
    }

    ~GtkWindow() override {
        destroy();
    }

    bool create(const std::string& title, int width, int height) override {
        width_ = width;
        height_ = height;

        // Create main window
        window_ = gtk_window_new();
        gtk_window_set_title(GTK_WINDOW(window_), title.c_str());
        gtk_window_set_default_size(GTK_WINDOW(window_), width, height);

        // Create drawing area for custom rendering
        drawing_area_ = gtk_drawing_area_new();
        gtk_window_set_child(GTK_WINDOW(window_), drawing_area_);

        // Set drawing function
        gtk_drawing_area_set_draw_func(
            GTK_DRAWING_AREA(drawing_area_),
            on_draw_static,
            this,
            nullptr
        );

        // Setup event controllers
        setup_event_controllers();

        // Register window in map
        window_map_[window_] = this;

        // Connect close event
        g_signal_connect(window_, "close-request", G_CALLBACK(on_close_request_static), this);

        return true;
    }

    void destroy() override {
        if (current_font_) {
            pango_font_description_free(current_font_);
            current_font_ = nullptr;
        }

        if (pango_layout_) {
            g_object_unref(pango_layout_);
            pango_layout_ = nullptr;
        }

        if (window_) {
            window_map_.erase(window_);
            gtk_window_destroy(GTK_WINDOW(window_));
            window_ = nullptr;
        }

        drawing_area_ = nullptr;
        cairo_context_ = nullptr;
    }

    void show() override {
        if (window_) {
            gtk_widget_set_visible(window_, TRUE);
        }
    }

    void hide() override {
        if (window_) {
            gtk_widget_set_visible(window_, FALSE);
        }
    }

    void set_title(const std::string& title) override {
        if (window_) {
            gtk_window_set_title(GTK_WINDOW(window_), title.c_str());
        }
    }

    void set_size(int width, int height) override {
        width_ = width;
        height_ = height;
        if (window_) {
            gtk_window_set_default_size(GTK_WINDOW(window_), width, height);
        }
    }

    Size get_size() const override {
        return Size{width_, height_};
    }

    void run_event_loop() override {
        // GTK4 uses GMainLoop
        GMainLoop* loop = g_main_loop_new(nullptr, FALSE);
        g_main_loop_run(loop);
        g_main_loop_unref(loop);
    }

    void process_events() override {
        // Process pending events without blocking
        while (g_main_context_pending(nullptr)) {
            g_main_context_iteration(nullptr, FALSE);
        }
    }

    void request_redraw() override {
        if (drawing_area_) {
            gtk_widget_queue_draw(drawing_area_);
        }
    }

    void* get_graphics_context() override {
        return cairo_context_;
    }

    void begin_paint() override {
        in_paint_ = true;
        // Cairo context is provided in draw callback
    }

    void end_paint() override {
        in_paint_ = false;
        // Cairo context is automatically flushed by GTK
    }

    PlatformFont create_font(const std::string& family, int size, bool bold, bool italic) override {
        PangoFontDescription* desc = pango_font_description_new();
        pango_font_description_set_family(desc, family.c_str());
        pango_font_description_set_size(desc, size * PANGO_SCALE);
        pango_font_description_set_weight(desc, bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
        pango_font_description_set_style(desc, italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);
        return desc;
    }

    void destroy_font(PlatformFont font) override {
        if (font) {
            pango_font_description_free(static_cast<PangoFontDescription*>(font));
        }
    }

    void set_font(PlatformFont font) override {
        if (current_font_) {
            pango_font_description_free(current_font_);
        }
        current_font_ = pango_font_description_copy(static_cast<PangoFontDescription*>(font));
    }

    Size measure_text(const std::string& text) override {
        if (!cairo_context_ || !current_font_) {
            return Size{0, 0};
        }

        // Create temporary layout if needed
        PangoLayout* layout = pango_layout_;
        if (!layout) {
            layout = pango_cairo_create_layout(cairo_context_);
        }

        pango_layout_set_font_description(layout, current_font_);
        pango_layout_set_text(layout, text.c_str(), -1);

        int w, h;
        pango_layout_get_pixel_size(layout, &w, &h);

        if (layout != pango_layout_) {
            g_object_unref(layout);
        }

        return Size{w, h};
    }

    void clear(const Color& color) override {
        if (!cairo_context_) return;

        cairo_set_source_rgb(cairo_context_, 
            color.r / 255.0, 
            color.g / 255.0, 
            color.b / 255.0
        );
        cairo_paint(cairo_context_);
    }

    void draw_rectangle(const Rect& rect, const Color& color, bool filled) override {
        if (!cairo_context_) return;

        cairo_set_source_rgba(cairo_context_, 
            color.r / 255.0, 
            color.g / 255.0, 
            color.b / 255.0,
            color.a / 255.0
        );

        cairo_rectangle(cairo_context_, rect.x, rect.y, rect.width, rect.height);

        if (filled) {
            cairo_fill(cairo_context_);
        } else {
            cairo_stroke(cairo_context_);
        }
    }

    void draw_text(const std::string& text, int x, int y, const Color& color) override {
        if (!cairo_context_ || !current_font_) return;

        if (!pango_layout_) {
            pango_layout_ = pango_cairo_create_layout(cairo_context_);
        }

        pango_layout_set_font_description(pango_layout_, current_font_);
        pango_layout_set_text(pango_layout_, text.c_str(), -1);

        cairo_set_source_rgba(cairo_context_,
            color.r / 255.0,
            color.g / 255.0,
            color.b / 255.0,
            color.a / 255.0
        );

        cairo_move_to(cairo_context_, x, y);
        pango_cairo_show_layout(cairo_context_, pango_layout_);
    }

    void draw_line(int x1, int y1, int x2, int y2, const Color& color) override {
        if (!cairo_context_) return;

        cairo_set_source_rgba(cairo_context_,
            color.r / 255.0,
            color.g / 255.0,
            color.b / 255.0,
            color.a / 255.0
        );

        cairo_move_to(cairo_context_, x1, y1);
        cairo_line_to(cairo_context_, x2, y2);
        cairo_stroke(cairo_context_);
    }

    std::string get_clipboard_text() override {
        GdkClipboard* clipboard = gdk_display_get_clipboard(gdk_display_get_default());
        // GTK4 clipboard is async, but we need sync API for compatibility
        // This is a simplified version - proper implementation would use async
        const char* text = ""; // Would need proper async handling
        return std::string(text);
    }

    void set_clipboard_text(const std::string& text) override {
        GdkClipboard* clipboard = gdk_display_get_clipboard(gdk_display_get_default());
        gdk_clipboard_set_text(clipboard, text.c_str());
    }

    void set_cursor_visible(bool visible) override {
        // GTK4 cursor visibility
        if (window_) {
            if (visible) {
                gtk_widget_set_cursor_from_name(window_, "text");
            } else {
                gtk_widget_set_cursor_from_name(window_, "none");
            }
        }
    }

    PlatformWindow get_native_handle() const override {
        return window_;
    }

private:
    void setup_event_controllers() {
        // Key event controller
        GtkEventController* key_controller = gtk_event_controller_key_new();
        g_signal_connect(key_controller, "key-pressed", G_CALLBACK(on_key_pressed_static), this);
        g_signal_connect(key_controller, "key-released", G_CALLBACK(on_key_released_static), this);
        gtk_widget_add_controller(window_, key_controller);

        // Mouse click controller
        GtkGesture* click_gesture = gtk_gesture_click_new();
        g_signal_connect(click_gesture, "pressed", G_CALLBACK(on_button_pressed_static), this);
        g_signal_connect(click_gesture, "released", G_CALLBACK(on_button_released_static), this);
        gtk_widget_add_controller(drawing_area_, GTK_EVENT_CONTROLLER(click_gesture));

        // Mouse motion controller
        GtkEventController* motion_controller = gtk_event_controller_motion_new();
        g_signal_connect(motion_controller, "motion", G_CALLBACK(on_motion_static), this);
        gtk_widget_add_controller(drawing_area_, motion_controller);

        // Scroll controller
        GtkEventController* scroll_controller = gtk_event_controller_scroll_new(
            GTK_EVENT_CONTROLLER_SCROLL_BOTH_AXES
        );
        g_signal_connect(scroll_controller, "scroll", G_CALLBACK(on_scroll_static), this);
        gtk_widget_add_controller(drawing_area_, scroll_controller);
    }

    // Static callbacks
    static void on_draw_static(GtkDrawingArea* area, cairo_t* cr, int width, int height, gpointer user_data) {
        GtkWindow* window = static_cast<GtkWindow*>(user_data);
        window->on_draw(cr, width, height);
    }

    static gboolean on_close_request_static(GtkWindow* widget, gpointer user_data) {
        GtkWindow* window = static_cast<GtkWindow*>(user_data);
        if (window->on_close) {
            window->on_close();
        }
        return TRUE;
    }

    static gboolean on_key_pressed_static(GtkEventControllerKey* controller, guint keyval, guint keycode, GdkModifierType state, gpointer user_data) {
        GtkWindow* window = static_cast<GtkWindow*>(user_data);
        return window->handle_key_event(keyval, keycode, state, true);
    }

    static gboolean on_key_released_static(GtkEventControllerKey* controller, guint keyval, guint keycode, GdkModifierType state, gpointer user_data) {
        GtkWindow* window = static_cast<GtkWindow*>(user_data);
        return window->handle_key_event(keyval, keycode, state, false);
    }

    static void on_button_pressed_static(GtkGestureClick* gesture, int n_press, double x, double y, gpointer user_data) {
        GtkWindow* window = static_cast<GtkWindow*>(user_data);
        int button = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture));
        window->handle_mouse_button(button, x, y, true);
    }

    static void on_button_released_static(GtkGestureClick* gesture, int n_press, double x, double y, gpointer user_data) {
        GtkWindow* window = static_cast<GtkWindow*>(user_data);
        int button = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture));
        window->handle_mouse_button(button, x, y, false);
    }

    static void on_motion_static(GtkEventControllerMotion* controller, double x, double y, gpointer user_data) {
        GtkWindow* window = static_cast<GtkWindow*>(user_data);
        window->handle_mouse_motion(x, y);
    }

    static gboolean on_scroll_static(GtkEventControllerScroll* controller, double dx, double dy, gpointer user_data) {
        GtkWindow* window = static_cast<GtkWindow*>(user_data);
        window->handle_scroll(dx, dy);
        return TRUE;
    }

    // Event handlers
    void on_draw(cairo_t* cr, int width, int height) {
        cairo_context_ = cr;
        width_ = width;
        height_ = height;

        if (on_paint) {
            PaintEvent event;
            on_paint(event);
        }

        cairo_context_ = nullptr;
    }

    bool handle_key_event(guint keyval, guint keycode, GdkModifierType state, bool pressed) {
        if (!on_key_event) return FALSE;

        KeyEvent event;
        event.key = translate_key(keyval);
        event.modifiers = translate_modifiers(state);
        event.pressed = pressed;
        event.character = (keyval < 128) ? static_cast<char>(keyval) : '\0';

        on_key_event(event);
        return TRUE;
    }

    void handle_mouse_button(int button, double x, double y, bool pressed) {
        if (!on_mouse_event) return;

        MouseEvent event;
        event.button = translate_mouse_button(button);
        event.x = static_cast<int>(x);
        event.y = static_cast<int>(y);
        event.modifiers = KeyModifier::None; // Would need to track modifier state
        event.pressed = pressed;
        event.wheel_delta = 0;

        on_mouse_event(event);
    }

    void handle_mouse_motion(double x, double y) {
        if (!on_mouse_event) return;

        MouseEvent event;
        event.button = MouseButton::None;
        event.x = static_cast<int>(x);
        event.y = static_cast<int>(y);
        event.modifiers = KeyModifier::None;
        event.pressed = false;
        event.wheel_delta = 0;

        on_mouse_event(event);
    }

    void handle_scroll(double dx, double dy) {
        if (!on_mouse_event) return;

        MouseEvent event;
        event.button = MouseButton::None;
        event.x = 0;
        event.y = 0;
        event.modifiers = KeyModifier::None;
        event.pressed = false;
        event.wheel_delta = static_cast<int>(-dy * 120); // Convert to Windows-style wheel delta

        on_mouse_event(event);
    }

    // Key translation
    Key translate_key(guint keyval) {
        // Letters A-Z
        if (keyval >= GDK_KEY_a && keyval <= GDK_KEY_z) {
            return static_cast<Key>(static_cast<int>(Key::A) + (keyval - GDK_KEY_a));
        }
        if (keyval >= GDK_KEY_A && keyval <= GDK_KEY_Z) {
            return static_cast<Key>(static_cast<int>(Key::A) + (keyval - GDK_KEY_A));
        }

        // Numbers 0-9
        if (keyval >= GDK_KEY_0 && keyval <= GDK_KEY_9) {
            return static_cast<Key>(static_cast<int>(Key::Num0) + (keyval - GDK_KEY_0));
        }

        // Function keys
        if (keyval >= GDK_KEY_F1 && keyval <= GDK_KEY_F12) {
            return static_cast<Key>(static_cast<int>(Key::F1) + (keyval - GDK_KEY_F1));
        }

        // Special keys
        switch (keyval) {
            case GDK_KEY_Return: return Key::Enter;
            case GDK_KEY_Escape: return Key::Escape;
            case GDK_KEY_BackSpace: return Key::Backspace;
            case GDK_KEY_Tab: return Key::Tab;
            case GDK_KEY_space: return Key::Space;
            case GDK_KEY_Left: return Key::Left;
            case GDK_KEY_Right: return Key::Right;
            case GDK_KEY_Up: return Key::Up;
            case GDK_KEY_Down: return Key::Down;
            case GDK_KEY_Home: return Key::Home;
            case GDK_KEY_End: return Key::End;
            case GDK_KEY_Page_Up: return Key::PageUp;
            case GDK_KEY_Page_Down: return Key::PageDown;
            case GDK_KEY_Insert: return Key::Insert;
            case GDK_KEY_Delete: return Key::Delete;
            default: return Key::Unknown;
        }
    }

    KeyModifier translate_modifiers(GdkModifierType state) {
        int modifiers = static_cast<int>(KeyModifier::None);
        if (state & GDK_SHIFT_MASK) modifiers |= static_cast<int>(KeyModifier::Shift);
        if (state & GDK_CONTROL_MASK) modifiers |= static_cast<int>(KeyModifier::Ctrl);
        if (state & GDK_ALT_MASK) modifiers |= static_cast<int>(KeyModifier::Alt);
        return static_cast<KeyModifier>(modifiers);
    }

    MouseButton translate_mouse_button(int button) {
        switch (button) {
            case 1: return MouseButton::Left;
            case 2: return MouseButton::Middle;
            case 3: return MouseButton::Right;
            default: return MouseButton::None;
        }
    }

    GtkWidget* window_;
    GtkWidget* drawing_area_;
    cairo_t* cairo_context_;
    PangoLayout* pango_layout_;
    PangoFontDescription* current_font_;
    int width_;
    int height_;
    bool in_paint_;
};

// Factory function for Linux
std::unique_ptr<IPlatformWindow> create_platform_window() {
    return std::make_unique<GtkWindow>();
}

#endif // PLATFORM_LINUX
