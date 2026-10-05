#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <atomic>
#include <array>
#include "ui/backend/Win32Surface.h"
#include "ui/backend/blur/Dx11Blur.h"

namespace nova::ui {
struct Dx11Callbacks {
    bool (*initialize)() = nullptr;
    void (*draw)() = nullptr;
    void (*stop)() = nullptr;
    void (*key)(unsigned key) = nullptr;
    bool (*captureInput)() = nullptr;
    // Executed on the window thread, including restoration before detaching.
    void (*windowCapture)(bool capture) = nullptr;
    // Emergency unload is thread-safe and independent of UI/bind capture and
    // Present cadence. Invoked on the HWND owner thread for the unload binding.
    void (*unload)() = nullptr;
};

// One overlay per runtime. No game interfaces or game memory in the renderer.
// Render/Shutdown run on the presenting thread; messages are queued by HWND's
// owner thread and consumed there, so ImGui never runs on two threads at once.
class Dx11Overlay {
public:
    explicit Dx11Overlay(Dx11Callbacks callbacks) : callbacks_(callbacks) {}
    bool Render(IDXGISwapChain* chain);
    void BeforeResize(IDXGISwapChain* chain);
    bool Shutdown();
    bool Started() const { return started_.load(); }
    bool Failed() const { return failed_.load(); }
    static bool WindowCallbacksDrained();
private:
    struct Message { HWND window; UINT message; WPARAM w; LPARAM l; };
    static LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
    void Enqueue(Message message);
    void DrainMessages();
    bool BindDevice(IDXGISwapChain* chain);
    void ReleaseDevice();
    void ReleaseTarget();
    static void CaptureLayer(const ImDrawList*, const ImDrawCmd*);
    Dx11Callbacks callbacks_;
    ImGuiContext* context_ = nullptr;
    nova::glhook::Win32Surface surface_;
    IDXGISwapChain* chain_ = nullptr; // identity only; the game owns its lifetime
    ID3D11Device* device_ = nullptr;
    ID3D11DeviceContext* deviceContext_ = nullptr;
    ID3D11RenderTargetView* target_ = nullptr;
    UINT targetWidth_ = 0, targetHeight_ = 0;
    blur::Dx11Blur blur_;
    blur::Dx11Blur layerBlur_;
    SRWLOCK renderLock_ = SRWLOCK_INIT, messagesLock_ = SRWLOCK_INIT;
    std::array<Message, 512> messages_{};
    size_t messageCount_ = 0;
    std::array<bool, 256> keysDown_{};
    std::array<bool, 5> mouseDown_{};
    bool initialized_ = false, stopping_ = false;
    std::atomic<bool> started_{false}, failed_{false}, captured_{false};
};
}
