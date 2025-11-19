#include "treesitter_bridge.h"
#include "syntax_highlighter.h" // for Token

#ifdef ENABLE_TREESITTER
#include <tree_sitter/api.h>

extern "C" {
    // Language parsers
    const TSLanguage *tree_sitter_c();
    const TSLanguage *tree_sitter_cpp();
    const TSLanguage *tree_sitter_python();
    const TSLanguage *tree_sitter_javascript();
    const TSLanguage *tree_sitter_typescript();
    const TSLanguage *tree_sitter_json();
    const TSLanguage *tree_sitter_yaml();
    const TSLanguage *tree_sitter_markdown();
}
#endif

struct TreeSitterBridge::Impl {
    bool available = false;
    std::string language;
    std::vector<std::string> lines;
#ifdef ENABLE_TREESITTER
    TSParser* parser = nullptr;
    TSTree* tree = nullptr;
    std::string joined_text;
    std::vector<size_t> line_offsets; // byte offset at start of each line
#endif
};

TreeSitterBridge::TreeSitterBridge() : impl_(new Impl) {}
TreeSitterBridge::~TreeSitterBridge() = default;

bool TreeSitterBridge::initialize(const std::string& lang_id) {
#ifdef ENABLE_TREESITTER
    impl_->language = lang_id;
    if (impl_->parser) { ts_parser_delete(impl_->parser); impl_->parser = nullptr; }
    if (impl_->tree) { ts_tree_delete(impl_->tree); impl_->tree = nullptr; }

    impl_->parser = ts_parser_new();
    if (!impl_->parser) { impl_->available = false; return false; }

    const TSLanguage* lang = nullptr;
    if (lang_id == "c") lang = tree_sitter_c();
    else if (lang_id == "cpp") lang = tree_sitter_cpp();
    else if (lang_id == "python") lang = tree_sitter_python();
    else if (lang_id == "javascript") lang = tree_sitter_javascript();
    else if (lang_id == "typescript") lang = tree_sitter_typescript();
    else if (lang_id == "json") lang = tree_sitter_json();
    else if (lang_id == "yaml") lang = tree_sitter_yaml();
    else if (lang_id == "markdown") lang = tree_sitter_markdown();

    if (!lang) {
        impl_->available = false;
        ts_parser_delete(impl_->parser); impl_->parser = nullptr;
        return false;
    }

    if (!ts_parser_set_language(impl_->parser, lang)) {
        impl_->available = false;
        ts_parser_delete(impl_->parser); impl_->parser = nullptr;
        return false;
    }
    impl_->available = true;
    return true;
#else
    (void)lang_id;
    impl_->available = false;
    return false;
#endif
}

void TreeSitterBridge::set_document_text(const std::vector<std::string>& lines) {
    impl_->lines = lines;
#ifdef ENABLE_TREESITTER
    if (!impl_->available || !impl_->parser) return;
    impl_->joined_text.clear();
    impl_->line_offsets.clear();
    impl_->line_offsets.reserve(lines.size() + 1);
    size_t offset = 0;
    for (size_t i = 0; i < lines.size(); ++i) {
        impl_->line_offsets.push_back(offset);
        impl_->joined_text.append(lines[i]);
        impl_->joined_text.push_back('\n');
        offset += lines[i].size() + 1; // assume 1 byte per char; fine for ASCII
    }
    impl_->line_offsets.push_back(offset);

    if (impl_->tree) { ts_tree_delete(impl_->tree); impl_->tree = nullptr; }
    impl_->tree = ts_parser_parse_string(impl_->parser, nullptr, impl_->joined_text.c_str(), (uint32_t)impl_->joined_text.size());
#endif
}

static bool node_overlaps_range(const TSNode& n, uint32_t start_byte, uint32_t end_byte) {
#ifdef ENABLE_TREESITTER
    uint32_t ns = ts_node_start_byte(n);
    uint32_t ne = ts_node_end_byte(n);
    return !(ne <= start_byte || ns >= end_byte);
#else
    (void)n; (void)start_byte; (void)end_byte; return false;
#endif
}

