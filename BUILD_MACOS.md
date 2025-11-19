# macOS Cocoa Build Instructions

## Prerequisites

macOS comes with the necessary frameworks pre-installed:
- Cocoa framework (AppKit)
- Core Graphics (Quartz 2D)
- Core Text

Install development tools:

```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install CMake and Ninja (via Homebrew)
brew install cmake ninja
```

## Building

```bash
# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..

# Build
ninja

# Run platform test
./platform_test

# Run editor
./editor_gui
```

## Platform-Specific Notes

The Cocoa implementation provides:
- NSWindow-based window management
- NSView for custom drawing area
- Core Graphics (Quartz 2D) for rendering
- Core Text for font rendering with CTFont
- NSPasteboard for clipboard integration
- Native macOS look and feel with window decorations
- Event handling through NSResponder chain

### Coordinate System

The implementation handles macOS's bottom-left origin coordinate system:
- Drawing uses Core Graphics (bottom-left)
- Text drawing flips Y coordinates automatically
- Mouse events are converted to top-left coordinates for consistency

### Key Features

- **Window Management**: NSWindow with standard decorations (close, minimize, resize)
- **Drawing**: Core Graphics context with CGContext API
- **Fonts**: Core Text with CTFont and attributed strings
- **Events**: Full keyboard, mouse, and scroll wheel support
- **Clipboard**: NSPasteboard integration for copy/paste
- **Retina Display**: Automatically handles high-DPI displays

### Building Universal Binaries

To build for both Intel and Apple Silicon:

```bash
cmake -G Ninja \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
  -DCMAKE_BUILD_TYPE=Release ..
ninja
```

## Troubleshooting

### Framework Not Found

If you get framework linking errors:
```bash
# Verify Xcode Command Line Tools
xcode-select -p
```

### Objective-C++ Compilation

The `.mm` extension indicates Objective-C++. Ensure CMake recognizes it:
- CMake automatically handles `.mm` files
- No special configuration needed

### Code Signing

For distribution, you'll need to sign the application:
```bash
codesign -s "Developer ID" ./editor_gui
```

## Performance Notes

The Cocoa implementation is optimized for macOS:
- Uses native rendering pipelines
- Leverages Core Animation when available
- Respects system preferences (dark mode, font smoothing)
- Integrates with Accessibility features
