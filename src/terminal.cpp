#include "terminal.h"
#include <iostream>

EmbeddedTerminal::EmbeddedTerminal()
    : process_handle_(nullptr)
    , thread_handle_(nullptr)
    , stdin_write_(nullptr)
    , stdout_read_(nullptr)
    , process_running_(false)
    , cursor_line_(0)
    , cursor_col_(0)
    , scroll_offset_(0)
    , history_index_(-1)
    , reader_thread_(nullptr)
{
    InitializeCriticalSection(&output_mutex_);
    buffer_.push_back(""); // Start with one line
}

EmbeddedTerminal::~EmbeddedTerminal() {
    stop_shell();
    DeleteCriticalSection(&output_mutex_);
}

bool EmbeddedTerminal::start_shell(const std::string& shell_path) {
    if (process_running_) {
        stop_shell();
    }
    
    // Create pipes for stdin/stdout
    HANDLE stdin_read = nullptr;
    HANDLE stdout_write = nullptr;
    
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;
    
    if (!CreatePipe(&stdin_read, &stdin_write_, &sa, 0)) {
        return false;
    }
    if (!CreatePipe(&stdout_read_, &stdout_write, &sa, 0)) {
        CloseHandle(stdin_read);
        CloseHandle(stdin_write_);
        return false;
    }
    
    // Ensure child doesn't inherit our end of pipes
    SetHandleInformation(stdin_write_, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(stdout_read_, HANDLE_FLAG_INHERIT, 0);
    
    // Create process
    STARTUPINFOA si{};
    si.cb = sizeof(STARTUPINFOA);
    si.hStdInput = stdin_read;
    si.hStdOutput = stdout_write;
    si.hStdError = stdout_write;
    si.dwFlags |= STARTF_USESTDHANDLES;
    
    PROCESS_INFORMATION pi{};
    
    std::string cmdline = shell_path;
    char* cmd_buffer = new char[cmdline.size() + 1];
    strcpy_s(cmd_buffer, cmdline.size() + 1, cmdline.c_str());
    
    BOOL success = CreateProcessA(
        nullptr,
        cmd_buffer,
        nullptr,
        nullptr,
        TRUE,
        CREATE_NO_WINDOW,
        nullptr,
        nullptr,
        &si,
        &pi
    );
    
    delete[] cmd_buffer;
    
    // Close child's ends
    CloseHandle(stdin_read);
    CloseHandle(stdout_write);
    
    if (!success) {
        CloseHandle(stdin_write_);
        CloseHandle(stdout_read_);
        stdin_write_ = nullptr;
        stdout_read_ = nullptr;
        return false;
    }
    
    process_handle_ = pi.hProcess;
    thread_handle_ = pi.hThread;
    process_running_ = true;
    
    // Start background reader thread
    reader_thread_ = CreateThread(nullptr, 0, read_output_thread, this, 0, nullptr);
    
    return true;
}

void EmbeddedTerminal::stop_shell() {
    if (!process_running_) return;
    
    process_running_ = false;
    
    // Close pipes to signal EOF
    if (stdin_write_) {
        CloseHandle(stdin_write_);
        stdin_write_ = nullptr;
    }
    
    // Wait for process to exit
    if (process_handle_) {
        WaitForSingleObject(process_handle_, 2000);
        TerminateProcess(process_handle_, 0);
        CloseHandle(process_handle_);
        CloseHandle(thread_handle_);
        process_handle_ = nullptr;
        thread_handle_ = nullptr;
    }
    
    // Wait for reader thread
    if (reader_thread_) {
        WaitForSingleObject(reader_thread_, 1000);
        CloseHandle(reader_thread_);
        reader_thread_ = nullptr;
    }
    
    if (stdout_read_) {
        CloseHandle(stdout_read_);
        stdout_read_ = nullptr;
    }
}

void EmbeddedTerminal::send_input(const std::string& text) {
    if (!process_running_ || !stdin_write_) return;
    
    DWORD written;
    WriteFile(stdin_write_, text.c_str(), static_cast<DWORD>(text.size()), &written, nullptr);
}

void EmbeddedTerminal::send_line(const std::string& line) {
    send_input(line + "\r\n");
    add_to_history(line);
}

std::string EmbeddedTerminal::get_output() {
    EnterCriticalSection(&output_mutex_);
    std::string output = pending_output_;
    pending_output_.clear();
    LeaveCriticalSection(&output_mutex_);
    
    if (!output.empty()) {
        process_output(output);
    }
    
    return output;
}

void EmbeddedTerminal::add_to_history(const std::string& command) {
    if (command.empty()) return;
    
    // Avoid duplicates
    if (!history_.empty() && history_.back() == command) return;
    
    history_.push_back(command);
    if (history_.size() > MAX_HISTORY) {
        history_.pop_front();
    }
    history_index_ = -1;
}

std::string EmbeddedTerminal::history_prev() {
    if (history_.empty()) return "";
    
    if (history_index_ < 0) {
        history_index_ = static_cast<int>(history_.size()) - 1;
    } else if (history_index_ > 0) {
        history_index_--;
    }
    
    return history_[history_index_];
}

std::string EmbeddedTerminal::history_next() {
    if (history_.empty() || history_index_ < 0) return "";
    
    history_index_++;
    if (history_index_ >= static_cast<int>(history_.size())) {
        history_index_ = -1;
        return "";
    }
    
    return history_[history_index_];
}

void EmbeddedTerminal::clear() {
    buffer_.clear();
    buffer_.push_back("");
    cursor_line_ = 0;
    cursor_col_ = 0;
    scroll_offset_ = 0;
}

void EmbeddedTerminal::process_output(const std::string& output) {
    for (char c : output) {
        if (c == '\r') {
            cursor_col_ = 0;
        } else if (c == '\n') {
            cursor_line_++;
            cursor_col_ = 0;
            if (cursor_line_ >= buffer_.size()) {
                buffer_.push_back("");
            }
        } else if (c == '\b') {
            if (cursor_col_ > 0) {
                cursor_col_--;
            }
        } else if (c == '\t') {
            // Tab = 4 spaces
            append_text("    ");
        } else if (c >= 32 || c == 27) { // Printable or ESC
            append_text(std::string(1, c));
        }
    }
    
    // Keep buffer from growing too large
    const size_t MAX_BUFFER = 10000;
    if (buffer_.size() > MAX_BUFFER) {
        buffer_.erase(buffer_.begin(), buffer_.begin() + (buffer_.size() - MAX_BUFFER));
        if (cursor_line_ >= MAX_BUFFER) {
            cursor_line_ -= (buffer_.size() - MAX_BUFFER);
        }
    }
}

void EmbeddedTerminal::append_text(const std::string& text) {
    if (cursor_line_ >= buffer_.size()) {
        buffer_.push_back("");
        cursor_line_ = buffer_.size() - 1;
    }
    
    std::string& line = buffer_[cursor_line_];
    
    // Extend line if needed
    if (cursor_col_ > line.size()) {
        line.resize(cursor_col_, ' ');
    }
    
    // Insert text at cursor
    if (cursor_col_ >= line.size()) {
        line += text;
    } else {
        line = line.substr(0, cursor_col_) + text + line.substr(cursor_col_);
    }
    
    cursor_col_ += text.size();
}

void EmbeddedTerminal::parse_ansi_escape(const std::string& seq) {
    // Basic ANSI escape sequence support (simplified)
    // Full VT100 support would be much more complex
    (void)seq; // TODO: Implement ANSI color codes, cursor movements, etc.
}

DWORD WINAPI EmbeddedTerminal::read_output_thread(LPVOID param) {
    EmbeddedTerminal* term = static_cast<EmbeddedTerminal*>(param);
    
    char buffer[4096];
    DWORD read;
    
    while (term->process_running_) {
        if (ReadFile(term->stdout_read_, buffer, sizeof(buffer) - 1, &read, nullptr) && read > 0) {
            buffer[read] = '\0';
            
            EnterCriticalSection(&term->output_mutex_);
            term->pending_output_.append(buffer, read);
            LeaveCriticalSection(&term->output_mutex_);
        } else {
            break;
        }
    }
    
    return 0;
}
