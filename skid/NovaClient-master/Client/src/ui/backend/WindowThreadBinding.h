#pragma once
#include <windows.h>
#include <commctrl.h>
#include <atomic>

namespace nova::glhook {
// Ixeris owns its HWND on a dedicated message thread. Subclass helpers must
// execute there, not on the GL thread. Commands are asynchronous so the two
// game threads never wait on one another. The owner must keep this object and
// the DLL alive until Detach() reports completion.
class WindowThreadBinding {
public:
    bool Attach(HWND window, SUBCLASSPROC callback, UINT_PTR id, DWORD_PTR data) {
        window_ = window; callback_ = callback; id_ = id; data_ = data;
        stop_.store(false);
        return Queue(State::Installing);
    }
    bool Installed() const { return state_.load(std::memory_order_acquire) == State::Installed; }
    bool Pending() const {
        const auto state = state_.load(std::memory_order_acquire);
        return state == State::Installing || state == State::Removing;
    }
    bool Detach() {
        stop_.store(true);
        if (Pending()) return false;
        if (attached_.load(std::memory_order_acquire)) {
            Queue(State::Removing);
            return false;
        }
        if (window_ && GetPropW(window_, Property()) == this) RemovePropW(window_, Property());
        window_ = nullptr;
        return true;
    }
    void Destroyed() { Complete(true); }
    DWORD Error() const { return error_.load(); }
private:
    enum class State { Idle, Installing, Installed, Removing, Removed, Failed };
    static const wchar_t* Property() { return L"Nova.EaseCation.WindowThreadBinding"; }
    static LRESULT CALLBACK Hook(int code, WPARAM wp, LPARAM lp) {
        if (code >= 0) {
            const auto* message = reinterpret_cast<const CWPSTRUCT*>(lp);
            auto* self = static_cast<WindowThreadBinding*>(GetPropW(message->hwnd, Property()));
            if (self && ((message->message == self->message_ && message->wParam == reinterpret_cast<WPARAM>(self)) ||
                         message->message == WM_NCDESTROY)) {
                self->Complete(message->message == WM_NCDESTROY);
            }
        }
        return CallNextHookEx(nullptr, code, wp, lp);
    }
    bool Queue(State command) {
        DWORD process = 0;
        const DWORD thread = GetWindowThreadProcessId(window_, &process);
        if (!thread || process != GetCurrentProcessId()) { state_.store(State::Failed); return false; }
        if (!message_) message_ = RegisterWindowMessageW(L"Nova.EaseCation.SubclassCommand.v1");
        const HANDLE previous = GetPropW(window_, Property());
        if (!message_ || (previous && previous != this) || !SetPropW(window_, Property(), this)) {
            error_.store(GetLastError()); state_.store(State::Failed); return false;
        }
        HHOOK hook = SetWindowsHookExW(WH_CALLWNDPROC, Hook, nullptr, thread);
        if (!hook) { error_.store(GetLastError()); state_.store(State::Failed); return false; }
        hook_.store(hook);
        state_.store(command, std::memory_order_release);
        if (SendNotifyMessageW(window_, message_, reinterpret_cast<WPARAM>(this), 0)) return true;
        error_.store(GetLastError());
        if (auto active = hook_.exchange(nullptr)) UnhookWindowsHookEx(active);
        state_.store(State::Failed, std::memory_order_release);
        return false;
    }
    void Complete(bool destroyed) {
        const auto state = state_.load(std::memory_order_acquire);
        if (!destroyed && state != State::Installing && state != State::Removing) return;
        bool ok = true;
        if (destroyed) attached_.store(false);
        else if (stop_.load() || state == State::Removing) {
            if (attached_.load()) {
                DWORD_PTR ignored = 0;
                ok = RemoveWindowSubclass(window_, callback_, id_) ||
                     !GetWindowSubclass(window_, callback_, id_, &ignored);
                if (ok) attached_.store(false);
            }
        } else {
            ok = SetWindowSubclass(window_, callback_, id_, data_) != FALSE;
            attached_.store(ok);
        }
        if (!ok) error_.store(GetLastError());
        if (auto active = hook_.exchange(nullptr)) UnhookWindowsHookEx(active);
        if (!attached_.load()) RemovePropW(window_, Property());
        state_.store(!ok ? State::Failed : attached_.load() ? State::Installed : State::Removed,
                     std::memory_order_release);
    }
    HWND window_ = nullptr;
    SUBCLASSPROC callback_ = nullptr;
    UINT_PTR id_ = 0;
    DWORD_PTR data_ = 0;
    UINT message_ = 0;
    std::atomic<HHOOK> hook_{nullptr};
    std::atomic<State> state_{State::Idle};
    std::atomic<bool> stop_{false}, attached_{false};
    std::atomic<DWORD> error_{0};
};
}
