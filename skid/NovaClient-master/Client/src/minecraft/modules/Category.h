#pragma once

#include <string_view>

namespace nova::module {

enum class Category : int {
    Combat = 0,
    Player,
    Movement,
    Render,
    Misc,
};

inline constexpr int kCategoryCount = 5;

enum class IconFace : int {
    Fatal = 0,
    Frost,
};

struct CategoryInfo {
    std::string_view name;
    const char8_t* icon;
    IconFace face;
};

constexpr CategoryInfo Describe(Category category) {
    constexpr CategoryInfo kInfo[kCategoryCount] = {
        {std::string_view{"COMBAT"},   u8"\ue66f", IconFace::Fatal},
        {std::string_view{"PLAYER"},   u8"\ue6df", IconFace::Fatal},
        {std::string_view{"MOVEMENT"}, u8"\ue86b", IconFace::Frost},
        {std::string_view{"RENDER"},   u8"\ue620", IconFace::Fatal},
        {std::string_view{"MISC"},     u8"\ue665", IconFace::Fatal},
    };
    const int index = static_cast<int>(category);
    return kInfo[(index >= 0 && index < kCategoryCount) ? index : 0];
}

}
