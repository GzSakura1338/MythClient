#include "cs2/hooks/Dx11Hooks.h"
#include "cs2/hooks/RadarHook.h"
#include "cs2/hooks/InputHook.h"
#include "cs2/hooks/FovHook.h"
#include "cs2/hooks/TimeHook.h"
#include "cs2/hooks/ChamsHook.h"
#include "cs2/sdk/radar/Radar.h"
#include "core/Instance.h"
#include "core/RuntimeControl.h"
#include "MinHook.h"
#include <atomic>

namespace nova::cs2::hooks {
namespace {
using Present = HRESULT (STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT);
using Resize = HRESULT (STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
struct State {
    ui::Dx11Overlay* overlay = nullptr;
    Present present = nullptr;
    Resize resize = nullptr;
    void* presentTarget = nullptr;
    void* resizeTarget = nullptr;
    std::atomic<unsigned> calls{0};
    std::atomic<bool> stopping{false}, ready{false}, failed{false}, tornDown{false};
    bool ownsMinHook = false;
    bool ownsPresent = false, ownsResize = false;
};
State& Get() { return Instance<State>(); }
struct Call {
    State& state;
    explicit Call(State& value) : state(value) { ++state.calls; }
    ~Call() { --state.calls; }
};
HRESULT STDMETHODCALLTYPE OnPresent(IDXGISwapChain* chain, UINT sync, UINT flags) {
    auto& state = Get(); Call call(state);
    // Test-only presents do not actually submit a frame.
    if (!(flags & DXGI_PRESENT_TEST)) {
        try {
            if (state.stopping.load()) {
                if (state.overlay->Shutdown()) state.tornDown.store(true);
            } else {
                if (state.overlay->Render(chain)) state.ready.store(true);
                if (state.overlay->Failed()) state.failed.store(true);
            }
        } catch (...) {
            state.failed.store(true);
            LogTo("CS2: exception in the DX11 UI callback; requesting unload");
            RequestUnload();
        }
    }
    return state.present(chain, sync, flags);
}
HRESULT STDMETHODCALLTYPE OnResize(IDXGISwapChain* chain, UINT count, UINT width, UINT height, DXGI_FORMAT format, UINT flags) {
    auto& state = Get(); Call call(state);
    state.overlay->BeforeResize(chain);
    return state.resize(chain, count, width, height, format, flags);
}
bool FindTargets(void*& present, void*& resize) {
    const HINSTANCE instance = GetModuleHandleW(nullptr);
    const wchar_t* name = L"Nova.CS2.DX11.Probe";
    WNDCLASSW wc{}; wc.hInstance = instance; wc.lpfnWndProc = DefWindowProcW; wc.lpszClassName = name;
    if (!RegisterClassW(&wc)) return false;
    const HWND window = CreateWindowW(name, L"", WS_OVERLAPPED, 0, 0, 8, 8, nullptr, nullptr, instance, nullptr);
    DXGI_SWAP_CHAIN_DESC desc{};
    desc.BufferDesc.Width = desc.BufferDesc.Height = 8;
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1; desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 1; desc.OutputWindow = window; desc.Windowed = TRUE;
    IDXGISwapChain* chain = nullptr; ID3D11Device* device = nullptr; ID3D11DeviceContext* context = nullptr;
    HRESULT result = window ? D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION, &desc, &chain, &device, nullptr, &context) : E_FAIL;
    if (SUCCEEDED(result)) {
        auto** table = *reinterpret_cast<void***>(chain);
        present = table[8]; resize = table[13];
    }
    if (context) context->Release();
    if (device) device->Release();
    if (chain) chain->Release();
    if (window) DestroyWindow(window);
    UnregisterClassW(name, instance);
    return SUCCEEDED(result);
}
}
bool Install(ui::Dx11Callbacks callbacks) {
    auto& state = Get();
    if (state.overlay) return false;
    if (!FindTargets(state.presentTarget, state.resizeTarget)) return false;
    const auto init = MH_Initialize();
    if (init != MH_OK && init != MH_ERROR_ALREADY_INITIALIZED) return false;
    state.ownsMinHook = init == MH_OK;
    state.overlay = new ui::Dx11Overlay(callbacks);
    state.ownsPresent = MH_CreateHook(state.presentTarget, reinterpret_cast<void*>(&OnPresent), reinterpret_cast<void**>(&state.present)) == MH_OK;
    if (state.ownsPresent)
        state.ownsResize = MH_CreateHook(state.resizeTarget, reinterpret_cast<void*>(&OnResize), reinterpret_cast<void**>(&state.resize)) == MH_OK;
    if (!state.ownsPresent || !state.ownsResize) {
        Finalize(); return false;
    }
    // Queue both changes before enabling, so initialization cannot see a half-installed pair.
    if (radar::Install(sdk::radar::UpdateTarget())) LogTo("CS2: native radar update hook installed");
    else LogTo("CS2: Radar ESP unavailable; shared UI and other modules remain available");
    if (input::Install(sdk::weapons::ResolveBinding(), sdk::aim::ResolveCapture(), sdk::aim::ResolveShotCapture())) LogTo("CS2: shared final input-angle hook installed for Recoil Control / Aimbot");
    else LogTo("CS2: Recoil Control unavailable; input binding version checks failed");
    if (input::FireAvailable()) LogTo("CS2: Auto Fire native input adapter installed");
    else LogTo("CS2: Auto Fire unavailable: %s", input::FireUnavailableReason());
    if (!input::AimAvailable()) LogTo("CS2: Aimbot unavailable; aim binding version checks failed");
    const auto cameraBinding = sdk::camera::ResolveBinding();
    if (fov::Install(cameraBinding)) LogTo("CS2: final camera setup hook installed for FOV Control");
    else LogTo("CS2: FOV Control unavailable; camera binding version checks failed");
    if (time::Install(sdk::environment::ResolveBinding())) LogTo("CS2: native exposure hook installed for Time");
    else LogTo("CS2: Time unavailable; exposure binding version checks failed");
    if (chams::Install(sdk::chams::ResolveBinding())) LogTo("CS2: scene material hook installed for Chams");
    else LogTo("CS2: Chams unavailable; material/scene version checks or creation failed");
    if (MH_QueueEnableHook(state.presentTarget) != MH_OK || MH_QueueEnableHook(state.resizeTarget) != MH_OK ||
        MH_ApplyQueued() != MH_OK) {
        state.failed.store(true);
        Stop(5000);
        return false;
    }
    LogTo("CS2: DXGI Present/ResizeBuffers hooks installed");
    return true;
}
bool Ready() { return Get().ready.load(); }
bool Failed() { return Get().failed.load(); }
bool Stop(unsigned timeoutMs) {
    auto& state = Get();
    state.stopping.store(true);
    if (!chams::Stop(timeoutMs)) return false;
    if (!time::Stop(timeoutMs)) return false;
    if (!fov::Stop(timeoutMs)) return false;
    if (!input::Stop(timeoutMs)) return false;
    if (!radar::Stop(timeoutMs)) return false;
    const auto deadline = GetTickCount64() + timeoutMs;
    while (state.overlay && state.overlay->Started() && !state.tornDown.load()) {
        if (GetTickCount64() >= deadline) return false;
        Sleep(10);
    }
    for (void* target : {state.ownsPresent ? state.presentTarget : nullptr, state.ownsResize ? state.resizeTarget : nullptr}) {
        if (!target) continue;
        const auto status = MH_DisableHook(target);
        if (status != MH_OK && status != MH_ERROR_DISABLED && status != MH_ERROR_NOT_CREATED) return false;
    }
    while (state.calls.load() || !ui::Dx11Overlay::WindowCallbacksDrained()) {
        if (GetTickCount64() >= deadline) return false;
        Sleep(1);
    }
    // A callback may have entered Render just before stopping was published.
    if (state.overlay && state.overlay->Started() && !state.tornDown.load()) return false;
    return true;
}
void Finalize() {
    auto& state = Get();
    chams::Finalize();
    time::Finalize();
    fov::Finalize();
    input::Finalize();
    radar::Finalize();
    if (state.ownsPresent) MH_RemoveHook(state.presentTarget);
    if (state.ownsResize) MH_RemoveHook(state.resizeTarget);
    if (state.ownsMinHook) MH_Uninitialize();
    state.ownsPresent = state.ownsResize = state.ownsMinHook = false;
    state.presentTarget = state.resizeTarget = nullptr;
    delete state.overlay; state.overlay = nullptr;
    state.present = nullptr; state.resize = nullptr;
    state.stopping.store(false); state.ready.store(false);
    state.failed.store(false); state.tornDown.store(false);
}
}
