#include "lsp_client.h"
#include "../external/json/json.hpp"
#include <sstream>
#include <iostream>
#include <atomic>

using json = nlohmann::json;

struct LSPClient::Impl {
    HANDLE child_stdin_write = nullptr;
    HANDLE child_stdout_read = nullptr;
    PROCESS_INFORMATION pi{};
    std::atomic<int> next_request_id{1};
    std::unordered_map<int, std::function<void(const json&)>> pending_requests;
    DiagnosticsCallback diagnostics_callback;
    bool initialized = false;
    bool running = false;
};

LSPClient::LSPClient() : impl_(new Impl) {}

LSPClient::~LSPClient() {
    shutdown();
}

bool LSPClient::start_server(const std::string& server_command, const std::string& workspace_root) {
    (void)workspace_root; // Will use in initialize

    // Create pipes for stdin/stdout
    HANDLE child_stdin_read = nullptr;
    HANDLE child_stdout_write = nullptr;
    
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    if (!CreatePipe(&child_stdin_read, &impl_->child_stdin_write, &sa, 0))
        return false;
    if (!SetHandleInformation(impl_->child_stdin_write, HANDLE_FLAG_INHERIT, 0))
        return false;

    if (!CreatePipe(&impl_->child_stdout_read, &child_stdout_write, &sa, 0))
        return false;
    if (!SetHandleInformation(impl_->child_stdout_read, HANDLE_FLAG_INHERIT, 0))
        return false;

    // Start the server process
    STARTUPINFOA si{};
    si.cb = sizeof(STARTUPINFOA);
    si.hStdInput = child_stdin_read;
    si.hStdOutput = child_stdout_write;
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    si.dwFlags |= STARTF_USESTDHANDLES;

    std::string cmd = server_command;
    if (!CreateProcessA(nullptr, &cmd[0], nullptr, nullptr, TRUE, 
                        CREATE_NO_WINDOW, nullptr, nullptr, &si, &impl_->pi)) {
        CloseHandle(child_stdin_read);
        CloseHandle(child_stdout_write);
        return false;
    }

    CloseHandle(child_stdin_read);
    CloseHandle(child_stdout_write);
    
    impl_->running = true;
    return true;
}

bool LSPClient::initialize(const std::string& workspace_root) {
    if (!impl_->running) return false;

    json params = {
        {"processId", GetCurrentProcessId()},
        {"clientInfo", {{"name", "VelocityEditor"}, {"version", "0.4.0"}}},
        {"rootUri", "file:///" + workspace_root},
        {"capabilities", {
            {"textDocument", {
                {"completion", {{"completionItem", {{"snippetSupport", false}}}}},
                {"hover", {{"contentFormat", {"plaintext"}}}},
                {"definition", {{"linkSupport", false}}},
                {"references", {{"dynamicRegistration", false}}},
                {"publishDiagnostics", {{"relatedInformation", false}}}
            }}
        }}
    };

    int req_id = impl_->next_request_id++;
    impl_->pending_requests[req_id] = [this](const json& result) {
        (void)result; // Server capabilities in result.capabilities
        impl_->initialized = true;
        // Send initialized notification
        send_notification("initialized", json::object());
    };

    send_request("initialize", params, req_id);
    
    // Process messages until initialized
    for (int i = 0; i < 100 && !impl_->initialized; ++i) {
        process_messages();
        Sleep(10);
    }

    return impl_->initialized;
}

void LSPClient::shutdown() {
    if (!impl_->running) return;

    if (impl_->initialized) {
        int req_id = impl_->next_request_id++;
        send_request("shutdown", json::object(), req_id);
        send_notification("exit", json::object());
    }

    if (impl_->child_stdin_write) CloseHandle(impl_->child_stdin_write);
    if (impl_->child_stdout_read) CloseHandle(impl_->child_stdout_read);
    if (impl_->pi.hProcess) {
        TerminateProcess(impl_->pi.hProcess, 0);
        CloseHandle(impl_->pi.hProcess);
        CloseHandle(impl_->pi.hThread);
    }

    impl_->running = false;
    impl_->initialized = false;
}

bool LSPClient::is_running() const {
    return impl_->running && impl_->initialized;
}

