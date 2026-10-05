// Standalone GPU test; never attaches to Minecraft.
#define NOMINMAX
#include "ui/backend/GlExt.h"
#include "ui/backend/blur/Blur.h"
#include "ui/backend/blur/Gaussian.h"
#include "imgui_impl_opengl3.h"
#include <cstdio>
#include <stdexcept>

namespace blur = nova::ui::blur;
static blur::Options options{true, 3, 5};
namespace nova::ui::blur { Options GetOptions() { return options; } }
void Check(bool ok, const char* text) { if (!ok) throw std::runtime_error(text); }
static int Pixel(int x, int y, int channel = 0) {
    unsigned char pixel[4]{};
    glReadPixels(x, 127 - y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    return pixel[channel];
}
static void Frame(bool pattern = true, int width = 128, int height = 128,
                  ImU32 tint = IM_COL32(0, 0, 0, 255), float opacity = 1.0f, float sceneAlpha = 1.0f) {
    glDisable(GL_SCISSOR_TEST); glViewport(0, 0, 128, 128);
    glClearColor(pattern ? 0.f : 1.f, 0, 0, sceneAlpha); glClear(GL_COLOR_BUFFER_BIT);
    if (pattern) {
        glEnable(GL_SCISSOR_TEST); glScissor(64, 0, 64, 128);
        glClearColor(1, 1, 1, sceneAlpha); glClear(GL_COLOR_BUFFER_BIT);
    }
    glEnable(GL_SCISSOR_TEST); glScissor(7, 9, 19, 21);
    glEnable(0x8DB9); // framebuffer sRGB must survive preparation.
    glActiveTexture(0x84C3);glColorMask(GL_FALSE,GL_TRUE,GL_FALSE,GL_FALSE);
    glEnable(GL_BLEND);glEnable(GL_DEPTH_TEST);glEnable(GL_CULL_FACE);
    blur::RenderOpenGL(options, width, height);
    GLint active=0;glGetIntegerv(GL_ACTIVE_TEXTURE,&active);Check(active==0x84C3,"active texture restored");
    GLboolean mask[4]{};glGetBooleanv(GL_COLOR_WRITEMASK,mask);
    Check(!mask[0]&&mask[1]&&!mask[2]&&!mask[3],"color mask restored");
    Check(glIsEnabled(GL_BLEND)&&glIsEnabled(GL_DEPTH_TEST)&&glIsEnabled(GL_CULL_FACE),"raster states restored");
    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glDisable(GL_BLEND);glDisable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);glActiveTexture(0x84C0);
    GLint box[4]{}; glGetIntegerv(GL_SCISSOR_BOX, box);
    Check(glIsEnabled(GL_SCISSOR_TEST) && box[0] == 7 && box[3] == 21, "scissor state");
    Check(glIsEnabled(0x8DB9), "sRGB state");
    glDisable(0x8DB9); glDisable(GL_SCISSOR_TEST);
    GLint unpack{}; glGetIntegerv(0x88EF, &unpack); Check(unpack != 0, "PBO binding restored");
    Check(glGetError() == GL_NO_ERROR, "blur preparation GL errors");
    glBindBuffer(0x88EC, 0); // Font atlas upload is outside this state-preservation test.
    ImGui::GetIO().DisplaySize = ImVec2(128, 128); ImGui::GetIO().DeltaTime = 1.f / 60;
    ImGui_ImplOpenGL3_NewFrame(); ImGui::NewFrame();
    auto* draws = ImGui::GetBackgroundDrawList();
    blur::Background(draws, ImVec2(16, 16), ImVec2(112, 112), tint, 16, 0, opacity);
    draws->AddRectFilled(ImVec2(24, 24), ImVec2(40, 40), IM_COL32(255, 0, 0, 255));
    ImGui::Render(); ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData()); glFinish();
    glBindBuffer(0x88EC, unpack);
    Check(Pixel(30, 30) > 250 && Pixel(30, 30, 1) < 5, "foreground stays sharp");
    Check(Pixel(63, 4) == (pattern ? 0 : 255) && Pixel(64, 4) == 255, "outside HUD is unchanged");
    Check(Pixel(16, 16) == (pattern ? 0 : 255), "rounded corner preserves scene");
    Check(glGetError() == GL_NO_ERROR, "ImGui GL errors");
}

