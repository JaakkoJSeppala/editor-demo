#include "platform_file.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#include <sys/stat.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <dirent.h>
#endif

namespace editor {

// Path manipulation
std::string PlatformFile::normalize_path(const std::string& path) {
    std::filesystem::path p(path);
    return p.lexically_normal().string();
}

std::string PlatformFile::join_path(const std::string& base, const std::string& rel) {
    std::filesystem::path result = std::filesystem::path(base) / rel;
    return result.string();
}

std::string PlatformFile::get_directory(const std::string& path) {
    std::filesystem::path p(path);
    return p.parent_path().string();
}

std::string PlatformFile::get_filename(const std::string& path) {
    std::filesystem::path p(path);
    return p.filename().string();
}

std::string PlatformFile::get_extension(const std::string& path) {
    std::filesystem::path p(path);
    return p.extension().string();
}

std::string PlatformFile::get_stem(const std::string& path) {
    std::filesystem::path p(path);
    return p.stem().string();
}

char PlatformFile::get_path_separator() {
#ifdef _WIN32
    return '\\';
#else
    return '/';
#endif
}

// File I/O with line ending conversion
bool PlatformFile::read_file(const std::string& path, std::string& content, LineEnding output_ending) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    content = buffer.str();
    
    if (output_ending != LineEnding::Auto) {
        LineEnding detected = detect_line_ending(content);
        if (detected != output_ending) {
            content = convert_line_endings(content, detected, output_ending);
        }
    }
    
    return true;
}

bool PlatformFile::read_file_binary(const std::string& path, std::vector<uint8_t>& data) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return false;
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    data.resize(size);
    return file.read(reinterpret_cast<char*>(data.data()), size).good();
}

bool PlatformFile::write_file(const std::string& path, const std::string& content, LineEnding line_ending) {
    std::string output = content;
    
    if (line_ending == LineEnding::Auto) {
        line_ending = get_platform_line_ending();
    }
    
    LineEnding current = detect_line_ending(content);
    if (current != line_ending) {
        output = convert_line_endings(content, current, line_ending);
    }
    
    std::ofstream file(path, std::ios::binary);
    if (!file) return false;
    
    file << output;
    return file.good();
}

bool PlatformFile::write_file_binary(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream file(path, std::ios::binary);
    if (!file) return false;
    
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    return file.good();
}

// File metadata
bool PlatformFile::exists(const std::string& path) {
    return std::filesystem::exists(path);
}

bool PlatformFile::is_file(const std::string& path) {
    return std::filesystem::is_regular_file(path);
}

bool PlatformFile::is_directory(const std::string& path) {
    return std::filesystem::is_directory(path);
}

uint64_t PlatformFile::get_file_size(const std::string& path) {
    try {
        return std::filesystem::file_size(path);
    } catch (...) {
        return 0;
    }
}

bool PlatformFile::get_modified_time(const std::string& path, uint64_t& timestamp) {
    try {
        auto ftime = std::filesystem::last_write_time(path);
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()
        );
        timestamp = std::chrono::system_clock::to_time_t(sctp);
        return true;
    } catch (...) {
        return false;
    }
}

// File operations
bool PlatformFile::create_file(const std::string& path) {
    std::ofstream file(path);
    return file.good();
}

bool PlatformFile::delete_file(const std::string& path) {
    try {
        return std::filesystem::remove(path);
    } catch (...) {
        return false;
    }
}

bool PlatformFile::copy_file(const std::string& from, const std::string& to, bool overwrite) {
    try {
        auto options = overwrite ? std::filesystem::copy_options::overwrite_existing 
                                 : std::filesystem::copy_options::none;
        std::filesystem::copy_file(from, to, options);
        return true;
    } catch (...) {
        return false;
    }
}

bool PlatformFile::move_file(const std::string& from, const std::string& to) {
    try {
        std::filesystem::rename(from, to);
        return true;
    } catch (...) {
        return false;
    }
}

bool PlatformFile::rename_file(const std::string& from, const std::string& to) {
    return move_file(from, to);
}

// Directory operations
bool PlatformFile::create_directory(const std::string& path) {
    try {
        return std::filesystem::create_directory(path);
    } catch (...) {
        return false;
    }
}

bool PlatformFile::create_directories(const std::string& path) {
    try {
        return std::filesystem::create_directories(path);
    } catch (...) {
        return false;
    }
}

bool PlatformFile::delete_directory(const std::string& path, bool recursive) {
    try {
        if (recursive) {
            return std::filesystem::remove_all(path) > 0;
        } else {
            return std::filesystem::remove(path);
        }
    } catch (...) {
        return false;
    }
}

bool PlatformFile::list_directory(const std::string& path, std::vector<std::string>& entries) {
    try {
        entries.clear();
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            entries.push_back(entry.path().filename().string());
        }
        return true;
    } catch (...) {
        return false;
    }
}

// Permissions
bool PlatformFile::get_permissions(const std::string& path, FilePermission& permissions) {
#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(path.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES) return false;
    
    permissions = FilePermission::OwnerRead | FilePermission::GroupRead | FilePermission::OthersRead;
    if ((attrs & FILE_ATTRIBUTE_READONLY) == 0) {
        permissions = permissions | FilePermission::OwnerWrite | FilePermission::GroupWrite | FilePermission::OthersWrite;
    }
    return true;
#else
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return false;
    
    permissions = static_cast<FilePermission>(st.st_mode & 0777);
    return true;
#endif
}

