#ifndef FIND_DIALOG_H
#define FIND_DIALOG_H

#include <windows.h>
#include <string>
#include <vector>

/**
 * SearchMatch represents a single match in the document
 */
struct SearchMatch {
    size_t position;  // Position in document
    size_t line;      // Line number
    size_t column;    // Column in line
    size_t length;    // Length of match
};

/**
 * FindDialog - Implements find and replace functionality
 */
class FindDialog {
public:
    FindDialog() 
        : case_sensitive_(false)
        , use_regex_(false)
        , current_match_index_(0)
    {}
    
    // Search operations
    std::vector<SearchMatch> find_all(const std::string& document_text, 
                                      const std::string& search_text);
    
    bool find_next(const std::string& document_text, 
                   const std::string& search_text,
                   size_t start_pos,
                   SearchMatch& match);
    
    bool find_previous(const std::string& document_text,
                       const std::string& search_text,
                       size_t start_pos,
                       SearchMatch& match);
    
    // Replace operations  
    bool replace_current(std::string& document_text,
                        const SearchMatch& match,
                        const std::string& replace_text);
    
    int replace_all(std::string& document_text,
                    const std::string& search_text,
                    const std::string& replace_text);
    
    // Settings
    void set_case_sensitive(bool enabled) { case_sensitive_ = enabled; }
    bool is_case_sensitive() const { return case_sensitive_; }
    
    void set_use_regex(bool enabled) { use_regex_ = enabled; }
    bool is_use_regex() const { return use_regex_; }
    
    // Match navigation
    void set_matches(const std::vector<SearchMatch>& matches) {
        matches_ = matches;
        current_match_index_ = 0;
    }
    
    size_t get_current_match_index() const { return current_match_index_; }
    size_t get_match_count() const { return matches_.size(); }
    
    bool has_matches() const { return !matches_.empty(); }
    
    const SearchMatch* get_current_match() const {
        if (current_match_index_ < matches_.size()) {
            return &matches_[current_match_index_];
        }
        return nullptr;
    }
    
    void next_match() {
        if (!matches_.empty()) {
            current_match_index_ = (current_match_index_ + 1) % matches_.size();
        }
    }
    
    void previous_match() {
        if (!matches_.empty()) {
            if (current_match_index_ == 0) {
                current_match_index_ = matches_.size() - 1;
            } else {
                current_match_index_--;
            }
        }
    }
    
    void clear_matches() {
        matches_.clear();
        current_match_index_ = 0;
    }
    
private:
    bool case_sensitive_;
    bool use_regex_;
    std::vector<SearchMatch> matches_;
    size_t current_match_index_;
    
    bool matches_at_position(const std::string& text, size_t pos, 
                             const std::string& search_text) const;
    
    std::string to_lower(const std::string& str) const;
};

#endif // FIND_DIALOG_H
