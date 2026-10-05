#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <commctrl.h>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "ui/backend/WindowThreadBinding.h"

namespace nova::glhook {
// The ImGui context survives window recreation. Only the platform backend and
// input subclass belong to the native window. Common-controls subclassing keeps
// other overlays' window procedures intact when this binding is removed.
class Win32Surface {
public:
    HWND Window() const { return window_; }
    bool Pending() const {
        // Installing the subclass and initializing ImGui are separate steps.
        // The window thread may finish between Bind() and the renderer's
        // readiness check. A successful subclass still needs the next render
        // thread Bind() to initialize the platform; it is not a failed bind.
        return threadedBinding_ && (threaded_.Pending() ||
            (!platformAttempted_ && threaded_.Installed()));
    }
    DWORD Error() const { return threadedBinding_ ? threaded_.Error() : GetLastError(); }
    bool Bind(HWND window, SUBCLASSPROC callback, bool allowThreaded = false) {
        if (threadedBinding_ && window == window_ && !destroyed_.load()) {
            if (!threaded_.Installed()) return false;
            if (!platformAttempted_) {
                platformAttempted_ = true;
                platformInitialized_ = ImGui_ImplWin32_Init(window);
            }
            return platformInitialized_;
        }
        if (allowThreaded && GetWindowThreadProcessId(window, nullptr) != GetCurrentThreadId()) {
            if (!Reset()) return false;
            if (!IsWindow(window)) return false;
            window_ = window; callback_ = callback; destroyed_.store(false);
            threadedBinding_ = true;
            threaded_.Attach(window, callback, Id(), reinterpret_cast<DWORD_PTR>(this));
            return false; // The window thread completes installation; poll next frame.
        }
        DWORD_PTR data = 0;
        if (window == window_ && !destroyed_.load() &&
            GetWindowSubclass(window_, callback_, Id(), &data)) return true;
        if (!Reset()) return false;
        if (!IsWindow(window) || !ImGui_ImplWin32_Init(window)) return false;
        if (!SetWindowSubclass(window, callback, Id(), reinterpret_cast<DWORD_PTR>(this))) {
            ImGui_ImplWin32_Shutdown();
            return false;
        }
        window_ = window;
        callback_ = callback;
        destroyed_.store(false);
        platformInitialized_ = true;
        ImGui::GetIO().ClearInputKeys();
        ImGui::GetIO().ClearEventsQueue();
        return true;
    }
    void Destroyed(HWND window) {
        if (window_ == window) {
            destroyed_.store(true);
            if (threadedBinding_) threaded_.Destroyed();
        }
    }
    bool Reset() {
        if (!window_) return true;
        if (threadedBinding_ && !threaded_.Detach()) return false;
        if (!threadedBinding_ && !destroyed_.load() && IsWindow(window_))
            RemoveWindowSubclass(window_, callback_, Id());
        if (platformInitialized_) ImGui_ImplWin32_Shutdown();
        window_ = nullptr;
        callback_ = nullptr;
        destroyed_.store(false);
        threadedBinding_ = false;
        platformInitialized_ = false;
        platformAttempted_ = false;
        return true;
    }
private:
    UINT_PTR Id() const { return reinterpret_cast<UINT_PTR>(this); }
    HWND window_ = nullptr;
    SUBCLASSPROC callback_ = nullptr;
    std::atomic<bool> destroyed_{false};
    WindowThreadBinding threaded_;
    bool threadedBinding_ = false;
    bool platformInitialized_ = false;
    bool platformAttempted_ = false;
};
}
