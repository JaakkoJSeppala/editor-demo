#ifndef GIT_INTEGRATION_H
#define GIT_INTEGRATION_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

/**
 * Git file status enumeration
 */
enum class GitFileStatus {
    Unmodified,
    Modified,
    Added,
    Deleted,
    Renamed,
    Untracked,
    Ignored
};

/**
 * Git branch information
 */
struct GitBranch {
    std::string name;
    bool is_current;
    std::string commit_hash;
};

/**
 * Git diff hunk for gutter display
 */
struct GitDiffHunk {
    size_t start_line;
    size_t line_count;
    enum class Type { Added, Modified, Deleted } type;
};

/**
 * GitManager - Lightweight git integration for editor
 * 
 * Provides essential git features without external dependencies:
 * - Repository detection
 * - File status tracking
 * - Basic diff viewing
 * - Commit/branch operations via git command execution
 */
class GitManager {
public:
    GitManager();
    ~GitManager();
    
    // Repository detection
    bool detect_repository(const std::string& directory);
    bool is_git_repository() const { return is_repo_; }
    std::string get_repo_root() const { return repo_root_; }
    
    // Status operations
    void refresh_status();
    GitFileStatus get_file_status(const std::string& file_path) const;
    std::vector<std::string> get_modified_files() const;
    std::vector<std::string> get_staged_files() const;
    std::vector<std::string> get_untracked_files() const;
    
    // Diff operations
    std::vector<GitDiffHunk> get_file_diff(const std::string& file_path) const;
    std::string get_file_diff_text(const std::string& file_path) const;
    
    // Staging operations
    bool stage_file(const std::string& file_path);
    bool unstage_file(const std::string& file_path);
    bool stage_all();
    
    // Commit operations
    bool commit(const std::string& message);
    bool amend_commit(const std::string& message);
    std::vector<std::string> get_commit_history(int count = 10);
    
    // Branch operations
    std::vector<GitBranch> get_branches();
    std::string get_current_branch() const { return current_branch_; }
    bool create_branch(const std::string& branch_name);
    bool switch_branch(const std::string& branch_name);
    bool delete_branch(const std::string& branch_name);
    
private:
    // Helper functions
    std::string execute_git_command(const std::string& command) const;
    void parse_status_output(const std::string& output);
    void parse_diff_output(const std::string& output, std::vector<GitDiffHunk>& hunks) const;
    std::string make_relative_path(const std::string& file_path) const;
    
    bool is_repo_;
    std::string repo_root_;
    std::string current_branch_;
    
    // File status cache
    std::unordered_map<std::string, GitFileStatus> file_status_;
    std::vector<std::string> staged_files_;
    std::vector<std::string> modified_files_;
    std::vector<std::string> untracked_files_;
};

#endif // GIT_INTEGRATION_H