bool PlatformFile::set_permissions(const std::string& path, FilePermission permissions) {
#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(path.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES) return false;
    
    bool writable = (static_cast<uint32_t>(permissions) & static_cast<uint32_t>(FilePermission::OwnerWrite)) != 0;
    if (writable) {
        attrs &= ~FILE_ATTRIBUTE_READONLY;
    } else {
        attrs |= FILE_ATTRIBUTE_READONLY;
    }
    
    return SetFileAttributesA(path.c_str(), attrs) != 0;
#else
    return chmod(path.c_str(), static_cast<mode_t>(permissions)) == 0;
#endif
}

bool PlatformFile::is_readable(const std::string& path) {
#ifdef _WIN32
    std::wstring wpath(path.begin(), path.end());
    return _waccess(wpath.c_str(), 4) == 0; // R_OK = 4
#else
    return access(path.c_str(), R_OK) == 0;
#endif
}

bool PlatformFile::is_writable(const std::string& path) {
#ifdef _WIN32
    std::wstring wpath(path.begin(), path.end());
    return _waccess(wpath.c_str(), 2) == 0; // W_OK = 2
#else
    return access(path.c_str(), W_OK) == 0;
#endif
}

bool PlatformFile::is_executable(const std::string& path) {
#ifdef _WIN32
    // On Windows, check extension
    std::string ext = get_extension(path);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".exe" || ext == ".bat" || ext == ".cmd" || ext == ".com";
#else
    return access(path.c_str(), X_OK) == 0;
#endif
}

// Line ending detection and conversion
LineEnding PlatformFile::detect_line_ending(const std::string& content) {
    bool has_crlf = content.find("\r\n") != std::string::npos;
    bool has_lf = content.find('\n') != std::string::npos;
    bool has_cr = content.find('\r') != std::string::npos;
    
    if (has_crlf) return LineEnding::CRLF;
    if (has_lf) return LineEnding::LF;
    if (has_cr) return LineEnding::CR;
    
    return get_platform_line_ending();
}

LineEnding PlatformFile::get_platform_line_ending() {
#ifdef _WIN32
    return LineEnding::CRLF;
#else
    return LineEnding::LF;
#endif
}

std::string PlatformFile::convert_line_endings(const std::string& content, LineEnding from, LineEnding to) {
    if (from == to) return content;
    
    // First normalize to LF
    std::string normalized = content;
    if (from == LineEnding::CRLF) {
        size_t pos = 0;
        while ((pos = normalized.find("\r\n", pos)) != std::string::npos) {
            normalized.replace(pos, 2, "\n");
            pos += 1;
        }
    } else if (from == LineEnding::CR) {
        std::replace(normalized.begin(), normalized.end(), '\r', '\n');
    }
    
    // Then convert to target
    if (to == LineEnding::CRLF) {
        size_t pos = 0;
        while ((pos = normalized.find('\n', pos)) != std::string::npos) {
            normalized.replace(pos, 1, "\r\n");
            pos += 2;
        }
    } else if (to == LineEnding::CR) {
        std::replace(normalized.begin(), normalized.end(), '\n', '\r');
    }
    
    return normalized;
}

// Temporary files
std::string PlatformFile::get_temp_directory() {
#ifdef _WIN32
    char temp_path[MAX_PATH];
    if (GetTempPathA(MAX_PATH, temp_path) > 0) {
        return std::string(temp_path);
    }
    return "C:\\Temp";
#else
    const char* tmpdir = getenv("TMPDIR");
    if (tmpdir) return std::string(tmpdir);
    return "/tmp";
#endif
}

std::string PlatformFile::create_temp_file(const std::string& prefix) {
    std::string temp_dir = get_temp_directory();
    std::string base = prefix.empty() ? "temp" : prefix;
    
    for (int i = 0; i < 1000; ++i) {
        std::string path = join_path(temp_dir, base + std::to_string(i) + ".tmp");
        if (!exists(path)) {
            if (create_file(path)) {
                return path;
            }
        }
    }
    
    return "";
}

// Current directory
std::string PlatformFile::get_current_directory() {
    return std::filesystem::current_path().string();
}

bool PlatformFile::set_current_directory(const std::string& path) {
    try {
        std::filesystem::current_path(path);
        return true;
    } catch (...) {
        return false;
    }
}

// Home directory
std::string PlatformFile::get_home_directory() {
#ifdef _WIN32
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, path))) {
        return std::string(path);
    }
    const char* userprofile = getenv("USERPROFILE");
    if (userprofile) return std::string(userprofile);
    return "C:\\";
#else
    const char* home = getenv("HOME");
    if (home) return std::string(home);
    
    struct passwd* pw = getpwuid(getuid());
    if (pw && pw->pw_dir) return std::string(pw->pw_dir);
    
    return "/";
#endif
}

// Platform-specific path conversion
std::string PlatformFile::to_native_path(const std::string& path) {
    std::string result = path;
#ifdef _WIN32
    std::replace(result.begin(), result.end(), '/', '\\');
#else
    std::replace(result.begin(), result.end(), '\\', '/');
#endif
    return result;
}

std::string PlatformFile::from_native_path(const std::string& path) {
    // Convert to forward slashes (cross-platform standard)
    std::string result = path;
    std::replace(result.begin(), result.end(), '\\', '/');
    return result;
}

} // namespace editor
