#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <wrl/client.h>
#include <array>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include "ui/backend/Dx11Overlay.h"
#include "ui/backend/blur/Blur.h"
#include "ui/clickgui/panel/ClientSettings.h"
#include "cs2/modules/render/esp2d/Projection.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "Stretch test failed at %d: %s\n", __LINE__, #x); std::exit(1); } } while (false)
namespace {
using Microsoft::WRL::ComPtr;
unsigned draws = 0, clicks = 0;
ImVec2 display{}, density{}, mouse{};
bool hovered = false;
void Draw() {
    ++draws;
    display = ImGui::GetIO().DisplaySize;
    density = ImGui::GetIO().DisplayFramebufferScale;
    mouse = ImGui::GetIO().MousePos;
    auto* draw = ImGui::GetBackgroundDrawList();
    auto point = [](float x, float y) { return ImVec2(x * display.x, y * display.y); };
    // Independent synthetic world: the box must enclose NDC x[-.6,-.36],
    // y[-.4,.4], i.e. framebuffer fractions x[.2,.32], y[.3,.7].
    nova::cs2::sdk::players::Player player;
    player.origin = {-.48f, 0, 0}; player.mins = {-.12f, -.1f, -.4f}; player.maxs = {.12f, .1f, .4f};
    nova::cs2::sdk::players::Matrix matrix{};
    matrix[0][0] = matrix[1][2] = matrix[3][3] = 1;
    auto bounds = nova::cs2::esp2d::Project(player, matrix, display.x, display.y);
    CHECK(bounds);
    const float scale = std::clamp(display.y / 540.f, 1.f, 4.f);
    bounds->left /= scale; bounds->right /= scale; bounds->top /= scale; bounds->bottom /= scale;
    using namespace nova::ui::esp2d;
    Draw(*bounds, scale, BoxMode::Normal, HealthMode::Health, 100, 100, 100, true, RasterMode::PixelCoverage, 100, true);
    DrawName(*bounds, scale, "Stretch test", false);
    // Right/bottom coverage and a scissor region; neither may use HWND pixels
    // as physical buffer pixels when the axes have different scale factors.
    draw->AddRectFilled(point(.95f, .95f), point(1, 1), IM_COL32(255, 0, 0, 255));
    draw->PushClipRect(point(.7f, .65f), point(.75f, .7f), true);
    draw->AddRectFilled(point(.6f, .6f), point(.85f, .85f), IM_COL32(255, 0, 255, 255));
    draw->PopClipRect();
    nova::ui::blur::Background(draw, point(.55f, .2f), point(.9f, .5f), IM_COL32(0, 0, 0, 1), 8, 0, 1, 0);
    ImGui::SetNextWindowPos(point(.7f, .78f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(point(.2f, .12f), ImGuiCond_Always);
    ImGui::Begin("Stretch input", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove);
    ImGui::SetCursorScreenPos(point(.73f, .8f));
    if (ImGui::Button("Click", point(.12f, .06f))) ++clicks;
    hovered = ImGui::IsItemHovered();
    ImGui::End();
}
struct Pixels {
    UINT width, height;
    std::vector<std::array<unsigned char, 4>> data;
    const auto& at(float x, float y) const { return data[std::min(UINT(y * height), height - 1) * width + std::min(UINT(x * width), width - 1)]; }
};
Pixels Read(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11Texture2D* buffer) {
    D3D11_TEXTURE2D_DESC desc{}; buffer->GetDesc(&desc);
    Pixels pixels{desc.Width, desc.Height, {}}; pixels.data.resize(desc.Width * desc.Height);
    desc.Usage = D3D11_USAGE_STAGING; desc.BindFlags = 0; desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ; desc.MiscFlags = 0;
    ComPtr<ID3D11Texture2D> copy; CHECK(SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &copy)));
    context->CopyResource(copy.Get(), buffer);
    D3D11_MAPPED_SUBRESOURCE map{}; CHECK(SUCCEEDED(context->Map(copy.Get(), 0, D3D11_MAP_READ, 0, &map)));
    for (UINT y = 0; y < desc.Height; ++y)
        memcpy(pixels.data.data() + y * desc.Width, static_cast<char*>(map.pData) + y * map.RowPitch, desc.Width * 4);
    context->Unmap(copy.Get(), 0); return pixels;
}
}
void TestStretch() {
    WNDCLASSW wc{}; wc.hInstance = GetModuleHandleW(nullptr); wc.lpfnWndProc = DefWindowProcW; wc.lpszClassName = L"Nova.Stretch.Test";
    CHECK(RegisterClassW(&wc));
    // Hidden, owned test window. Never resize, focus or inject into the game.
    HWND window = CreateWindowW(wc.lpszClassName, L"", WS_POPUP, 0, 0, 1280, 720, nullptr, nullptr, wc.hInstance, nullptr);
    CHECK(window);
    DXGI_SWAP_CHAIN_DESC desc{}; desc.BufferDesc.Width = 1280; desc.BufferDesc.Height = 720;
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; desc.SampleDesc.Count = 1;
    desc.BufferCount = 1; desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; desc.OutputWindow = window;
    desc.Windowed = TRUE; desc.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;
    ComPtr<ID3D11Device> device; ComPtr<ID3D11DeviceContext> context; ComPtr<IDXGISwapChain> chain;
    CHECK(SUCCEEDED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
        D3D11_SDK_VERSION, &desc, &chain, &device, nullptr, &context)));
    Properties::BoolProperty* blur = nullptr;
    for (const auto& row : nova::menu::panel::ClientSettings::Get().rows())
        if (std::string_view(row.getName()) == "Blur") blur = dynamic_cast<Properties::BoolProperty*>(row.getProp());
    CHECK(blur); const bool previousBlur = blur->get(); blur->set(true);
    nova::ui::Dx11Callbacks callbacks; callbacks.draw = Draw;
    nova::ui::Dx11Overlay overlay(callbacks);
    struct Size { int cw, ch; UINT bw, bh; };
    // Equal sizes, 4:3 and 5:4 stretched to 16:9, uniform DPI-like scaling,
    // ultrawide, client-only resize, then back to 1:1. Same overlay throughout.
    for (auto size : {Size{1280,720,1280,720}, {1920,1080,1280,960}, {1920,1080,1280,1024},
                     {1280,720,1920,1080}, {2560,1080,1280,720}, {1280,800,1280,720}, {1280,720,1280,720}}) {
        CHECK(SetWindowPos(window, nullptr, 0, 0, size.cw, size.ch, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE));
        DXGI_SWAP_CHAIN_DESC current{}; CHECK(SUCCEEDED(chain->GetDesc(&current)));
        if (current.BufferDesc.Width != size.bw || current.BufferDesc.Height != size.bh) {
            overlay.BeforeResize(chain.Get());
            CHECK(SUCCEEDED(chain->ResizeBuffers(1, size.bw, size.bh, DXGI_FORMAT_UNKNOWN, 0)));
        }
        ComPtr<ID3D11Texture2D> buffer; CHECK(SUCCEEDED(chain->GetBuffer(0, IID_PPV_ARGS(&buffer))));
        // Horizontal gray ramp makes blur sampling at the wrong UV observable.
        std::vector<unsigned> ramp(size.bw * size.bh);
        for (UINT y = 0; y < size.bh; ++y) for (UINT x = 0; x < size.bw; ++x) {
            const unsigned gray = x * 255 / size.bw;
            ramp[y * size.bw + x] = 0xff000000u | gray * 0x010101;
        }
        D3D11_VIEWPORT sentinel{13, 17, 47, 59, 0, 1}; // Could be a final post-process viewport.
        const auto frame = [&] {
            context->UpdateSubresource(buffer.Get(), 0, nullptr, ramp.data(), size.bw * 4, 0);
            context->RSSetViewports(1, &sentinel);
            CHECK(overlay.Render(chain.Get()));
            D3D11_VIEWPORT restored{}; UINT count = 1; context->RSGetViewports(&count, &restored);
            CHECK(count == 1 && memcmp(&restored, &sentinel, sizeof(sentinel)) == 0);
        };
        frame(); frame();
        auto pixels = Read(device.Get(), context.Get(), buffer.Get());
        CHECK(pixels.at(.975f, .975f)[0] > 250 && pixels.at(.975f, .975f)[1] < 2);
        CHECK(pixels.at(.72f, .67f)[0] > 250 && pixels.at(.72f, .67f)[1] < 2 && pixels.at(.72f, .67f)[2] > 250);
        CHECK(std::abs(int(pixels.at(.68f, .67f)[0]) - int(pixels.at(.68f, .67f)[1])) < 2);
        CHECK(std::abs(int(pixels.at(.8f, .67f)[0]) - int(pixels.at(.8f, .67f)[1])) < 2);
        CHECK(std::abs(int(pixels.at(.65f, .35f)[0]) - int(.65f * 255)) <= 3);
        CHECK(std::abs(int(pixels.at(.85f, .35f)[0]) - int(.85f * 255)) <= 3);
        // Find the health stroke near the independently calculated left edge.
        int health = 0, name = 0, armor = 0;
        for (UINT y = UINT(.45f * size.bh); y < UINT(.55f * size.bh); ++y)
            for (int x = int(.2f * size.bw) - 10; x < int(.2f * size.bw); ++x) {
                const auto p = pixels.data[y * size.bw + x];
                health = std::max(health, int(p[1]) - std::max(int(p[0]), int(p[2])));
            }
        for (int y = int(.3f * size.bh) - 30; y < int(.3f * size.bh) - 3; ++y)
            for (UINT x = UINT(.21f * size.bw); x < UINT(.31f * size.bw); ++x) {
                const auto p = pixels.data[y * size.bw + x]; name = std::max(name, std::min({int(p[0]), int(p[1]), int(p[2])}));
            }
        for (UINT y = UINT(.7f * size.bh) + 1; y < UINT(.7f * size.bh) + 8; ++y)
            for (UINT x = UINT(.24f * size.bw); x < UINT(.28f * size.bw); ++x) {
                const auto p = pixels.data[y * size.bw + x]; armor = std::max(armor, std::abs(int(p[0]) - int(p[2])));
            }
        printf("ESP coverage: client=%dx%d buffer=%ux%u health=%d name=%d armor=%d\n", size.cw, size.ch, size.bw, size.bh, health, name, armor);
        CHECK(health > 30 && name > 150 && armor > 10);
        CHECK(display.x == size.cw && display.y == size.ch);
        CHECK(std::abs(density.x * display.x - size.bw) < .01f && std::abs(density.y * display.y - size.bh) < .01f);
        SendMessageW(window, WM_MOUSEMOVE, 0, MAKELPARAM(int(.79f * size.cw), int(.83f * size.ch)));
        frame(); frame(); CHECK(hovered);
        CHECK(std::abs(mouse.x - int(.79f * size.cw)) < 1 && std::abs(mouse.y - int(.83f * size.ch)) < 1);
        const auto before = clicks;
        SendMessageW(window, WM_LBUTTONDOWN, MK_LBUTTON, 0); frame();
        SendMessageW(window, WM_LBUTTONUP, 0, 0); frame(); CHECK(clicks == before + 1);
        printf("PASS: stretch client=%dx%d buffer=%ux%u, ESP/name/bars, edge/scissor, blur UV, mouse hit, viewport restore\n", size.cw, size.ch, size.bw, size.bh);
    }
    const auto before = draws;
    CHECK(SetWindowPos(window, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE));
    CHECK(!overlay.Render(chain.Get()) && draws == before);
    CHECK(SetWindowPos(window, nullptr, 0, 0, 1280, 720, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE));
    CHECK(overlay.Render(chain.Get()) && draws == before + 1);
    CHECK(overlay.Shutdown()); blur->set(previousBlur);
    chain.Reset(); context.Reset(); device.Reset();
    DestroyWindow(window); UnregisterClassW(wc.lpszClassName, wc.hInstance);
    puts("PASS: zero-sized client skip and recovery; no game injection");
}
