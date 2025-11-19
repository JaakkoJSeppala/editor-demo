#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <deque>
#include <memory>

/**
 * EmbeddedTerminal - Integrated terminal panel for Win32 editor
 * 
 * Features:
 * - Process spawning (PowerShell, CMD)
 * - Asynchronous I/O with pipes
 * - VT100-style ANSI escape sequence support
 * - Scrollback buffer
 * - Command history
 * - Text selection and copy
 */
class EmbeddedTerminal {
public:
    EmbeddedTerminal();
    ~EmbeddedTerminal();
    
    // Terminal lifecycle
    bool start_shell(const std::string& shell_path = "powershell.exe");
    void stop_shell();
    bool is_running() const { return process_running_; }
    
    // Input/Output
    void send_input(const std::string& text);
    void send_line(const std::string& line);
    std::string get_output(); // Get and clear pending output
    
    // Display buffer access
    const std::vector<std::string>& get_buffer() const { return buffer_; }
    size_t get_buffer_size() const { return buffer_.size(); }
    size_t get_cursor_line() const { return cursor_line_; }
    size_t get_cursor_column() const { return cursor_col_; }
    
    // Scrolling
    void set_scroll_offset(int offset) { scroll_offset_ = offset; }
    int get_scroll_offset() const { return scroll_offset_; }
    
    // History
    void add_to_history(const std::string& command);
    std::string history_prev();
    std::string history_next();
    
    // Clear
    void clear();
    
private:
    // Process management
    HANDLE process_handle_;
    HANDLE thread_handle_;
    HANDLE stdin_write_;
    HANDLE stdout_read_;
    bool process_running_;
    
    // Display buffer
    std::vector<std::string> buffer_;
    size_t cursor_line_;
    size_t cursor_col_;
    int scroll_offset_;
    
    // Command history
    std::deque<std::string> history_;
    int history_index_;
    static const size_t MAX_HISTORY = 100;
    
    // Pending output
    std::string pending_output_;
    
    // ANSI escape sequence parser
    void process_output(const std::string& output);
    void parse_ansi_escape(const std::string& seq);
    void append_text(const std::string& text);
    
    // Background reading
    static DWORD WINAPI read_output_thread(LPVOID param);
    HANDLE reader_thread_;
    CRITICAL_SECTION output_mutex_;
};
