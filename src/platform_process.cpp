#include "platform_process.h"
#include <cstring>
#include <sstream>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <cstdint>
#include <process.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>
#include <poll.h>
#include <spawn.h>
extern char **environ;
#endif

namespace editor {

// PlatformProcess implementation

PlatformProcess::PlatformProcess()
    : process_handle_(nullptr)
    , stdin_pipe_(nullptr)
    , stdout_pipe_(nullptr)
    , stderr_pipe_(nullptr)
    , pid_(0)
    , exit_code_(0)
    , running_(false) {
}

PlatformProcess::~PlatformProcess() {
    if (running_) {
        terminate(1000);
    }
    cleanup();
}

bool PlatformProcess::start(const std::string& executable, 
                            const std::vector<std::string>& args,
                            const ProcessOptions& options) {
    if (running_) {
        return false;
    }
    
    cleanup();
    
#ifdef _WIN32
    // Windows implementation using CreateProcess
    HANDLE stdin_read = NULL, stdin_write = NULL;
    HANDLE stdout_read = NULL, stdout_write = NULL;
    HANDLE stderr_read = NULL, stderr_write = NULL;
    
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;
    
    // Create pipes for I/O redirection
    if (options.io.redirect_stdin) {
        if (!CreatePipe(&stdin_read, &stdin_write, &sa, 0)) return false;
        SetHandleInformation(stdin_write, HANDLE_FLAG_INHERIT, 0);
        stdin_pipe_ = stdin_write;
    }
    
    if (options.io.redirect_stdout) {
        if (!CreatePipe(&stdout_read, &stdout_write, &sa, 0)) {
            cleanup();
            return false;
        }
        SetHandleInformation(stdout_read, HANDLE_FLAG_INHERIT, 0);
        stdout_pipe_ = stdout_read;
    }
    
    if (options.io.redirect_stderr && !options.io.merge_stderr_to_stdout) {
        if (!CreatePipe(&stderr_read, &stderr_write, &sa, 0)) {
            cleanup();
            return false;
        }
        SetHandleInformation(stderr_read, HANDLE_FLAG_INHERIT, 0);
        stderr_pipe_ = stderr_read;
    }
    
    // Build command line
    std::string cmdline = "\"" + executable + "\"";
    for (const auto& arg : args) {
        cmdline += " " + ProcessUtils::escape_argument(arg);
    }
    
    // Setup startup info
    STARTUPINFOA si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = stdin_read ? stdin_read : GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = stdout_write ? stdout_write : GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError = (options.io.merge_stderr_to_stdout && stdout_write) ? stdout_write :
                   (stderr_write ? stderr_write : GetStdHandle(STD_ERROR_HANDLE));
    
    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));
    
    DWORD flags = 0;
    if (options.create_new_console) flags |= CREATE_NEW_CONSOLE;
    
    // Create process
    BOOL success = CreateProcessA(
        NULL,
        const_cast<char*>(cmdline.c_str()),
        NULL, NULL,
        TRUE,  // Inherit handles
        flags,
        NULL,
        options.working_directory.empty() ? NULL : options.working_directory.c_str(),
        &si,
        &pi
    );
    
    // Close child handles
    if (stdin_read) CloseHandle(stdin_read);
    if (stdout_write) CloseHandle(stdout_write);
    if (stderr_write) CloseHandle(stderr_write);
    
    if (!success) {
        cleanup();
        return false;
    }
    
    process_handle_ = pi.hProcess;
    pid_ = pi.dwProcessId;
    CloseHandle(pi.hThread);
    running_ = true;
    
