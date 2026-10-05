#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <algorithm>

namespace nova::chrome {
inline constexpr wchar_t RuntimeVersion[] = L"webview2-154.0.4258.37-v1";
inline constexpr uint32_t ProtocolVersion = 1, Magic = 0x4E425257;
inline constexpr int MaxWidth = 1920, MaxHeight = 1080, MaxTabs = 4;
inline constexpr size_t MaxPixels = size_t(MaxWidth)*MaxHeight*4, QueueSize = 128;
enum class Op : uint32_t { Navigate, Back, Forward, Reload, Resize, Visible, Focus,
    MouseMove, MouseButton, Wheel, Key, Character, Ime, ImeCommit, ImeCancel,
    Zoom, Fps, BackgroundAudio, NewTab, SelectTab, CloseTab, Edit, Shutdown };
struct Command {
    Op op{}; int a{},b{},c{},d{},modifiers{};
    wchar_t text[2048]{};
};
struct TabInfo { int id{}; wchar_t title[160]{}; };
struct Status {
    int ready{}, loading{}, back{}, forward{}, count{}, selected{}, cursor{};
    int caretX{},caretY{},caretHeight{};
    TabInfo tabs[MaxTabs]{};
    wchar_t url[2048]{}, error[256]{};
};
// Single complete latest-frame mailbox, guarded by a named mutex. Neither
// producer nor consumer ever waits on the game's render thread. Dropped frames
// are safe because every publication contains the complete composed image.
struct Shared {
    uint32_t magic{},version{},read{},write{};
    Command commands[QueueSize]{};
    Status status{};
    uint64_t sequence{};
    int width{},height{};
    unsigned char pixels[MaxPixels];
};
template<size_t N> void CopyText(wchar_t (&out)[N], std::wstring_view text) {
    const auto n=std::min(text.size(),N-1);std::copy_n(text.data(),n,out);out[n]=0;
}
inline bool ValidSize(int w,int h) {return w>0 && h>0 && w<=MaxWidth && h<=MaxHeight;}
inline bool AllowedUrl(std::wstring_view u) {
    return u==L"about:blank" || u.starts_with(L"https://") || u.starts_with(L"http://");
}
}
