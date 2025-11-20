#include <iostream>
#include <string>
#include "../include/piece_table.h"
#include "../include/rope_table.h"

int main() {
    std::cout << "Editor-demo kevyt testausohjelma\n";
    std::cout << "===============================\n";

    // PieceTable kevyt testi
    {
        PieceTable pt;
        pt.insert(0, "Test");
        std::string result = pt.get_text(0, 4);
        std::cout << "PieceTable: " << result << "\n";
        if (result != "Test") std::cout << "[FAIL] PieceTable basic\n";
    }

    // RopeTable kevyt testi
    {
        RopeTable rope;
        rope.insert(0, "xy");
        rope.insert(2, "z");
        std::string result = rope.get_text(0, rope.get_total_length());
        std::cout << "RopeTable: " << result << "\n";
        if (result != "xyz") std::cout << "[FAIL] RopeTable basic\n";
    }

    std::cout << "Kevyet testit suoritettu.\n";
    return 0;
}
