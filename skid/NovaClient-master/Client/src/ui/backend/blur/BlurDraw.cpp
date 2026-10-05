#include "ui/backend/blur/Blur.h"

namespace nova::ui::blur {
namespace {
ImTextureID backdrop = 0;
bool inverted = false;
ImTextureRef overlay;
ImVec2 overlayMin{}, overlayMax{};
bool hasOverlay = false;
ImTextureID layerBackdrop = 0;
bool layerInverted = false, samplingLayer = false;
ImDrawCallback layerCapture = nullptr;
void* layerData = nullptr;

// Compose coverage c as ONE glass layer:
// imageAlpha(c) * (1 - tintAlpha * opacity * c) = opacity * c * (1 - tintAlpha).
// Linear alpha on two independent AA fringes breaks this equation. Use the
// exact same outline as the tint and sample its nonlinear image alpha in a
// narrow, subdivided fringe. No inset, border, shader or extra render target.
void GlassImage(ImDrawList* draws, ImTextureRef texture, ImVec2 min, ImVec2 max, ImVec2 uv0, ImVec2 uv1,
                float tintAlpha, float opacity, float rounding, ImDrawFlags flags) {
    const auto alphaAt = [&](float coverage) {
        const float a = opacity * coverage;
        return static_cast<ImU32>(255.0f * a * (1.0f - tintAlpha) / (1.0f - a * tintAlpha) + 0.5f);
    };
    if (!(draws->Flags & ImDrawListFlags_AntiAliasedFill) || rounding < 0.5f ||
        (flags & ImDrawFlags_RoundCornersMask_) == ImDrawFlags_RoundCornersNone) {
        draws->AddImageRounded(texture, min, max, uv0, uv1,
            IM_COL32(255, 255, 255, alphaAt(1)), rounding, flags);
        return;
    }
    // Bundled ImGui AddConvexPolyFilled emits alternating inner/outer AA
    // vertices. Reuse those positions so image and tint coverage coincide.
    ImDrawList outline(draws->_Data);
    outline._ResetForNewFrame();
    outline.Flags = draws->Flags;
    outline._FringeScale = draws->_FringeScale;
    outline.AddRectFilled(min, max, IM_COL32_WHITE, rounding, flags);
    const int count = outline.VtxBuffer.Size / 2;
    if (count < 3) return;
    constexpr int bands = 16;
    draws->PushTexture(texture);
    draws->PrimReserve((count - 2) * 3 + count * bands * 6, count * (bands + 1));
    const unsigned base = draws->_VtxCurrentIdx;
    for (int band = 0; band <= bands; ++band) {
        const float c = 1.0f - static_cast<float>(band) / bands;
        const ImU32 col = IM_COL32(255, 255, 255, alphaAt(c));
        for (int i = 0; i < count; ++i) {
            const ImVec2 inner = outline.VtxBuffer[i * 2].pos;
            const ImVec2 outer = outline.VtxBuffer[i * 2 + 1].pos;
            const ImVec2 pos(outer.x + (inner.x - outer.x) * c, outer.y + (inner.y - outer.y) * c);
            const float u = std::clamp((pos.x - min.x) / (max.x - min.x), 0.0f, 1.0f);
            const float v = std::clamp((pos.y - min.y) / (max.y - min.y), 0.0f, 1.0f);
            draws->PrimWriteVtx(pos, ImVec2(uv0.x + (uv1.x - uv0.x) * u, uv0.y + (uv1.y - uv0.y) * v), col);
        }
    }
    auto triangle = [&](unsigned a, unsigned b, unsigned c) {
        draws->PrimWriteIdx(static_cast<ImDrawIdx>(base + a));
        draws->PrimWriteIdx(static_cast<ImDrawIdx>(base + b));
        draws->PrimWriteIdx(static_cast<ImDrawIdx>(base + c));
    };
    for (int i = 2; i < count; ++i) triangle(0, i - 1, i);
    for (int band = 0; band < bands; ++band) {
        const unsigned inside = band * count, outside = inside + count;
        for (int i = 0, previous = count - 1; i < count; previous = i++) {
            triangle(inside + i, inside + previous, outside + previous);
            triangle(outside + previous, outside + i, inside + i);
        }
    }
    draws->PopTexture();
}

void PopupBackground(ImU32 tint) {
    auto* draws = ImGui::GetWindowDrawList();
    const auto min = ImGui::GetWindowPos();
    const auto size = ImGui::GetWindowSize();
    // Prepend the background to native decorations, preserving scrollbars and
    // borders above it. A local splitter also leaves widget channel use intact.
    ImDrawListSplitter layers;
    layers.Split(draws, 2);
    layers.SetCurrentChannel(draws, 1);
    draws->PushClipRectFullScreen();
    LayerBackground(draws, min, ImVec2(min.x + size.x, min.y + size.y), tint, ImGui::GetStyle().PopupRounding);
    draws->PopClipRect();
    layers.SetCurrentChannel(draws, 0);
    draws->CmdBuffer.swap(layers._Channels[1]._CmdBuffer);
    draws->IdxBuffer.swap(layers._Channels[1]._IdxBuffer);
    layers.Merge(draws);
}
}
void Publish(ImTextureID texture, bool flipY) { backdrop = texture; inverted = flipY; }
bool Available() { return backdrop != 0 && GetOptions().enabled; }
ImTextureID BackdropTexture() { return Available() ? backdrop : ImTextureID_Invalid; }
ImVec2 BackdropUv(ImVec2 p) {
    const auto* viewport = ImGui::GetMainViewport();
    ImVec2 uv((p.x-viewport->Pos.x)/std::max(1.f,viewport->Size.x),
              (p.y-viewport->Pos.y)/std::max(1.f,viewport->Size.y));
    if(inverted)uv.y=1.f-uv.y;
    return uv;
}
void SetOverlayBackdrop(ImTextureRef texture, ImVec2 min, ImVec2 max) {
    overlay=texture;overlayMin=min;overlayMax=max;hasOverlay=max.x>min.x&&max.y>min.y;
}
void ClearOverlayBackdrop(){overlay=ImTextureRef();hasOverlay=false;}
void SetLayerBackdrop(ImTextureID texture, bool flipY, ImDrawCallback capture, void* data) {
    layerBackdrop=texture;layerInverted=flipY;layerCapture=capture;layerData=data;
}
void ClearLayerBackdrop(){SetLayerBackdrop(0,false,nullptr);}
bool IsBackdropTexture(ImTextureID texture){return texture && (texture==backdrop || texture==layerBackdrop);}
ImTextureID CaptureLayerBackdrop(ImDrawList* draws) {
    if(!Available())return ImTextureID_Invalid;
    if(!draws || !layerBackdrop || !layerCapture)return backdrop;
    draws->AddCallback(layerCapture,layerData);
    draws->AddCallback(ImDrawCallback_ResetRenderState);
    return layerBackdrop;
}
void LayerBackground(ImDrawList* draws, ImVec2 min, ImVec2 max, ImU32 tint,
                     float rounding, ImDrawFlags flags, float opacity, float tintScale) {
    const bool live=draws && Available() && layerBackdrop && layerCapture && opacity>0 && max.x>min.x && max.y>min.y;
    if(live)CaptureLayerBackdrop(draws);
    samplingLayer=live;
    Background(draws,min,max,tint,rounding,flags,opacity,tintScale);
    samplingLayer=false;
}

void Background(ImDrawList* draws, ImVec2 min, ImVec2 max, ImU32 tint,
                float rounding, ImDrawFlags flags, float opacity, float tintScale) {
    opacity = std::clamp(opacity, 0.0f, 1.0f);
    if (opacity <= 0.0f) return;
    if (!draws || max.x <= min.x || max.y <= min.y) return;
    float tintAlpha = static_cast<float>((tint & IM_COL32_A_MASK) >> IM_COL32_A_SHIFT) / 255.0f;
    if (Available()) {
        const auto* viewport = ImGui::GetMainViewport();
        if (viewport->Size.x > 0 && viewport->Size.y > 0) {
            ImVec2 uv0((min.x - viewport->Pos.x) / viewport->Size.x,
                       (min.y - viewport->Pos.y) / viewport->Size.y);
            ImVec2 uv1((max.x - viewport->Pos.x) / viewport->Size.x,
                       (max.y - viewport->Pos.y) / viewport->Size.y);
            if (samplingLayer ? layerInverted : inverted) { uv0.y = 1.0f - uv0.y; uv1.y = 1.0f - uv1.y; }
            if (const auto linear = ImGui::GetPlatformIO().DrawCallback_SetSamplerLinear)
                draws->AddCallback(linear, nullptr);
            // Color alpha controls tint, not blur coverage. Fade the composed
            // glass as one layer: at full opacity no unblurred scene leaks in.
            tintAlpha *= std::clamp(tintScale, 0.0f, 1.0f);
            GlassImage(draws, ImTextureRef(samplingLayer?layerBackdrop:backdrop), min, max, uv0, uv1, tintAlpha, opacity, rounding, flags);
            if(!samplingLayer && hasOverlay && max.x>overlayMin.x && min.x<overlayMax.x && max.y>overlayMin.y && min.y<overlayMax.y){
                const ImVec2 size=overlayMax-overlayMin;
                const ImVec2 a((min.x-overlayMin.x)/size.x,(min.y-overlayMin.y)/size.y);
                const ImVec2 b((max.x-overlayMin.x)/size.x,(max.y-overlayMin.y)/size.y);
                draws->PushClipRect(overlayMin,overlayMax,true);
                GlassImage(draws,overlay,min,max,a,b,tintAlpha,opacity,rounding,flags);
                draws->PopClipRect();
            }
        }
    }
    const auto alpha = static_cast<ImU32>(tintAlpha * opacity * 255.0f + 0.5f);
    tint = (tint & ~IM_COL32_A_MASK) | (alpha << IM_COL32_A_SHIFT);
    draws->AddRectFilled(min, max, tint, rounding, flags);
}

bool BeginPopup(const char* id, ImGuiWindowFlags flags) {
    if (!Available()) return ImGui::BeginPopup(id, flags);
    const auto tint = ImGui::GetColorU32(ImGuiCol_PopupBg);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, 0u);
    const bool open = ImGui::BeginPopup(id, flags);
    ImGui::PopStyleColor();
    if (open) PopupBackground(tint);
    return open;
}

