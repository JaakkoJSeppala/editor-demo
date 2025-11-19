// Cocoa implementation of IPlatformWindow
// macOS platform window using Cocoa and Core Graphics

#ifdef PLATFORM_MACOS

#include "platform_window.h"
#import <Cocoa/Cocoa.h>
#import <CoreGraphics/CoreGraphics.h>
#import <CoreText/CoreText.h>
#include <memory>
#include <unordered_map>

// Forward declaration
@class EditorView;
@class EditorWindowDelegate;

namespace {
    std::unordered_map<NSWindow*, class CocoaWindow*> window_map_;
}

// Custom view for drawing
@interface EditorView : NSView
@property (nonatomic, assign) class CocoaWindow* owner;
@end

// Window delegate for event handling
@interface EditorWindowDelegate : NSObject<NSWindowDelegate>
@property (nonatomic, assign) class CocoaWindow* owner;
@end

class CocoaWindow : public IPlatformWindow {
public:
    CocoaWindow() 
        : window_(nil)
        , view_(nil)
        , delegate_(nil)
        , current_font_(nullptr)
        , width_(0)
        , height_(0)
        , in_paint_(false) {
    }

    ~CocoaWindow() override {
        destroy();
    }

    bool create(const std::string& title, int width, int height) override {
        @autoreleasepool {
            width_ = width;
            height_ = height;

            // Create window
            NSRect frame = NSMakeRect(100, 100, width, height);
            NSWindowStyleMask style = NSWindowStyleMaskTitled 
                                    | NSWindowStyleMaskClosable 
                                    | NSWindowStyleMaskMiniaturizable 
                                    | NSWindowStyleMaskResizable;
            
            window_ = [[NSWindow alloc] initWithContentRect:frame
                                                  styleMask:style
                                                    backing:NSBackingStoreBuffered
                                                      defer:NO];
            
            [window_ setTitle:[NSString stringWithUTF8String:title.c_str()]];
            [window_ setAcceptsMouseMovedEvents:YES];
            
            // Create custom view
            view_ = [[EditorView alloc] initWithFrame:frame];
            view_.owner = this;
            [window_ setContentView:view_];
            
            // Create and set delegate
            delegate_ = [[EditorWindowDelegate alloc] init];
            delegate_.owner = this;
            [window_ setDelegate:delegate_];
            
            // Register in map
            window_map_[window_] = this;
            
            return true;
        }
    }

    void destroy() override {
        @autoreleasepool {
            if (current_font_) {
                CFRelease(current_font_);
                current_font_ = nullptr;
            }
            
            if (window_) {
                window_map_.erase(window_);
                [window_ setDelegate:nil];
                [window_ close];
                window_ = nil;
            }
            
            view_ = nil;
            delegate_ = nil;
        }
    }

    void show() override {
        @autoreleasepool {
            if (window_) {
                [window_ makeKeyAndOrderFront:nil];
                [NSApp activateIgnoringOtherApps:YES];
            }
        }
    }

    void hide() override {
        @autoreleasepool {
            if (window_) {
                [window_ orderOut:nil];
            }
        }
    }

    void set_title(const std::string& title) override {
        @autoreleasepool {
            if (window_) {
                [window_ setTitle:[NSString stringWithUTF8String:title.c_str()]];
            }
        }
    }

    void set_size(int width, int height) override {
        @autoreleasepool {
            width_ = width;
            height_ = height;
            if (window_) {
                NSRect frame = [window_ frame];
                frame.size = NSMakeSize(width, height);
                [window_ setFrame:frame display:YES];
            }
        }
    }

    Size get_size() const override {
        return Size{width_, height_};
    }

    void run_event_loop() override {
        @autoreleasepool {
            [NSApp run];
        }
    }

