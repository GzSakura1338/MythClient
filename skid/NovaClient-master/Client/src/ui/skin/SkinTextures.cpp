#include "ui/skin/SkinTextures.h"
#include "imgui_internal.h"
#include <mutex>
#include <memory>
#include <unordered_map>
#include <cstring>

namespace nova::ui::skins {
namespace {
constexpr std::uint64_t owned = 1ull << 63;
struct Skin {std::string key;int width{},height{};std::vector<std::uint32_t> pixels;std::unique_ptr<ImTextureData> texture;};
std::mutex mutex;
std::unordered_map<std::uint64_t,Skin> images;
std::unordered_map<std::string,std::uint64_t> keys;
std::uint64_t sequence=0;
}
std::uint64_t Find(const std::string& key){std::lock_guard lock(mutex);auto it=keys.find(key);return it==keys.end()?0:it->second;}
std::uint64_t Store(const std::string& key,int width,int height,std::vector<std::uint32_t> pixels){
    if(width<=0||height<=0||width>1024||height>1024||pixels.size()!=static_cast<size_t>(width)*height)return 0;
    std::lock_guard lock(mutex);if(auto it=keys.find(key);it!=keys.end())return it->second;
    // Stable handles are retained while snapshots may reference them; bound total memory.
    if(images.size()>=256)return 0;
    auto id=owned|++sequence;images.emplace(id,Skin{key,width,height,std::move(pixels),{}});keys.emplace(key,id);return id;
}
ImTextureRef Resolve(std::uint64_t handle){
    if(!(handle&owned))return ImTextureRef(static_cast<ImTextureID>(handle));
    std::lock_guard lock(mutex);auto it=images.find(handle);if(it==images.end()||!ImGui::GetCurrentContext())return {};
    auto& skin=it->second;
    if(!skin.texture){
        skin.texture=std::make_unique<ImTextureData>();skin.texture->Create(ImTextureFormat_RGBA32,skin.width,skin.height);
        std::memcpy(skin.texture->GetPixels(),skin.pixels.data(),skin.pixels.size()*sizeof(std::uint32_t));
        ImGui::RegisterUserTexture(skin.texture.get());
    }
    return skin.texture->GetTexRef();
}
void Release(){
    std::lock_guard lock(mutex);
    for(auto& [id,skin]:images)if(skin.texture&&ImGui::GetCurrentContext())ImGui::UnregisterUserTexture(skin.texture.get());
    images.clear();keys.clear();
}
}