#else
    // Unix implementation using posix_spawn or fork/exec
    int stdin_fds[2] = {-1, -1};
    int stdout_fds[2] = {-1, -1};
    int stderr_fds[2] = {-1, -1};
    
    // Create pipes
    if (options.io.redirect_stdin) {
        if (pipe(stdin_fds) != 0) return false;
        stdin_pipe_ = stdin_fds[1];  // Write end
    }
    
    if (options.io.redirect_stdout) {
        if (pipe(stdout_fds) != 0) {
            cleanup();
            return false;
        }
        stdout_pipe_ = stdout_fds[0];  // Read end
    }
    
    if (options.io.redirect_stderr && !options.io.merge_stderr_to_stdout) {
        if (pipe(stderr_fds) != 0) {
            cleanup();
            return false;
        }
        stderr_pipe_ = stderr_fds[0];  // Read end
    }
    
    pid_t pid = fork();
    if (pid < 0) {
        cleanup();
        return false;
    }
    
    if (pid == 0) {
        // Child process
        
        // Redirect I/O
        if (options.io.redirect_stdin) {
            dup2(stdin_fds[0], STDIN_FILENO);
            close(stdin_fds[0]);
            close(stdin_fds[1]);
        }
        
        if (options.io.redirect_stdout) {
            dup2(stdout_fds[1], STDOUT_FILENO);
            close(stdout_fds[0]);
            close(stdout_fds[1]);
        }
        
        if (options.io.redirect_stderr) {
            if (options.io.merge_stderr_to_stdout) {
                dup2(STDOUT_FILENO, STDERR_FILENO);
            } else {
                dup2(stderr_fds[1], STDERR_FILENO);
                close(stderr_fds[0]);
                close(stderr_fds[1]);
            }
        }
        
        // Change working directory
        if (!options.working_directory.empty()) {
            chdir(options.working_directory.c_str());
        }
        
        // Set environment
        for (const auto& env : options.environment) {
            setenv(env.first.c_str(), env.second.c_str(), 1);
        }
        
        // Detach if requested
        if (options.detached) {
            setsid();
        }
        
        // Build argv
        std::vector<char*> argv;
        argv.push_back(const_cast<char*>(executable.c_str()));
        for (const auto& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);
        
        // Execute
        execvp(executable.c_str(), argv.data());
        
        // If we get here, exec failed
        _exit(127);
    }
    
    // Parent process
    // Close child ends of pipes
    if (stdin_fds[0] != -1) close(stdin_fds[0]);
    if (stdout_fds[1] != -1) close(stdout_fds[1]);
    if (stderr_fds[1] != -1) close(stderr_fds[1]);
    
    process_handle_ = pid;
    pid_ = pid;
    running_ = true;
#endif
    
    return true;
}

bool PlatformProcess::wait(int timeout_ms) {
    ProcessExit exit_info;
    return wait(exit_info, timeout_ms);
}

bool PlatformProcess::wait(ProcessExit& exit_info, int timeout_ms) {
    if (!running_) return false;
    
#ifdef _WIN32
    DWORD wait_time = (timeout_ms < 0) ? INFINITE : timeout_ms;
    DWORD result = WaitForSingleObject(process_handle_, wait_time);
    
    if (result == WAIT_OBJECT_0) {
        DWORD exit_code;
        GetExitCodeProcess(process_handle_, &exit_code);
        exit_code_ = exit_code;
        exit_info.exit_code = exit_code;
        exit_info.success = (exit_code == 0);
        exit_info.terminated = false;
        running_ = false;
        return true;
    }
    
    return false;
#else
    int status;
    int options = (timeout_ms == 0) ? WNOHANG : 0;
    
    // For timeout, we'd need to poll
    pid_t result = waitpid(process_handle_, &status, options);
    
    if (result == process_handle_) {
        if (WIFEXITED(status)) {
            exit_code_ = WEXITSTATUS(status);
            exit_info.exit_code = exit_code_;
            exit_info.success = (exit_code_ == 0);
            exit_info.terminated = false;
        } else if (WIFSIGNALED(status)) {
            exit_code_ = WTERMSIG(status);
            exit_info.exit_code = exit_code_;
            exit_info.success = false;
            exit_info.terminated = true;
        }
        running_ = false;
        return true;
    }
    
    return false;
#endif
}

bool PlatformProcess::is_running() const {
    return running_;
}

uint32_t PlatformProcess::get_pid() const {
    return pid_;
}

