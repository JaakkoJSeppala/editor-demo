#ifndef BUILD_SYSTEM_H
#define BUILD_SYSTEM_H

#include <string>
#include <vector>
#include <functional>

enum class BuildSystemType {
    None,
    CMake,
    Makefile,
    Meson
};

struct BuildCommand {
    std::string label;
    std::string command;
    std::string working_dir;
};

class BuildSystem {
public:
    BuildSystem(const std::string& workspace_root);
    BuildSystemType detect_type() const;
    std::vector<BuildCommand> get_default_commands() const;
    void run_command(const BuildCommand& cmd, std::function<void(const std::string& output)> on_output);
    static BuildSystemType detect_in_dir(const std::string& dir);
private:
    std::string root_;
    BuildSystemType type_;
};

#endif // BUILD_SYSTEM_H
