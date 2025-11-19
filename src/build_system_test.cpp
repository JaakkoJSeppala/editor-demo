#include "build_system.h"
#include <iostream>

int main() {
    std::string workspace = ".";
    BuildSystem build(workspace);
    BuildSystemType type = build.detect_type();
    std::cout << "Detected build system: ";
    switch (type) {
        case BuildSystemType::CMake: std::cout << "CMake"; break;
        case BuildSystemType::Makefile: std::cout << "Makefile"; break;
        case BuildSystemType::Meson: std::cout << "Meson"; break;
        default: std::cout << "None"; break;
    }
    std::cout << std::endl;

    auto cmds = build.get_default_commands();
    for (const auto& cmd : cmds) {
        std::cout << "Running: " << cmd.label << " -> " << cmd.command << std::endl;
        build.run_command(cmd, [](const std::string& output) {
            std::cout << "Output:\n" << output << std::endl;
        });
    }
    return 0;
}
