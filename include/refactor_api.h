#ifndef REFACTOR_API_H
#define REFACTOR_API_H

#include <string>
#include <vector>
#include <functional>

class LSPClient;

struct RefactorResult {
    bool success;
    std::string message;
    std::vector<std::string> changed_files;
};

class RefactorAPI {
public:
    RefactorAPI(LSPClient* lsp);

    // Rename symbol at position
    void rename_symbol(const std::string& uri, int line, int character, const std::string& new_name, std::function<void(RefactorResult)> callback);

    // Move function/class to another file
    void move_symbol(const std::string& uri, int line, int character, const std::string& target_uri, std::function<void(RefactorResult)> callback);

    // Code cleanup (format, organize imports, etc.)
    void code_cleanup(const std::string& uri, std::function<void(RefactorResult)> callback);

private:
    LSPClient* lsp_;
};

#endif // REFACTOR_API_H
