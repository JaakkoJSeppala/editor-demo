#include "find_dialog.h"
#include <algorithm>
#include <cctype>

std::string FindDialog::to_lower(const std::string& str) const {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

bool FindDialog::matches_at_position(const std::string& text, size_t pos,
                                     const std::string& search_text) const {
    if (pos + search_text.length() > text.length()) {
        return false;
    }
    
    if (case_sensitive_) {
        return text.compare(pos, search_text.length(), search_text) == 0;
    } else {
        std::string text_substr = text.substr(pos, search_text.length());
        return to_lower(text_substr) == to_lower(search_text);
    }
}

std::vector<SearchMatch> FindDialog::find_all(const std::string& document_text,
                                              const std::string& search_text) {
    std::vector<SearchMatch> matches;
    
    if (search_text.empty()) {
        return matches;
    }
    
    // Find all occurrences
    size_t pos = 0;
    while (pos < document_text.length()) {
        if (matches_at_position(document_text, pos, search_text)) {
            SearchMatch match;
            match.position = pos;
            match.length = search_text.length();
            
            // Calculate line and column
            size_t line = 0;
            size_t line_start = 0;
            for (size_t i = 0; i < pos; ++i) {
                if (document_text[i] == '\n') {
                    line++;
                    line_start = i + 1;
                }
            }
            match.line = line;
            match.column = pos - line_start;
            
            matches.push_back(match);
            pos += search_text.length();
        } else {
            pos++;
        }
    }
    
    return matches;
}

bool FindDialog::find_next(const std::string& document_text,
                           const std::string& search_text,
                           size_t start_pos,
                           SearchMatch& match) {
    if (search_text.empty()) {
        return false;
    }
    
    // Search forward from start_pos
    for (size_t pos = start_pos; pos < document_text.length(); ++pos) {
        if (matches_at_position(document_text, pos, search_text)) {
            match.position = pos;
            match.length = search_text.length();
            
            // Calculate line and column
            size_t line = 0;
            size_t line_start = 0;
            for (size_t i = 0; i < pos; ++i) {
                if (document_text[i] == '\n') {
                    line++;
                    line_start = i + 1;
                }
            }
            match.line = line;
            match.column = pos - line_start;
            
            return true;
        }
    }
    
    return false;
}

bool FindDialog::find_previous(const std::string& document_text,
                               const std::string& search_text,
                               size_t start_pos,
                               SearchMatch& match) {
    if (search_text.empty() || start_pos == 0) {
        return false;
    }
    
    // Search backward from start_pos
    for (size_t pos = start_pos - 1; pos < document_text.length(); --pos) {
        if (matches_at_position(document_text, pos, search_text)) {
            match.position = pos;
            match.length = search_text.length();
            
            // Calculate line and column
            size_t line = 0;
            size_t line_start = 0;
            for (size_t i = 0; i < pos; ++i) {
                if (document_text[i] == '\n') {
                    line++;
                    line_start = i + 1;
                }
            }
            match.line = line;
            match.column = pos - line_start;
            
            return true;
        }
        
        if (pos == 0) break;  // Prevent underflow
    }
    
    return false;
}

bool FindDialog::replace_current(std::string& document_text,
                                const SearchMatch& match,
                                const std::string& replace_text) {
    if (match.position + match.length > document_text.length()) {
        return false;
    }
    
    document_text.replace(match.position, match.length, replace_text);
    return true;
}

int FindDialog::replace_all(std::string& document_text,
                            const std::string& search_text,
                            const std::string& replace_text) {
    if (search_text.empty()) {
        return 0;
    }
    
    int replace_count = 0;
    size_t pos = 0;
    
    while (pos < document_text.length()) {
        if (matches_at_position(document_text, pos, search_text)) {
            document_text.replace(pos, search_text.length(), replace_text);
            replace_count++;
            pos += replace_text.length();
        } else {
            pos++;
        }
    }
    
    return replace_count;
}
