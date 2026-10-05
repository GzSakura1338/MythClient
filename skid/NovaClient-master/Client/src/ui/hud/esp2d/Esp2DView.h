#pragma once
#include <algorithm>
#include <cmath>
#include <string_view>
#include "utils/Color.h"

namespace nova::ui::esp2d {
using color::Value;
enum class BoxMode { Normal, CS, None };
enum class HealthMode { Health, Standard, Gradient, None };
enum class RasterMode { Solid, PixelCoverage };
enum class Layout { Legacy, Compact };
struct Bounds { double left, top, right, bottom; };
void Draw(Bounds bounds,float scale,BoxMode box,HealthMode healthMode,
    float health,float maximum,float armor,bool armorBar,RasterMode raster = RasterMode::Solid,
    float maximumArmor = 20, bool showEmptyArmor = false, Layout layout = Layout::Legacy, bool outline = true);
// Optional plain player label for hosts that supply names. Uses Nova's shared
// font/theme; the existing Minecraft box/bar draw path is unchanged.
void DrawName(Bounds bounds, float scale, std::string_view name, bool shadow);

// Asphyxia-style separate frame/name/bar spacing with Nova colors and raster.
// Fixed side padding keeps the bars clear of either full or corner frames.
template<class Draw> void RenderCompact(Draw& draw, Bounds b, BoxMode box, HealthMode mode, float health,
        float maximum, float armor, bool armorBar, Value first, Value second, float maximumArmor, bool showEmptyArmor, bool outlined) {
    const double x = b.left, y = b.top, r = b.right, bottom = b.bottom, w = r - x, h = bottom - y;
    if (!std::isfinite(w) || !std::isfinite(h) || w < 1 || h < 1) return;
    constexpr Value outline = 0xDC080808, backing = 0xB4080808;
    constexpr double stroke = .5, edge = .5;
    auto frame = [&](double inset, double thickness, Value tint) {
        const double left = x + inset, top = y + inset, width = w - 2 * inset, height = h - 2 * inset;
        if (width <= 0 || height <= 0) return;
        draw.Rect(left, top, width, thickness, tint);
        draw.Rect(left, top + height - thickness, width, thickness, tint);
        draw.Rect(left, top + thickness, thickness, height - 2 * thickness, tint);
        draw.Rect(left + width - thickness, top + thickness, thickness, height - 2 * thickness, tint);
    };
    if (box == BoxMode::Normal) {
        if (outlined) frame(-stroke * .5 - edge, stroke + 2 * edge, outline);
        draw.Gradient(x - stroke * .5, y - stroke * .5, w + stroke, stroke, first, second, false);
        draw.Gradient(x - stroke * .5, bottom - stroke * .5, w + stroke, stroke, first, second, false);
        draw.Rect(x - stroke * .5, y + stroke * .5, stroke, h - stroke, first);
        draw.Rect(r - stroke * .5, y + stroke * .5, stroke, h - stroke, second);
    } else if (box == BoxMode::CS) {
        const double cw = w * .25, ch = h * .25;
        // Draw every outline before the color, so joins don't darken a corner.
        for (int pass = outlined ? 0 : 1; pass < 2; ++pass) {
            const double thickness = pass ? stroke : stroke + edge * 2;
            const double pad = thickness * .5;
            for (int corner = 0; corner < 4; ++corner) {
                const bool right = corner & 1, lower = corner & 2;
                const Value tint = pass ? (right ? second : first) : outline;
                const double cx = right ? r : x, cy = lower ? bottom : y;
                draw.Rect(right ? cx - cw : cx - pad, cy - pad, cw + pad, thickness, tint);
                draw.Rect(cx - pad, lower ? cy - ch : cy - pad, thickness, ch + pad, tint);
            }
        }
    }
    if (mode != HealthMode::None && std::isfinite(health) && std::isfinite(maximum) && maximum > 0) {
        const float ratio = std::clamp(health / maximum, 0.f, 1.f);
        draw.Rect(x - 4, y - .5, 2, h + 1, backing);
        if (ratio > 0) {
            const double filled = h * ratio;
            if (mode == HealthMode::Health) draw.Rect(x - 3.5, bottom - filled, 1, filled, color::HsbToBgr(ratio / 3.f, 1, 1) | 0xFF000000);
            else if (mode == HealthMode::Standard) draw.Rect(x - 3.5, bottom - filled, 1, filled, first);
            else draw.Gradient(x - 3.5, bottom - filled, 1, filled, color::Interpolate(first, second, ratio), second, true);
        }
    }
    if (armorBar && std::isfinite(armor) && std::isfinite(maximumArmor) && maximumArmor > 0 && (armor > 0 || showEmptyArmor)) {
        const float ratio = std::clamp(armor / maximumArmor, 0.f, 1.f);
        draw.Rect(x - .5, bottom + 2, w + 1, 2, backing);
        if (ratio > 0) draw.Gradient(x, bottom + 2.5, w * ratio, 1, first, color::Interpolate(first, second, ratio), false);
    }
}

// Emit Rise's exact rectangles in GUI units. Axis gradients retain the reference
// helpers' actual direction (horizontalCenteredGradient is left-to-right).
template<class Draw> void Render(Draw& draw, Bounds b, BoxMode box, HealthMode mode, float health,
        float maximum, float armor, bool armorBar, Value first, Value second,
        float maximumArmor = 20, bool showEmptyArmor = false, Layout layout = Layout::Legacy, bool outlined = true) {
    if (layout == Layout::Compact) {
        RenderCompact(draw, b, box, mode, health, maximum, armor, armorBar, first, second, maximumArmor, showEmptyArmor, outlined);
        return;
    }
    const double x=b.left,y=b.top,r=b.right,bottom=b.bottom,w=r-x,h=bottom-y;
    constexpr Value black=0xFF000000, backing=0xB4000000;
    if (box==BoxMode::Normal) {
        if (outlined) {
        draw.Rect(x-.5,y+1,1.5,h-1.5,black);
        draw.Rect(x-.5,y-.5,w+1.5,1.5,black);
        draw.Rect(r-.5,y+1,1.5,h,black);
        draw.Rect(x-.5,bottom-.5,w,1.5,black);
        }
        draw.Gradient(x,y+.5,.5,h-.5,first,second,false);
        draw.Gradient(x,y,w,.5,first,second,false);
        draw.Gradient(r,y,.5,h,first,second,false);
        draw.Gradient(x,bottom,w+.5,.5,first,second,false);
    } else if (box==BoxMode::CS) {
        const double cw=w/4,ch=h/4;
        if (outlined) {
        draw.Rect(x-.5,y+1,1.5,ch,black);
        draw.Rect(x-.5,bottom-ch-.5,1.5,ch,black);
        draw.Rect(x-.5,y-.5,cw+1,1.5,black);
        draw.Rect(r-cw-.5,y-.5,cw+1.5,1.5,black);
        draw.Rect(r-.5,y+1,1.5,ch,black);
        draw.Rect(r-.5,bottom-ch-.5,1.5,ch,black);
        draw.Rect(x-.5,bottom-.5,cw+1,1.5,black);
        draw.Rect(r-cw-.5,bottom-.5,cw+1.5,1.5,black);
        }
        draw.Rect(x,y+.5,.5,ch,second);
        draw.Rect(x,bottom-ch,.5,ch,second);
        draw.Rect(x,y,cw,.5,second);
        draw.Rect(r-cw,y,cw,.5,second);
        draw.Rect(r,y,.5,ch+.5,second);
        draw.Rect(r,bottom-ch,.5,ch+.5,second);
        draw.Rect(x,bottom,cw,.5,second);
        draw.Rect(r-cw,bottom,cw,.5,second);
    }
    if (mode!=HealthMode::None && std::isfinite(health) && std::isfinite(maximum) && maximum>0) {
        const float ratio=std::clamp(health/maximum,0.f,1.f);
        const double missing=(h-2)*(1-ratio);
        draw.Rect(x-2.5,y-.5,1.5,h+1.5,backing);
        if (mode==HealthMode::Health) {
            const auto tint=color::HsbToBgr(ratio/3.f,1,1)|0xFF000000;
            draw.Rect(x-2,y+missing,.5,h-missing+.5,tint);
        } else if (mode==HealthMode::Standard) {
            draw.Rect(x-2,y+missing,.5,h-missing+.5,first);
        } else {
            draw.Gradient(x-2,y+missing,.5,h-missing+.5,color::Interpolate(first,second,ratio),second,true);
        }
    }
    if (armorBar && std::isfinite(armor) && std::isfinite(maximumArmor) && maximumArmor>0 && (armor>0 || showEmptyArmor)) {
        const float ratio=std::clamp(armor/maximumArmor,0.f,1.f);
        draw.Rect(x-.5,bottom+1.5,w+1.5,1.5,backing);
        if (ratio>0) draw.Gradient(x,bottom+2,(w+.5)*ratio,.5,first,color::Interpolate(first,second,ratio),false);
    }
}
}
