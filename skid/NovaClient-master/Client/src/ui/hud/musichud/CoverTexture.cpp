#include "ui/hud/musichud/CoverTexture.h"
#include "imgui_internal.h"
#include <cstring>

namespace nova::ui::music {
namespace {
struct Entry { std::uint64_t version{}; std::unique_ptr<ImTextureData> texture; int retirementFrame = -1; };
std::vector<Entry> covers;
}
void RetainCovers(std::uint64_t current, std::uint64_t previous) {
    for (auto it = covers.begin(); it != covers.end();) {
        if (it->texture->WantDestroyNextFrame || (it->version != current && it->version != previous)) {
            it->texture->WantDestroyNextFrame = true;
            // ImGui only advances this lifecycle automatically for font atlas textures.
            // User textures must request destruction and count their unused frames.
            if (it->texture->TexID == ImTextureID_Invalid && !it->texture->BackendUserData)
                it->texture->SetStatus(ImTextureStatus_Destroyed);
            if (it->texture->Status == ImTextureStatus_Destroyed) {
                ImGui::UnregisterUserTexture(it->texture.get()); it = covers.erase(it); continue;
            }
            it->texture->SetStatus(ImTextureStatus_WantDestroy);
            if (it->retirementFrame != ImGui::GetFrameCount()) {
                it->retirementFrame = ImGui::GetFrameCount(); ++it->texture->UnusedFrames;
            }
        }
        ++it;
    }
}
bool DrawCover(ImDrawList* draws, const std::shared_ptr<const smtc::Artwork>& artwork,
    ImVec2 min, ImVec2 max, float rounding, float opacity) {
    if (!artwork || artwork->width <= 0 || artwork->height <= 0 || artwork->rgba.size() !=
        static_cast<size_t>(artwork->width) * artwork->height) return false;
    auto found = std::find_if(covers.begin(), covers.end(), [&](const auto& e) {
        return e.version == artwork->version && !e.texture->WantDestroyNextFrame;
    });
    if (found == covers.end()) {
        // Two visible covers plus a bounded queue awaiting GPU frame retirement.
        if (covers.size() >= 8) return false;
        Entry entry; entry.version = artwork->version; entry.texture = std::make_unique<ImTextureData>();
        entry.texture->Create(ImTextureFormat_RGBA32, artwork->width, artwork->height);
        std::memcpy(entry.texture->GetPixels(), artwork->rgba.data(), artwork->rgba.size() * 4);
        ImGui::RegisterUserTexture(entry.texture.get()); covers.push_back(std::move(entry)); found = std::prev(covers.end());
    }
    const float cropX = artwork->width > artwork->height ? (1.f - float(artwork->height) / artwork->width) * .5f : 0;
    const float cropY = artwork->height > artwork->width ? (1.f - float(artwork->width) / artwork->height) * .5f : 0;
    if (found->texture->Status == ImTextureStatus_Destroyed)
        found->texture->SetStatus(ImTextureStatus_WantCreate); // Backend/context recreation.
    if (const auto linear = ImGui::GetPlatformIO().DrawCallback_SetSamplerLinear) draws->AddCallback(linear, nullptr);
    draws->AddImageRounded(found->texture->GetTexRef(), min, max, ImVec2(cropX, cropY), ImVec2(1-cropX, 1-cropY),
        IM_COL32(255, 255, 255, int(std::clamp(opacity, 0.f, 1.f) * 255)), rounding);
    return true;
}
void ReleaseCovers() {
    for (auto& cover : covers) ImGui::UnregisterUserTexture(cover.texture.get());
    covers.clear();
}
}