    void process_events() override {
        @autoreleasepool {
            NSEvent* event;
            while ((event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                               untilDate:[NSDate distantPast]
                                                  inMode:NSDefaultRunLoopMode
                                                 dequeue:YES])) {
                [NSApp sendEvent:event];
            }
        }
    }

    void request_redraw() override {
        @autoreleasepool {
            if (view_) {
                [view_ setNeedsDisplay:YES];
            }
        }
    }

    void* get_graphics_context() override {
        return graphics_context_;
    }

    void begin_paint() override {
        in_paint_ = true;
    }

    void end_paint() override {
        in_paint_ = false;
    }

    PlatformFont create_font(const std::string& family, int size, bool bold, bool italic) override {
        @autoreleasepool {
            NSString* fontName = [NSString stringWithUTF8String:family.c_str()];
            NSFont* nsFont = [NSFont fontWithName:fontName size:size];
            
            if (!nsFont) {
                nsFont = [NSFont systemFontOfSize:size];
            }
            
            // Apply bold/italic traits
            if (bold || italic) {
                NSFontDescriptor* descriptor = [nsFont fontDescriptor];
                NSFontDescriptorSymbolicTraits traits = 0;
                if (bold) traits |= NSFontDescriptorTraitBold;
                if (italic) traits |= NSFontDescriptorTraitItalic;
                
                descriptor = [descriptor fontDescriptorWithSymbolicTraits:traits];
                nsFont = [NSFont fontWithDescriptor:descriptor size:size];
            }
            
            // Create CTFont for Core Text
            CTFontRef ctFont = CTFontCreateWithName((__bridge CFStringRef)fontName, size, nullptr);
            return (PlatformFont)ctFont;
        }
    }

    void destroy_font(PlatformFont font) override {
        if (font) {
            CFRelease((CTFontRef)font);
        }
    }

    void set_font(PlatformFont font) override {
        if (current_font_) {
            CFRelease(current_font_);
        }
        current_font_ = (CTFontRef)font;
        if (current_font_) {
            CFRetain(current_font_);
        }
    }

    Size measure_text(const std::string& text) override {
        @autoreleasepool {
            if (!current_font_ || text.empty()) {
                return Size{0, 0};
            }
            
            NSString* nsText = [NSString stringWithUTF8String:text.c_str()];
            CFStringRef string = (__bridge CFStringRef)nsText;
            
            CFMutableAttributedStringRef attrString = CFAttributedStringCreateMutable(kCFAllocatorDefault, 0);
            CFAttributedStringReplaceString(attrString, CFRangeMake(0, 0), string);
            CFAttributedStringSetAttribute(attrString, CFRangeMake(0, CFStringGetLength(string)),
                                          kCTFontAttributeName, current_font_);
            
            CTLineRef line = CTLineCreateWithAttributedString(attrString);
            CGRect bounds = CTLineGetBoundsWithOptions(line, 0);
            
            CFRelease(line);
            CFRelease(attrString);
            
            return Size{(int)ceil(bounds.size.width), (int)ceil(bounds.size.height)};
        }
    }

    void clear(const Color& color) override {
        if (!graphics_context_) return;
        
        CGContextRef ctx = (CGContextRef)graphics_context_;
        CGContextSetRGBFillColor(ctx, color.r / 255.0, color.g / 255.0, color.b / 255.0, color.a / 255.0);
        CGContextFillRect(ctx, CGRectMake(0, 0, width_, height_));
    }

    void draw_rectangle(const Rect& rect, const Color& color, bool filled) override {
        if (!graphics_context_) return;
        
        CGContextRef ctx = (CGContextRef)graphics_context_;
        CGContextSetRGBFillColor(ctx, color.r / 255.0, color.g / 255.0, color.b / 255.0, color.a / 255.0);
        CGContextSetRGBStrokeColor(ctx, color.r / 255.0, color.g / 255.0, color.b / 255.0, color.a / 255.0);
        
        CGRect cgRect = CGRectMake(rect.x, rect.y, rect.width, rect.height);
        
        if (filled) {
            CGContextFillRect(ctx, cgRect);
        } else {
            CGContextStrokeRect(ctx, cgRect);
        }
    }

    void draw_text(const std::string& text, int x, int y, const Color& color) override {
        @autoreleasepool {
            if (!graphics_context_ || !current_font_ || text.empty()) return;
            
            CGContextRef ctx = (CGContextRef)graphics_context_;
            
            NSString* nsText = [NSString stringWithUTF8String:text.c_str()];
            CFStringRef string = (__bridge CFStringRef)nsText;
            
            CFMutableAttributedStringRef attrString = CFAttributedStringCreateMutable(kCFAllocatorDefault, 0);
            CFAttributedStringReplaceString(attrString, CFRangeMake(0, 0), string);
            CFAttributedStringSetAttribute(attrString, CFRangeMake(0, CFStringGetLength(string)),
                                          kCTFontAttributeName, current_font_);
            
            // Set text color
            CGColorRef cgColor = CGColorCreateGenericRGB(color.r / 255.0, color.g / 255.0, 
                                                         color.b / 255.0, color.a / 255.0);
            CFAttributedStringSetAttribute(attrString, CFRangeMake(0, CFStringGetLength(string)),
                                          kCTForegroundColorAttributeName, cgColor);
            
            CTLineRef line = CTLineCreateWithAttributedString(attrString);
            
            // Core Graphics uses bottom-left origin, adjust y coordinate
            CGContextSetTextMatrix(ctx, CGAffineTransformIdentity);
            CGContextSetTextPosition(ctx, x, height_ - y);
            CTLineDraw(line, ctx);
            
            CFRelease(line);
            CFRelease(attrString);
            CGColorRelease(cgColor);
        }
    }

    void draw_line(int x1, int y1, int x2, int y2, const Color& color) override {
        if (!graphics_context_) return;
        
        CGContextRef ctx = (CGContextRef)graphics_context_;
        CGContextSetRGBStrokeColor(ctx, color.r / 255.0, color.g / 255.0, color.b / 255.0, color.a / 255.0);
        CGContextBeginPath(ctx);
        CGContextMoveToPoint(ctx, x1, y1);
        CGContextAddLineToPoint(ctx, x2, y2);
        CGContextStrokePath(ctx);
    }

    std::string get_clipboard_text() override {
        @autoreleasepool {
            NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
            NSString* text = [pasteboard stringForType:NSPasteboardTypeString];
            if (text) {
                return std::string([text UTF8String]);
            }
            return "";
        }
    }

    void set_clipboard_text(const std::string& text) override {
        @autoreleasepool {
            NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
            [pasteboard clearContents];
            [pasteboard setString:[NSString stringWithUTF8String:text.c_str()]
                          forType:NSPasteboardTypeString];
        }
    }

    void set_cursor_visible(bool visible) override {
        @autoreleasepool {
            if (visible) {
                [NSCursor unhide];
            } else {
                [NSCursor hide];
            }
        }
    }

    PlatformWindow get_native_handle() const override {
        return (__bridge void*)window_;
    }

    // Event handlers called from Objective-C
    void handle_draw(CGContextRef ctx, int width, int height) {
        graphics_context_ = ctx;
        width_ = width;
        height_ = height;
        
        if (on_paint) {
            PaintEvent event;
            on_paint(event);
        }
        
        graphics_context_ = nullptr;
    }

    void handle_key_event(NSEvent* event, bool pressed) {
        if (!on_key_event) return;
        
        KeyEvent keyEvent;
        keyEvent.key = translate_key(event);
        keyEvent.modifiers = translate_modifiers(event);
        keyEvent.pressed = pressed;
        
        NSString* chars = [event characters];
        keyEvent.character = ([chars length] > 0) ? [chars characterAtIndex:0] : '\0';
        
        on_key_event(keyEvent);
    }

    void handle_mouse_event(NSEvent* event, MouseButton button, bool pressed) {
        if (!on_mouse_event) return;
        
        NSPoint location = [view_ convertPoint:[event locationInWindow] fromView:nil];
        
        MouseEvent mouseEvent;
        mouseEvent.button = button;
        mouseEvent.x = (int)location.x;
        mouseEvent.y = height_ - (int)location.y; // Flip Y coordinate
        mouseEvent.modifiers = translate_modifiers(event);
        mouseEvent.pressed = pressed;
        mouseEvent.wheel_delta = 0;
        
        on_mouse_event(mouseEvent);
    }

    void handle_mouse_moved(NSEvent* event) {
        if (!on_mouse_event) return;
        
        NSPoint location = [view_ convertPoint:[event locationInWindow] fromView:nil];
        
        MouseEvent mouseEvent;
        mouseEvent.button = MouseButton::None;
        mouseEvent.x = (int)location.x;
        mouseEvent.y = height_ - (int)location.y;
        mouseEvent.modifiers = KeyModifier::None;
        mouseEvent.pressed = false;
        mouseEvent.wheel_delta = 0;
        
        on_mouse_event(mouseEvent);
    }

    void handle_scroll(NSEvent* event) {
        if (!on_mouse_event) return;
        
        MouseEvent mouseEvent;
        mouseEvent.button = MouseButton::None;
        mouseEvent.x = 0;
        mouseEvent.y = 0;
        mouseEvent.modifiers = translate_modifiers(event);
        mouseEvent.pressed = false;
        mouseEvent.wheel_delta = (int)([event scrollingDeltaY] * 120.0); // Convert to Windows-style
        
        on_mouse_event(mouseEvent);
    }

    void handle_resize(int width, int height) {
        width_ = width;
        height_ = height;
        
        if (on_resize) {
            ResizeEvent event;
            event.new_size = Size{width, height};
            on_resize(event);
        }
    }

    void handle_close() {
        if (on_close) {
            on_close();
        }
    }

