#ifndef SYNTAX_HIGHLIGHTER_H
#define SYNTAX_HIGHLIGHTER_H

#include <string>
#include <vector>
#include <unordered_set>
#include <windows.h>

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
    SyntaxHighlighter() {
        // C++ keywords
        keywords_ = {
            "alignas", "alignof", "and", "and_eq", "asm", "auto",
            "bitand", "bitor", "bool", "break", "case", "catch",
            "char", "char16_t", "char32_t", "class", "compl", "const",
            "constexpr", "const_cast", "continue", "decltype", "default",
            "delete", "do", "double", "dynamic_cast", "else", "enum",
            "explicit", "export", "extern", "false", "float", "for",
            "friend", "goto", "if", "inline", "int", "long",
            "mutable", "namespace", "new", "noexcept", "not", "not_eq",
            "nullptr", "operator", "or", "or_eq", "private", "protected",
            "public", "register", "reinterpret_cast", "return", "short",
            "signed", "sizeof", "static", "static_assert", "static_cast",
            "struct", "switch", "template", "this", "thread_local", "throw",
            "true", "try", "typedef", "typeid", "typename", "union",
            "unsigned", "using", "virtual", "void", "volatile", "wchar_t",
            "while", "xor", "xor_eq", "override", "final"
        };
    }
    
    // Tokenize a line of code
    std::vector<Token> tokenize_line(const std::string& line) {
        std::vector<Token> tokens;
        size_t i = 0;
        
        while (i < line.length()) {
            // Skip whitespace
            if (std::isspace(line[i])) {
                i++;
                continue;
            }
            
            // Line comment
            if (i + 1 < line.length() && line[i] == '/' && line[i+1] == '/') {
                tokens.push_back({Token::COMMENT, i, line.length() - i});
                break; // Rest of line is comment
            }
            
            // Block comment start
            if (i + 1 < line.length() && line[i] == '/' && line[i+1] == '*') {
                size_t start = i;
                i += 2;
                while (i + 1 < line.length()) {
                    if (line[i] == '*' && line[i+1] == '/') {
                        i += 2;
                        break;
                    }
                    i++;
                }
                tokens.push_back({Token::COMMENT, start, i - start});
                continue;
            }
            
            // Preprocessor
            if (line[i] == '#') {
                tokens.push_back({Token::PREPROCESSOR, i, line.length() - i});
                break; // Rest of line is preprocessor
            }
            
            // String literal
            if (line[i] == '"') {
                size_t start = i;
                i++;
                while (i < line.length() && line[i] != '"') {
                    if (line[i] == '\\' && i + 1 < line.length()) {
                        i += 2; // Skip escaped character
                    } else {
                        i++;
                    }
                }
                if (i < line.length()) i++; // Include closing quote
                tokens.push_back({Token::STRING, start, i - start});
                continue;
            }
            
            // Character literal
            if (line[i] == '\'') {
                size_t start = i;
                i++;
                while (i < line.length() && line[i] != '\'') {
                    if (line[i] == '\\' && i + 1 < line.length()) {
                        i += 2;
                    } else {
                        i++;
                    }
                }
                if (i < line.length()) i++;
                tokens.push_back({Token::STRING, start, i - start});
                continue;
            }
            
            // Number
            if (std::isdigit(line[i])) {
                size_t start = i;
                while (i < line.length() && (std::isdigit(line[i]) || line[i] == '.' || 
                       line[i] == 'x' || line[i] == 'X' || line[i] == 'f' || line[i] == 'F' ||
                       line[i] == 'u' || line[i] == 'U' || line[i] == 'l' || line[i] == 'L')) {
                    i++;
                }
                tokens.push_back({Token::NUMBER, start, i - start});
                continue;
            }
            
            // Identifier or keyword
            if (std::isalpha(line[i]) || line[i] == '_') {
                size_t start = i;
                while (i < line.length() && (std::isalnum(line[i]) || line[i] == '_')) {
                    i++;
                }
                
                std::string word = line.substr(start, i - start);
                Token::Type type = keywords_.count(word) > 0 ? Token::KEYWORD : Token::NORMAL;
                tokens.push_back({type, start, i - start});
                continue;
            }
            
            // Default: skip single character
            i++;
        }
        
        return tokens;
    }
    
private:
    std::unordered_set<std::string> keywords_;
};

#endif // SYNTAX_HIGHLIGHTER_H
