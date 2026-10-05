#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "cs2/sdk/Interfaces.h"
#include "cs2/sdk/players/Players.h"
#include "core/RuntimeControl.h"

namespace nova::cs2::sdk {
namespace {
Interfaces services;
using GetFocus = void* (*)();
using GetRelative = bool (*)(void*);
using SetRelative = bool (*)(void*, bool);
GetFocus getFocus = nullptr;
GetRelative getRelative = nullptr;
SetRelative setRelative = nullptr;
void* capturedWindow = nullptr;
bool previousRelative = false;
void* Find(const wchar_t* module, const char* name) {
    const auto image = GetModuleHandleW(module);
    using Factory = void* (*)(const char*, int*);
    auto create = image ? reinterpret_cast<Factory>(GetProcAddress(image, "CreateInterface")) : nullptr;
    void* value = create ? create(name, nullptr) : nullptr;
    LogTo("CS2: interface %s %s", name, value ? "resolved" : "unavailable");
    return value;
}
}
bool Initialize() {
    services = {Find(L"schemasystem.dll", "SchemaSystem_001"),
                Find(L"engine2.dll", "Source2EngineToClient001"),
                Find(L"inputsystem.dll", "InputSystemVersion001")};
    if (auto sdl = GetModuleHandleW(L"SDL3.dll")) {
        getFocus = reinterpret_cast<GetFocus>(GetProcAddress(sdl, "SDL_GetKeyboardFocus"));
        getRelative = reinterpret_cast<GetRelative>(GetProcAddress(sdl, "SDL_GetWindowRelativeMouseMode"));
        setRelative = reinterpret_cast<SetRelative>(GetProcAddress(sdl, "SDL_SetWindowRelativeMouseMode"));
    }
    const bool ready = services.schema && services.engine && services.input && getFocus && getRelative && setRelative;
    if (ready) players::Initialize(); // A version mismatch disables ESP, not the shared UI.
    return ready;
}
const Interfaces& Services() { return services; }
void CaptureMenuMouse(bool capture) {
    if (!getFocus || !getRelative || !setRelative) return;
    void* focus = getFocus();
    if (capture && focus) {
        if (capturedWindow != focus) {
            capturedWindow = focus;
            previousRelative = getRelative(focus);
        }
        if (getRelative(focus)) setRelative(focus, false);
        ClipCursor(nullptr);
    } else if (!capture && capturedWindow) {
        // Never call into SDL with a stale window after a window replacement.
        if (focus == capturedWindow) setRelative(focus, previousRelative);
        capturedWindow = nullptr;
    }
}
void Shutdown() {
    players::Shutdown();
    services = {};
    getFocus = nullptr; getRelative = nullptr; setRelative = nullptr;
}
}
