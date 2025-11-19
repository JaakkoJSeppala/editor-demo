#include "treesitter_bridge.h"
#include "syntax_highlighter.h" // for Token

struct TreeSitterBridge::Impl {
    bool available = false;
    std::string language;
    std::vector<std::string> lines;
};

TreeSitterBridge::TreeSitterBridge() : impl_(new Impl) {}
TreeSitterBridge::~TreeSitterBridge() = default;

bool TreeSitterBridge::initialize(const std::string& lang_id) {
#ifdef ENABLE_TREESITTER
    // TODO: hook real Tree-sitter parser here and set available=true on success
    impl_->language = lang_id;
    impl_->available = false; // until real integration
    return impl_->available;
#else
    (void)lang_id;
    impl_->available = false;
    return false;
#endif
}

void TreeSitterBridge::set_document_text(const std::vector<std::string>& lines) {
    impl_->lines = lines;
}

bool TreeSitterBridge::get_line_tokens(size_t line_index, std::vector<Token>& out_tokens) const {
    (void)out_tokens;
    if (!impl_->available) return false;
#ifdef ENABLE_TREESITTER
    // TODO: read syntax nodes for the line and convert to Token list
    (void)line_index;
    return false;
#else
    (void)line_index;
    return false;
#endif
}

bool TreeSitterBridge::is_available() const { return impl_->available; }
