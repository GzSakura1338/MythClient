#pragma once
#include <algorithm>
#include <chrono>
#include "imgui.h"

namespace nova::ui::blur {
struct Options { bool enabled = false; int strength = 3; int fps = 30; };
Options GetOptions();

// Cache only the game backdrop. Individual panels sample it at the game FPS.
struct Refresh {
    using Clock = std::chrono::steady_clock;
    Clock::time_point last{};
    bool valid = false;
    int strength = 0;
    bool Due(Options options, Clock::time_point now = Clock::now()) const {
        return !valid || strength != options.strength ||
               now - last >= std::chrono::duration<double>(1.0 / std::clamp(options.fps, 5, 120));
    }
    void Updated(Options options, Clock::time_point now = Clock::now()) {
        last = now; valid = true; strength = options.strength;
    }
};
void RenderOpenGL(Options options, int width, int height);
void ReleaseOpenGL();
// Retire the context's cache, optionally without touching a lost context's GL names.
void ResetOpenGLContext(bool contextAvailable);
// Render-thread only. Backends publish a GPU texture; panels supply their own
// geometry, clipping and animation. Nothing is composited over the whole scene.
void Publish(ImTextureID texture, bool flipY = false);
bool Available();
// Snapshot geometry keeps blur live instead of retaining a backend texture.
ImTextureID BackdropTexture();
ImVec2 BackdropUv(ImVec2 screenPosition);
// Optional UI surface above the game backdrop. Scope to that surface's popups
// and clear after drawing them. ImTextureRef also supports pending uploads.
void SetOverlayBackdrop(ImTextureRef texture, ImVec2 min, ImVec2 max);
void ClearOverlayBackdrop();
// Captured by a renderer callback after lower windows have actually been drawn.
// Separate from the throttled world cache so popup captures cannot pollute HUDs.
void SetLayerBackdrop(ImTextureID texture, bool flipY, ImDrawCallback capture, void* data = nullptr);
void ClearLayerBackdrop();
bool IsBackdropTexture(ImTextureID texture);
ImTextureID CaptureLayerBackdrop(ImDrawList* draws);
void Background(ImDrawList* draws, ImVec2 min, ImVec2 max, ImU32 tint,
                float rounding = 0.0f, ImDrawFlags flags = 0, float opacity = 1.0f,
                float tintScale = 0.8f);
void LayerBackground(ImDrawList* draws, ImVec2 min, ImVec2 max, ImU32 tint,
                     float rounding = 0.0f, ImDrawFlags flags = 0, float opacity = 1.0f,
                     float tintScale = 0.8f);
// Pair a successful begin with ImGui::EndPopup(), including nested popups.
bool BeginPopup(const char* id, ImGuiWindowFlags flags = 0);
bool BeginPopupContextWindow(const char* id, ImGuiPopupFlags flags = ImGuiPopupFlags_MouseButtonRight);
// A real rounded card, as opposed to a transparent layout-only child.
bool BeginChild(const char* id, ImVec2 size, ImGuiChildFlags childFlags = 0, ImGuiWindowFlags flags = 0);
}
