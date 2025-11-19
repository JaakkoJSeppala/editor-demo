# Linux GTK4 Build Instructions

## Prerequisites

Install GTK4 development libraries:

```bash
# Ubuntu/Debian
sudo apt-get install libgtk-4-dev libcairo2-dev libpango1.0-dev pkg-config cmake ninja-build

# Fedora
sudo dnf install gtk4-devel cairo-devel pango-devel pkg-config cmake ninja-build

# Arch
sudo pacman -S gtk4 cairo pango pkg-config cmake ninja
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

## Troubleshooting

If GTK4 is not found:
```bash
pkg-config --modversion gtk4
```

If the version is less than 4.0, install GTK4 development package.

## Platform-Specific Notes

The GTK4 implementation provides:
- Window creation and management
- Event handling (keyboard, mouse, scroll)
- Cairo-based drawing primitives
- Pango font rendering
- Clipboard integration
- Native Linux look and feel

The implementation automatically compiles on Linux systems when CMake detects the platform.
