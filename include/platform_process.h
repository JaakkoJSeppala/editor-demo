#ifndef PLATFORM_PROCESS_H
#define PLATFORM_PROCESS_H

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <cstdint>

namespace editor {

// Process handle (platform-specific)
#ifdef _WIN32
using ProcessHandle = void*; // HANDLE
using PipeHandle = void*;    // HANDLE
#else
using ProcessHandle = int;   // pid_t
using PipeHandle = int;      // file descriptor
#endif

// Process exit status
struct ProcessExit {
    int exit_code;
    bool success;
    bool terminated;  // true if killed/terminated
};

// Process I/O configuration
struct ProcessIO {
    bool redirect_stdin = false;
    bool redirect_stdout = false;
    bool redirect_stderr = false;
    bool merge_stderr_to_stdout = false;
};

// Process startup options
struct ProcessOptions {
    std::string working_directory;
    std::vector<std::pair<std::string, std::string>> environment; // key-value pairs
    bool create_new_console = false;  // Windows: CREATE_NEW_CONSOLE
    bool detached = false;            // Unix: setsid()
    ProcessIO io;
};

// Forward declaration
class PlatformProcess;

// Callback for async output
using ProcessOutputCallback = std::function<void(const std::string& data, bool is_stderr)>;

// Platform-agnostic process management
class PlatformProcess {
public:
    PlatformProcess();
    ~PlatformProcess();
    
    // Prevent copying
    PlatformProcess(const PlatformProcess&) = delete;
    PlatformProcess& operator=(const PlatformProcess&) = delete;
    
    // Start a process
    bool start(const std::string& executable, 
               const std::vector<std::string>& args = {},
               const ProcessOptions& options = {});
    
    // Wait for process to exit
    bool wait(int timeout_ms = -1); // -1 = infinite
    bool wait(ProcessExit& exit_info, int timeout_ms = -1);
    
    // Check if running
    bool is_running() const;
    
    // Get process ID
    uint32_t get_pid() const;
    
    // Terminate process
    bool terminate(int timeout_ms = 5000); // Graceful termination
    bool kill();                           // Force kill
    
    // I/O operations (if pipes are redirected)
    bool write_stdin(const std::string& data);
    bool read_stdout(std::string& data, int timeout_ms = 0);
    bool read_stderr(std::string& data, int timeout_ms = 0);
    bool close_stdin();
    
    // Get pipe handles for custom I/O handling
    PipeHandle get_stdin_pipe() const { return stdin_pipe_; }
    PipeHandle get_stdout_pipe() const { return stdout_pipe_; }
    PipeHandle get_stderr_pipe() const { return stderr_pipe_; }
    
    // Get process handle
    ProcessHandle get_handle() const { return process_handle_; }
    
    // Get exit code (only valid after process exits)
    int get_exit_code() const { return exit_code_; }
    
private:
    ProcessHandle process_handle_;
    PipeHandle stdin_pipe_;
    PipeHandle stdout_pipe_;
    PipeHandle stderr_pipe_;
    uint32_t pid_;
    int exit_code_;
    bool running_;
    
    void cleanup();
    bool create_pipe(PipeHandle& read_handle, PipeHandle& write_handle);
    void close_pipe(PipeHandle& handle);
};

// Utility functions
class ProcessUtils {
public:
    // Execute and wait for completion
    static bool execute(const std::string& command, 
                       std::string& output,
                       int& exit_code,
                       int timeout_ms = -1);
    
    static bool execute(const std::string& executable,
                       const std::vector<std::string>& args,
                       std::string& output,
                       int& exit_code,
                       int timeout_ms = -1);
    
    // Find executable in PATH
    static std::string find_executable(const std::string& name);
    
    // Get current process ID
    static uint32_t get_current_pid();
    
    // Check if process exists
    static bool is_process_running(uint32_t pid);
    
    // Kill process by PID
    static bool kill_process(uint32_t pid);
    
    // Parse command line into executable + args
    static bool parse_command_line(const std::string& command_line,
                                   std::string& executable,
                                   std::vector<std::string>& args);
    
    // Escape argument for shell
    static std::string escape_argument(const std::string& arg);
    
    // Get platform shell
    static std::string get_shell();
    
    // Get environment variable
    static std::string get_env(const std::string& name);
    static bool set_env(const std::string& name, const std::string& value);
};

} // namespace editor

#endif // PLATFORM_PROCESS_H
