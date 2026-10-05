#include "ui/hud/esp2d/Esp2DView.h"
#include "imgui.h"
#include "ui/clickgui/panel/ClientSettings.h"
#include "ui/clickgui/panel/Theme.h"
#include "ui/clickgui/Fonts.h"
#include <string>

namespace nova::ui::esp2d {
namespace {
struct Slice { double begin, end, coverage; };
struct Slices { Slice values[3]{}; int count = 0; };
Slices SplitPixels(double begin, double end) {
    Slices result;
    const double first = std::floor(begin), last = std::floor(end);
    if (first == last) {
        result.values[result.count++] = {first, first + 1, end - begin};
    } else {
        result.values[result.count++] = {first, first + 1, first + 1 - begin};
        if (last > first + 1) result.values[result.count++] = {first + 1, last, 1};
        if (end > last) result.values[result.count++] = {last, last + 1, end - last};
    }
    return result;
}
struct Painter {
    ImDrawList* draws;
    float scale;
    RasterMode raster;
    void Covered(double x, double y, double w, double h, Value first, Value second, bool vertical) {
        // A 0.5 GUI-unit stroke can be less than one framebuffer pixel. Solid
        // triangles toggle between full coverage and none as the camera moves.
        // Integrate the axis-aligned rectangle over edge pixels instead. This
        // needs at most nine quads and no MSAA, extra pass or position smoothing.
        const auto fb = ImGui::GetIO().DisplayFramebufferScale;
        if (!(fb.x > 0 && fb.y > 0)) return;
        const double left = x * scale * fb.x, top = y * scale * fb.y;
        const double right = (x + w) * scale * fb.x, bottom = (y + h) * scale * fb.y;
        if (!(right > left && bottom > top) || !std::isfinite(left) || !std::isfinite(top) ||
            !std::isfinite(right) || !std::isfinite(bottom)) return;
        const auto xs = SplitPixels(left, right), ys = SplitPixels(top, bottom);
        for (int i = 0; i < xs.count; ++i) for (int j = 0; j < ys.count; ++j) {
            const auto a = xs.values[i], b = ys.values[j];
            const double coverage = a.coverage * b.coverage;
            auto tint = [&](double coordinate) {
                const float t = float(std::clamp(vertical ? (coordinate - top) / (bottom - top) :
                    (coordinate - left) / (right - left), 0., 1.));
                const auto color = color::Interpolate(first, second, t);
                return color::ReAlpha(color, int(std::lround(color::Alpha(color) * coverage)));
            };
            const auto start = tint(vertical ? std::max(b.begin, top) : std::max(a.begin, left));
            const auto end = tint(vertical ? std::min(b.end, bottom) : std::min(a.end, right));
            draws->AddRectFilledMultiColor({float(a.begin / fb.x), float(b.begin / fb.y)},
                {float(a.end / fb.x), float(b.end / fb.y)}, start, vertical ? start : end, end, vertical ? end : start);
        }
    }
    void Rect(double x,double y,double w,double h,Value tint) {
        if(w<=0 || h<=0) return;
        if (raster == RasterMode::PixelCoverage) { Covered(x,y,w,h,tint,tint,false); return; }
        draws->AddRectFilled({float(x)*scale,float(y)*scale},{float(x+w)*scale,float(y+h)*scale},tint);
    }
    void Gradient(double x,double y,double w,double h,Value first,Value second,bool vertical) {
        if(w<=0 || h<=0) return;
        if (raster == RasterMode::PixelCoverage) { Covered(x,y,w,h,first,second,vertical); return; }
        draws->AddRectFilledMultiColor({float(x)*scale,float(y)*scale},{float(x+w)*scale,float(y+h)*scale},
            first,vertical?first:second,second,vertical?second:first);
    }
};
}
void Draw(Bounds bounds,float scale,BoxMode box,HealthMode healthMode,float health,float maximum,float armor,bool armorBar,RasterMode raster,
        float maximumArmor, bool showEmptyArmor, Layout layout, bool outlined) {
    Painter painter{ImGui::GetBackgroundDrawList(),scale,raster};
    const auto& theme=menu::panel::ThemeColors(menu::panel::ClientSettings::Get().themeValue());
    Render(painter,bounds,box,healthMode,health,maximum,armor,armorBar,theme.iconHighlightColor,theme.clientIconColor,maximumArmor,showEmptyArmor,layout,outlined);
}
void DrawName(Bounds bounds, float scale, std::string_view name, bool shadow) {
    if (name.empty() || !ImGui::GetCurrentContext() || !std::isfinite(scale) || scale <= 0) return;
    auto* font = Medium12 ? Medium12 : ImGui::GetFont();
    const float size = 7.f * scale;
    const auto display = ImGui::GetIO().DisplaySize;
    const float margin = 2.f * scale, available = std::min(140.f * scale, display.x - 2 * margin);
    if (available <= size || display.y < size + 2 * margin) return;
    std::string clipped;
    const char* begin = name.data(); const char* end = begin + name.size();
    if (font->CalcTextSizeA(size, FLT_MAX, 0, begin, end).x > available) {
        const char* remaining = begin;
        const float dots = font->CalcTextSizeA(size, FLT_MAX, 0, "...").x;
        font->CalcTextSizeA(size, available - dots, 0, begin, end, &remaining);
        clipped.assign(begin, remaining); clipped += "...";
        begin = clipped.data(); end = begin + clipped.size();
    }
    const auto dimensions = font->CalcTextSizeA(size, FLT_MAX, 0, begin, end);
    const ImVec2 position{std::clamp(float((bounds.left + bounds.right) * .5) * scale - dimensions.x * .5f,
            margin, std::max(margin, display.x - margin - dimensions.x)),
        std::clamp(float(bounds.top) * scale - dimensions.y - 2.f * scale, margin, display.y - margin - dimensions.y)};
    auto* draws = ImGui::GetBackgroundDrawList();
    const auto& theme = menu::panel::ThemeColors(menu::panel::ClientSettings::Get().themeValue());
    if (shadow) draws->AddText(font, size, {position.x + scale * .5f, position.y + scale * .5f}, IM_COL32(0, 0, 0, 180), begin, end);
    draws->AddText(font, size, position, theme.textHighlightColor, begin, end);
}
}
