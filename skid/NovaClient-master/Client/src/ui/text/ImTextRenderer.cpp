#include "ui/text/ImTextRenderer.h"

namespace nova::text {
namespace {

constexpr char kSectionSignHigh = static_cast<char>(0xC2);
constexpr char kSectionSignLow = static_cast<char>(0xA7);

bool IsCodeChar(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F') ||
           (c >= 'k' && c <= 'o') || (c >= 'K' && c <= 'O') || c == 'r' || c == 'R';
}

int CodeLength(const char* at) {
    if (at[0] != kSectionSignHigh || at[1] != kSectionSignLow)
        return 0;
    return IsCodeChar(at[2]) ? 3 : 0;
}

int ColorIndex(char code) {
    if (code >= '0' && code <= '9')
        return code - '0';
    if (code >= 'a' && code <= 'f')
        return 10 + (code - 'a');
    return -1;
}

const char* RunEnd(const char* at) {
    while (*at != '\0' && CodeLength(at) == 0)
        ++at;
    return at;
}

float Measure(const char* begin, const char* end) {
    return ImGui::CalcTextSize(begin, end, false, -1.0f).x;
}

}

ImU32 Dimmed(ImU32 color) {
    return ((color & 0x00FCFCFCu) >> 2) | (color & 0xFF000000u);
}

ImU32 ColorByIndex(int index) {
    if (index < 0 || index >= 16)
        return 0;
    const int base = ((index >> 3) & 1) * 85;
    int red = ((index >> 2) & 1) * 170 + base;
    const int green = ((index >> 1) & 1) * 170 + base;
    const int blue = (index & 1) * 170 + base;
    if (index == 6)
        red += 85;
    return static_cast<ImU32>(red) | (static_cast<ImU32>(green) << 8) |
           (static_cast<ImU32>(blue) << 16) | (0xFFu << 24);
}

float TextHeight() { return ImGui::GetTextLineHeight(); }

float ShadowOffset() { return (TextHeight() / 18.0f) * 1.2f; }

float TextWidth(const char* text) {
    float width = 0.0f;
    bool bold = false;
    const char* at = text;
    while (*at != '\0') {
        const int code = CodeLength(at);
        if (code > 0) {
            if (at[2] == 'r')
                bold = false;
            else if (at[2] == 'l')
                bold = true;
            at += code;
            continue;
        }
        const char* const end = RunEnd(at);
        width += Measure(at, end) + (bold ? 1.0f : 0.0f);
        at = end;
    }
    return width;
}

void DrawTextRun(ImDrawList* list, const char* begin, const char* end,
                 float x, float y, ImU32 color, ImU32 shadowColor, bool bold) {
    if (shadowColor != 0) {
        const float offset = ShadowOffset();
        list->AddText(ImVec2(x + offset, y + offset), shadowColor, begin, end);
        if (bold)
            list->AddText(ImVec2(x + 1.0f + offset, y + offset), shadowColor, begin, end);
    }
    list->AddText(ImVec2(x, y), color, begin, end);
    if (bold)
        list->AddText(ImVec2(x + 1.0f, y), color, begin, end);
}

void DrawText(ImDrawList* list, const char* text, float x, float y, ImU32 color, bool shadow) {
    ImU32 currentColor = color;
    ImU32 currentShadowColor = shadow ? Dimmed(color) : 0;
    bool currentBold = false;
    float currentX = x;

    const char* at = text;
    while (*at != '\0') {
        const int code = CodeLength(at);
        if (code > 0) {
            const char letter = at[2];
            const int index = ColorIndex(letter);
            if (index >= 0) {
                currentColor = ColorByIndex(index);
            } else if (letter == 'r') {
                currentColor = color;
                currentBold = false;
            } else if (letter == 'l') {
                currentBold = true;
            }
            if (shadow)
                currentShadowColor = Dimmed(currentColor);
            at += code;
            continue;
        }
        const char* const end = RunEnd(at);
        DrawTextRun(list, at, end, currentX, y, currentColor, currentShadowColor, currentBold);
        currentX += Measure(at, end) + (currentBold ? 1.0f : 0.0f);
        at = end;
    }
}

ImVec2 CenterText(const char* text, float x, float y, bool horizontal, bool vertical) {
    return ImVec2(horizontal ? x - TextWidth(text) / 2.0f : x,
                  vertical ? y - TextHeight() / 2.0f : y);
}

}
