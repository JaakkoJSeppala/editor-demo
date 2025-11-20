#include "refactor_api.h"
#include "lsp_client.h"
#include <iostream>
#include <cassert>

int main() {
    LSPClient lsp;
    RefactorAPI refactor(&lsp);
    std::string test_uri = "file:///test_file.cpp";
    int test_line = 10, test_char = 5;
    bool rename_called = false, cleanup_called = false;
    refactor.rename_symbol(test_uri, test_line, test_char, "newSymbolName", [&](RefactorResult result) {
        assert(result.success);
        rename_called = true;
    });
    refactor.code_cleanup(test_uri, [&](RefactorResult result) {
        assert(result.success);
        cleanup_called = true;
    });
    assert(rename_called && cleanup_called);
    std::cout << "RefactorAPI tests passed.\n";
    return 0;
}