static void PopupFrames() {
    glBindBuffer(0x88EC, 0);
    options = {true, 3, 120};
    for (int frame = 0; frame < 3; ++frame) {
        glDisable(GL_SCISSOR_TEST); glClearColor(0, 0, 0, 1); glClear(GL_COLOR_BUFFER_BIT);
        glEnable(GL_SCISSOR_TEST); glScissor(64, 0, 64, 128);
        glClearColor(1, 1, 1, 1); glClear(GL_COLOR_BUFFER_BIT); glDisable(GL_SCISSOR_TEST);
        blur::RenderOpenGL(options, 128, 128);
        ImGui_ImplOpenGL3_NewFrame(); ImGui::NewFrame();
        ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 8.f);
        ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.f);
        ImGui::PushStyleColor(ImGuiCol_PopupBg, IM_COL32(0, 0, 0, 255));
        ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(255, 0, 0, 255));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, IM_COL32(255, 0, 255, 255));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, IM_COL32(255, 0, 255, 255));
        if (frame == 0) ImGui::OpenPopup("outer");
        ImGui::SetNextWindowPos(ImVec2(8, 8)); ImGui::SetNextWindowSize(ImVec2(112, 112));
        if (blur::BeginPopup("outer", ImGuiWindowFlags_AlwaysVerticalScrollbar)) {
            ImGui::SetCursorScreenPos(ImVec2(16, 16));
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.f);
            ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(0, 0, 0, 255));
            blur::BeginChild("card", ImVec2(96, 96), ImGuiChildFlags_Borders, ImGuiWindowFlags_AlwaysVerticalScrollbar);
            ImGui::Dummy(ImVec2(1, 200));
            ImGui::EndChild();
            ImGui::PopStyleColor(); ImGui::PopStyleVar();
            if (frame == 0) ImGui::OpenPopup("nested");
            ImGui::SetNextWindowPos(ImVec2(32, 32)); ImGui::SetNextWindowSize(ImVec2(64, 64));
            if (blur::BeginPopup("nested")) {
                auto* draws = ImGui::GetWindowDrawList();
                // Exercise the same widget channel splitting as mode selectors.
                draws->ChannelsSplit(2); draws->ChannelsSetCurrent(1);
                draws->AddRectFilled(ImVec2(72, 44), ImVec2(88, 56), IM_COL32(0, 255, 0, 255));
                draws->ChannelsSetCurrent(0); draws->ChannelsMerge();
                ImGui::EndPopup();
            }
            ImGui::EndPopup();
        }
        ImGui::PopStyleColor(4); ImGui::PopStyleVar(2);
        ImGui::Render(); ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData()); glFinish();
    }
    Check(Pixel(63, 80) > 5 && Pixel(63, 80) < 45, "nested popup blur");
    Check(Pixel(80, 48, 1) > 250 && Pixel(80, 48) < 5, "popup widget foreground");
    Check(Pixel(32, 64) > 240 && Pixel(32, 64, 1) < 5, "popup native border");
    Check(Pixel(112, 80) > 240 && Pixel(112, 80, 2) > 240, "native scrollbar above blur");
    Check(Pixel(16, 64) > 240 && Pixel(16, 64, 1) < 5, "child native border above blur");
    Check(Pixel(106, 80) > 240 && Pixel(106, 80, 2) > 240, "child scrollbar above blur");
    Check(Pixel(0, 80) == 0 && Pixel(127, 80) == 255, "popup exterior unchanged");
    Check(glGetError() == GL_NO_ERROR, "popup GL errors");
}
static void NestedCornerFrames() {
    glBindBuffer(0x88EC, 0);
    for (float scale : {0.75f, 1.0f, 1.5f})
    for (float opacity : {0.25f, 0.5f, 1.0f})
    for (auto tint : {IM_COL32(20, 20, 20, 255), IM_COL32(220, 220, 220, 255), IM_COL32(20, 20, 20, 160)}) {
        blur::ReleaseOpenGL();
        glDisable(GL_SCISSOR_TEST); glClearColor(1, 1, 1, 1); glClear(GL_COLOR_BUFFER_BIT);
        blur::RenderOpenGL(options, 128, 128);
        ImGui_ImplOpenGL3_NewFrame(); ImGui::NewFrame();
        auto* draws = ImGui::GetBackgroundDrawList();
        blur::Background(draws, ImVec2(0, 0), ImVec2(128, 128), tint, 0);
        blur::Background(draws, ImVec2(16.25f, 16.25f), ImVec2(111.75f, 111.75f), tint, 12 * scale, 0, opacity);
        ImGui::Render(); ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData()); glFinish();
        const int interior = Pixel(64, 64);
        // Identical glass over itself must keep every edge/corner flat,
        // including light/dark themes, translucent tint and fade animation.
        for (int y = 12; y < 116; ++y)
            for (int x = 12; x < 116; ++x)
                Check(std::abs(Pixel(x, y) - interior) <= 2, "nested blur bright or dark edge fringe");
    }
}

