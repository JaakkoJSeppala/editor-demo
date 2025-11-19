#include "git_integration.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <windows.h>

namespace fs = std::filesystem;

GitManager::GitManager() 
    : is_repo_(false)
    , current_branch_("master")
{
}

GitManager::~GitManager() {
}

bool GitManager::detect_repository(const std::string& directory) {
    // Look for .git directory
    fs::path dir_path(directory);
    fs::path current = dir_path;
    
    // Walk up directory tree looking for .git
    while (current.has_parent_path()) {
        fs::path git_dir = current / ".git";
        if (fs::exists(git_dir) && fs::is_directory(git_dir)) {
            repo_root_ = current.string();
            is_repo_ = true;
            
            // Get current branch
            std::string branch_output = execute_git_command("git rev-parse --abbrev-ref HEAD");
            if (!branch_output.empty()) {
                current_branch_ = branch_output;
                // Remove trailing newline
                if (!current_branch_.empty() && current_branch_.back() == '\n') {
                    current_branch_.pop_back();
                }
            }
            
            refresh_status();
            return true;
        }
        
        if (current == current.parent_path()) {
            break; // Reached root
        }
        current = current.parent_path();
    }
    
    is_repo_ = false;
    return false;
}

std::string GitManager::execute_git_command(const std::string& command) const {
    if (!is_repo_) return "";
    
    // Create pipe for reading command output
    HANDLE hReadPipe, hWritePipe;
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE };
    
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
        return "";
    }
    
    // Set up process startup info
    STARTUPINFOA si = {};
    si.cb = sizeof(STARTUPINFOA);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;
    
    PROCESS_INFORMATION pi = {};
    
    // Build command with directory change
    std::string full_command = "cmd.exe /c cd /d \"" + repo_root_ + "\" && " + command;
    
    // Execute command
    if (!CreateProcessA(nullptr, const_cast<char*>(full_command.c_str()),
                       nullptr, nullptr, TRUE, CREATE_NO_WINDOW,
                       nullptr, nullptr, &si, &pi)) {
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return "";
    }
    
    CloseHandle(hWritePipe);
    
    // Read output
    std::string output;
    char buffer[4096];
    DWORD bytes_read;
    
    while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytes_read, nullptr) && bytes_read > 0) {
        buffer[bytes_read] = '\0';
        output += buffer;
    }
    
    WaitForSingleObject(pi.hProcess, 5000); // 5 second timeout
    
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hReadPipe);
    
    return output;
}

void GitManager::refresh_status() {
    if (!is_repo_) return;
    
    // Clear caches
    file_status_.clear();
    staged_files_.clear();
    modified_files_.clear();
    untracked_files_.clear();
    
    // Get git status
    std::string status_output = execute_git_command("git status --porcelain");
    parse_status_output(status_output);
}

void GitManager::parse_status_output(const std::string& output) {
    std::istringstream stream(output);
    std::string line;
    
    while (std::getline(stream, line)) {
        if (line.length() < 4) continue;
        
        char index_status = line[0];
        char worktree_status = line[1];
        std::string file_path = line.substr(3);
        
        // Remove quotes if present
        if (!file_path.empty() && file_path[0] == '"') {
            file_path = file_path.substr(1, file_path.length() - 2);
        }
        
        GitFileStatus status = GitFileStatus::Unmodified;
        
        // Parse status codes
        if (index_status == '?' && worktree_status == '?') {
            status = GitFileStatus::Untracked;
            untracked_files_.push_back(file_path);
        } else if (index_status == 'A' || worktree_status == 'A') {
            status = GitFileStatus::Added;
            if (index_status == 'A') staged_files_.push_back(file_path);
        } else if (index_status == 'M' || worktree_status == 'M') {
            status = GitFileStatus::Modified;
            modified_files_.push_back(file_path);
            if (index_status == 'M') staged_files_.push_back(file_path);
        } else if (index_status == 'D' || worktree_status == 'D') {
            status = GitFileStatus::Deleted;
            if (index_status == 'D') staged_files_.push_back(file_path);
        } else if (index_status == 'R' || worktree_status == 'R') {
            status = GitFileStatus::Renamed;
        }
        
        file_status_[file_path] = status;
    }
}

GitFileStatus GitManager::get_file_status(const std::string& file_path) const {
    std::string rel_path = make_relative_path(file_path);
    auto it = file_status_.find(rel_path);
    return (it != file_status_.end()) ? it->second : GitFileStatus::Unmodified;
}

std::string GitManager::make_relative_path(const std::string& file_path) const {
    if (!is_repo_ || repo_root_.empty()) return file_path;
    
    fs::path abs_path = fs::absolute(file_path);
    fs::path repo_path = fs::absolute(repo_root_);
    
    if (abs_path.string().find(repo_path.string()) == 0) {
        fs::path relative = fs::relative(abs_path, repo_path);
        std::string result = relative.string();
        // Convert backslashes to forward slashes for git
        std::replace(result.begin(), result.end(), '\\', '/');
        return result;
    }
    
    return file_path;
}

std::vector<std::string> GitManager::get_modified_files() const {
    return modified_files_;
}

std::vector<std::string> GitManager::get_staged_files() const {
    return staged_files_;
}

std::vector<std::string> GitManager::get_untracked_files() const {
    return untracked_files_;
}

std::vector<GitDiffHunk> GitManager::get_file_diff(const std::string& file_path) const {
    std::vector<GitDiffHunk> hunks;
    if (!is_repo_) return hunks;
    
    std::string rel_path = make_relative_path(file_path);
    std::string diff_output = execute_git_command("git diff --unified=0 \"" + rel_path + "\"");
    
    parse_diff_output(diff_output, hunks);
    return hunks;
}

