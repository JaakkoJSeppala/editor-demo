#include "platform_types.h"
#include <sstream>
#include <iomanip>

namespace editor {

Color Color::from_hex(const std::string& hex) {
    if (hex.empty() || hex[0] != '#') {
        return Color();
    }
    
    std::string hex_str = hex.substr(1);
    if (hex_str.length() != 6 && hex_str.length() != 8) {
        return Color();
    }
    
    unsigned int value;
    std::stringstream ss;
    ss << std::hex << hex_str;
    ss >> value;
    
    if (hex_str.length() == 6) {
        // #RRGGBB
        return Color(
            static_cast<uint8_t>((value >> 16) & 0xFF),
            static_cast<uint8_t>((value >> 8) & 0xFF),
            static_cast<uint8_t>(value & 0xFF)
        );
    } else {
        // #RRGGBBAA
        return Color(
            static_cast<uint8_t>((value >> 24) & 0xFF),
            static_cast<uint8_t>((value >> 16) & 0xFF),
            static_cast<uint8_t>((value >> 8) & 0xFF),
            static_cast<uint8_t>(value & 0xFF)
        );
    }
}

std::string Color::to_hex() const {
    std::stringstream ss;
    ss << "#" 
       << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(r)
       << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(g)
       << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(b);
    return ss.str();
}

} // namespace editor
