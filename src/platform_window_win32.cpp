#include "platform_window.h"

#ifdef PLATFORM_WINDOWS

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>  // For GET_X_LPARAM, GET_Y_LPARAM
#include <unordered_map>
#undef min
#undef max

#include "platform_window.h"

namespace editor {

// Win32 implementation of IPlatformWindow
class Win32Window : public IPlatformWindow {
public:
    Win32Window();
    ~Win32Window() override;
    
    // IPlatformWindow implementation
    bool create(const std::string& title, int width, int height) override;
    void destroy() override;
    void show() override;
    void hide() override;
    void set_title(const std::string& title) override;
    void set_size(int width, int height) override;
    Size get_size() const override;
    
    void run_event_loop() override;
    void process_events() override;
    void request_redraw() override;
    
    PlatformGraphicsContext get_graphics_context() override;
    void begin_paint() override;
    void end_paint() override;
    
    PlatformFont create_font(const std::string& family, int size, bool bold, bool italic) override;
    void destroy_font(PlatformFont font) override;
    void set_font(PlatformFont font) override;
    Size measure_text(const std::string& text) override;
    
    void clear(const Color& color) override;
    void draw_rectangle(const Rect& rect, const Color& color, bool filled) override;
    void draw_text(const std::string& text, int x, int y, const Color& color) override;
    void draw_line(int x1, int y1, int x2, int y2, const Color& color) override;
    
    std::string get_clipboard_text() override;
    void set_clipboard_text(const std::string& text) override;
    
    void set_cursor_visible(bool visible) override;
    
    PlatformWindow get_native_handle() const override { return hwnd_; }
    
private:
    HWND hwnd_;
    HDC hdc_;
    HFONT current_font_;
    PAINTSTRUCT ps_;
    bool in_paint_;
    
    static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    static std::unordered_map<HWND, Win32Window*> window_map_;
    
    Key translate_key(WPARAM wparam);
    KeyModifier get_modifiers();
};

std::unordered_map<HWND, Win32Window*> Win32Window::window_map_;

Win32Window::Win32Window()
    : hwnd_(nullptr)
    , hdc_(nullptr)
    , current_font_(nullptr)
    , in_paint_(false)
{
}

Win32Window::~Win32Window() {
    destroy();
}

bool Win32Window::create(const std::string& title, int width, int height) {
    // Register window class
    static bool class_registered = false;
    if (!class_registered) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = window_proc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = L"VelocityEditorWindow";
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
        
        if (!RegisterClassW(&wc)) {
            return false;
        }
        class_registered = true;
    }
    
    // Convert title to wide string
    int wlen = MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, nullptr, 0);
    std::wstring wtitle(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, &wtitle[0], wlen);
    
    // Create window
    hwnd_ = CreateWindowW(
        L"VelocityEditorWindow",
        wtitle.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        width, height,
        nullptr, nullptr,
        GetModuleHandle(nullptr),
        nullptr
    );
    
    if (!hwnd_) {
        return false;
    }
    
    // Store this pointer for window proc
    window_map_[hwnd_] = this;
    
    // Get DC
    hdc_ = GetDC(hwnd_);
    
    return true;
}

void Win32Window::destroy() {
    if (hwnd_) {
        window_map_.erase(hwnd_);
        if (hdc_) {
            ReleaseDC(hwnd_, hdc_);
            hdc_ = nullptr;
        }
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
}

void Win32Window::show() {
    if (hwnd_) {
        ShowWindow(hwnd_, SW_SHOW);
        UpdateWindow(hwnd_);
    }
}

void Win32Window::hide() {
    if (hwnd_) {
        ShowWindow(hwnd_, SW_HIDE);
    }
}

void Win32Window::set_title(const std::string& title) {
    if (hwnd_) {
        int wlen = MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, nullptr, 0);
        std::wstring wtitle(wlen, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, &wtitle[0], wlen);
        SetWindowTextW(hwnd_, wtitle.c_str());
    }
}

