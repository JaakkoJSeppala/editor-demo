#ifndef PLATFORM_TYPES_H
#define PLATFORM_TYPES_H

#include <cstdint>
#include <string>

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS 1
#elif defined(__linux__)
    #define PLATFORM_LINUX 1
#elif defined(__APPLE__) && defined(__MACH__)
    #define PLATFORM_MACOS 1
#else
    #error "Unsupported platform"
#endif

namespace editor {

// Color type (platform-agnostic RGBA)
struct Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
    
    Color() : r(0), g(0), b(0), a(255) {}
    Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)
        : r(red), g(green), b(blue), a(alpha) {}
    
    // Create from Windows COLORREF (0x00BBGGRR)
    static Color from_colorref(uint32_t colorref) {
        return Color(
            static_cast<uint8_t>(colorref & 0xFF),        // R
            static_cast<uint8_t>((colorref >> 8) & 0xFF), // G
            static_cast<uint8_t>((colorref >> 16) & 0xFF) // B
        );
    }
    
    // Convert to Windows COLORREF
    uint32_t to_colorref() const {
        return r | (g << 8) | (b << 16);
    }
    
    // Create from hex string "#RRGGBB" or "#RRGGBBAA"
    static Color from_hex(const std::string& hex);
    
    // Convert to hex string "#RRGGBB"
    std::string to_hex() const;
};

// Rectangle (platform-agnostic)
struct Rect {
    int x;
    int y;
    int width;
    int height;
    
    Rect() : x(0), y(0), width(0), height(0) {}
    Rect(int x_, int y_, int w, int h) : x(x_), y(y_), width(w), height(h) {}
    
    int right() const { return x + width; }
    int bottom() const { return y + height; }
    
    bool contains(int px, int py) const {
        return px >= x && px < x + width && py >= y && py < y + height;
    }
};

// Point
struct Point {
    int x;
    int y;
    
    Point() : x(0), y(0) {}
    Point(int x_, int y_) : x(x_), y(y_) {}
};

// Size
struct Size {
    int width;
    int height;
    
    Size() : width(0), height(0) {}
    Size(int w, int h) : width(w), height(h) {}
};

// Key codes (platform-agnostic)
enum class Key {
    Unknown = 0,
    
    // Letters
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    
    // Numbers
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    
    // Function keys
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    
    // Control keys
    Escape, Tab, CapsLock, Shift, Control, Alt, Space,
    Enter, Backspace, Delete, Insert,
    Home, End, PageUp, PageDown,
    Left, Right, Up, Down,
    
    // Punctuation
    Minus, Equal, LeftBracket, RightBracket,
    Semicolon, Quote, Comma, Period, Slash, Backslash, Grave,
    
    // Keypad
    NumpadAdd, NumpadSubtract, NumpadMultiply, NumpadDivide,
    Numpad0, Numpad1, Numpad2, Numpad3, Numpad4,
    Numpad5, Numpad6, Numpad7, Numpad8, Numpad9
};

// Mouse button
enum class MouseButton {
    None = 0,
    Left,
    Middle,
    Right
};

// Modifier keys
enum class KeyModifier {
    None = 0,
    Shift = 1 << 0,
    Control = 1 << 1,
    Alt = 1 << 2,
    Super = 1 << 3  // Windows key / Command key
};

inline KeyModifier operator|(KeyModifier a, KeyModifier b) {
    return static_cast<KeyModifier>(static_cast<int>(a) | static_cast<int>(b));
}

inline bool operator&(KeyModifier a, KeyModifier b) {
    return (static_cast<int>(a) & static_cast<int>(b)) != 0;
}

// Platform-specific handle types
#ifdef PLATFORM_WINDOWS
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #undef min
    #undef max
    using PlatformWindow = HWND;
    using PlatformGraphicsContext = HDC;
    using PlatformFont = HFONT;
#elif defined(PLATFORM_LINUX)
    // Forward declarations for GTK types
    typedef struct _GtkWidget GtkWidget;
    typedef struct _cairo cairo_t;
    typedef struct _PangoFontDescription PangoFontDescription;
    
    using PlatformWindow = GtkWidget*;
    using PlatformGraphicsContext = cairo_t*;
    using PlatformFont = PangoFontDescription*;
#elif defined(PLATFORM_MACOS)
    // Forward declarations for Cocoa types
    #ifdef __OBJC__
        @class NSWindow;
        @class NSView;
        typedef NSWindow* PlatformWindow;
        typedef NSView* PlatformView;
    #else
        using PlatformWindow = void*;
        using PlatformView = void*;
    #endif
    using PlatformGraphicsContext = void*;  // CGContextRef
    using PlatformFont = void*;              // CTFontRef
#endif

} // namespace editor

#endif // PLATFORM_TYPES_H
