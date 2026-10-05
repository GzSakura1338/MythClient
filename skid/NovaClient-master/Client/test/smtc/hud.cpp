// Real MusicHUD/ImGui rendering with a fake media service. Never controls a player.
#include <windows.h>
#include <GL/gl.h>
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_opengl3.h"
#include "ui/hud/musichud/MusicHud.h"
#include "ui/hud/musichud/CoverTexture.h"
#include "smtc/SmtcService.h"
#include "ui/clickgui/Fonts.h"
#include "ui/clickgui/panel/ClientSettings.h"
#include "ui/backend/blur/Blur.h"
#include "ui/overlay/HudBackground.h"
#include <cstdio>
#include <fstream>
#include <stdexcept>

static bool editing{}, coverPanel{};
static int commands{};
static std::string selectedSource;
static ImVec2 cardMin, cardMax;
static auto snapshot = std::make_shared<nova::smtc::MediaSnapshot>();
namespace nova::clickgui { bool IsOpen() { return editing; } }
namespace nova::smtc {
void Start() {} void RequestStop() {} void SelectSource(std::string value) { selectedSource = std::move(value); }
void SetLyricsEnabled(bool) {}
std::shared_ptr<const MediaSnapshot> ReadSnapshot() { return snapshot; }
bool Enqueue(MediaCommand command) { ++commands; return command.session == snapshot->session && command.track == snapshot->track; }
}
namespace nova::ui::blur { Options GetOptions() { return {}; } }
namespace nova::ui::hud {
void Background(ImDrawList* draws, ImVec2 min, ImVec2 max, ImU32 color, float radius, ImDrawFlags flags, float visibility) {
    if (color == 0xCC331914) { cardMin = min; cardMax = max; }
    blur::Background(draws, min, max, color, radius, flags, visibility);
}
}
static void Check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
int main(int argc, char** argv) {
    try {
        WNDCLASSW wc{}; wc.lpfnWndProc = DefWindowProcW; wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = L"NovaMusicFixture"; wc.style = CS_OWNDC; RegisterClassW(&wc);
        auto window = CreateWindowW(wc.lpszClassName, L"MusicHUD fixture", WS_POPUP, 0, 0, 800, 500, nullptr, nullptr, wc.hInstance, nullptr);
        Check(window != nullptr, "hidden fixture window"); auto dc = GetDC(window);
        PIXELFORMATDESCRIPTOR pfd{}; pfd.nSize = sizeof(pfd); pfd.nVersion = 1;
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER; pfd.iPixelType = PFD_TYPE_RGBA; pfd.cColorBits = 32;
        Check(SetPixelFormat(dc, ChoosePixelFormat(dc, &pfd), &pfd), "pixel format");
        auto gl = wglCreateContext(dc); Check(gl && wglMakeCurrent(dc, gl), "GL context");
        ImGui::CreateContext(); ImGui::GetIO().IniFilename = nullptr;
        InitializeFatalMenuFonts(); Check(ImGui_ImplOpenGL3_Init("#version 130"), "backend");
        auto hud = std::make_unique<nova::ui::music::MusicHud>(); hud->SetEnabled(true);
        auto frame = [&] {
            auto& io = ImGui::GetIO(); io.DisplaySize = ImVec2(800, 500); io.DeltaTime = 1.f / 60;
            ImGui_ImplOpenGL3_NewFrame(); ImGui::NewFrame();
            hud->Draw(editing);
            if (coverPanel) {
                ImGui::SetNextWindowPos(cardMin); ImGui::SetNextWindowSize(ImVec2(300, 140));
                ImGui::Begin("Mock ClickGUI panel", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDecoration);
                ImGui::TextUnformatted("Panel above the HUD"); ImGui::End();
            }
            ImGui::Render(); glViewport(0, 0, 800, 500); glClearColor(.08f, .10f, .15f, 1); glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData()); glFinish();
            Check(glGetError() == GL_NO_ERROR, "HUD GL errors");
        };
        auto settle = [&] { for (int i = 0; i < 45; ++i) frame(); };
        auto clickToggle = [&] {
            ImGui::GetIO().AddMousePosEvent(cardMax.x - 64, cardMin.y + 77); frame(); frame();
            ImGui::GetIO().AddMouseButtonEvent(0, true); frame();
            ImGui::GetIO().AddMouseButtonEvent(0, false); frame();
        };
        editing = true; settle(); clickToggle(); Check(commands == 0, "preview must not control media");
        snapshot->state = nova::smtc::State::Ready; snapshot->session = snapshot->track = 1;
        snapshot->title = "A song for the road"; snapshot->artist = "Nova Music";
        snapshot->playing = snapshot->canToggle = snapshot->canPrevious = snapshot->canNext = snapshot->canSeek = true;
        snapshot->end = 240; snapshot->position = 85;
        snapshot->sources = {{"Chrome", "Chrome"}, {"cloudmusic.exe", "NetEase Cloud Music"}};
        auto art = std::make_shared<nova::smtc::Artwork>(); art->width = art->height = 64; art->version = 1;
        for (int y = 0; y < 64; ++y) for (int x = 0; x < 64; ++x) art->rgba.push_back(IM_COL32(100 + x * 2, 70 + y, 180, 255));
        snapshot->artwork = art; settle();
        snapshot->lyrics = nova::smtc::ParseYrcLyrics("[0,40000](0,40000,0)A song for the road\n"
            "[40000,40000](40000,40000,0)A line before\n"
            "[80000,40000](80000,10000,0)Every (90000,10000,0)step (100000,10000,0)brings us (110000,10000,0)home\n"
            "[120000,40000](120000,40000,0)The next line\n[160000,40000](160000,40000,0)Upcoming lyrics",
            "[01:20]A translated line");
        snapshot->rate = 0; settle();
        auto* lyricsWindow = ImGui::FindWindowByName("overlays.musiclyrics");
        auto* musicWindow = ImGui::FindWindowByName("overlays.musichud");
        Check(lyricsWindow && lyricsWindow->Active && musicWindow && lyricsWindow != musicWindow,
            "lyrics has an independent draggable overlay under MusicHUD");
        const auto musicPosition = musicWindow->Pos;
        ImGui::SetWindowPos(lyricsWindow, ImVec2(420, 220)); settle();
        Check(musicWindow->Pos.x == musicPosition.x && musicWindow->Pos.y == musicPosition.y, "moving lyrics does not move music card");
        snapshot->position = 125; settle(); snapshot->position = 85; settle();
        Check(cardMax.x - cardMin.x > 259 && cardMax.y - cardMin.y > 93, "editing card fully expanded");
        clickToggle(); Check(commands == 1, "editing play/pause interaction");
        coverPanel = true; settle(); clickToggle(); Check(commands == 1, "covered HUD must not steal panel clicks");
        coverPanel = false; editing = false; settle(); clickToggle(); Check(commands == 1, "gameplay must not control media");
        editing = true; settle();
        auto save = [&](const std::string& path) {
            std::vector<unsigned char> pixels(800 * 500 * 4); glReadPixels(0, 0, 800, 500, 0x80E1, GL_UNSIGNED_BYTE, pixels.data());
            BITMAPFILEHEADER file{}; file.bfType = 0x4d42; file.bfOffBits = sizeof(file) + sizeof(BITMAPINFOHEADER); file.bfSize = file.bfOffBits + static_cast<DWORD>(pixels.size());
            BITMAPINFOHEADER info{}; info.biSize = sizeof(info); info.biWidth = 800; info.biHeight = 500; info.biPlanes = 1; info.biBitCount = 32;
            std::ofstream out(path, std::ios::binary); out.write(reinterpret_cast<char*>(&file), sizeof(file)); out.write(reinterpret_cast<char*>(&info), sizeof(info)); out.write(reinterpret_cast<char*>(pixels.data()), pixels.size());
        };
        auto click = [&](float x, float y) {
            ImGui::GetIO().AddMousePosEvent(x, y); frame(); frame();
            ImGui::GetIO().AddMouseButtonEvent(0, true); frame();
            ImGui::GetIO().AddMouseButtonEvent(0, false); frame(); settle();
        };
        for (int theme = 0; theme < 2; ++theme) {
            nova::menu::panel::ClientSettings::Get().theme().set(theme);
            click(cardMax.x - 14, cardMin.y + 77);
            auto& popups = ImGui::GetCurrentContext()->OpenPopupStack;
            Check(!popups.empty() && popups.back().Window, "media source menu opened");
            auto* popup = popups.back().Window;
            Check(popup->WindowRounding >= 11 && popup->WindowPadding.x >= 5, "rounded padded source menu");
            Check(popup->Pos.x >= 0 && popup->Pos.y >= 0 && popup->Pos.y + popup->Size.y <= 500, "source menu fits display");
            if (argc > 1) save(std::string(argv[1]) + (theme ? ".dark.bmp" : ".light.bmp"));
            click(popup->Pos.x + popup->Size.x * .5f, popup->Pos.y + popup->Size.y - 20);
            Check(selectedSource == "cloudmusic.exe", "source selection propagated");
            Check(ImGui::GetCurrentContext()->OpenPopupStack.empty(), "source selection closes menu");
        }
        Check(commands == 1, "source menu does not send playback commands");
        if (argc > 1) save(argv[1]);
        ImGui::GetStyle().FontScaleMain = 1.5f; settle(); Check(cardMax.x - cardMin.x > 389, "global HUD scaling");
        hud->SetEnabled(false); settle(); hud.reset();
        ImGui_ImplOpenGL3_Shutdown(); nova::ui::music::ReleaseCovers(); ImGui::DestroyContext(); ResetFatalMenuFonts();
        wglMakeCurrent(nullptr, nullptr); wglDeleteContext(gl); ReleaseDC(window, dc); DestroyWindow(window);
        puts("PASS: MusicHUD preview, expand/close, scaling, edit controls and panel/gameplay input isolation");
    } catch (const std::exception& error) { fprintf(stderr, "FAIL: %s\n", error.what()); return 1; }
}