bool TreeSitterBridge::get_line_tokens(size_t line_index, std::vector<Token>& out_tokens) const {
    out_tokens.clear();
    if (!impl_->available) return false;
#ifdef ENABLE_TREESITTER
    if (!impl_->tree || line_index >= impl_->lines.size()) return false;
    uint32_t start_b = (uint32_t)impl_->line_offsets[line_index];
    uint32_t end_b = (uint32_t)impl_->line_offsets[line_index + 1];

    TSNode root = ts_tree_root_node(impl_->tree);

    // Simple DFS collecting comment/string/preproc-like nodes overlapping the line
    std::vector<TSNode> stack; stack.push_back(root);
    while (!stack.empty()) {
        TSNode n = stack.back(); stack.pop_back();
        if (!node_overlaps_range(n, start_b, end_b)) continue;
        const char* t = ts_node_type(n);
        if (!t) continue;

        Token::Type tokType = Token::NORMAL;
        std::string typeStr(t);
        
        // Map Tree-sitter node types to our Token types
        if (typeStr.find("comment") != std::string::npos) {
            tokType = Token::COMMENT;
        }
        else if (typeStr.find("string") != std::string::npos || 
                 typeStr.find("template_string") != std::string::npos ||
                 typeStr.find("char_literal") != std::string::npos) {
            tokType = Token::STRING;
        }
        else if (typeStr.rfind("preproc", 0) == 0 || 
                 typeStr.find("preproc") != std::string::npos ||
                 typeStr == "import_statement" ||
                 typeStr == "package_declaration") {
            tokType = Token::PREPROCESSOR;
        }
        else if (typeStr.find("number") != std::string::npos || 
                 typeStr.find("float") != std::string::npos || 
                 typeStr.find("integer") != std::string::npos ||
                 typeStr == "true" || typeStr == "false" || typeStr == "null" ||
                 typeStr == "None" || typeStr == "True" || typeStr == "False") {
            tokType = Token::NUMBER;
        }
        // Keywords and language constructs
        else if (typeStr == "if" || typeStr == "else" || typeStr == "while" || typeStr == "for" ||
                 typeStr == "return" || typeStr == "class" || typeStr == "function" || typeStr == "def" ||
                 typeStr == "import" || typeStr == "from" || typeStr == "const" || typeStr == "let" ||
                 typeStr == "var" || typeStr == "async" || typeStr == "await" || typeStr == "yield" ||
                 typeStr == "lambda" || typeStr == "try" || typeStr == "except" || typeStr == "finally" ||
                 typeStr == "switch" || typeStr == "case" || typeStr == "break" || typeStr == "continue" ||
                 typeStr == "public" || typeStr == "private" || typeStr == "protected" || typeStr == "static" ||
                 typeStr == "virtual" || typeStr == "override" || typeStr == "namespace" || typeStr == "using" ||
                 typeStr == "template" || typeStr == "typename" || typeStr == "struct" || typeStr == "enum" ||
                 typeStr == "interface" || typeStr == "type" || typeStr == "fn" || typeStr == "impl" ||
                 typeStr == "trait" || typeStr == "mod" || typeStr == "pub" || typeStr == "use") {
            tokType = Token::KEYWORD;
        }
        // Primitive types
        else if (typeStr == "int" || typeStr == "float" || typeStr == "double" || typeStr == "char" ||
                 typeStr == "bool" || typeStr == "void" || typeStr == "string" || typeStr == "auto" ||
                 typeStr == "long" || typeStr == "short" || typeStr == "unsigned" || typeStr == "signed" ||
                 typeStr.find("_type") != std::string::npos ||
                 typeStr == "primitive_type" || typeStr == "type_identifier") {
            tokType = Token::KEYWORD;
        }

        if (tokType != Token::NORMAL) {
            uint32_t ns = ts_node_start_byte(n);
            uint32_t ne = ts_node_end_byte(n);
            uint32_t s = (ns > start_b) ? ns : start_b;
            uint32_t e = (ne < end_b) ? ne : end_b;
            if (e > s) {
                size_t rel_start = s - start_b;
                size_t len = e - s;
                out_tokens.push_back({tokType, rel_start, len});
            }
            // For leaf token types, no need to traverse children
            continue;
        }

        // Push children to inspect deeper
        uint32_t child_count = ts_node_child_count(n);
        for (int32_t i = (int32_t)child_count - 1; i >= 0; --i) {
            TSNode c = ts_node_child(n, (uint32_t)i);
            stack.push_back(c);
        }
    }

    // If we found none, return false to allow legacy tokenizer
    return !out_tokens.empty();
#else
    (void)line_index; (void)out_tokens; return false;
#endif
}

bool TreeSitterBridge::is_available() const { return impl_->available; }
