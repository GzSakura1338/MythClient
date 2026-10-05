#include "ui/backend/OpenGLSurface.h"
#include "ui/backend/PixelUnpackGuard.h"
#include "ui/clickgui/Fonts.h"
#include "ui/backend/Win32Surface.h"
#include "ui/backend/blur/Blur.h"
#include "ui/skin/SkinTextures.h"
#include "ui/hud/musichud/CoverTexture.h"
#include "imgui_internal.h"
#include "imgui_impl_opengl3.h"
#include <cassert>
#include <cstdio>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <optional>

namespace nova::ui::blur {
static ImTextureID published = 0;
void Publish(ImTextureID texture, bool) { published = texture; }
// This surface test exercises GPU cache lifetime without the ClickGUI settings
// manager or popup compositor. Keep those UI callbacks out of its link graph.
Options GetOptions() { return {true, 3, 30}; }
void SetLayerBackdrop(ImTextureID, bool, ImDrawCallback, void*) {}
void ClearLayerBackdrop() {}
}
using nova::glhook::OpenGLSurface;
using nova::glhook::Win32Surface;
namespace {
constexpr UINT probe = WM_APP + 12;
int delivered = 0, foreignDelivered = 0;
bool useCoreProfile = false;
LRESULT CALLBACK GameProc(HWND window, UINT message, WPARAM wp, LPARAM lp) {
    if (message == probe) { ++delivered; return 123; }
    return DefWindowProcW(window, message, wp, lp);
}
LRESULT CALLBACK OverlayProc(HWND window, UINT message, WPARAM wp, LPARAM lp, UINT_PTR id, DWORD_PTR data) {
    if (message == WM_NCDESTROY) {
        reinterpret_cast<Win32Surface*>(data)->Destroyed(window);
        RemoveWindowSubclass(window, OverlayProc, id);
    }
    return DefSubclassProc(window, message, wp, lp);
}
LRESULT CALLBACK ForeignProc(HWND window, UINT message, WPARAM wp, LPARAM lp, UINT_PTR, DWORD_PTR) {
    if (message == probe) ++foreignDelivered;
    return DefSubclassProc(window, message, wp, lp);
}
struct Window {
    HWND window{};
    HDC dc{};
    HGLRC context{};
    Window() {
        window = CreateWindowW(L"NovaSurfaceTest", L"Nova hidden render test", WS_OVERLAPPEDWINDOW,
            0, 0, 400, 300, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
        assert(window);
        dc = GetDC(window);
        PIXELFORMATDESCRIPTOR pfd{sizeof(pfd), 1};
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 32;
        pfd.cDepthBits = 24;
        const int format = ChoosePixelFormat(dc, &pfd);
        assert(format && SetPixelFormat(dc, format, &pfd));
        context = wglCreateContext(dc);
        assert(context);
        if (useCoreProfile) {
            const HDC previousDC = wglGetCurrentDC();
            const HGLRC previousContext = wglGetCurrentContext();
            assert(wglMakeCurrent(dc, context));
            using CreateContext = HGLRC(WINAPI*)(HDC, HGLRC, const int*);
            auto create = reinterpret_cast<CreateContext>(wglGetProcAddress("wglCreateContextAttribsARB"));
            assert(create);
            const int attributes[]{0x2091, 3, 0x2092, 2, 0x9126, 1, 0};
            HGLRC core = create(dc, nullptr, attributes);
            assert(core);
            assert(wglMakeCurrent(previousDC, previousContext));
            assert(wglDeleteContext(context));
            context = core;
        }
    }
    void Current() { assert(wglMakeCurrent(dc, context)); }
    void DestroyContext() {
        if (!context) return;
        if (wglGetCurrentContext() == context) assert(wglMakeCurrent(nullptr, nullptr));
        assert(wglDeleteContext(context)); context = nullptr;
    }
    void DestroyWindowOnly() {
        if (!window) return;
        ReleaseDC(window, dc);
        assert(DestroyWindow(window)); window = nullptr; dc = nullptr;
    }
    ~Window() { DestroyContext(); DestroyWindowOnly(); }
};
using Gen = void(APIENTRY*)(GLsizei, GLuint*);
using Del = void(APIENTRY*)(GLsizei, const GLuint*);
using Attach = void(APIENTRY*)(GLenum, GLenum, GLenum, GLuint, GLint);
void StartRenderer() {
    assert(ImGui_ImplOpenGL3_Init());
    assert(ImGui_ImplOpenGL3_CreateDeviceObjects());
}
void CheckDynamicFonts() {
    InitializeFatalMenuFonts();
    auto gen = reinterpret_cast<Gen>(nova::gl::resolve("glGenBuffers"));
    auto del = reinterpret_cast<Del>(nova::gl::resolve("glDeleteBuffers"));
    GLuint pbo{};
    gen(1, &pbo);
    const char* names[]{"NOVA", "ZEN", "NOVA SE"};
    for (int frame = 0; frame < 3; ++frame) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui::GetIO().DisplaySize = {400, 300};
        ImGui::NewFrame();
        ImGui::PushFont(ExtraBold20, frame == 2 ? 31.5f : 42.f);
        ImGui::GetBackgroundDrawList()->AddText({20, 20}, IM_COL32_WHITE, names[frame]);
        ImGui::PopFont();
        ImGui::Render();
        // The first upload uses an empty PBO, so an unguarded baseline fails
        // safely. Later uploads also cover CPU pointers with stale pixel offsets.
        const GLuint hostPbo = frame % 2 == 0 ? pbo : 0;
        glBindBuffer(0x88EC, hostPbo);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 8);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 19);
        glPixelStorei(GL_UNPACK_SKIP_ROWS, 2);
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, 3);
        {
            std::optional<nova::glhook::PixelUnpackGuard> guard;
            if (!std::getenv("NOVA_TEST_UNGUARDED")) guard.emplace();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }
        if (glGetError() != GL_NO_ERROR) {
            std::fputs("FAIL: font upload inherited host pixel-unpack state\n", stderr);
            std::exit(2);
        }
        const GLenum keys[]{0x88EF, GL_UNPACK_ALIGNMENT, GL_UNPACK_ROW_LENGTH,
                            GL_UNPACK_SKIP_ROWS, GL_UNPACK_SKIP_PIXELS};
        const GLint expected[]{static_cast<GLint>(hostPbo), 8, 19, 2, 3};
        for (int i = 0; i < 5; ++i) {
            GLint actual{}; glGetIntegerv(keys[i], &actual); assert(actual == expected[i]);
        }
        glBindBuffer(0x88EC, 0);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
        // Verify every uploaded pixel, not merely that some text is visible.
        for (ImTextureData* tex : ImGui::GetIO().Fonts->TexList) {
            assert(tex->Status == ImTextureStatus_OK);
            std::vector<unsigned char> gpu(tex->Width * tex->Height * 4);
            glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(tex->TexID));
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, gpu.data());
            assert(std::memcmp(gpu.data(), tex->GetPixels(), gpu.size()) == 0);
        }
    }
    del(1, &pbo);
    std::puts("PASS: NOVA -> ZEN -> NOVA SE dynamic font pixels and host unpack state restoration");
}
bool Retire(OpenGLSurface& owner, bool expectedAvailable) {
    const HGLRC incoming = wglGetCurrentContext();
    const bool result = owner.Retire([&](bool available) {
        assert(available == expectedAvailable);
        if (available) assert(wglGetCurrentContext() == owner.context);
        nova::ui::blur::ResetOpenGLContext(available);
        ImGui_ImplOpenGL3_Shutdown(!available);
    });
    assert(result && wglGetCurrentContext() == incoming);
    assert(nova::ui::blur::published == 0);
    nova::gl::ResetPointers();
    return result;
}
void Draw(HWND window, std::uint64_t skin, const std::shared_ptr<const nova::smtc::Artwork>& cover) {
    RECT rect{}; assert(GetClientRect(window, &rect));
    const int w = rect.right, h = rect.bottom;
    // A private FBO makes readback deterministic even for hidden/occluded windows.
    auto gen = reinterpret_cast<Gen>(nova::gl::resolve("glGenFramebuffers"));
    auto del = reinterpret_cast<Del>(nova::gl::resolve("glDeleteFramebuffers"));
    auto attach = reinterpret_cast<Attach>(nova::gl::resolve("glFramebufferTexture2D"));
    assert(gen && del && attach);
    GLuint texture{}, fbo{};
    glGenTextures(1, &texture); glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, 0x8058, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    gen(1, &fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    attach(GL_DRAW_FRAMEBUFFER, 0x8CE0, GL_TEXTURE_2D, texture, 0);
    glViewport(0, 0, w, h);
    glClearColor(0, 0, 0, 1); glClear(GL_COLOR_BUFFER_BIT);
    nova::ui::blur::RenderOpenGL({true, 3, 30}, w, h);
    assert(nova::ui::blur::published && glIsTexture(static_cast<GLuint>(nova::ui::blur::published)));
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplWin32_NewFrame();
    assert(ImGui::GetIO().DisplaySize.x == w && ImGui::GetIO().DisplaySize.y == h);
    ImGui::NewFrame();
    ImDrawList* draws = ImGui::GetBackgroundDrawList();
    draws->AddRectFilled({10, 10}, {50, 50}, IM_COL32(255, 0, 0, 255));
    draws->AddText({60, 20}, IM_COL32_WHITE, "Nova F11");
    draws->AddImage(nova::ui::skins::Resolve(skin), {10, 70}, {50, 110});
    assert(nova::ui::music::DrawCover(draws, cover, {60, 70}, {100, 110}, 0));
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    const auto pixel = [h](int x, int y, int channel) {
        unsigned char rgba[4]{};
        glReadPixels(x, h - y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
        assert(rgba[channel] > 240);
        assert(rgba[(channel + 1) % 3] < 5);
    };
    pixel(20, 20, 0); // geometry
    pixel(20, 80, 1); // retained skin pixels
    pixel(70, 80, 2); // retained music cover
    std::vector<unsigned char> textPixels(100 * 30 * 4);
    glReadPixels(60, h - 45, 100, 30, GL_RGBA, GL_UNSIGNED_BYTE, textPixels.data());
    bool textVisible = false;
    for (size_t i = 0; i < textPixels.size(); i += 4)
        textVisible |= textPixels[i] > 100 && textPixels[i + 1] > 100 && textPixels[i + 2] > 100;
    assert(textVisible);
    assert(glGetError() == GL_NO_ERROR);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    del(1, &fbo); glDeleteTextures(1, &texture);
}
}
int main(int argc, char**) {
    useCoreProfile = argc > 1;
    WNDCLASSW wc{}; wc.style = CS_OWNDC; wc.lpfnWndProc = GameProc;
    wc.hInstance = GetModuleHandleW(nullptr); wc.lpszClassName = L"NovaSurfaceTest";
    assert(RegisterClassW(&wc));
    Window a, b;
    a.Current();
    std::printf("GL: %s\n", glGetString(GL_VERSION));
    ImGuiContext* ui = ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    Win32Surface platform;
    OpenGLSurface owner;
    assert(platform.Bind(a.window, OverlayProc));
    StartRenderer(); owner.Capture();
    CheckDynamicFonts();
    auto skin = nova::ui::skins::Store("test", 2, 2, std::vector<std::uint32_t>(4, IM_COL32(0, 255, 0, 255)));
    auto cover = std::make_shared<nova::smtc::Artwork>();
    cover->version = 1; cover->width = cover->height = 2;
    cover->rgba.assign(4, IM_COL32(0, 0, 255, 255));
    Draw(a.window, skin, cover);
    void* renderer = ImGui::GetIO().BackendRendererUserData;
    void* originalPlatform = ImGui::GetIO().BackendPlatformUserData;
    assert(SetWindowPos(a.window, nullptr, 0, 0, 600, 400, SWP_NOMOVE | SWP_NOZORDER));
    assert(platform.Bind(a.window, OverlayProc));
    assert(ImGui::GetIO().BackendPlatformUserData == originalPlatform);
    Draw(a.window, skin, cover);
    // Window-only recreation, including preserving another overlay's subclass.
    assert(SetWindowSubclass(a.window, ForeignProc, 99, 0));
    assert(wglMakeCurrent(b.dc, a.context));
    assert(platform.Bind(b.window, OverlayProc)); owner.Capture();
    assert(ImGui::GetIO().BackendRendererUserData == renderer);
    assert(SendMessageW(a.window, probe, 0, 0) == 123);
    assert(delivered == 1 && foreignDelivered == 1);
    Draw(b.window, skin, cover);
    a.DestroyWindowOnly();
    // Replacement GL context; the old context remains alive on the same window.
    b.Current(); Retire(owner, true); StartRenderer(); owner.Capture();
    Draw(b.window, skin, cover);
    // Actual loss of both the bound native window and its context. Precreate the
    // replacement context so Windows cannot recycle the destroyed HGLRC value.
    Window c;
    c.Current(); b.DestroyContext(); b.DestroyWindowOnly();
    assert(platform.Bind(c.window, OverlayProc));
    std::vector<GLuint> gameTextures(128);
    glGenTextures(static_cast<GLsizei>(gameTextures.size()), gameTextures.data());
    for (GLuint texture : gameTextures) { glBindTexture(GL_TEXTURE_2D, texture); glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr); }
    Retire(owner, false);
    for (GLuint texture : gameTextures) assert(glIsTexture(texture));
    StartRenderer(); owner.Capture(); Draw(c.window, skin, cover);
    glDeleteTextures(static_cast<GLsizei>(gameTextures.size()), gameTextures.data());
    // Context destroyed while the original window/DC remains valid: failed
    // wglMakeCurrent must still restore the incoming context.
    Window d;
    d.Current(); c.DestroyContext();
    Retire(owner, false);
    assert(platform.Bind(d.window, OverlayProc));
    StartRenderer(); owner.Capture(); Draw(d.window, skin, cover);
    // Repeated toggles must neither lose input nor recreate the ImGui context.
    Window e;
    for (int i = 0; i < 12; ++i) {
        Window& target = i % 2 ? d : e;
        target.Current(); Retire(owner, true);
        assert(platform.Bind(target.window, OverlayProc));
        StartRenderer(); owner.Capture(); Draw(target.window, skin, cover);
        assert(ImGui::GetCurrentContext() == ui);
        assert(SendMessageW(target.window, probe, 0, 0) == 123);
    }
    Retire(owner, true);
    nova::ui::skins::Release(); nova::ui::music::ReleaseCovers();
    platform.Reset(); ImGui::DestroyContext();
    std::puts("PASS: resize, window/context recreation, lost contexts, subclass chain, repeated toggles, font/skin/cover/blur restoration and framebuffer pixels");
}