void GitManager::parse_diff_output(const std::string& output, std::vector<GitDiffHunk>& hunks) const {
    std::istringstream stream(output);
    std::string line;
    
    while (std::getline(stream, line)) {
        if (line.empty() || line[0] != '@') continue;
        
        // Parse @@ -old +new @@ format
        size_t plus_pos = line.find('+');
        if (plus_pos == std::string::npos) continue;
        
        size_t comma_pos = line.find(',', plus_pos);
        size_t space_pos = line.find(' ', plus_pos);
        
        if (space_pos == std::string::npos) continue;
        
        GitDiffHunk hunk;
        hunk.start_line = std::stoul(line.substr(plus_pos + 1, 
            (comma_pos != std::string::npos && comma_pos < space_pos) ? comma_pos - plus_pos - 1 : space_pos - plus_pos - 1));
        
        if (comma_pos != std::string::npos && comma_pos < space_pos) {
            hunk.line_count = std::stoul(line.substr(comma_pos + 1, space_pos - comma_pos - 1));
        } else {
            hunk.line_count = 1;
        }
        
        hunk.type = GitDiffHunk::Type::Modified;
        hunks.push_back(hunk);
    }
}

std::string GitManager::get_file_diff_text(const std::string& file_path) const {
    if (!is_repo_) return "";
    std::string rel_path = make_relative_path(file_path);
    return execute_git_command("git diff \"" + rel_path + "\"");
}

bool GitManager::stage_file(const std::string& file_path) {
    if (!is_repo_) return false;
    std::string rel_path = make_relative_path(file_path);
    std::string output = execute_git_command("git add \"" + rel_path + "\"");
    refresh_status();
    return true;
}

bool GitManager::unstage_file(const std::string& file_path) {
    if (!is_repo_) return false;
    std::string rel_path = make_relative_path(file_path);
    std::string output = execute_git_command("git reset HEAD \"" + rel_path + "\"");
    refresh_status();
    return true;
}

bool GitManager::stage_all() {
    if (!is_repo_) return false;
    execute_git_command("git add -A");
    refresh_status();
    return true;
}

bool GitManager::commit(const std::string& message) {
    if (!is_repo_ || message.empty()) return false;
    
    // Escape quotes in message
    std::string escaped_msg = message;
    size_t pos = 0;
    while ((pos = escaped_msg.find('"', pos)) != std::string::npos) {
        escaped_msg.insert(pos, "\\");
        pos += 2;
    }
    
    std::string output = execute_git_command("git commit -m \"" + escaped_msg + "\"");
    refresh_status();
    return output.find("nothing to commit") == std::string::npos;
}

bool GitManager::amend_commit(const std::string& message) {
    if (!is_repo_) return false;
    
    std::string escaped_msg = message;
    size_t pos = 0;
    while ((pos = escaped_msg.find('"', pos)) != std::string::npos) {
        escaped_msg.insert(pos, "\\");
        pos += 2;
    }
    
    std::string cmd = message.empty() ? 
        "git commit --amend --no-edit" : 
        "git commit --amend -m \"" + escaped_msg + "\"";
    
    execute_git_command(cmd);
    refresh_status();
    return true;
}

std::vector<std::string> GitManager::get_commit_history(int count) {
    std::vector<std::string> history;
    if (!is_repo_) return history;
    
    std::string output = execute_git_command("git log -" + std::to_string(count) + " --oneline");
    std::istringstream stream(output);
    std::string line;
    
    while (std::getline(stream, line)) {
        if (!line.empty()) {
            history.push_back(line);
        }
    }
    
    return history;
}

std::vector<GitBranch> GitManager::get_branches() {
    std::vector<GitBranch> branches;
    if (!is_repo_) return branches;
    
    std::string output = execute_git_command("git branch -v");
    std::istringstream stream(output);
    std::string line;
    
    while (std::getline(stream, line)) {
        if (line.length() < 3) continue;
        
        GitBranch branch;
        branch.is_current = (line[0] == '*');
        
        size_t name_start = branch.is_current ? 2 : 2;
        size_t name_end = line.find(' ', name_start);
        if (name_end == std::string::npos) continue;
        
        branch.name = line.substr(name_start, name_end - name_start);
        
        size_t hash_start = name_end + 1;
        size_t hash_end = line.find(' ', hash_start);
        if (hash_end != std::string::npos) {
            branch.commit_hash = line.substr(hash_start, hash_end - hash_start);
        }
        
        branches.push_back(branch);
    }
    
    return branches;
}

bool GitManager::create_branch(const std::string& branch_name) {
    if (!is_repo_ || branch_name.empty()) return false;
    std::string output = execute_git_command("git branch \"" + branch_name + "\"");
    return output.find("fatal") == std::string::npos;
}

bool GitManager::switch_branch(const std::string& branch_name) {
    if (!is_repo_ || branch_name.empty()) return false;
    std::string output = execute_git_command("git checkout \"" + branch_name + "\"");
    
    if (output.find("fatal") == std::string::npos) {
        current_branch_ = branch_name;
        refresh_status();
        return true;
    }
    return false;
}

bool GitManager::delete_branch(const std::string& branch_name) {
    if (!is_repo_ || branch_name.empty() || branch_name == current_branch_) return false;
    std::string output = execute_git_command("git branch -d \"" + branch_name + "\"");
    return output.find("fatal") == std::string::npos;
}
