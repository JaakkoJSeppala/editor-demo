#ifndef INDEXER_H
#define INDEXER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <atomic>

/**
 * SearchResult - Result from the indexer search
 */
struct SearchResult {
    std::string file_path;
    size_t line_number;
    size_t column;
    std::string line_content;
};

/**
 * BackgroundIndexer - Separate service for project analysis
 * 
 * Runs in background thread, maintains inverted index
 * This allows instant search even in million-line codebases
 */
class BackgroundIndexer {
public:
    BackgroundIndexer();
    ~BackgroundIndexer();
    
    // Index operations
    void index_file(const std::string& file_path, const std::string& content);
    void remove_file(const std::string& file_path);
    
    // Search - returns results from in-memory index
    std::vector<SearchResult> search(const std::string& query, size_t max_results = 100);
    
    // Status
    bool is_indexing() const { return is_indexing_.load(); }
    size_t get_indexed_file_count() const;
    
    // Start/stop background indexing
    void start();
    void stop();
    
private:
    // Inverted index: word -> list of (file, line, column)
    struct Location {
        std::string file_path;
        size_t line_number;
        size_t column;
    };
    
    std::unordered_map<std::string, std::vector<Location>> index_;
    std::unordered_map<std::string, std::vector<std::string>> file_lines_;
    
    std::thread indexing_thread_;
    std::mutex index_mutex_;
    std::atomic<bool> is_indexing_;
    std::atomic<bool> should_stop_;
    
    void indexing_worker();
    void tokenize_and_index(const std::string& file_path, const std::string& content);
};

#endif // INDEXER_H
