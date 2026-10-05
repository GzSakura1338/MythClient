#include "ui/backend/Dx11Overlay.h"
#include "chrome/BrowserService.h"
#include "ui/workspace/ForegroundWorkspace.h"
#include "ui/clickgui/browser/BrowserPage.h"
#include "imgui_impl_dx11.h"
#include "ui/clickgui/Fonts.h"
#include "ui/backend/blur/Blur.h"
#include "ui/skin/SkinTextures.h"
#include "ui/hud/musichud/CoverTexture.h"
#include "bind/InputManager.h"
#include "bind/UnloadKey.h"
#include <algorithm>
#include <windowsx.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);
namespace nova::ui {
namespace {
std::atomic<Dx11Overlay*> active{nullptr};
std::atomic<unsigned> windowCalls{0};
struct WindowCall {
    WindowCall() { ++windowCalls; }
    ~WindowCall() { --windowCalls; }
};
struct Lock {
    SRWLOCK& lock;
    explicit Lock(SRWLOCK& value) : lock(value) { AcquireSRWLockExclusive(&lock); }
    ~Lock() { ReleaseSRWLockExclusive(&lock); }
};
struct Context {
    ImGuiContext* previous = ImGui::GetCurrentContext();
    explicit Context(ImGuiContext* value) { ImGui::SetCurrentContext(value); }
    ~Context() { ImGui::SetCurrentContext(previous); }
};
bool InputMessage(UINT message) {
    return (message >= WM_MOUSEFIRST && message <= WM_MOUSELAST) ||
           (message >= WM_KEYFIRST && message <= WM_KEYLAST) ||
           message == WM_SETFOCUS || message == WM_KILLFOCUS || message == WM_MOUSELEAVE;
}
int BindingKey(WPARAM key, LPARAM flags) {
    if (key == VK_CONTROL) return flags & (1LL << 24) ? VK_RCONTROL : VK_LCONTROL;
    if (key == VK_MENU) return flags & (1LL << 24) ? VK_RMENU : VK_LMENU;
    if (key == VK_SHIFT) {
        const auto resolved = MapVirtualKeyW(static_cast<UINT>((flags >> 16) & 255), MAPVK_VSC_TO_VK_EX);
        return resolved ? static_cast<int>(resolved) : VK_SHIFT;
    }
    return static_cast<int>(key);
}
}
void Dx11Overlay::Enqueue(Message message) {
    Lock lock(messagesLock_);
    if (messageCount_ == messages_.size()) messageCount_ = 0;
    messages_[messageCount_++] = message;
}
void Dx11Overlay::DrainMessages() {
    std::array<Message, 512> pending;
    size_t count;
    { Lock lock(messagesLock_); count = messageCount_; std::copy_n(messages_.begin(), count, pending.begin()); messageCount_ = 0; }
    for (size_t i = 0; i < count; ++i) {
        const auto& m = pending[i];
        ImGui_ImplWin32_WndProcHandler(m.window, m.message, m.w, m.l);
        auto& input = nova::bind::InputManager::Get();
        if (m.message == WM_KEYDOWN || m.message == WM_KEYUP || m.message == WM_SYSKEYDOWN || m.message == WM_SYSKEYUP) {
            const bool down = m.message == WM_KEYDOWN || m.message == WM_SYSKEYDOWN;
            const int key = BindingKey(m.w, m.l);
            if (key <= 0 || key >= static_cast<int>(keysDown_.size())) continue;
            if (down && ((m.l & (1LL << 30)) || keysDown_[key])) continue;
            keysDown_[key] = down;
            const bool pass = input.onKey(key, down);
            if (down && pass && callbacks_.key) callbacks_.key(static_cast<unsigned>(m.w));
        } else if (m.message == WM_KILLFOCUS) {
            for (size_t k = 0; k < keysDown_.size(); ++k) if (keysDown_[k]) { input.onKey(static_cast<int>(k), false); keysDown_[k] = false; }
            for (size_t b = 0; b < mouseDown_.size(); ++b) if (mouseDown_[b]) { input.onMouseButton(static_cast<int>(b), false); mouseDown_[b] = false; }
        } else if (m.message == WM_MOUSEWHEEL || m.message == WM_MOUSEHWHEEL) {
            const float delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(m.w)) / WHEEL_DELTA;
            input.onMouseScroll(m.message == WM_MOUSEHWHEEL ? delta : 0, m.message == WM_MOUSEWHEEL ? delta : 0);
        } else if (m.message == WM_CHAR || m.message == WM_SYSCHAR) {
            input.onChar(static_cast<char>(m.w & 255));
        } else {
            int button = -1; bool down = false;
            switch (m.message) {
            case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK: down = true; [[fallthrough]];
            case WM_LBUTTONUP: button = 0; break;
            case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK: down = true; [[fallthrough]];
            case WM_RBUTTONUP: button = 1; break;
            case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK: down = true; [[fallthrough]];
            case WM_MBUTTONUP: button = 2; break;
            case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK: down = true; [[fallthrough]];
            case WM_XBUTTONUP: button = GET_XBUTTON_WPARAM(m.w) == XBUTTON1 ? 3 : 4; break;
            }
            if (button >= 0 && (!down || !mouseDown_[button])) {
                mouseDown_[button] = down; input.onMouseButton(button, down);
            }
        }
    }
}
LRESULT CALLBACK Dx11Overlay::WindowProc(HWND window, UINT message, WPARAM w, LPARAM l, UINT_PTR, DWORD_PTR) {
    WindowCall call;
    auto* self = active.load(std::memory_order_acquire);
    if (!self) return DefSubclassProc(window, message, w, l);
    if (self->callbacks_.unload) {
        int key = 0; bool down = false;
        switch (message) {
        case WM_KEYDOWN: case WM_SYSKEYDOWN:
            if (!(l & (1LL << 30))) { key = BindingKey(w, l); down = true; }
            break;
        case WM_KEYUP: case WM_SYSKEYUP: key = BindingKey(w, l); break;
        case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK: down = true; [[fallthrough]];
        case WM_LBUTTONUP: key = -1; break;
        case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK: down = true; [[fallthrough]];
        case WM_RBUTTONUP: key = -2; break;
        case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK: down = true; [[fallthrough]];
        case WM_MBUTTONUP: key = -3; break;
        case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK: down = true; [[fallthrough]];
        case WM_XBUTTONUP: key = GET_XBUTTON_WPARAM(w) == XBUTTON1 ? -4 : -5; break;
        }
        if (key && bind::unload::Input(key, down)) {
            self->callbacks_.unload();
            return 0;
        }
    }
    if(workspace::WindowMessage(message,w,l))return 0;
    const bool capture = message != WM_NCDESTROY && self->callbacks_.captureInput && self->callbacks_.captureInput();
    const bool browserConsumed=capture&&chrome::WindowMessage(window,message,w,l);
    // Releases must still reach hold bindings when focus moved to the browser
    // after the corresponding key-down. The normal capture path blocks the game.
    if(browserConsumed&&message!=WM_KEYUP&&message!=WM_SYSKEYUP)return 0;
    if (self->callbacks_.windowCapture) self->callbacks_.windowCapture(capture);
    self->captured_.store(capture);
    if (message == WM_NCDESTROY) self->surface_.Destroyed(window);
    if (InputMessage(message)) self->Enqueue({window, message, w, l});
    if (capture) {
        if (message == WM_SETCURSOR) { SetCursor(LoadCursorW(nullptr, MAKEINTRESOURCEW(32512))); return TRUE; }
        if (message == WM_INPUT) return DefWindowProcW(window, message, w, l);
        if ((message >= WM_MOUSEFIRST && message <= WM_MOUSELAST) ||
            (message >= WM_KEYFIRST && message <= WM_KEYLAST)) return 0;
    }
    return DefSubclassProc(window, message, w, l);
}
bool Dx11Overlay::WindowCallbacksDrained() { return windowCalls.load() == 0; }
void Dx11Overlay::ReleaseTarget() {
    blur::Publish(0); blur::ClearLayerBackdrop();
    blur_.Resize();
    layerBlur_.Resize();
    if (target_) target_->Release();
    target_ = nullptr;
    targetWidth_ = targetHeight_ = 0;
}
void Dx11Overlay::ReleaseDevice() {
    ReleaseTarget();
    blur_.Release();
    layerBlur_.Release();
    if (context_ && ImGui::GetIO().BackendRendererUserData) ImGui_ImplDX11_Shutdown();
    if (deviceContext_) deviceContext_->Release();
    if (device_) device_->Release();
    deviceContext_ = nullptr; device_ = nullptr; chain_ = nullptr;
}
bool Dx11Overlay::BindDevice(IDXGISwapChain* chain) {
    ID3D11Device* device = nullptr;
    if (FAILED(chain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&device)))) return false;
    if (device != device_) {
        ReleaseDevice();
        device_ = device;
        device_->GetImmediateContext(&deviceContext_);
        if (!ImGui_ImplDX11_Init(device_, deviceContext_)) { ReleaseDevice(); return false; }
    } else device->Release();
    if (chain_ != chain) { ReleaseTarget(); chain_ = chain; }
    if (!target_) {
        ID3D11Texture2D* buffer = nullptr;
        if (FAILED(chain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&buffer)))) return false;
        D3D11_TEXTURE2D_DESC texture{};
        buffer->GetDesc(&texture);
        const HRESULT result = device_->CreateRenderTargetView(buffer, nullptr, &target_);
        buffer->Release();
        if (FAILED(result)) return false;
        targetWidth_ = texture.Width;
        targetHeight_ = texture.Height;
    }
    return true;
}
bool Dx11Overlay::Render(IDXGISwapChain* chain) {
    Lock lock(renderLock_);
    if (stopping_ || failed_.load()) return false;
    DXGI_SWAP_CHAIN_DESC desc{};
    if (!chain || FAILED(chain->GetDesc(&desc)) || !IsWindow(desc.OutputWindow)) return false;
    DWORD process = 0; GetWindowThreadProcessId(desc.OutputWindow, &process);
    if (process != GetCurrentProcessId()) return false;
    if (surface_.Window() && IsWindow(surface_.Window()) && surface_.Window() != desc.OutputWindow) return false;
    if (!context_) {
        auto* previous = ImGui::GetCurrentContext();
        context_ = ImGui::CreateContext();
        started_.store(true);
        ImGui::GetIO().IniFilename = nullptr;
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
        InitializeFatalMenuFonts();
        ImGui::SetCurrentContext(previous);
        active.store(this, std::memory_order_release);
    }
    Context context(context_);
    if (!surface_.Bind(desc.OutputWindow, WindowProc, true)) return false;
    if (!BindDevice(chain)) return false;
    if (!initialized_) {
        if (callbacks_.initialize && !callbacks_.initialize()) { failed_.store(true); return false; }
        initialized_ = true;
    }
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    DrainMessages();
    auto& io = ImGui::GetIO();
    io.DisplayFramebufferScale = {1, 1};
    if (io.DisplaySize.x <= 0 || io.DisplaySize.y <= 0 || !targetWidth_ || !targetHeight_) {
        blur::Publish(0);
        return false;
    }
    // Preserve HWND coordinates for layout and both Win32 mouse input paths.
    // The DX11 viewport, scissor rectangles and ESP pixel coverage convert to
    // actual backbuffer pixels with this per-axis ratio.  GPU/display stretch
    // then applies equally to the game and overlay.  A Present-time RS viewport
    // may belong to post-processing, so it is not a reliable world viewport.
    // Read the texture dimensions when acquiring the RTV; ResizeBuffers and
    // device/swap-chain replacement invalidate them together with the target.
    io.DisplayFramebufferScale = {targetWidth_ / io.DisplaySize.x, targetHeight_ / io.DisplaySize.y};
    ImGui::NewFrame();
    const auto options = blur::GetOptions();
    blur::Publish(blur_.Render(device_, deviceContext_, chain, options));
    blur::SetLayerBackdrop(layerBlur_.Prepare(device_, chain, options), false, CaptureLayer, this);
    if (callbacks_.draw) callbacks_.draw();
    ImGui::Render();
    ID3D11RenderTargetView* targets[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT]{};
    ID3D11DepthStencilView* depth = nullptr;
    deviceContext_->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, targets, &depth);
    deviceContext_->OMSetRenderTargets(1, &target_, nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    deviceContext_->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, targets, depth);
    for (auto* target : targets) if (target) target->Release();
    if (depth) depth->Release();
    const bool capture = callbacks_.captureInput && callbacks_.captureInput();
    if (capture != captured_.load()) PostMessageW(desc.OutputWindow, WM_NULL, 0, 0);
    return true;
}
void Dx11Overlay::CaptureLayer(const ImDrawList*, const ImDrawCmd* command) {
    auto* self = static_cast<Dx11Overlay*>(command->UserCallbackData);
    self->layerBlur_.Render(self->device_, self->deviceContext_, self->chain_, blur::GetOptions(), true);
}
void Dx11Overlay::BeforeResize(IDXGISwapChain* chain) {
    Lock lock(renderLock_);
    if (chain == chain_) ReleaseTarget();
}
bool Dx11Overlay::Shutdown() {
    Lock lock(renderLock_);
    if (!context_) return true;
    Context context(context_);
    if (!stopping_) {
        stopping_ = true;
        workspace::Reset(); // Release detached capture before waiting for the HWND callback.
        if (initialized_ && callbacks_.stop) callbacks_.stop();
    }
    if (captured_.load() && IsWindow(surface_.Window())) {
        PostMessageW(surface_.Window(), WM_NULL, 0, 0);
        return false;
    }
    if (!surface_.Reset()) return false;
    active.store(nullptr, std::memory_order_release);
    ReleaseDevice();
    blur::Publish(0);
    skins::Release(); music::ReleaseCovers();
    browser::ReleaseTextures();
    // Do not restore a context that this teardown just destroyed.
    if (context.previous == context_) context.previous = nullptr;
    ImGui::DestroyContext(context_); context_ = nullptr;
    ResetFatalMenuFonts();
    initialized_ = false;
    return true;
}
}