int main() {
    try {
        blur::Refresh timer; const auto start = blur::Refresh::Clock::time_point{};
        Check(timer.Due(options, start), "first refresh"); timer.Updated(options, start);
        Check(!timer.Due(options, start + std::chrono::milliseconds(199)), "FPS throttling");
        Check(timer.Due(options, start + std::chrono::milliseconds(200)), "refresh deadline");
        Check(timer.Due({true, 6, 5}, start), "strength forces refresh");
        WNDCLASSW wc{}; wc.lpfnWndProc = DefWindowProcW; wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = L"NovaBlurTest"; wc.style = CS_OWNDC; RegisterClassW(&wc);
        auto win = CreateWindowW(wc.lpszClassName, L"Nova blur test", WS_POPUP, 0, 0, 128, 128, nullptr, nullptr, wc.hInstance, nullptr);
        auto dc = GetDC(win); PIXELFORMATDESCRIPTOR pfd{}; pfd.nSize = sizeof(pfd); pfd.nVersion = 1;
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER; pfd.iPixelType = PFD_TYPE_RGBA; pfd.cColorBits = 32; pfd.cAlphaBits = 8;
        Check(SetPixelFormat(dc, ChoosePixelFormat(dc, &pfd), &pfd), "pixel format");
        auto context = wglCreateContext(dc); Check(context && wglMakeCurrent(dc, context), "GL context");
        printf("OpenGL: %s\n", glGetString(GL_RENDERER));
        ImGui::CreateContext(); ImGui::GetIO().IniFilename = nullptr;
        Check(ImGui_ImplOpenGL3_Init("#version 130"), "ImGui renderer");
        GLuint pbo{};
        reinterpret_cast<void(APIENTRY*)(GLsizei, GLuint*)>(nova::gl::resolve("glGenBuffers"))(1, &pbo);
        glBindBuffer(0x88EC, pbo);
        reinterpret_cast<void(APIENTRY*)(GLenum, ptrdiff_t, const void*, GLenum)>(nova::gl::resolve("glBufferData"))(0x88EC, 1, nullptr, 0x88E4);
        Frame(); Check(blur::Available() && Pixel(63, 80) > 5 && Pixel(63, 80) < 45, "actual blurred edge");
        // Compare the GPU's half-resolution result to the analytical Gaussian
        // convolution of the step edge, not merely a softer-looking pixel.
        glBindTexture(GL_TEXTURE_2D,static_cast<GLuint>(blur::BackdropTexture()));
        unsigned char gaussian[64*64*3]{};glGetTexImage(GL_TEXTURE_2D,0,GL_RGB,GL_UNSIGNED_BYTE,gaussian);
        const blur::GaussianKernel kernel(3);
        for(int x=8;x<56;++x) {
            float expected=0;
            for(int k=-kernel.radius;k<=kernel.radius;++k)if(x+k>=32)expected+=kernel.weights[std::abs(k)];
            Check(std::abs(gaussian[(32*64+x)*3]-int(std::lround(expected*255)))<=2,"GPU Gaussian step response");
        }
        blur::ReleaseOpenGL();
        Frame(true, 128, 128, IM_COL32(0, 0, 0, 255), 1.f, 0.f);
        Check(Pixel(4, 4, 3) == 0, "test scene really has zero alpha");
        Check(Pixel(63, 80) > 5 && Pixel(63, 80) < 45, "zero-alpha game scene still produces visible blur");
        options.strength = 1; Frame(); const int weak = Pixel(50, 80);
        options.strength = 6; Frame(); Check(Pixel(50, 80) > weak + 5, "strength widens blur");
        const int cached = Pixel(80, 80); Frame(false);
        Check(Pixel(80, 80) == cached, "cached background between refreshes");
        const auto hudTint = IM_COL32(0, 0, 0, 204);
        options.strength = 3; Frame(true, 128, 128, hudTint);
        const int glass = Pixel(80, 80);
        Frame(false, 128, 128, hudTint);
        Check(Pixel(80, 80) == glass && Pixel(80, 80, 1) == glass,
              "HUD color alpha does not leak unblurred scene");
        Frame(false, 128, 128, hudTint, 0.5f);
        Check(std::abs(Pixel(80, 80) - (255 + glass) / 2) <= 2 &&
              std::abs(Pixel(80, 80, 1) - glass / 2) <= 2, "glass opacity fades both layers together");
        Frame(false, 128, 128, hudTint, 0.0f);
        Check(Pixel(80, 80) == 255 && Pixel(80, 80, 1) == 0, "zero opacity removes tint and blur");
        options.enabled = false; Frame(false); Check(!blur::Available() && Pixel(80, 80) == 0, "disable restores normal panel");
        options.enabled = true; Frame(false); Check(Pixel(80, 80, 1) == 0, "reenable refreshes scene");
        Frame(false, 96, 96); Check(blur::Available(), "resize");
        blur::ReleaseOpenGL(); Frame(); Check(blur::Available(), "resource recreation");
        PopupFrames();
        NestedCornerFrames();
        blur::ReleaseOpenGL(); glBindBuffer(0x88EC, 0);
        reinterpret_cast<void(APIENTRY*)(GLsizei, const GLuint*)>(nova::gl::resolve("glDeleteBuffers"))(1, &pbo);
        ImGui_ImplOpenGL3_Shutdown(); ImGui::DestroyContext();
        wglMakeCurrent(nullptr, nullptr); wglDeleteContext(context); ReleaseDC(win, dc); DestroyWindow(win);
        puts("PASS: local rounded blur, sharp foreground, unchanged scene, strength, FPS cache, PBO/sRGB state, resize/recreation, nested popups, native border/scrollbar and widget channels");
        return 0;
    } catch (const std::exception& error) { fprintf(stderr, "FAIL: %s\n", error.what()); return 1; }
}
