#include "indexer.h"
#include <sstream>
#include <algorithm>
#include <cctype>

BackgroundIndexer::BackgroundIndexer() 
    : is_indexing_(false), should_stop_(false) {
}

BackgroundIndexer::~BackgroundIndexer() {
    stop();
}

void BackgroundIndexer::start() {
    if (is_indexing_.load()) return;
    
    should_stop_.store(false);
    is_indexing_.store(true);
    indexing_thread_ = std::thread(&BackgroundIndexer::indexing_worker, this);
}

void BackgroundIndexer::stop() {
    should_stop_.store(true);
    if (indexing_thread_.joinable()) {
        indexing_thread_.join();
    }
    is_indexing_.store(false);
}

void BackgroundIndexer::index_file(const std::string& file_path, const std::string& content) {
    std::lock_guard<std::mutex> lock(index_mutex_);
    
    // Remove old entries for this file
    for (auto& [word, locations] : index_) {
        locations.erase(
            std::remove_if(locations.begin(), locations.end(),
                [&file_path](const Location& loc) {
                    return loc.file_path == file_path;
                }),
            locations.end()
        );
    }
    
    // Tokenize and index the new content
    tokenize_and_index(file_path, content);
}

void BackgroundIndexer::remove_file(const std::string& file_path) {
    std::lock_guard<std::mutex> lock(index_mutex_);
    
    // Remove all entries for this file
    for (auto& [word, locations] : index_) {
        locations.erase(
            std::remove_if(locations.begin(), locations.end(),
                [&file_path](const Location& loc) {
                    return loc.file_path == file_path;
                }),
            locations.end()
        );
    }
    
    file_lines_.erase(file_path);
}

void BackgroundIndexer::tokenize_and_index(const std::string& file_path, const std::string& content) {
    std::vector<std::string> lines;
    std::istringstream stream(content);
    std::string line;
    
    while (std::getline(stream, line)) {
        lines.push_back(line);
    }
    
    file_lines_[file_path] = lines;
    
    // Index each word in each line
    for (size_t line_num = 0; line_num < lines.size(); ++line_num) {
        const std::string& current_line = lines[line_num];
        std::string word;
        size_t column = 0;
        
        for (size_t i = 0; i < current_line.length(); ++i) {
            char c = current_line[i];
            
            if (std::isalnum(c) || c == '_') {
                if (word.empty()) {
                    column = i;
                }
                word += std::tolower(c);
            } else {
                if (!word.empty() && word.length() > 2) {
                    Location loc;
                    loc.file_path = file_path;
                    loc.line_number = line_num;
                    loc.column = column;
                    
                    index_[word].push_back(loc);
                }
                word.clear();
            }
        }
        
        // Don't forget the last word
        if (!word.empty() && word.length() > 2) {
            Location loc;
            loc.file_path = file_path;
            loc.line_number = line_num;
            loc.column = column;
            
            index_[word].push_back(loc);
        }
    }
}

std::vector<SearchResult> BackgroundIndexer::search(const std::string& query, size_t max_results) {
    std::lock_guard<std::mutex> lock(index_mutex_);
    
    std::vector<SearchResult> results;
    
    // Convert query to lowercase for case-insensitive search
    std::string lower_query;
    for (char c : query) {
        lower_query += std::tolower(c);
    }
    
    // Find in index
    auto it = index_.find(lower_query);
    if (it == index_.end()) {
        return results;
    }
    
    // Convert locations to search results
    for (const auto& loc : it->second) {
        if (results.size() >= max_results) {
            break;
        }
        
        SearchResult result;
        result.file_path = loc.file_path;
        result.line_number = loc.line_number;
        result.column = loc.column;
        
        // Get the line content
        auto file_it = file_lines_.find(loc.file_path);
        if (file_it != file_lines_.end() && loc.line_number < file_it->second.size()) {
            result.line_content = file_it->second[loc.line_number];
        }
        
        results.push_back(result);
    }
    
    return results;
}

size_t BackgroundIndexer::get_indexed_file_count() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(index_mutex_));
    return file_lines_.size();
}

void BackgroundIndexer::indexing_worker() {
    // This would normally watch for file changes and re-index
    // For the demo, we just keep the thread alive
    while (!should_stop_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