void Win32Window::set_size(int width, int height) {
    if (hwnd_) {
        SetWindowPos(hwnd_, nullptr, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
    }
}

Size Win32Window::get_size() const {
    if (hwnd_) {
        RECT rect;
        GetClientRect(hwnd_, &rect);
        return Size(rect.right - rect.left, rect.bottom - rect.top);
    }
    return Size();
}

void Win32Window::run_event_loop() {
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void Win32Window::process_events() {
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void Win32Window::request_redraw() {
    if (hwnd_) {
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
}

PlatformGraphicsContext Win32Window::get_graphics_context() {
    return in_paint_ ? ps_.hdc : hdc_;
}

void Win32Window::begin_paint() {
    if (hwnd_) {
        BeginPaint(hwnd_, &ps_);
        in_paint_ = true;
    }
}

void Win32Window::end_paint() {
    if (hwnd_ && in_paint_) {
        EndPaint(hwnd_, &ps_);
        in_paint_ = false;
    }
}

PlatformFont Win32Window::create_font(const std::string& family, int size, bool bold, bool italic) {
    int wlen = MultiByteToWideChar(CP_UTF8, 0, family.c_str(), -1, nullptr, 0);
    std::wstring wfamily(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, family.c_str(), -1, &wfamily[0], wlen);
    
    return CreateFontW(
        size, 0, 0, 0,
        bold ? FW_BOLD : FW_NORMAL,
        italic ? TRUE : FALSE,
        FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        FIXED_PITCH | FF_MODERN,
        wfamily.c_str()
    );
}

void Win32Window::destroy_font(PlatformFont font) {
    if (font) {
        DeleteObject(font);
    }
}

void Win32Window::set_font(PlatformFont font) {
    current_font_ = font;
    if (hdc_) {
        SelectObject(hdc_, font);
    }
}

Size Win32Window::measure_text(const std::string& text) {
    if (!hdc_) return Size();
    
    SIZE size;
    GetTextExtentPoint32A(hdc_, text.c_str(), static_cast<int>(text.length()), &size);
    return Size(size.cx, size.cy);
}

void Win32Window::clear(const Color& color) {
    if (!hdc_) return;
    
    RECT rect;
    GetClientRect(hwnd_, &rect);
    HBRUSH brush = CreateSolidBrush(color.to_colorref());
    FillRect(hdc_, &rect, brush);
    DeleteObject(brush);
}

void Win32Window::draw_rectangle(const Rect& rect, const Color& color, bool filled) {
    if (!hdc_) return;
    
    HBRUSH brush = CreateSolidBrush(color.to_colorref());
    RECT r = {rect.x, rect.y, rect.right(), rect.bottom()};
    
    if (filled) {
        FillRect(hdc_, &r, brush);
    } else {
        FrameRect(hdc_, &r, brush);
    }
    
    DeleteObject(brush);
}

void Win32Window::draw_text(const std::string& text, int x, int y, const Color& color) {
    if (!hdc_) return;
    
    SetTextColor(hdc_, color.to_colorref());
    SetBkMode(hdc_, TRANSPARENT);
    TextOutA(hdc_, x, y, text.c_str(), static_cast<int>(text.length()));
}

void Win32Window::draw_line(int x1, int y1, int x2, int y2, const Color& color) {
    if (!hdc_) return;
    
    HPEN pen = CreatePen(PS_SOLID, 1, color.to_colorref());
    HPEN old_pen = static_cast<HPEN>(SelectObject(hdc_, pen));
    
    MoveToEx(hdc_, x1, y1, nullptr);
    LineTo(hdc_, x2, y2);
    
    SelectObject(hdc_, old_pen);
    DeleteObject(pen);
}

std::string Win32Window::get_clipboard_text() {
    if (!OpenClipboard(hwnd_)) {
        return "";
    }
    
    HANDLE data = GetClipboardData(CF_TEXT);
    if (!data) {
        CloseClipboard();
        return "";
    }
    
    const char* text = static_cast<const char*>(GlobalLock(data));
    std::string result = text ? text : "";
    GlobalUnlock(data);
    CloseClipboard();
    
    return result;
}

void Win32Window::set_clipboard_text(const std::string& text) {
    if (!OpenClipboard(hwnd_)) {
        return;
    }
    
    EmptyClipboard();
    
    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, text.length() + 1);
    if (mem) {
        char* ptr = static_cast<char*>(GlobalLock(mem));
        if (ptr) {
            memcpy(ptr, text.c_str(), text.length() + 1);
            GlobalUnlock(mem);
            SetClipboardData(CF_TEXT, mem);
        }
    }
    
    CloseClipboard();
}

void Win32Window::set_cursor_visible(bool visible) {
    ShowCursor(visible ? TRUE : FALSE);
}

Key Win32Window::translate_key(WPARAM wparam) {
    // Simplified key translation - full implementation would map all VK_ codes
    switch (wparam) {
        case VK_ESCAPE: return Key::Escape;
        case VK_TAB: return Key::Tab;
        case VK_RETURN: return Key::Enter;
        case VK_BACK: return Key::Backspace;
        case VK_DELETE: return Key::Delete;
        case VK_INSERT: return Key::Insert;
        case VK_HOME: return Key::Home;
        case VK_END: return Key::End;
        case VK_PRIOR: return Key::PageUp;
        case VK_NEXT: return Key::PageDown;
        case VK_LEFT: return Key::Left;
        case VK_RIGHT: return Key::Right;
        case VK_UP: return Key::Up;
        case VK_DOWN: return Key::Down;
        case VK_F1: return Key::F1;
        case VK_F2: return Key::F2;
        case VK_F3: return Key::F3;
        case VK_F4: return Key::F4;
        case VK_F5: return Key::F5;
        case VK_F12: return Key::F12;
        default:
            if (wparam >= 'A' && wparam <= 'Z') {
                return static_cast<Key>(static_cast<int>(Key::A) + (wparam - 'A'));
            }
            if (wparam >= '0' && wparam <= '9') {
                return static_cast<Key>(static_cast<int>(Key::Num0) + (wparam - '0'));
            }
            return Key::Unknown;
    }
}

KeyModifier Win32Window::get_modifiers() {
    KeyModifier mods = KeyModifier::None;
    if (GetKeyState(VK_SHIFT) & 0x8000) mods = mods | KeyModifier::Shift;
    if (GetKeyState(VK_CONTROL) & 0x8000) mods = mods | KeyModifier::Control;
    if (GetKeyState(VK_MENU) & 0x8000) mods = mods | KeyModifier::Alt;
    return mods;
}

LRESULT CALLBACK Win32Window::window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    auto it = window_map_.find(hwnd);
    if (it == window_map_.end()) {
        return DefWindowProc(hwnd, msg, wparam, lparam);
    }
    
    Win32Window* window = it->second;
    
    switch (msg) {
        case WM_CLOSE:
            if (window->on_close) {
                window->on_close();
            }
            return 0;
            
        case WM_PAINT:
            if (window->on_paint) {
                window->begin_paint();
                PaintEvent evt;
                evt.damaged_rect = Rect(
                    window->ps_.rcPaint.left,
                    window->ps_.rcPaint.top,
                    window->ps_.rcPaint.right - window->ps_.rcPaint.left,
                    window->ps_.rcPaint.bottom - window->ps_.rcPaint.top
                );
                window->on_paint(evt);
                window->end_paint();
            }
            return 0;
            
        case WM_SIZE:
            if (window->on_resize) {
                ResizeEvent evt;
                evt.new_size = Size(LOWORD(lparam), HIWORD(lparam));
                window->on_resize(evt);
            }
            return 0;
            
        case WM_KEYDOWN:
        case WM_KEYUP:
            if (window->on_key_event) {
                KeyEvent evt;
                evt.key = window->translate_key(wparam);
                evt.modifiers = window->get_modifiers();
                evt.pressed = (msg == WM_KEYDOWN);
                evt.character = '\0';
                window->on_key_event(evt);
            }
            return 0;
            
        case WM_CHAR:
            if (window->on_key_event) {
                KeyEvent evt;
                evt.key = Key::Unknown;
                evt.modifiers = window->get_modifiers();
                evt.pressed = true;
                evt.character = static_cast<char>(wparam);
                window->on_key_event(evt);
            }
            return 0;
            
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
            if (window->on_mouse_event) {
                MouseEvent evt;
                evt.button = (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP) ? MouseButton::Left :
                            (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONUP) ? MouseButton::Right :
                            MouseButton::Middle;
                evt.position = Point(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
                evt.modifiers = window->get_modifiers();
                evt.pressed = (msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN);
                evt.wheel_delta = 0;
                window->on_mouse_event(evt);
            }
            return 0;
            
        case WM_MOUSEWHEEL:
            if (window->on_mouse_event) {
                MouseEvent evt;
                evt.button = MouseButton::None;
                POINT pt = {GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
                ScreenToClient(hwnd, &pt);
                evt.position = Point(pt.x, pt.y);
                evt.modifiers = window->get_modifiers();
                evt.pressed = false;
                evt.wheel_delta = GET_WHEEL_DELTA_WPARAM(wparam);
                window->on_mouse_event(evt);
            }
            return 0;
    }
    
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

// Factory function
std::unique_ptr<IPlatformWindow> create_platform_window() {
    return std::make_unique<Win32Window>();
}

} // namespace editor

#endif // PLATFORM_WINDOWS
