#ifndef SYNTAX_HIGHLIGHTER_H
#define SYNTAX_HIGHLIGHTER_H

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <cctype>
#include <windows.h>
#include "treesitter_bridge.h"

/**
 * Token represents a syntax element with its color
 */
struct Token {
    enum Type {
        NORMAL,
        KEYWORD,
        STRING,
        COMMENT,
        NUMBER,
        PREPROCESSOR
    };
    
    Type type;
    size_t start;
    size_t length;
    
    COLORREF get_color() const {
        switch (type) {
            case KEYWORD:      return RGB(86, 156, 214);   // Blue
            case STRING:       return RGB(206, 145, 120);  // Orange/tan
            case COMMENT:      return RGB(87, 166, 74);    // Green
            case NUMBER:       return RGB(181, 206, 168);  // Light green
            case PREPROCESSOR: return RGB(155, 155, 155);  // Gray
            case NORMAL:
            default:           return RGB(220, 220, 220);  // White
        }
    }
};

/**
 * SyntaxHighlighter - Simple C++ syntax highlighting
 */
class SyntaxHighlighter {
public:
    enum class Language { Auto, Cpp, Python, JavaScript, TypeScript, Rust, Go, JSON, YAML, Markdown };

    struct LineState {
        bool in_block_comment = false;     // /* ... */ or /*# ... #*/ style
        bool in_triple_string = false;     // Python/Markdown code fences
        char string_delim = 0;             // ' or " or ` for template strings
    };

    SyntaxHighlighter() { set_language(Language::Cpp); }

    void set_language(Language lang) {
        language_ = lang;
        rebuild_keywords();
        // Tree-sitter bridge removed for minimal build
    }

    void set_language_by_filename(const std::string& filename) {
        auto dot = filename.find_last_of('.')
;        std::string ext = (dot == std::string::npos) ? std::string() : filename.substr(dot + 1);
        for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (ext == "c" || ext == "cpp" || ext == "cc" || ext == "cxx" || ext == "h" || ext == "hpp") {
            set_language(Language::Cpp);
        } else if (ext == "py") {
            set_language(Language::Python);
        } else if (ext == "js") {
            set_language(Language::JavaScript);
        } else if (ext == "ts" || ext == "tsx") {
            set_language(Language::TypeScript);
        } else if (ext == "rs") {
            set_language(Language::Rust);
        } else if (ext == "go") {
            set_language(Language::Go);
        } else if (ext == "json") {
            set_language(Language::JSON);
        } else if (ext == "yml" || ext == "yaml") {
            set_language(Language::YAML);
        } else if (ext == "md" || ext == "markdown") {
            set_language(Language::Markdown);
        } else {
            set_language(Language::Cpp); // fallback reasonable default
        }
        // Tree-sitter bridge removed for minimal build
    }
    
    // Incremental tokenization with line state
    std::vector<Token> tokenize_line(const std::string& line, const LineState& in_state, LineState& out_state);

    // Backwards-compatible single-line tokenization (no cross-line state)
    std::vector<Token> tokenize_line(const std::string& line) {
        LineState s_in{}, s_out{}; return tokenize_line(line, s_in, s_out);
    }

    // Tree-sitter bridge removed for minimal build
    
private:
    Language language_ = Language::Cpp;
    std::unordered_set<std::string> keywords_;

    bool is_line_comment_start(const std::string& line, size_t i) const {
        if (language_ == Language::Python || language_ == Language::YAML) return line[i] == '#';
        if (language_ == Language::JSON) return false;
        return (i + 1 < line.size() && line[i] == '/' && line[i+1] == '/');
    }

    bool is_block_comment_start(const std::string& line, size_t i) const {
        if (language_ == Language::JSON || language_ == Language::YAML || language_ == Language::Markdown || language_ == Language::Python) return false;
        return (i + 1 < line.size() && line[i] == '/' && line[i+1] == '*');
    }

