#include "build_error_parser.h"
#include <regex>

std::vector<BuildError> BuildErrorParser::parse(const std::string& build_output) {
    std::vector<BuildError> errors;
    std::regex gcc_clang_regex(R"(([^:\n]+):(\d+):(\d+): (error|warning|note): (.+))");
    std::smatch match;
    std::string::const_iterator searchStart(build_output.cbegin());
    while (std::regex_search(searchStart, build_output.cend(), match, gcc_clang_regex)) {
        BuildError err;
        err.file = match[1];
        err.line = std::stoi(match[2]);
        err.type = match[4];
        err.message = match[5];
        errors.push_back(err);
        searchStart = match.suffix().first;
    }
    // MSVC style: file(line): error|warning|note Cxxxx: message
    std::regex msvc_regex(R"(([^\(\)]+)\((\d+)\): (error|warning|note) [A-Za-z0-9]+: (.+))");
    searchStart = build_output.cbegin();
    while (std::regex_search(searchStart, build_output.cend(), match, msvc_regex)) {
        BuildError err;
        err.file = match[1];
        err.line = std::stoi(match[2]);
        err.type = match[3];
        err.message = match[4];
        errors.push_back(err);
        searchStart = match.suffix().first;
    }
    return errors;
}