private:
    Key translate_key(NSEvent* event) {
        unsigned short keyCode = [event keyCode];
        
        // Letter keys (A-Z)
        if (keyCode >= 0 && keyCode <= 11) {
            static const Key letters[] = {
                Key::A, Key::S, Key::D, Key::F, Key::H, Key::G, Key::Z, Key::X, Key::C, Key::V, Key::B, Key::Q
            };
            return letters[keyCode];
        }
        if (keyCode >= 12 && keyCode <= 34) {
            static const Key letters[] = {
                Key::W, Key::E, Key::R, Key::Y, Key::T, Key::Num1, Key::Num2, Key::Num3, Key::Num4,
                Key::Num6, Key::Num5, Key::Unknown, Key::Num9, Key::Num7, Key::Unknown, Key::Num8,
                Key::Num0, Key::Unknown, Key::O, Key::U, Key::Unknown, Key::I, Key::P
            };
            return letters[keyCode - 12];
        }
        if (keyCode >= 37 && keyCode <= 46) {
            static const Key letters[] = {
                Key::L, Key::J, Key::Unknown, Key::K, Key::Unknown, Key::Unknown, Key::N, Key::M
            };
            return letters[keyCode - 37];
        }
        
        // Function keys
        if (keyCode >= 122 && keyCode <= 133) {
            return static_cast<Key>(static_cast<int>(Key::F1) + (keyCode - 122));
        }
        
        // Special keys
        switch (keyCode) {
            case 36: return Key::Enter;
            case 53: return Key::Escape;
            case 51: return Key::Backspace;
            case 48: return Key::Tab;
            case 49: return Key::Space;
            case 123: return Key::Left;
            case 124: return Key::Right;
            case 125: return Key::Down;
            case 126: return Key::Up;
            case 115: return Key::Home;
            case 119: return Key::End;
            case 116: return Key::PageUp;
            case 121: return Key::PageDown;
            case 114: return Key::Insert;
            case 117: return Key::Delete;
            default: return Key::Unknown;
        }
    }

    KeyModifier translate_modifiers(NSEvent* event) {
        NSEventModifierFlags flags = [event modifierFlags];
        int modifiers = static_cast<int>(KeyModifier::None);
        
        if (flags & NSEventModifierFlagShift) modifiers |= static_cast<int>(KeyModifier::Shift);
        if (flags & NSEventModifierFlagControl) modifiers |= static_cast<int>(KeyModifier::Ctrl);
        if (flags & NSEventModifierFlagOption) modifiers |= static_cast<int>(KeyModifier::Alt);
        if (flags & NSEventModifierFlagCommand) modifiers |= static_cast<int>(KeyModifier::Super);
        
        return static_cast<KeyModifier>(modifiers);
    }

    NSWindow* window_;
    EditorView* view_;
    EditorWindowDelegate* delegate_;
    CTFontRef current_font_;
    void* graphics_context_;
    int width_;
    int height_;
    bool in_paint_;
};

