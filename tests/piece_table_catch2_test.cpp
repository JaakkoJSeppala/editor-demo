#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include "../include/piece_table.h"

TEST_CASE("PieceTable basic insert and get_text", "[piece_table][basic]") {
    PieceTable pt;
    pt.insert(0, "Hello");
    pt.insert(5, " World");
    REQUIRE(pt.get_text(0, pt.length()) == "Hello World");
}

TEST_CASE("PieceTable edge cases", "[piece_table][edge]") {
    PieceTable pt;
    pt.insert(0, "");
    REQUIRE(pt.length() == 0);
    pt.insert(0, "A");
    pt.delete_range(0, 1);
    REQUIRE(pt.length() == 0);
}

TEST_CASE("PieceTable large insert", "[piece_table][performance]") {
    PieceTable pt;
    std::string big(100000, 'x');
    pt.insert(0, big);
    REQUIRE(pt.length() == 100000);
}
