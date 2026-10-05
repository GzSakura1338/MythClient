#include "minecraft/modules/misc/imefix/ImeFix.h"
#include "minecraft/modules/misc/imefix/ChatInput.h"
#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/compat/Profile.h"
#include "ui/clickgui/ClickGui.h"
#include "chrome/BrowserService.h"
#include <atomic>
#include <mutex>

namespace nova::module::imefix {
namespace {
class ImeFixModule;
std::atomic<ImeFixModule*> instance{nullptr};

// This adapter owns all local references in one frame and never attaches an
// arbitrary native window thread to the JVM. A pre-existing game exception is
// untouched. Only exceptions produced within our own JNI calls are cleared.
struct LocalFrame {
    JNIEnv* env = nullptr;
    LocalFrame() {
        JNIEnv* candidate = nullptr;
        if (!g_vm || g_vm->GetEnv(reinterpret_cast<void**>(&candidate), JNI_VERSION_1_6) != JNI_OK ||
            !candidate || candidate->ExceptionCheck()) return;
        if (candidate->PushLocalFrame(32) != JNI_OK) {
            if (candidate->ExceptionCheck()) candidate->ExceptionClear();
            return;
        }
        env = candidate;
    }
    ~LocalFrame() {
        if (!env) return;
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
            LogTo("IME Fix: JNI operation failed; original input retained where possible");
        }
        env->PopLocalFrame(nullptr);
    }
};

class ImeFixModule final : public Module {
public:
    ImeFixModule() : Module("IME Fix", Category::Misc, "General", "General") { instance.store(this); }
    ~ImeFixModule() override { instance.store(nullptr); onDisable(); }

    bool Message(HWND window, UINT message, WPARAM wp, LPARAM lp) {
        if (!enabled_.load(std::memory_order_acquire) || !minecraft::input::ImeInput::Relevant(message)) return false;
        std::lock_guard lock(mutex_);
        if (!enabled_.load() || !ready_) return false;
        // Both checks matter: HWND ownership alone doesn't imply the game thread.
        if (GetWindowThreadProcessId(window, nullptr) != GetCurrentThreadId()) return false;
        LocalFrame frame;
        if (!frame.env || !chat_.OnClientThread(frame.env)) {
            if (!threadWarning_) { threadWarning_ = true; LogTo("IME Fix: window is not on the client thread; passthrough"); }
            return false;
        }
        bool changed = false;
        const bool available = GetForegroundWindow() == window && !clickgui::CapturesInput() &&
            !chrome::Focused() && chat_.RefreshTarget(frame.env, changed);
        if (changed || !available) input_.InvalidateTarget();
        if (!available) {
            chat_.ClearTarget(frame.env);
        }
        if (available && message == WM_IME_STARTCOMPOSITION && compositionReports_++ < 3)
            LogTo("IME Fix: IME composition started on the client window thread");
        return input_.Handle(window, message, wp, lp, available,
            [&](HWND handle, std::u16string& text) {
                const bool read = minecraft::input::ImeInput::ReadCommittedText(handle, text);
                if (!read && readWarnings_++ < 3)
                    LogTo("IME Fix: no valid IMM Unicode result; original input retained");
                return read;
            },
            [&](std::u16string_view text) { return available && chat_.Insert(frame.env, text); });
    }

    std::string suffix() const override { return ready_ ? "" : "Unavailable"; }
private:
    void onEnable() override {
        std::lock_guard lock(mutex_);
        attempted_ = threadWarning_ = false;
        readWarnings_ = 0;
        compositionReports_ = 0;
        enabled_.store(true, std::memory_order_release);
    }
    void onDisable() override {
        enabled_.store(false, std::memory_order_release);
        std::lock_guard lock(mutex_);
        input_.Reset();
        JniAttach env;
        if (env) chat_.Release(env.env);
        ready_ = false;
    }
    void onUpdate() override {
        std::lock_guard lock(mutex_);
        LocalFrame frame;
        if (!frame.env) return;
        if (!attempted_) {
            attempted_ = true;
            ready_ = (compat::IsLunar12111() || compat::IsLunar189()) && chat_.Resolve(frame.env);
            if (!ready_) LogTo("IME Fix: adapter unavailable; disable/enable to retry");
        }
        if (!ready_ || !chat_.OnClientThread(frame.env)) return;
        bool changed = false;
        if (clickgui::CapturesInput() || chrome::Focused() || !chat_.RefreshTarget(frame.env, changed)) {
            input_.InvalidateTarget();
            chat_.ClearTarget(frame.env);
        } else if (changed) input_.InvalidateTarget();
    }
    std::atomic<bool> enabled_{false};
    // JNI editing can run Lunar responders synchronously. Allow same-thread
    // reentry, while module teardown waits for in-flight window work.
    std::recursive_mutex mutex_;
    std::atomic<bool> ready_{false};
    bool attempted_ = false, threadWarning_ = false;
    unsigned readWarnings_ = 0;
    unsigned compositionReports_ = 0;
    ChatInput chat_;
    minecraft::input::ImeInput input_;
};
}

bool WindowMessage(HWND window, UINT message, WPARAM wp, LPARAM lp) {
    auto* module = instance.load(std::memory_order_acquire);
    if (!module || IsCircuitOpen()) return false;
    try { return module->Message(window, message, wp, lp); }
    catch (...) {
        // Do not let allocations in input handling escape into GLFW/Java.
        LogTo("IME Fix: native input exception; message passed through");
        return false;
    }
}
}

namespace nova::module {
std::unique_ptr<Module> MakeImeFixModule() { return std::make_unique<imefix::ImeFixModule>(); }
}
