#include "build_system.h"
#include "build_error_parser.h"
#include <iostream>

int main() {
    std::string workspace = ".";
    BuildSystem build(workspace);
    auto cmds = build.get_default_commands();
    for (const auto& cmd : cmds) {
        std::cout << "Running: " << cmd.label << " -> " << cmd.command << std::endl;
        build.run_command(cmd, [](const std::string& output) {
            std::cout << "Raw build output:\n" << output << std::endl;
            auto errors = BuildErrorParser::parse(output);
            std::cout << "\nParsed errors/warnings:" << std::endl;
            for (const auto& err : errors) {
                std::cout << err.type << " in " << err.file << ":" << err.line << "\n  " << err.message << std::endl;
            }
        });
    }
    return 0;
}