void LSPClient::did_open(const std::string& uri, const std::string& language_id, const std::string& text) {
    if (!is_running()) return;

    json params = {
        {"textDocument", {
            {"uri", uri},
            {"languageId", language_id},
            {"version", 1},
            {"text", text}
        }}
    };
    send_notification("textDocument/didOpen", params);
}

void LSPClient::did_change(const std::string& uri, const std::string& text) {
    if (!is_running()) return;

    json params = {
        {"textDocument", {{"uri", uri}, {"version", 2}}},
        {"contentChanges", {{{"text", text}}}}
    };
    send_notification("textDocument/didChange", params);
}

void LSPClient::did_save(const std::string& uri) {
    if (!is_running()) return;

    json params = {{"textDocument", {{"uri", uri}}}};
    send_notification("textDocument/didSave", params);
}

void LSPClient::did_close(const std::string& uri) {
    if (!is_running()) return;

    json params = {{"textDocument", {{"uri", uri}}}};
    send_notification("textDocument/didClose", params);
}

void LSPClient::request_completion(const std::string& uri, int line, int character, CompletionCallback callback) {
    if (!is_running()) return;

    json params = {
        {"textDocument", {{"uri", uri}}},
        {"position", {{"line", line}, {"character", character}}}
    };

    int req_id = impl_->next_request_id++;
    impl_->pending_requests[req_id] = [callback](const json& result) {
        std::vector<CompletionItem> items;
        json list = result.is_array() ? result : result.value("items", json::array());
        for (const auto& item : list) {
            CompletionItem ci;
            ci.label = item.value("label", "");
            ci.kind = item.value("kind", 1);
            ci.detail = item.value("detail", "");
            ci.insertText = item.value("insertText", ci.label);
            items.push_back(ci);
        }
        callback(items);
    };

    send_request("textDocument/completion", params, req_id);
}

void LSPClient::request_hover(const std::string& uri, int line, int character, HoverCallback callback) {
    if (!is_running()) return;

    json params = {
        {"textDocument", {{"uri", uri}}},
        {"position", {{"line", line}, {"character", character}}}
    };

    int req_id = impl_->next_request_id++;
    impl_->pending_requests[req_id] = [callback](const json& result) {
        Hover hover;
        if (result.contains("contents")) {
            auto contents = result["contents"];
            if (contents.is_string()) {
                hover.contents = contents.get<std::string>();
            } else if (contents.is_object() && contents.contains("value")) {
                hover.contents = contents["value"].get<std::string>();
            }
        }
        callback(hover);
    };

    send_request("textDocument/hover", params, req_id);
}

void LSPClient::request_definition(const std::string& uri, int line, int character, LocationCallback callback) {
    if (!is_running()) return;

    json params = {
        {"textDocument", {{"uri", uri}}},
        {"position", {{"line", line}, {"character", character}}}
    };

    int req_id = impl_->next_request_id++;
    impl_->pending_requests[req_id] = [callback](const json& result) {
        std::vector<Location> locations;
        json locs = result.is_array() ? result : json::array({result});
        for (const auto& loc : locs) {
            if (loc.is_null()) continue;
            Location l;
            l.uri = loc.value("uri", "");
            if (loc.contains("range")) {
                auto r = loc["range"];
                l.range.start.line = r["start"]["line"];
                l.range.start.character = r["start"]["character"];
                l.range.end.line = r["end"]["line"];
                l.range.end.character = r["end"]["character"];
            }
            locations.push_back(l);
        }
        callback(locations);
    };

    send_request("textDocument/definition", params, req_id);
}

void LSPClient::request_references(const std::string& uri, int line, int character, LocationCallback callback) {
    if (!is_running()) return;

    json params = {
        {"textDocument", {{"uri", uri}}},
        {"position", {{"line", line}, {"character", character}}},
        {"context", {{"includeDeclaration", true}}}
    };

    int req_id = impl_->next_request_id++;
    impl_->pending_requests[req_id] = [callback](const json& result) {
        std::vector<Location> locations;
        if (result.is_array()) {
            for (const auto& loc : result) {
                Location l;
                l.uri = loc.value("uri", "");
                if (loc.contains("range")) {
                    auto r = loc["range"];
                    l.range.start.line = r["start"]["line"];
                    l.range.start.character = r["start"]["character"];
                    l.range.end.line = r["end"]["line"];
                    l.range.end.character = r["end"]["character"];
                }
                locations.push_back(l);
            }
        }
        callback(locations);
    };

    send_request("textDocument/references", params, req_id);
}

