#pragma once
#include <string_view>
namespace nova::java::mlg {
// 1.8 writes minecraft:water[level=0]; modern versions wrap the ID in Block{}.
inline bool IsWaterState(std::string_view value) {
    if (value.starts_with("Block{")) value.remove_prefix(6);
    const auto end = value.find_first_of("}[");
    const auto id = value.substr(0, end);
    return id == "minecraft:water" || id == "minecraft:flowing_water";
}
inline bool IsSourceState(std::string_view value) {
    return IsWaterState(value) &&
        (value.find("[level=0]") != value.npos || value.find("[level=0,") != value.npos ||
         value.find(",level=0]") != value.npos || value.find(",level=0,") != value.npos);
}
}