bool PlatformProcess::terminate(int timeout_ms) {
    if (!running_) return true;
    
#ifdef _WIN32
    if (!TerminateProcess(process_handle_, 0)) return false;
    return wait(timeout_ms);
#else
    if (kill(process_handle_, SIGTERM) != 0) return false;
    
    // Wait for graceful termination
    for (int i = 0; i < timeout_ms / 100; ++i) {
        if (!is_running()) return true;
        usleep(100000); // 100ms
    }
    
    return !is_running();
#endif
}

bool PlatformProcess::kill() {
    if (!running_) return true;
    
#ifdef _WIN32
    return TerminateProcess(process_handle_, 1) != 0;
#else
    return ::kill(process_handle_, SIGKILL) == 0;
#endif
}

bool PlatformProcess::write_stdin(const std::string& data) {
    if (!stdin_pipe_) return false;
    
#ifdef _WIN32
    DWORD written;
    return WriteFile(stdin_pipe_, data.c_str(), data.size(), &written, NULL) != 0;
#else
    return write(stdin_pipe_, data.c_str(), data.size()) >= 0;
#endif
}

bool PlatformProcess::read_stdout(std::string& data, int timeout_ms) {
    if (!stdout_pipe_) return false;
    
#ifdef _WIN32
    DWORD available;
    if (!PeekNamedPipe(stdout_pipe_, NULL, 0, NULL, &available, NULL)) return false;
    if (available == 0) return false;
    
    char buffer[4096];
    DWORD read;
    if (ReadFile(stdout_pipe_, buffer, sizeof(buffer) - 1, &read, NULL)) {
        buffer[read] = '\0';
        data = std::string(buffer, read);
        return true;
    }
    return false;
#else
    // Set non-blocking if timeout is 0
    if (timeout_ms == 0) {
        int flags = fcntl(stdout_pipe_, F_GETFL, 0);
        fcntl(stdout_pipe_, F_SETFL, flags | O_NONBLOCK);
    }
    
    char buffer[4096];
    ssize_t bytes = read(stdout_pipe_, buffer, sizeof(buffer) - 1);
    
    if (bytes > 0) {
        buffer[bytes] = '\0';
        data = std::string(buffer, bytes);
        return true;
    }
    
    return false;
#endif
}

bool PlatformProcess::read_stderr(std::string& data, int timeout_ms) {
    if (!stderr_pipe_) return false;
    
    // Similar to read_stdout
#ifdef _WIN32
    DWORD available;
    if (!PeekNamedPipe(stderr_pipe_, NULL, 0, NULL, &available, NULL)) return false;
    if (available == 0) return false;
    
    char buffer[4096];
    DWORD read;
    if (ReadFile(stderr_pipe_, buffer, sizeof(buffer) - 1, &read, NULL)) {
        buffer[read] = '\0';
        data = std::string(buffer, read);
        return true;
    }
    return false;
#else
    char buffer[4096];
    ssize_t bytes = read(stderr_pipe_, buffer, sizeof(buffer) - 1);
    
    if (bytes > 0) {
        buffer[bytes] = '\0';
        data = std::string(buffer, bytes);
        return true;
    }
    
    return false;
#endif
}

bool PlatformProcess::close_stdin() {
    if (!stdin_pipe_) return false;
    
    close_pipe(stdin_pipe_);
    return true;
}

void PlatformProcess::cleanup() {
    close_pipe(stdin_pipe_);
    close_pipe(stdout_pipe_);
    close_pipe(stderr_pipe_);
    
#ifdef _WIN32
    if (process_handle_) {
        CloseHandle(process_handle_);
        process_handle_ = NULL;
    }
#endif
}

void PlatformProcess::close_pipe(PipeHandle& handle) {
    if (!handle) return;
    
#ifdef _WIN32
    CloseHandle(handle);
    handle = NULL;
#else
    close(handle);
    handle = -1;
#endif
}

// ProcessUtils implementation

bool ProcessUtils::execute(const std::string& command, 
                          std::string& output,
                          int& exit_code,
                          int timeout_ms) {
    std::string executable;
    std::vector<std::string> args;
    
    if (!parse_command_line(command, executable, args)) {
        return false;
    }
    
    return execute(executable, args, output, exit_code, timeout_ms);
}

