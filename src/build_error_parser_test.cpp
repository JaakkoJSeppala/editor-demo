#include "build_error_parser.h"
#include <iostream>

int main() {
    std::string sample_output =
        "src/main.cpp:42:13: error: 'foo' was not declared in this scope\n"
        "src/main.cpp:43:5: warning: unused variable 'bar' [-Wunused-variable]\n"
        "C:/project/file.cpp(27): error C2143: syntax error: missing ';' before '}'\n";

    auto errors = BuildErrorParser::parse(sample_output);
    for (const auto& err : errors) {
        std::cout << err.type << " in " << err.file << ":" << err.line << "\n  " << err.message << std::endl;
    }
    return 0;
}