    bool is_preprocessor_start(const std::string& line, size_t i) const {
        if (language_ == Language::Cpp || language_ == Language::JavaScript || language_ == Language::TypeScript || language_ == Language::Rust || language_ == Language::Go) {
            return line[i] == '#';
        }
        if (language_ == Language::YAML) {
            // Start of document marker or key-like header (---)
            return (i + 2 < line.size() && line[i] == '-' && line[i+1] == '-' && line[i+2] == '-');
        }
        if (language_ == Language::Markdown) {
            return line[i] == '#';
        }
        return false;
    }

    void rebuild_keywords() {
        keywords_.clear();
        switch (language_) {
            case Language::Cpp: keywords_ = cpp_keywords(); break;
            case Language::Python: keywords_ = python_keywords(); break;
            case Language::JavaScript: keywords_ = javascript_keywords(); break;
            case Language::TypeScript: {
                keywords_ = javascript_keywords();
                auto ts = typescript_extras(); keywords_.insert(ts.begin(), ts.end());
                break;
            }
            case Language::Rust: keywords_ = rust_keywords(); break;
            case Language::Go: keywords_ = go_keywords(); break;
            case Language::JSON: keywords_ = json_keywords(); break;
            case Language::YAML: keywords_ = yaml_keywords(); break;
            case Language::Markdown: keywords_.clear(); break;
            default: break;
        }
    }

    static std::string language_to_id(Language lang) {
        switch (lang) {
            case Language::Cpp: return "cpp";
            case Language::Python: return "python";
            case Language::JavaScript: return "javascript";
            case Language::TypeScript: return "typescript";
            case Language::Rust: return "rust";
            case Language::Go: return "go";
            case Language::JSON: return "json";
            case Language::YAML: return "yaml";
            case Language::Markdown: return "markdown";
            default: return "";
        }
    }

    static std::unordered_set<std::string> cpp_keywords() {
        return {
            "alignas","alignof","and","and_eq","asm","auto","bitand","bitor","bool","break","case","catch","char","char16_t","char32_t","class","compl","const","constexpr","const_cast","continue","decltype","default","delete","do","double","dynamic_cast","else","enum","explicit","export","extern","false","float","for","friend","goto","if","inline","int","long","mutable","namespace","new","noexcept","not","not_eq","nullptr","operator","or","or_eq","private","protected","public","register","reinterpret_cast","return","short","signed","sizeof","static","static_assert","static_cast","struct","switch","template","this","thread_local","throw","true","try","typedef","typeid","typename","union","unsigned","using","virtual","void","volatile","wchar_t","while","xor","xor_eq","override","final"
        };
    }
    static std::unordered_set<std::string> python_keywords() {
        return {"and","as","assert","break","class","continue","def","del","elif","else","except","False","finally","for","from","global","if","import","in","is","lambda","None","nonlocal","not","or","pass","raise","return","True","try","while","with","yield"};
    }
    static std::unordered_set<std::string> javascript_keywords() {
        return {"break","case","catch","class","const","continue","debugger","default","delete","do","else","export","extends","finally","for","function","if","import","in","instanceof","let","new","return","super","switch","this","throw","try","typeof","var","void","while","with","yield","true","false","null","undefined"};
    }
    static std::unordered_set<std::string> typescript_extras() {
        return {"interface","type","enum","implements","readonly","keyof","unknown","never"};
    }
    static std::unordered_set<std::string> rust_keywords() {
        return {"as","break","const","continue","crate","else","enum","extern","false","fn","for","if","impl","in","let","loop","match","mod","move","mut","pub","ref","return","self","Self","static","struct","super","trait","true","type","unsafe","use","where","while"};
    }
    static std::unordered_set<std::string> go_keywords() {
        return {"break","default","func","interface","select","case","defer","go","map","struct","chan","else","goto","package","switch","const","fallthrough","if","range","type","continue","for","import","return","var"};
    }
    static std::unordered_set<std::string> json_keywords() {
        return {"true","false","null"};
    }
    static std::unordered_set<std::string> yaml_keywords() {
        return {"true","false","null","y","n","on","off"};
    }
};

#endif // SYNTAX_HIGHLIGHTER_H