bool BeginPopupContextWindow(const char* id, ImGuiPopupFlags flags) {
    if (!Available()) return ImGui::BeginPopupContextWindow(id, flags);
    const auto tint = ImGui::GetColorU32(ImGuiCol_PopupBg);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, 0u);
    const bool open = ImGui::BeginPopupContextWindow(id, flags);
    ImGui::PopStyleColor();
    if (open) PopupBackground(tint);
    return open;
}

bool BeginChild(const char* id, ImVec2 size, ImGuiChildFlags childFlags, ImGuiWindowFlags flags) {
    const auto tint = ImGui::GetColorU32(ImGuiCol_ChildBg);
    if (!Available() || (flags & ImGuiWindowFlags_NoBackground) || (tint & IM_COL32_A_MASK) == 0)
        return ImGui::BeginChild(id, size, childFlags, flags);
    const float rounding = ImGui::GetStyle().ChildRounding;
    auto* parentDraws = ImGui::GetWindowDrawList();
    ImDrawListSplitter layers;
    layers.Split(parentDraws, 2);
    layers.SetCurrentChannel(parentDraws, 1);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, 0u);
    const bool visible = ImGui::BeginChild(id, size, childFlags, flags);
    ImGui::PopStyleColor();
    layers.SetCurrentChannel(parentDraws, 0);
    if (visible) {
        const auto min = ImGui::GetWindowPos();
        const auto actualSize = ImGui::GetWindowSize();
        // ImGui may put child decorations in the parent draw list. Insert the
        // glass before those decorations, retaining the parent's scroll clip.
        LayerBackground(parentDraws, min, ImVec2(min.x + actualSize.x, min.y + actualSize.y), tint, rounding);
    }
    layers.Merge(parentDraws);
    return visible;
}
}
