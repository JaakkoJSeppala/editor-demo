#include "build_system.h"
#include <filesystem>
#include <cstdlib>
#include <iostream>
#include <sstream>

BuildSystem::BuildSystem(const std::string& workspace_root)
    : root_(workspace_root), type_(detect_in_dir(workspace_root)) {}

BuildSystemType BuildSystem::detect_type() const {
    return type_;
}

BuildSystemType BuildSystem::detect_in_dir(const std::string& dir) {
    namespace fs = std::filesystem;
    if (fs::exists(fs::path(dir) / "CMakeLists.txt")) return BuildSystemType::CMake;
    if (fs::exists(fs::path(dir) / "Makefile")) return BuildSystemType::Makefile;
    if (fs::exists(fs::path(dir) / "meson.build")) return BuildSystemType::Meson;
    return BuildSystemType::None;
}

std::vector<BuildCommand> BuildSystem::get_default_commands() const {
    std::vector<BuildCommand> cmds;
    switch (type_) {
        case BuildSystemType::CMake:
            cmds.push_back({"Configure", "cmake -S . -B build", root_});
            cmds.push_back({"Build", "cmake --build build", root_});
            cmds.push_back({"Test", "ctest --test-dir build", root_});
            break;
        case BuildSystemType::Makefile:
            cmds.push_back({"Build", "make", root_});
            cmds.push_back({"Test", "make test", root_});
            break;
        case BuildSystemType::Meson:
            cmds.push_back({"Setup", "meson setup build", root_});
            cmds.push_back({"Build", "meson compile -C build", root_});
            cmds.push_back({"Test", "meson test -C build", root_});
            break;
        default:
            break;
    }
    return cmds;
}

void BuildSystem::run_command(const BuildCommand& cmd, std::function<void(const std::string& output)> on_output) {
    std::string full_cmd = "cd " + cmd.working_dir + " && " + cmd.command;
    std::array<char, 128> buffer;
    std::string result;
    FILE* pipe = _popen(full_cmd.c_str(), "r");
    if (!pipe) {
        on_output("Failed to run command: " + full_cmd);
        return;
    }
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }
    _pclose(pipe);
    on_output(result);
}
