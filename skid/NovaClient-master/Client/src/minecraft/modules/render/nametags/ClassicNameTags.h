#pragma once
#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include "minecraft/utils/rotation/Vec3.h"

namespace nova::module::nametags {
struct TextStyle { int color = -1, rgb = -1; bool bold = false, italic = false, underline = false, strike = false; };
template<class Measure, class Draw> float WalkText(std::string_view text, float pixel, Measure&& measure, Draw&& draw) {
    TextStyle style;
    float x = 0;
    while (!text.empty()) {
        if (text.size() >= 3 && text[0] == '\xC2' && text[1] == '\xA7') {
            char c = text[2]; if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
            // Internal RGB escape for styled Components on modern versions.
            if (c == '#' && text.size() >= 9) {
                int rgb = 0;
                bool valid = true;
                for (char digit : text.substr(3, 6)) {
                    if (digit >= 'A' && digit <= 'F') digit += 'a' - 'A';
                    const auto nibble = std::string_view("0123456789abcdef").find(digit);
                    if (nibble == std::string_view::npos) { valid = false; break; }
                    rgb = (rgb << 4) | int(nibble);
                }
                if (valid) { style = {}; style.rgb = rgb; text.remove_prefix(9); continue; }
            }
            const auto index = std::string_view("0123456789abcdef").find(c);
            if (index != std::string_view::npos) { style = {}; style.color = int(index); }
            else if (c == 'r') style = {};
            else if (c == 'l') style.bold = true;
            else if (c == 'o') style.italic = true;
            else if (c == 'n') style.underline = true;
            else if (c == 'm') style.strike = true;
            text.remove_prefix(3); continue;
        }
        const unsigned char c = text[0];
        std::size_t length = c < 0x80 ? 1 : c < 0xE0 ? 2 : c < 0xF0 ? 3 : 4;
        length = std::min(length, text.size());
        const auto glyph = text.substr(0, length);
        const float width = measure(glyph) + (style.bold ? pixel : 0);
        draw(glyph, x, width, style);
        x += width;
        text.remove_prefix(length);
    }
    return x;
}
inline bool Listed(std::string_view list, std::string_view name) {
    auto separator = [](char c) { return c == ',' || c == ';' || c == ' ' || c == '\t' || c == '\n' || c == '\r'; };
    while (!list.empty()) {
        while (!list.empty() && separator(list.front())) list.remove_prefix(1);
        const auto end = std::find_if(list.begin(), list.end(), separator);
        const auto token = list.substr(0, end - list.begin());
        if (!token.empty() && token.size() == name.size() && std::equal(token.begin(), token.end(), name.begin(),
            [](unsigned char a, unsigned char b) { return (a >= 'A' && a <= 'Z' ? a + 32 : a) == (b >= 'A' && b <= 'Z' ? b + 32 : b); })) return true;
        list.remove_prefix(token.size());
    }
    return false;
}
inline std::string ClassicLabel(std::string name, float health, bool team, bool target, bool friendTag, bool shortened, float maxHealth = 20.f) {
    const int rounded = std::isfinite(health) ? static_cast<int>(std::floor(std::clamp(double(health), 0.0, 1000000.0) + .5)) : 0;
    // Raven's health palette is relative to maximum health, not a fixed dark red.
    if (!std::isfinite(maxHealth) || maxHealth <= 0.f) maxHealth = 20.f;
    const float ratio = std::isfinite(health) ? std::max(health, 0.f) / maxHealth : 0.f;
    const char color = ratio < .3f ? 'c' : ratio < .5f ? '6' : ratio < .7f ? 'e' : 'a';
    name += std::string(" \xC2\xA7" "7[\xC2\xA7") + color + "\xE2\x9D\xA4" + std::to_string(rounded) + "\xC2\xA7" "7]";
    if (team) name = std::string("\xC2\xA7" "a\xC2\xA7" "l") + (shortened ? "[TM]" : "[TEAM]") + "\xC2\xA7" "r " + name;
    if (target) name = std::string("\xC2\xA7" "4\xC2\xA7" "l") + (shortened ? "[T]" : "[TARGET]") + "\xC2\xA7" "r " + name;
    if (friendTag) name = std::string("\xC2\xA7" "b\xC2\xA7" "l") + (shortened ? "[F]" : "[FRIEND]") + "\xC2\xA7" "r " + name;
    return name;
}
struct ClassicAnchor { float x, y; };
// Rise projects all eight corners of its expanded entity box, then centers the
// tag on the top of those screen bounds. Keep interpolation in render time.
template<class Project> std::optional<ClassicAnchor> ProjectClassic(rotation::Vec3 now, rotation::Vec3 before,
        double width, double height, bool sneak, float partial, Project&& project) {
    if (!std::isfinite(width) || !std::isfinite(height) || width <= 0 || height <= 0 || !std::isfinite(partial)) return {};
    const double t = std::clamp(double(partial), 0.0, 1.0);
    const rotation::Vec3 at{before.x + (now.x-before.x)*t, before.y + (now.y-before.y)*t, before.z + (now.z-before.z)*t};
    double left = std::numeric_limits<double>::infinity(), right = -left, top = left;
    int count = 0;
    for (int i = 0; i < 8; ++i) {
        const rotation::Vec3 corner{at.x + (i&1 ? 1 : -1)*(width+.14)/2,
            at.y + (i&2 ? height + (sneak ? -.1 : .2) + .01 : 0), at.z + (i&4 ? 1 : -1)*(width+.14)/2};
        float x, y;
        if (!project(corner, x, y) || !std::isfinite(x) || !std::isfinite(y)) continue;
        left = std::min(left, double(x)); right = std::max(right, double(x)); top = std::min(top, double(y)); ++count;
    }
    if (!count) return {};
    return ClassicAnchor{float((left+right)/2), float(top)};
}
}
