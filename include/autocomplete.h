#ifndef AUTOCOMPLETE_H
#define AUTOCOMPLETE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <memory>

class PieceTable;

class AutocompleteManager {
public:
    void rebuild_from_document(const std::shared_ptr<PieceTable>& doc) {
        freq_.clear();
        if (!doc) return;
        size_t lines = doc->get_line_count();
        for (size_t i = 0; i < lines; ++i) {
            const std::string line = doc->get_line(i);
            add_words_from_line(line);
        }
    }

    void on_text_inserted(const std::string& text) { add_words_from_line(text); }

    std::vector<std::string> suggest(const std::string& prefix, size_t max_items = 8) const {
        if (prefix.empty()) return {};
        std::vector<std::pair<std::string,int>> candidates;
        for (const auto& kv : freq_) {
            const std::string& w = kv.first;
            if (w.size() <= prefix.size()) continue;
            if (starts_with_case_insensitive(w, prefix)) candidates.emplace_back(w, kv.second);
        }
        std::sort(candidates.begin(), candidates.end(), [&](auto& a, auto& b){
            if (a.second != b.second) return a.second > b.second;
            return a.first < b.first;
        });
        std::vector<std::string> out;
        for (size_t i = 0; i < candidates.size() && out.size() < max_items; ++i) out.push_back(candidates[i].first);
        return out;
    }

private:
    std::unordered_map<std::string,int> freq_;

    static bool is_ident_char(char c) {
        unsigned char uc = static_cast<unsigned char>(c);
        return std::isalnum(uc) || c == '_';
    }
    static bool is_ident_start(char c) {
        unsigned char uc = static_cast<unsigned char>(c);
        return std::isalpha(uc) || c == '_';
    }
    static bool starts_with_case_insensitive(const std::string& s, const std::string& p) {
        if (s.size() < p.size()) return false;
        for (size_t i = 0; i < p.size(); ++i) {
            char a = static_cast<char>(std::tolower(static_cast<unsigned char>(s[i])));
            char b = static_cast<char>(std::tolower(static_cast<unsigned char>(p[i])));
            if (a != b) return false;
        }
        return true;
    }
    void add_words_from_line(const std::string& line) {
        size_t i = 0, n = line.size();
        while (i < n) {
            if (is_ident_start(line[i])) {
                size_t j = i + 1;
                while (j < n && is_ident_char(line[j])) j++;
                std::string w = line.substr(i, j - i);
                // ignore very short tokens
                if (w.size() >= 3) freq_[w]++;
                i = j;
            } else {
                i++;
            }
        }
    }
};

#endif // AUTOCOMPLETE_H