bool ProcessUtils::execute(const std::string& executable,
                          const std::vector<std::string>& args,
                          std::string& output,
                          int& exit_code,
                          int timeout_ms) {
    ProcessOptions options;
    options.io.redirect_stdout = true;
    options.io.redirect_stderr = true;
    options.io.merge_stderr_to_stdout = true;
    
    PlatformProcess process;
    if (!process.start(executable, args, options)) {
        return false;
    }
    
    output.clear();
    std::string chunk;
    while (process.is_running()) {
        if (process.read_stdout(chunk, 10)) {
            output += chunk;
        }
    }
    
    // Read remaining output
    while (process.read_stdout(chunk, 0)) {
        output += chunk;
    }
    
    ProcessExit exit_info;
    if (process.wait(exit_info, timeout_ms)) {
        exit_code = exit_info.exit_code;
        return exit_info.success;
    }
    
    return false;
}

std::string ProcessUtils::find_executable(const std::string& name) {
#ifdef _WIN32
    char path[MAX_PATH];
    if (SearchPathA(NULL, name.c_str(), ".exe", MAX_PATH, path, NULL)) {
        return std::string(path);
    }
    return "";
#else
    std::string path_env = get_env("PATH");
    std::istringstream iss(path_env);
    std::string dir;
    
    while (std::getline(iss, dir, ':')) {
        std::string full_path = dir + "/" + name;
        if (access(full_path.c_str(), X_OK) == 0) {
            return full_path;
        }
    }
    
    return "";
#endif
}

uint32_t ProcessUtils::get_current_pid() {
#ifdef _WIN32
    return GetCurrentProcessId();
#else
    return getpid();
#endif
}

bool ProcessUtils::is_process_running(uint32_t pid) {
#ifdef _WIN32
    HANDLE handle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!handle) return false;
    
    DWORD exit_code;
    bool running = GetExitCodeProcess(handle, &exit_code) && exit_code == STILL_ACTIVE;
    CloseHandle(handle);
    return running;
#else
    return kill(pid, 0) == 0;
#endif
}

bool ProcessUtils::kill_process(uint32_t pid) {
#ifdef _WIN32
    HANDLE handle = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (!handle) return false;
    
    bool success = TerminateProcess(handle, 1) != 0;
    CloseHandle(handle);
    return success;
#else
    return kill(pid, SIGKILL) == 0;
#endif
}

bool ProcessUtils::parse_command_line(const std::string& command_line,
                                      std::string& executable,
                                      std::vector<std::string>& args) {
    args.clear();
    
    std::istringstream iss(command_line);
    std::string token;
    bool first = true;
    
    while (iss >> token) {
        if (first) {
            executable = token;
            first = false;
        } else {
            args.push_back(token);
        }
    }
    
    return !executable.empty();
}

std::string ProcessUtils::escape_argument(const std::string& arg) {
    if (arg.find(' ') == std::string::npos && 
        arg.find('"') == std::string::npos &&
        arg.find('\'') == std::string::npos) {
        return arg;
    }
    
    std::string escaped = "\"";
    for (char c : arg) {
        if (c == '"') {
            escaped += "\\\"";
        } else if (c == '\\') {
            escaped += "\\\\";
        } else {
            escaped += c;
        }
    }
    escaped += "\"";
    return escaped;
}

std::string ProcessUtils::get_shell() {
#ifdef _WIN32
    const char* comspec = getenv("COMSPEC");
    return comspec ? std::string(comspec) : "cmd.exe";
#else
    const char* shell = getenv("SHELL");
    return shell ? std::string(shell) : "/bin/sh";
#endif
}

std::string ProcessUtils::get_env(const std::string& name) {
    const char* value = getenv(name.c_str());
    return value ? std::string(value) : "";
}

bool ProcessUtils::set_env(const std::string& name, const std::string& value) {
#ifdef _WIN32
    return SetEnvironmentVariableA(name.c_str(), value.c_str()) != 0;
#else
    return setenv(name.c_str(), value.c_str(), 1) == 0;
#endif
}

} // namespace editor
