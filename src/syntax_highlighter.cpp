
#include "syntax_highlighter.h"
#include <regex>
#include <unordered_set>

// Minimal implementation for tokenize_line
std::vector<Token> SyntaxHighlighter::tokenize_line(const std::string& line, const LineState& in_state, LineState& out_state) {
    std::vector<Token> tokens;
    out_state = in_state;
    // C++ keywords
    static const std::unordered_set<std::string> cpp_keywords = {
        "int", "float", "double", "if", "else", "for", "while", "return", "class", "struct", "public", "private", "protected", "void", "const", "static", "virtual", "override", "namespace", "using", "include"
    };
    // Python keywords
    static const std::unordered_set<std::string> python_keywords = {
        "def", "class", "import", "from", "as", "if", "elif", "else", "for", "while", "return", "with", "try", "except", "finally", "lambda", "pass", "break", "continue", "global", "nonlocal", "assert", "yield", "del", "raise"
    };
    // Detect keywords
    std::regex word_regex("[A-Za-z_][A-Za-z0-9_]*");
    auto words_begin = std::sregex_iterator(line.begin(), line.end(), word_regex);
    auto words_end = std::sregex_iterator();
    for (auto it = words_begin; it != words_end; ++it) {
        std::string word = it->str();
        size_t pos = it->position();
        Token::Type type = Token::NORMAL;
        if (language_ == Language::Cpp && cpp_keywords.count(word)) type = Token::KEYWORD;
        if (language_ == Language::Python && python_keywords.count(word)) type = Token::KEYWORD;
        tokens.push_back({type, pos, word.length()});
    }
    // String literals
    std::regex string_regex("\"([^\"]*)\"");
    auto str_begin = std::sregex_iterator(line.begin(), line.end(), string_regex);
    for (auto it = str_begin; it != words_end; ++it) {
        size_t pos = it->position();
        tokens.push_back({Token::STRING, pos, it->str().length()});
    }
    // Comments
    if (language_ == Language::Cpp) {
        size_t comment_pos = line.find("//");
        if (comment_pos != std::string::npos) tokens.push_back({Token::COMMENT, comment_pos, line.length() - comment_pos});
    } else if (language_ == Language::Python) {
        size_t comment_pos = line.find("#");
        if (comment_pos != std::string::npos) tokens.push_back({Token::COMMENT, comment_pos, line.length() - comment_pos});
    }
    return tokens;
}
