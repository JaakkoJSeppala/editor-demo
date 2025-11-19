#include "refactor_api.h"
#include "lsp_client.h"
#include <iostream>

RefactorAPI::RefactorAPI(LSPClient* lsp) : lsp_(lsp) {}

void RefactorAPI::rename_symbol(const std::string& uri, int line, int character, const std::string& new_name, std::function<void(RefactorResult)> callback) {
    // LSP: textDocument/rename
    nlohmann::json params = {
        {"textDocument", { {"uri", uri} }},
        {"position", { {"line", line}, {"character", character} }},
        {"newName", new_name}
    };
    int req_id = 1; // For demo, should be unique per request
    lsp_->send_request("textDocument/rename", params, req_id);
    // For demo, simulate result
    RefactorResult result{true, "Symbol renamed", {uri}};
    callback(result);
}

void RefactorAPI::move_symbol(const std::string& uri, int line, int character, const std::string& target_uri, std::function<void(RefactorResult)> callback) {
    // Not standard in LSP, would require custom implementation
    RefactorResult result{false, "Move not implemented in LSP", {}};
    callback(result);
}

void RefactorAPI::code_cleanup(const std::string& uri, std::function<void(RefactorResult)> callback) {
    // LSP: textDocument/codeAction (organize imports, format, etc.)
    nlohmann::json params = {
        {"textDocument", { {"uri", uri} }},
        {"range", { {"start", { {"line", 0}, {"character", 0} }}, {"end", { {"line", 0}, {"character", 0} }} }}
    };
    int req_id = 2; // For demo
    lsp_->send_request("textDocument/codeAction", params, req_id);
    RefactorResult result{true, "Code cleanup requested", {uri}};
    callback(result);
}
