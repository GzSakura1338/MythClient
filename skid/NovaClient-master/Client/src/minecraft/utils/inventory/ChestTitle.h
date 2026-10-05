#pragma once

#include <string>
#include <string_view>

namespace nova::inventory {

inline std::string NormalizeChestTitle(std::string_view title) {
    std::string result;
    result.reserve(title.size());
    for (size_t i = 0; i < title.size(); ++i) {
        if (static_cast<unsigned char>(title[i]) == 0xC2 && i + 2 < title.size() &&
            static_cast<unsigned char>(title[i + 1]) == 0xA7) {
            const char code = title[i + 2];
            if ((code >= '0' && code <= '9') || (code >= 'a' && code <= 'f') ||
                (code >= 'k' && code <= 'o') || code == 'r' ||
                (code >= 'A' && code <= 'F') || (code >= 'K' && code <= 'O') || code == 'R') {
                i += 2;
                continue;
            }
        }
        result.push_back(title[i]);
    }
    const auto first = result.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return {};
    const auto last = result.find_last_not_of(" \t\r\n");
    return result.substr(first, last - first + 1);
}

inline bool MatchesChestTitle(std::string_view title, std::string_view chest,
                             std::string_view largeChest, std::string_view enderChest,
                             bool allowEnderChest) {
    const std::string name = NormalizeChestTitle(title);
    if (name.empty())
        return true;
    if (name == NormalizeChestTitle(chest) || name == NormalizeChestTitle(largeChest) || name == "Chest")
        return true;
    return allowEnderChest && name == NormalizeChestTitle(enderChest);
}

}
