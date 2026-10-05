#pragma once

namespace nova::cs2::sdk {
// Borrowed Source 2 services. Game-specific vtable layouts and entity APIs are
// intentionally not guessed from an old dump. Add them behind this boundary.
struct Interfaces {
    void* schema = nullptr;
    void* engine = nullptr;
    void* input = nullptr;
};
bool Initialize();
const Interfaces& Services();
void Shutdown();
// SDL calls must be made on the window-owning thread.
void CaptureMenuMouse(bool capture);
}
