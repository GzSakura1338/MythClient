#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <atomic>
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <thread>
#include <vector>
#include "cs2/host/Presentation.h"
#include "cs2/hooks/Dx11Hooks.h"
#include "ui/clickgui/ClickGui.h"
#include "chrome/BrowserService.h"
#include "ui/workspace/ForegroundWorkspace.h"
#include "config/BrowserSettings.h"
#include "imgui_internal.h"
#include "ui/clickgui/FeatureTree.h"
#include "ui/clickgui/panel/ClientSettings.h"
#include "config/ConfigManager.h"
#include "cs2/modules/ModuleManager.h"
#include "bind/BindCapture.h"
#include "bind/UnloadKey.h"
#include "ui/backend/blur/Blur.h"
#include "ui/hud/esp2d/Esp2DView.h"
#include "ui/hud/aim/FovCircle.h"
#include "ui/hud/watermark/WatermarkHud.h"

#define CHECK(value) do { if (!(value)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #value); std::exit(1); } } while (false)
namespace {
HWND window = nullptr;
HANDLE windowReady;
DWORD windowThread;
std::atomic<bool> unload{false}, capture{false}, wrongThread{false};
void (*productionDraw)() = nullptr;
void (*workspaceAction)() = nullptr;
bool floatingActive=false;
bool previewEsp = false;
bool previewAim = false;
nova::ui::watermark::WatermarkHud* previewWatermark = nullptr;
nova::ui::watermark::Snapshot previewWatermarkData;
bool probeEspEdge = false;
float probeScale = 1.3333333f, probePhase = 0;
nova::ui::esp2d::RasterMode probeRaster = nova::ui::esp2d::RasterMode::Solid;
nova::ui::esp2d::Layout probeLayout = nova::ui::esp2d::Layout::Legacy;
void DrawPreview() {
    if (previewWatermark) { previewWatermark->Draw(true, previewWatermarkData); return; }
    if (probeEspEdge) {
        using namespace nova::ui::esp2d;
        Draw({(80 + probePhase) / probeScale, 40 / probeScale, 160 / probeScale, 160 / probeScale},
            probeScale, BoxMode::None, HealthMode::Health, 100, 100, 0, false, probeRaster, 100, false, probeLayout);
        return;
    }
    if(workspaceAction){const auto action=workspaceAction;workspaceAction=nullptr;action();}
    productionDraw();
    const auto* floating=ImGui::FindWindowByName("##ChromeForeground");floatingActive=floating&&floating->Active;
    if (previewAim) nova::ui::aim::Draw(nova::ui::aim::Radius(5, 4.f / 3.f, ImGui::GetIO().DisplaySize.y));
    if (!previewEsp) return;
    using namespace nova::ui::esp2d;
    Draw({120, 100, 200, 280}, 2, BoxMode::Normal, HealthMode::Health, 75, 100, 0, true, RasterMode::PixelCoverage, 100, true, Layout::Compact);
    DrawName({120, 100, 200, 280}, 2, "Normal / Armor 0", true);
    Draw({290, 100, 370, 280}, 2, BoxMode::CS, HealthMode::Gradient, 40, 100, 1, true, RasterMode::PixelCoverage, 100, true, Layout::Compact);
    DrawName({290, 100, 370, 280}, 2, "CS / Armor 1", true);
    Draw({460, 100, 540, 280}, 2, BoxMode::Normal, HealthMode::Standard, 90, 100, 100, true, RasterMode::PixelCoverage, 100, true, Layout::Compact, false);
    DrawName({460, 100, 540, 280}, 2, "Outline off / Armor 100", false);
    DrawName({-20, 30, 20, 130}, 2, "A very long player name near the left edge should remain readable", true);
}
LRESULT CALLBACK WindowProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_CLOSE) { DestroyWindow(h); return 0; }
    if (m == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcW(h, m, w, l);
}
void WindowMain() {
    windowThread = GetCurrentThreadId();
    WNDCLASSW wc{}; wc.hInstance = GetModuleHandleW(nullptr); wc.lpszClassName = L"Nova.CS2.Test"; wc.lpfnWndProc = WindowProc;
    CHECK(RegisterClassW(&wc));
    window = CreateWindowW(wc.lpszClassName, L"NOVA DX11 integration test", WS_OVERLAPPEDWINDOW,
        0, 0, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);
    CHECK(window); SetEvent(windowReady);
    MSG message;
    while (GetMessageW(&message, nullptr, 0, 0) > 0) { TranslateMessage(&message); DispatchMessageW(&message); }
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
}
void Capture(bool value) { if (GetCurrentThreadId() != windowThread) wrongThread = true; capture = value; }
double EdgeEnergy(IDXGISwapChain* chain, ID3D11Device* device, ID3D11DeviceContext* context) {
    ID3D11Texture2D* buffer = nullptr; CHECK(SUCCEEDED(chain->GetBuffer(0, IID_PPV_ARGS(&buffer))));
    D3D11_TEXTURE2D_DESC desc{}; buffer->GetDesc(&desc);
    desc.Usage = D3D11_USAGE_STAGING; desc.BindFlags = 0; desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ; desc.MiscFlags = 0;
    ID3D11Texture2D* copy = nullptr; CHECK(SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &copy)));
    context->CopyResource(copy, buffer); buffer->Release();
    D3D11_MAPPED_SUBRESOURCE map{}; CHECK(SUCCEEDED(context->Map(copy, 0, D3D11_MAP_READ, 0, &map)));
    double energy = 0;
    for (int y = 80; y < 88; ++y) for (int x = 64; x < 96; ++x) {
        const auto* pixel = static_cast<const unsigned char*>(map.pData) + y * map.RowPitch + x * 4;
        energy += std::max(0, int(pixel[1]) - std::max(int(pixel[0]), int(pixel[2])));
    }
    context->Unmap(copy, 0); copy->Release(); return energy / 8;
}
void Image(IDXGISwapChain* chain, ID3D11Device* device, ID3D11DeviceContext* context, const std::filesystem::path& file) {
    ID3D11Texture2D* buffer = nullptr; CHECK(SUCCEEDED(chain->GetBuffer(0, IID_PPV_ARGS(&buffer))));
    D3D11_TEXTURE2D_DESC desc{}; buffer->GetDesc(&desc);
    desc.Usage = D3D11_USAGE_STAGING; desc.BindFlags = 0; desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ; desc.MiscFlags = 0;
    ID3D11Texture2D* copy = nullptr; CHECK(SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &copy)));
    context->CopyResource(copy, buffer); buffer->Release();
    D3D11_MAPPED_SUBRESOURCE map{}; CHECK(SUCCEEDED(context->Map(copy, 0, D3D11_MAP_READ, 0, &map)));
    std::vector<unsigned char> pixels(desc.Width * desc.Height * 4);
    size_t changed = 0;
    for (UINT y = 0; y < desc.Height; ++y) for (UINT x = 0; x < desc.Width; ++x) {
        auto* source = static_cast<const unsigned char*>(map.pData) + y * map.RowPitch + x * 4;
        auto* dest = pixels.data() + (y * desc.Width + x) * 4;
        dest[0] = source[2]; dest[1] = source[1]; dest[2] = source[0]; dest[3] = 255;
        if (source[0] > 45 || source[1] > 45 || source[2] > 45) ++changed;
    }
    context->Unmap(copy, 0); copy->Release();
    CHECK(changed > 500);
    BITMAPFILEHEADER header{}; header.bfType = 0x4d42; header.bfOffBits = sizeof(header) + sizeof(BITMAPINFOHEADER);
    header.bfSize = header.bfOffBits + static_cast<DWORD>(pixels.size());
    BITMAPINFOHEADER info{}; info.biSize = sizeof(info); info.biWidth = desc.Width; info.biHeight = -static_cast<LONG>(desc.Height);
    info.biPlanes = 1; info.biBitCount = 32; info.biCompression = BI_RGB;
    std::ofstream out(file, std::ios::binary); out.write(reinterpret_cast<char*>(&header), sizeof(header));
    out.write(reinterpret_cast<char*>(&info), sizeof(info)); out.write(reinterpret_cast<char*>(pixels.data()), pixels.size());
    CHECK(out.good());
}
}
void LogTo(const char* format, ...) { va_list args; va_start(args, format); vprintf(format, args); va_end(args); puts(""); }
void RequestUnload() { unload = true; }
void TestModules();
void TestUnloadBinding();
void TestEsp();
void TestRadar();
void TestStretch();
void TestRecoil();
void TestFov();
void TestTime();
void TestChams();
void TestAim();
void TestAutoFire();
void TestHud();
int ProbeHud(unsigned long pid);
int ProbeEsp(unsigned long pid);
void TestBlur(ID3D11Device*, ID3D11DeviceContext*, IDXGISwapChain*);
int main(int argc, char** argv) {
    if (argc == 2 && std::string_view(argv[1]) == "--recoil-only") { TestRecoil(); return 0; }
    if (argc == 2 && std::string_view(argv[1]) == "--fov-only") { TestFov(); return 0; }
    if (argc == 2 && std::string_view(argv[1]) == "--time-only") { TestTime(); return 0; }
    if (argc == 2 && std::string_view(argv[1]) == "--chams-only") { TestChams(); return 0; }
    if (argc == 2 && std::string_view(argv[1]) == "--hud-only") { TestHud(); return 0; }
    if (argc == 2 && std::string_view(argv[1]) == "--aim-only") { TestAim(); return 0; }
    if (argc == 2 && std::string_view(argv[1]) == "--autofire-only") { TestAutoFire(); return 0; }
    if (argc == 2 && std::string_view(argv[1]) == "--stretch-only") { TestStretch(); return 0; }
    if (argc == 3 && std::string_view(argv[1]) == "--probe-esp") return ProbeEsp(std::strtoul(argv[2], nullptr, 10));
    if (argc == 3 && std::string_view(argv[1]) == "--probe-hud") return ProbeHud(std::strtoul(argv[2], nullptr, 10));
    TestModules();
    TestUnloadBinding();
    TestEsp();
    TestRadar();
    TestStretch();
    TestRecoil();
    TestFov();
    TestTime();
    TestChams();
    TestAim();
    TestAutoFire();
    TestHud();
    const auto output = argc > 1 ? std::filesystem::path(argv[1]) : std::filesystem::temp_directory_path() / L"NovaCs2Smoke";
    std::filesystem::create_directories(output);
    windowReady = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    std::thread messages(WindowMain); CHECK(WaitForSingleObject(windowReady, 5000) == WAIT_OBJECT_0); CloseHandle(windowReady);
    DXGI_SWAP_CHAIN_DESC desc{}; desc.BufferDesc.Width = 1280; desc.BufferDesc.Height = 800;
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; desc.SampleDesc.Count = 1;
    desc.BufferCount = 1; desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; desc.OutputWindow = window;
    desc.Windowed = TRUE; desc.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;
    ID3D11Device* device = nullptr; ID3D11DeviceContext* context = nullptr; IDXGISwapChain* chain = nullptr;
    CHECK(SUCCEEDED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
        D3D11_SDK_VERSION, &desc, &chain, &device, nullptr, &context)));
    TestBlur(device, context, chain);
    nova::cs2::presentation::Configure(output / "config", true);
    auto callbacks = nova::cs2::presentation::Callbacks(); callbacks.windowCapture = Capture;
    productionDraw = callbacks.draw; callbacks.draw = DrawPreview;
    CHECK(nova::cs2::hooks::Install(callbacks));
    ID3D11RenderTargetView* target = nullptr;
    auto createTarget = [&] { ID3D11Texture2D* buffer = nullptr; CHECK(SUCCEEDED(chain->GetBuffer(0, IID_PPV_ARGS(&buffer))));
        CHECK(SUCCEEDED(device->CreateRenderTargetView(buffer, nullptr, &target))); buffer->Release(); };
    createTarget();
    auto frame = [&] {
        const float color[]{probeEspEdge ? 0.f : .08f, probeEspEdge ? 0.f : .1f, probeEspEdge ? 0.f : .14f, 1};
        context->OMSetRenderTargets(1, &target, nullptr); context->ClearRenderTargetView(target, color);
        CHECK(SUCCEEDED(chain->Present(0, 0)));
        ID3D11RenderTargetView* restored = nullptr; context->OMGetRenderTargets(1, &restored, nullptr);
        CHECK(restored == target); if (restored) restored->Release();
        CHECK(!nova::cs2::hooks::Failed()); Sleep(10);
    };
    for (int i = 0; i < 200 && !nova::cs2::hooks::Ready(); ++i) frame();
    CHECK(nova::cs2::hooks::Ready());
    probeEspEdge = true;
    double minimumEdge = 100000, maximumEdge = 0;
    for (int phase = 0; phase < 16; ++phase) {
        probePhase = phase / 16.f; frame();
        const double energy = EdgeEnergy(chain, device, context);
        minimumEdge = std::min(minimumEdge, energy); maximumEdge = std::max(maximumEdge, energy);
    }
    printf("ESP EDGE BASELINE: scale=%.3f min=%.1f max=%.1f\n", probeScale, minimumEdge, maximumEdge);
    CHECK(minimumEdge == 0 && maximumEdge >= 250); // Reproduce the original flicker.
    probeRaster = nova::ui::esp2d::RasterMode::PixelCoverage;
    for (float scale : {1.f, 1.3333333f, 1.4814815f, 2.f, 2.6666667f, 4.f}) {
        probeScale = scale; minimumEdge = 100000; maximumEdge = 0;
        for (int phase = 0; phase < 16; ++phase) {
            probePhase = phase / 16.f; frame();
            const double energy = EdgeEnergy(chain, device, context);
            minimumEdge = std::min(minimumEdge, energy); maximumEdge = std::max(maximumEdge, energy);
        }
        printf("ESP EDGE COVERAGE: scale=%.3f min=%.1f max=%.1f\n", probeScale, minimumEdge, maximumEdge);
        CHECK(minimumEdge > 80 && maximumEdge - minimumEdge <= 5);
    }
    probeLayout = nova::ui::esp2d::Layout::Compact;
    for (float scale : {1.f, 1.3333333f, 2.f, 2.6666667f}) {
        probeScale = scale; minimumEdge = 100000; maximumEdge = 0;
        for (int phase = 0; phase < 16; ++phase) {
            probePhase = phase / 16.f; frame();
            const double energy = EdgeEnergy(chain, device, context);
            minimumEdge = std::min(minimumEdge, energy); maximumEdge = std::max(maximumEdge, energy);
        }
        printf("ESP COMPACT COVERAGE: scale=%.3f min=%.1f max=%.1f\n", probeScale, minimumEdge, maximumEdge);
        CHECK(minimumEdge > 160 && maximumEdge - minimumEdge <= 5);
    }
    probeEspEdge = false;
    CHECK(nova::menu::Features().features.size() == 14);
    CHECK(nova::cs2::modules::ModuleManager::Get().Find("Auto Fire"));
    auto* mode = nova::cs2::modules::ModuleManager::Get().Find("ClickGUI")->modeProperty();
    CHECK(mode); mode->set(0);
    for (const auto& row : nova::menu::panel::ClientSettings::Get().rows())
        if (std::string_view(row.getName()) == "Blur") dynamic_cast<Properties::BoolProperty*>(row.getProp())->set(true);
    PostMessageW(window, WM_KEYDOWN, VK_INSERT, 0); PostMessageW(window, WM_KEYUP, VK_INSERT, 0);
    for (int i = 0; i < 45; ++i) frame();
    CHECK(nova::clickgui::IsOpen()); CHECK(capture); CHECK(!wrongThread);
    CHECK(nova::ui::blur::Available());
    PostMessageW(window, WM_KEYDOWN, VK_INSERT, 1LL << 30);
    for (int i = 0; i < 3; ++i) frame(); CHECK(nova::clickgui::IsOpen());
    Image(chain, device, context, output / "panel.bmp");
    if(argc>2 && std::string_view(argv[2])=="--browser") {
        nova::chrome::Start();nova::clickgui::OpenBrowser();bool navigated=false,loaded=false;
        for(int i=0;i<1800;++i){
            frame();const auto browser=nova::chrome::Read();CHECK(browser.error.empty());
            if(i%300==0)printf("Browser DX11: ready=%d open=%d tabs=%d pixels=%d url=%s title=%s\n",browser.status.ready,nova::clickgui::IsOpen(),browser.status.count,browser.frame?browser.frame->width:0,nova::chrome::Utf8(browser.status.url).c_str(),nova::chrome::Utf8(browser.status.tabs[0].title).c_str());
            if(browser.status.ready&&!navigated){nova::chrome::Navigate(L"http://127.0.0.1:18795/test.html");navigated=true;}
            if(browser.frame&&std::wstring(browser.status.tabs[0].title)==L"Nova Browser Test"){loaded=true;break;}
        }
        CHECK(loaded);for(int i=0;i<15;++i)frame();Image(chain,device,context,output/"browser-dx11.bmp");
        const auto placement=nova::config::LoadBrowserPlacement();
        // Exercise the actual detached path after the ClickGUI module disables.
        workspaceAction=nova::ui::workspace::Detach;
        for(int i=0;i<45;++i)frame();
        CHECK(!nova::clickgui::IsOpen());CHECK(capture);
        CHECK(floatingActive);
        Image(chain,device,context,output/"workspace-dx11.bmp");
        PostMessageW(window,WM_KEYDOWN,VK_ESCAPE,0);PostMessageW(window,WM_KEYUP,VK_ESCAPE,0);
        for(int i=0;i<45;++i)frame();
        CHECK(nova::ui::workspace::Current()==nova::ui::workspace::State::Shelved);CHECK(!capture);
        workspaceAction=nova::ui::workspace::Restore;for(int i=0;i<45;++i)frame();CHECK(capture);
        context->OMSetRenderTargets(0,nullptr,nullptr);target->Release();target=nullptr;
        CHECK(SUCCEEDED(chain->ResizeBuffers(1,960,640,DXGI_FORMAT_UNKNOWN,0)));createTarget();
        for(int i=0;i<45;++i)frame();Image(chain,device,context,output/"browser-dx11-resize.bmp");
        workspaceAction=nova::ui::workspace::Dock;for(int i=0;i<5;++i)frame();
        CHECK(nova::clickgui::IsOpen());CHECK(!nova::ui::workspace::Detached());
        nova::config::SaveBrowserPlacement(placement);
        nova::chrome::Shutdown();puts("PASS: sandboxed Chromium in shared DX11 Panel and swap-chain resize");
    }
    CHECK(nova::menu::panel::ClientSettings::Get().blurEnabled());
    nova::bind::BindProperty* unloadSetting = nullptr;
    for (const auto& row : nova::menu::panel::ClientSettings::Get().rows())
        if (std::string_view(row.getName()) == "Unload Key") unloadSetting = dynamic_cast<nova::bind::BindProperty*>(row.getProp());
    CHECK(unloadSetting);
    unloadSetting->setKeybind(VK_F9);
    CHECK(nova::config::ConfigManager::Get().SaveAs("roundtrip"));
    unloadSetting->setKeybind(VK_DELETE);
    CHECK(nova::config::ConfigManager::Get().LoadByName("roundtrip"));
    CHECK(unloadSetting->keybind() == VK_F9);
    // ClickGUI's open state is transient, but its selected mode and bind persist.
    frame(); CHECK(!nova::clickgui::IsOpen());
    previewAim = true; frame(); Image(chain, device, context, output / "aim-range.bmp"); previewAim = false;
    previewEsp = true;
    for (int i = 0; i < 40; ++i) frame();
    Image(chain, device, context, output / "esp.bmp");
    previewEsp = false;
    {
        nova::ui::watermark::WatermarkHud watermark;
        auto& options = watermark.Options();
        options.player.set(true); options.ping.set(true); options.tps.set(true); options.clock.set(true);
        previewWatermarkData = {}; previewWatermarkData.hideUnavailable = true;
        previewWatermarkData.player = "Player"; previewWatermarkData.ping = 37; previewWatermarkData.tps = 64;
        previewWatermark = &watermark;
        for (int i = 0; i < 40; ++i) frame();
        Image(chain, device, context, output / "watermark-connected.bmp");
        previewWatermarkData = {}; previewWatermarkData.hideUnavailable = true;
        for (int i = 0; i < 40; ++i) frame();
        Image(chain, device, context, output / "watermark-menu.bmp");
        previewWatermark = nullptr;
    }
    auto* notifications = nova::cs2::modules::ModuleManager::Get().Find("Notifications");
    notifications->setEnabled(false); notifications->keybind().setKeybind(-5); frame();
    PostMessageW(window, WM_XBUTTONDOWN, MAKEWPARAM(0, XBUTTON2), 0);
    PostMessageW(window, WM_XBUTTONUP, MAKEWPARAM(0, XBUTTON2), 0);
    for (int i = 0; i < 3; ++i) frame(); CHECK(notifications->isEnabled());
    notifications->keybind().setKeybind(0);
    auto* gui = nova::cs2::modules::ModuleManager::Get().Find("ClickGUI");
    gui->keybind().setKeybind(VK_F8);
    PostMessageW(window, WM_KEYDOWN, VK_INSERT, 0); PostMessageW(window, WM_KEYUP, VK_INSERT, 0);
    for (int i = 0; i < 3; ++i) frame(); CHECK(!nova::clickgui::IsOpen());
    PostMessageW(window, WM_KEYDOWN, VK_F8, 0); PostMessageW(window, WM_KEYUP, VK_F8, 0);
    for (int i = 0; i < 10; ++i) frame(); CHECK(nova::clickgui::IsOpen());
    gui->keybind().setKeybind(VK_INSERT);
    context->OMSetRenderTargets(0, nullptr, nullptr); target->Release(); target = nullptr;
    CHECK(SUCCEEDED(chain->ResizeBuffers(1, 960, 640, DXGI_FORMAT_UNKNOWN, 0))); createTarget();
    for (int i = 0; i < 15; ++i) frame();
    CHECK(nova::ui::blur::Available());
    mode->set(1);
    for (int i = 0; i < 20; ++i) frame();
    CHECK(nova::clickgui::GetMode() == nova::clickgui::Mode::Classic);
    // Recording the unload key itself must not unload. Pause rendering while
    // the key arrives, then process capture, release, and arm the new binding.
    nova::bind::BindCapture::Get().Begin(unloadSetting);
    SendMessageW(window, WM_KEYDOWN, VK_F9, 0);
    CHECK(!unload);
    frame(); CHECK(!unload && !nova::bind::BindCapture::Get().listening());
    SendMessageW(window, WM_KEYUP, VK_F9, 0); frame();
    SendMessageW(window, WM_KEYDOWN, VK_DELETE, 0);
    SendMessageW(window, WM_KEYUP, VK_DELETE, 0); frame(); CHECK(!unload);
    // Custom key bypasses other bind editors with no Present frames running.
    nova::bind::BindCapture::Get().Begin(&gui->keybind());
    PostMessageW(window, WM_KEYDOWN, VK_F9, 0);
    for (int i = 0; i < 100 && !unload; ++i) Sleep(10);
    CHECK(unload);
    CHECK(gui->keybind().keybind() == VK_INSERT);
    nova::bind::BindCapture::Get().Cancel();
    if(argc>2&&std::string_view(argv[2])=="--browser"){
        workspaceAction=nova::ui::workspace::Detach;
        for(int i=0;i<35;++i)frame();CHECK(capture);
    }
    // A paused renderer may time out once. Resuming frames must allow retrying
    // the same teardown, rather than leaving the host permanently locked.
    CHECK(!nova::cs2::hooks::Stop(30));
    std::atomic<bool> stopped{false}; bool result = false;
    std::thread stop([&] { result = nova::cs2::hooks::Stop(5000); stopped = true; });
    for (int i = 0; i < 600 && !stopped; ++i) frame();
    stop.join(); CHECK(result); CHECK(!capture); CHECK(!wrongThread); CHECK(!nova::clickgui::IsOpen());
    nova::cs2::hooks::Finalize();
    // Reinstall the production hooks into the same process/window after cleanup.
    // Presentation's static catalog belongs to a DLL image; this second pass
    // uses a fresh minimal UI, as a newly mapped image would.
    nova::ui::Dx11Callbacks again;
    again.initialize = [] { return true; };
    again.draw = [] { ImGui::Begin("Reload"); ImGui::TextUnformatted("Reattached"); ImGui::End(); };
    again.windowCapture = Capture;
    CHECK(nova::cs2::hooks::Install(again));
    for (int i = 0; i < 100 && !nova::cs2::hooks::Ready(); ++i) frame();
    CHECK(nova::cs2::hooks::Ready());
    stopped = false; result = false;
    std::thread stopAgain([&] { result = nova::cs2::hooks::Stop(5000); stopped = true; });
    for (int i = 0; i < 600 && !stopped; ++i) frame();
    stopAgain.join(); CHECK(result);
    nova::cs2::hooks::Finalize();
    context->OMSetRenderTargets(0, nullptr, nullptr); target->Release(); chain->Release(); context->Release(); device->Release();
    PostMessageW(window, WM_CLOSE, 0, 0); messages.join();
    puts("PASS: DX11 hooks, shared UI/HUD, config, GPU state, resize, custom unload key/capture without frames, teardown retry and same-process reattach; no game injection");
}
