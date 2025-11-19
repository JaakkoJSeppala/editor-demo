
#include "rope_table.h"
#include <algorithm>
#include <functional>

RopeTable::RopeTable() : root_(nullptr) {}

RopeTable::RopeTable(const std::string& initial_text) {
    root_ = std::make_shared<RopeNode>(initial_text);
}

size_t RopeTable::get_total_length() const {
    return root_ ? root_->length() : 0;
}

std::string RopeTable::get_text(size_t start, size_t length) const {
    std::string result;
    result.reserve(length);
    std::function<void(std::shared_ptr<RopeNode>, size_t, size_t)> collect = [&](std::shared_ptr<RopeNode> node, size_t s, size_t l) {
        if (!node || l == 0) return;
        if (!node->left && !node->right) {
            result.append(node->data.substr(s, l));
            return;
        }
        size_t left_len = node->left ? node->left->length() : 0;
        if (s < left_len) {
            size_t take = std::min(left_len - s, l);
            collect(node->left, s, take);
            if (l > take) collect(node->right, 0, l - take);
        } else {
            collect(node->right, s - left_len, l);
        }
    };
    collect(root_, start, length);
    return result;
}

std::shared_ptr<RopeNode> RopeTable::concat(std::shared_ptr<RopeNode> left, std::shared_ptr<RopeNode> right) {
    return std::make_shared<RopeNode>(left, right);
}

std::pair<std::shared_ptr<RopeNode>, std::shared_ptr<RopeNode>> RopeTable::split(std::shared_ptr<RopeNode> node, size_t pos) {
    if (!node) return {nullptr, nullptr};
    if (!node->left && !node->right) {
        if (pos >= node->data.length()) return {node, nullptr};
        return {
            std::make_shared<RopeNode>(node->data.substr(0, pos)),
            std::make_shared<RopeNode>(node->data.substr(pos))
        };
    }
    size_t left_len = node->left ? node->left->length() : 0;
    if (pos < left_len) {
        auto [l, r] = split(node->left, pos);
        return {l, concat(r, node->right)};
    } else {
        auto [l, r] = split(node->right, pos - left_len);
        return {concat(node->left, l), r};
    }
}

void RopeTable::insert(size_t position, const std::string& text) {
    auto [left, right] = split(root_, position);
    auto middle = std::make_shared<RopeNode>(text);
    root_ = concat(concat(left, middle), right);
}

void RopeTable::remove(size_t position, size_t length) {
    auto [left, mid] = split(root_, position);
    auto [_, right] = split(mid, length);
    root_ = concat(left, right);
}
