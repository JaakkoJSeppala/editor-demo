#ifndef TREESITTER_BRIDGE_H
#define TREESITTER_BRIDGE_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

#include <windows.h>

struct Token; // forward from syntax_highlighter.h

// Minimal facade to Tree-sitter. When ENABLE_TREESITTER is not defined,
// this becomes a no-op shim that returns false for availability.
class TreeSitterBridge {
public:
    struct Config {
        std::string language; // e.g. "cpp", "python", "javascript"
    };

    TreeSitterBridge();
    ~TreeSitterBridge();

    // Initialize parser for given language, returns true if supported and backend available
    bool initialize(const std::string& lang_id);

    // Incrementally (re)parse full text. For simplicity, we accept full text for now.
    void set_document_text(const std::vector<std::string>& lines);

    // Tokenize a single line using the parse tree. Returns true if tokens were produced by TS.
    bool get_line_tokens(size_t line_index, std::vector<Token>& out_tokens) const;

    // Report availability
    bool is_available() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

#endif // TREESITTER_BRIDGE_H