// EditorView implementation
@implementation EditorView

- (BOOL)isFlipped {
    return YES; // Use top-left origin
}

- (void)drawRect:(NSRect)dirtyRect {
    CGContextRef ctx = [[NSGraphicsContext currentContext] CGContext];
    if (self.owner) {
        self.owner->handle_draw(ctx, (int)self.bounds.size.width, (int)self.bounds.size.height);
    }
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)keyDown:(NSEvent*)event {
    if (self.owner) {
        self.owner->handle_key_event(event, true);
    }
}

- (void)keyUp:(NSEvent*)event {
    if (self.owner) {
        self.owner->handle_key_event(event, false);
    }
}

- (void)mouseDown:(NSEvent*)event {
    if (self.owner) {
        self.owner->handle_mouse_event(event, MouseButton::Left, true);
    }
}

- (void)mouseUp:(NSEvent*)event {
    if (self.owner) {
        self.owner->handle_mouse_event(event, MouseButton::Left, false);
    }
}

- (void)rightMouseDown:(NSEvent*)event {
    if (self.owner) {
        self.owner->handle_mouse_event(event, MouseButton::Right, true);
    }
}

- (void)rightMouseUp:(NSEvent*)event {
    if (self.owner) {
        self.owner->handle_mouse_event(event, MouseButton::Right, false);
    }
}

