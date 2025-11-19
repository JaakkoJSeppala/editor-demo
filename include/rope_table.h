#ifndef ROPE_TABLE_H
#define ROPE_TABLE_H

#include <memory>
#include <string>
#include <vector>

/**
 * RopeNode - solmu Rope-puussa
 */
class RopeNode {
public:
    std::shared_ptr<RopeNode> left;
    std::shared_ptr<RopeNode> right;
    std::string data;
    size_t weight; // vasemman alipuun pituus

    RopeNode(const std::string& str)
        : left(nullptr), right(nullptr), data(str), weight(str.length()) {}
    RopeNode(std::shared_ptr<RopeNode> l, std::shared_ptr<RopeNode> r)
        : left(l), right(r), data(""), weight(l ? l->length() : 0) {}

    size_t length() const {
        if (!left && !right) return data.length();
        size_t l = left ? left->length() : 0;
        size_t r = right ? right->length() : 0;
        return l + r;
    }
};

/**
 * RopeTable - puupohjainen tekstirakenne
 */
class RopeTable {
public:
    RopeTable();
    explicit RopeTable(const std::string& initial_text);

    void insert(size_t position, const std::string& text);
    void remove(size_t position, size_t length);
    std::string get_text(size_t start, size_t length) const;
    size_t get_total_length() const;

private:
    std::shared_ptr<RopeNode> root_;
    // apumetodit
    std::shared_ptr<RopeNode> concat(std::shared_ptr<RopeNode> left, std::shared_ptr<RopeNode> right);
    std::pair<std::shared_ptr<RopeNode>, std::shared_ptr<RopeNode>> split(std::shared_ptr<RopeNode> node, size_t pos);
};

#endif // ROPE_TABLE_H