void LSPClient::set_diagnostics_callback(DiagnosticsCallback callback) {
    impl_->diagnostics_callback = callback;
}

void LSPClient::process_messages() {
    if (!impl_->running) return;

    // Check if data is available (non-blocking)
    DWORD bytes_avail = 0;
    if (!PeekNamedPipe(impl_->child_stdout_read, nullptr, 0, nullptr, &bytes_avail, nullptr))
        return;
    
    if (bytes_avail > 0) {
        std::string message = read_message();
        if (!message.empty()) {
            handle_message(message);
        }
    }
}

void LSPClient::send_request(const std::string& method, const json& params, int request_id) {
    json message = {
        {"jsonrpc", "2.0"},
        {"id", request_id},
        {"method", method},
        {"params", params}
    };
    write_message(message.dump());
}

void LSPClient::send_notification(const std::string& method, const json& params) {
    json message = {
        {"jsonrpc", "2.0"},
        {"method", method},
        {"params", params}
    };
    write_message(message.dump());
}

std::string LSPClient::read_message() {
    // Read headers
    std::string headers;
    size_t content_length = 0;
    
    while (true) {
        char ch;
        DWORD read;
        if (!ReadFile(impl_->child_stdout_read, &ch, 1, &read, nullptr) || read == 0)
            return "";
        
        headers += ch;
        if (headers.size() >= 2 && headers.substr(headers.size()-2) == "\r\n") {
            std::string line = headers.substr(0, headers.size()-2);
            headers.clear();
            
            if (line.empty()) break; // End of headers
            
            if (line.find("Content-Length:") == 0) {
                content_length = std::stoul(line.substr(15));
            }
        }
    }

    if (content_length == 0) return "";

    // Read content
    std::string content(content_length, '\0');
    DWORD total_read = 0;
    while (total_read < content_length) {
        DWORD read;
        if (!ReadFile(impl_->child_stdout_read, &content[total_read], 
                     (DWORD)(content_length - total_read), &read, nullptr))
            return "";
        total_read += read;
    }

    return content;
}

void LSPClient::write_message(const std::string& message) {
    std::ostringstream oss;
    oss << "Content-Length: " << message.size() << "\r\n\r\n" << message;
    std::string full_message = oss.str();
    
    DWORD written;
    WriteFile(impl_->child_stdin_write, full_message.c_str(), 
              (DWORD)full_message.size(), &written, nullptr);
}

void LSPClient::handle_message(const std::string& message) {
    try {
        json j = json::parse(message);
        
        if (j.contains("id")) {
            // Response
            int id = j["id"];
            if (impl_->pending_requests.count(id)) {
                if (j.contains("result")) {
                    impl_->pending_requests[id](j["result"]);
                }
                impl_->pending_requests.erase(id);
            }
        } else if (j.contains("method")) {
            // Notification
            std::string method = j["method"];
            json params = j.value("params", json::object());
            handle_notification(method, params);
        }
    } catch (...) {
        // Ignore parse errors
    }
}

void LSPClient::handle_response(int id, const json& result) {
    if (impl_->pending_requests.count(id)) {
        impl_->pending_requests[id](result);
        impl_->pending_requests.erase(id);
    }
}

void LSPClient::handle_notification(const std::string& method, const json& params) {
    if (method == "textDocument/publishDiagnostics") {
        std::string uri = params.value("uri", "");
        std::vector<Diagnostic> diagnostics;
        
        for (const auto& diag : params.value("diagnostics", json::array())) {
            Diagnostic d;
            if (diag.contains("range")) {
                auto r = diag["range"];
                d.range.start.line = r["start"]["line"];
                d.range.start.character = r["start"]["character"];
                d.range.end.line = r["end"]["line"];
                d.range.end.character = r["end"]["character"];
            }
            d.severity = diag.value("severity", 1);
            d.message = diag.value("message", "");
            d.source = diag.value("source", "");
            diagnostics.push_back(d);
        }
        
        if (impl_->diagnostics_callback) {
            impl_->diagnostics_callback(uri, diagnostics);
        }
    }
}