- (void)otherMouseDown:(NSEvent*)event {
    if (self.owner) {
        self.owner->handle_mouse_event(event, MouseButton::Middle, true);
    }
}

- (void)otherMouseUp:(NSEvent*)event {
    if (self.owner) {
        self.owner->handle_mouse_event(event, MouseButton::Middle, false);
    }
}

- (void)mouseMoved:(NSEvent*)event {
    if (self.owner) {
        self.owner->handle_mouse_moved(event);
    }
}

- (void)mouseDragged:(NSEvent*)event {
    if (self.owner) {
        self.owner->handle_mouse_moved(event);
    }
}

- (void)scrollWheel:(NSEvent*)event {
    if (self.owner) {
        self.owner->handle_scroll(event);
    }
}

@end

// EditorWindowDelegate implementation
@implementation EditorWindowDelegate

- (void)windowDidResize:(NSNotification*)notification {
    if (self.owner) {
        NSWindow* window = [notification object];
        NSRect frame = [[window contentView] frame];
        self.owner->handle_resize((int)frame.size.width, (int)frame.size.height);
    }
}

- (BOOL)windowShouldClose:(NSWindow*)sender {
    if (self.owner) {
        self.owner->handle_close();
        return NO; // Let the owner decide when to close
    }
    return YES;
}

@end

// Factory function for macOS
std::unique_ptr<IPlatformWindow> create_platform_window() {
    return std::make_unique<CocoaWindow>();
}

#endif // PLATFORM_MACOS
