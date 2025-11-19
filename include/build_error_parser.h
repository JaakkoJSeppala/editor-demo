#ifndef BUILD_ERROR_PARSER_H
#define BUILD_ERROR_PARSER_H

#include <string>
#include <vector>

struct BuildError {
    std::string file;
    int line = 0;
    std::string message;
    std::string type; // error, warning, note
};

class BuildErrorParser {
public:
    static std::vector<BuildError> parse(const std::string& build_output);
};

#endif // BUILD_ERROR_PARSER_H
