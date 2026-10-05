#pragma once
#include "ui/backend/GlExt.h"

namespace nova::glhook {
// Render-thread ownership only. An HDC may change while its HGLRC stays alive
// (LWJGL fullscreen); update it even when no GPU rebuild is necessary.
struct OpenGLSurface {
    HGLRC context = nullptr;
    HDC dc = nullptr;
    HWND window = nullptr;

    void Capture() {
        context = wglGetCurrentContext();
        dc = wglGetCurrentDC();
        window = WindowFromDC(dc);
    }

    // cleanup receives true only while the resource-owning context is current.
    // A destroyed context is abandoned, never deleted through the new context.
    template<class Cleanup>
    bool Retire(Cleanup cleanup) {
        const HGLRC incomingContext = wglGetCurrentContext();
        const HDC incomingDC = wglGetCurrentDC();
        const bool alreadyCurrent = context && context == incomingContext;
        const bool attempted = !alreadyCurrent && context && dc &&
            IsWindow(window) && WindowFromDC(dc) == window;
        const bool available = alreadyCurrent || (attempted && wglMakeCurrent(dc, context));
        // Restore even if binding the old context failed: WGL may have unbound
        // the incoming context on failure.
        struct Restore {
            bool needed; HDC dc; HGLRC context;
            ~Restore() { if (needed) wglMakeCurrent(dc, context); }
        } restore{attempted, incomingDC, incomingContext};
        cleanup(available);
        *this = {};
        if (!attempted) return true;
        const bool restored = wglMakeCurrent(incomingDC, incomingContext) != FALSE;
        restore.needed = !restored;
        return restored;
    }
};
}
