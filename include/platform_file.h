#ifndef PLATFORM_FILE_H
#define PLATFORM_FILE_H

#include <string>
#include <vector>
#include <cstdint>
#include <filesystem>

namespace editor {

// Line ending types
enum class LineEnding {
    LF,      // Unix/Linux/macOS (\n)
    CRLF,    // Windows (\r\n)
    CR,      // Classic Mac (\r) - rare
    Auto     // Detect from file or use platform default
};

// File permissions (cross-platform subset)
enum class FilePermission : uint32_t {
    None = 0,
    OwnerRead = 0x0100,
    OwnerWrite = 0x0080,
    OwnerExecute = 0x0040,
    GroupRead = 0x0020,
    GroupWrite = 0x0010,
    GroupExecute = 0x0008,
    OthersRead = 0x0004,
    OthersWrite = 0x0002,
    OthersExecute = 0x0001,
    
    // Common combinations
    OwnerAll = OwnerRead | OwnerWrite | OwnerExecute,
    GroupAll = GroupRead | GroupWrite | GroupExecute,
    OthersAll = OthersRead | OthersWrite | OthersExecute,
    AllRead = OwnerRead | GroupRead | OthersRead,
    AllWrite = OwnerWrite | GroupWrite | OthersWrite,
    AllExecute = OwnerExecute | GroupExecute | OthersExecute
};

inline FilePermission operator|(FilePermission a, FilePermission b) {
    return static_cast<FilePermission>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline FilePermission operator&(FilePermission a, FilePermission b) {
    return static_cast<FilePermission>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

// Platform-agnostic file operations
class PlatformFile {
public:
    // Path manipulation
    static std::string normalize_path(const std::string& path);
    static std::string join_path(const std::string& base, const std::string& rel);
    static std::string get_directory(const std::string& path);
    static std::string get_filename(const std::string& path);
    static std::string get_extension(const std::string& path);
    static std::string get_stem(const std::string& path);
    static char get_path_separator();
    
    // File I/O with line ending conversion
    static bool read_file(const std::string& path, std::string& content, LineEnding output_ending = LineEnding::LF);
    static bool read_file_binary(const std::string& path, std::vector<uint8_t>& data);
    static bool write_file(const std::string& path, const std::string& content, LineEnding line_ending = LineEnding::Auto);
    static bool write_file_binary(const std::string& path, const std::vector<uint8_t>& data);
    
    // File metadata
    static bool exists(const std::string& path);
    static bool is_file(const std::string& path);
    static bool is_directory(const std::string& path);
    static uint64_t get_file_size(const std::string& path);
    static bool get_modified_time(const std::string& path, uint64_t& timestamp);
    
    // File operations
    static bool create_file(const std::string& path);
    static bool delete_file(const std::string& path);
    static bool copy_file(const std::string& from, const std::string& to, bool overwrite = false);
    static bool move_file(const std::string& from, const std::string& to);
    static bool rename_file(const std::string& from, const std::string& to);
    
    // Directory operations
    static bool create_directory(const std::string& path);
    static bool create_directories(const std::string& path); // Recursive
    static bool delete_directory(const std::string& path, bool recursive = false);
    static bool list_directory(const std::string& path, std::vector<std::string>& entries);
    
    // Permissions
    static bool get_permissions(const std::string& path, FilePermission& permissions);
    static bool set_permissions(const std::string& path, FilePermission permissions);
    static bool is_readable(const std::string& path);
    static bool is_writable(const std::string& path);
    static bool is_executable(const std::string& path);
    
    // Line ending detection and conversion
    static LineEnding detect_line_ending(const std::string& content);
    static LineEnding get_platform_line_ending();
    static std::string convert_line_endings(const std::string& content, LineEnding from, LineEnding to);
    
    // Temporary files
    static std::string get_temp_directory();
    static std::string create_temp_file(const std::string& prefix = "");
    
    // Current directory
    static std::string get_current_directory();
    static bool set_current_directory(const std::string& path);
    
    // Home directory
    static std::string get_home_directory();
    
    // Platform-specific path conversion (for display)
    static std::string to_native_path(const std::string& path);
    static std::string from_native_path(const std::string& path);

private:
    static std::string normalize_line_endings_internal(const std::string& content, const std::string& ending);
};

} // namespace editor

#endif // PLATFORM_FILE_H
