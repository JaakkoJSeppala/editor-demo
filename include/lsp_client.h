#ifndef LSP_CLIENT_H
#define LSP_CLIENT_H

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <windows.h>
#include "../external/json/json.hpp"

/**
 * LSP Client - Language Server Protocol client implementation
 * Handles JSON-RPC 2.0 communication with language servers
 */
class LSPClient {
public:
    struct Position {
        int line;      // 0-based
        int character; // 0-based (UTF-16 code unit)
    };

    struct Range {
        Position start;
        Position end;
    };

    struct Location {
        std::string uri;
        Range range;
    };

    struct Diagnostic {
        Range range;
        int severity; // 1=Error, 2=Warning, 3=Info, 4=Hint
        std::string message;
        std::string source;
    };

    struct CompletionItem {
        std::string label;
        int kind; // 1=Text, 2=Method, 3=Function, 6=Variable, etc.
        std::string detail;
        std::string documentation;
        std::string insertText;
    };

    struct Hover {
        std::string contents;
        Range range;
    };

    using DiagnosticsCallback = std::function<void(const std::string& uri, const std::vector<Diagnostic>&)>;
    using CompletionCallback = std::function<void(const std::vector<CompletionItem>&)>;
    using HoverCallback = std::function<void(const Hover&)>;
    using LocationCallback = std::function<void(const std::vector<Location>&)>;

    LSPClient();
    ~LSPClient();

    // Server lifecycle
    bool start_server(const std::string& server_command, const std::string& workspace_root);
    bool initialize(const std::string& workspace_root);
    void shutdown();
    bool is_running() const;

    // Document synchronization
    void did_open(const std::string& uri, const std::string& language_id, const std::string& text);
    void did_change(const std::string& uri, const std::string& text);
    void did_save(const std::string& uri);
    void did_close(const std::string& uri);

    // Language features
    void request_completion(const std::string& uri, int line, int character, CompletionCallback callback);
    void request_hover(const std::string& uri, int line, int character, HoverCallback callback);
    void request_definition(const std::string& uri, int line, int character, LocationCallback callback);
    void request_references(const std::string& uri, int line, int character, LocationCallback callback);

    // Callbacks
    void set_diagnostics_callback(DiagnosticsCallback callback);

    // Process incoming messages (call periodically)
    void process_messages();

public:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    void send_request(const std::string& method, const nlohmann::json& params, int request_id);
    void send_notification(const std::string& method, const nlohmann::json& params);
    std::string read_message();
    void write_message(const std::string& message);
    void handle_message(const std::string& message);
    void handle_response(int id, const nlohmann::json& result);
    void handle_notification(const std::string& method, const nlohmann::json& params);
};

#endif // LSP_CLIENT_H
