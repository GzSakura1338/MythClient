#include "minecraft/host/render/OverlayHost.h"
#include "ui/backend/blur/Blur.h"
#include "minecraft/java/api/base.h"
#include "minecraft/java/api/game_input.h"
#include "minecraft/java/api/game_click.h"
#include "bind/InputManager.h"
#include "bind/BindCapture.h"
#include "bind/UnloadKey.h"
#include "minecraft/events/Events.h"
#include "minecraft/modules/ModuleManager.h"
#include "minecraft/modules/misc/imefix/ImeFix.h"
#include "ui/clickgui/ClickGui.h"
#include "utils/Keyboard.h"
#include "third_party/minhook/include/MinHook.h"
#include "third_party/fatalwidgets/imgui/imgui.h"
#include "third_party/fatalwidgets/imgui/imgui_internal.h"
#include "third_party/fatalwidgets/imgui/imgui_impl_win32.h"
#include "third_party/fatalwidgets/imgui/imgui_impl_opengl3.h"
#include "ui/clickgui/Fonts.h"
#include "ui/backend/PixelUnpackGuard.h"
#include "ui/backend/GlExt.h"
#include "ui/backend/OpenGLSurface.h"
#include "ui/backend/Win32Surface.h"
#include "config/ConfigManager.h"
#include "ui/overlay/WindowPlacement.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/compat/lunar189/LunarRuntime.h"
#include "minecraft/compat/minecraft263/Minecraft263.h"
#include "ui/skin/SkinTextures.h"
#include "ui/hud/musichud/CoverTexture.h"
#include "ui/clickgui/browser/BrowserPage.h"
#include "chrome/BrowserService.h"
#include "ui/workspace/ForegroundWorkspace.h"
#include <atomic>
#include <exception>
#include <optional>
#include <mutex>
#include <deque>

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace nova::glhook {
using SwapFn = void (__cdecl*)(void*);
using GetWindowFn = HWND (__cdecl*)(void*);
static SwapFn originalSwap = nullptr;
using LwjglSwapFn = void (JNICALL*)(JNIEnv*, jclass, jobject);
static LwjglSwapFn originalLwjglSwap = nullptr;
// SDL3 returns a C bool (not Win32 BOOL), and the original return value must survive.
using SdlSwapFn = bool (__cdecl*)(void*);
static SdlSwapFn originalSdlSwap = nullptr;
static OpenGLSurface glSurface;
static Win32Surface win32Surface;
// Forge and modern GLFW clients may own the HWND on a non-render thread.
static std::atomic<bool> threadedInput{false}, threadedCapture{false}, threadedAccept{false};
struct QueuedWindowInput { HWND window; UINT message; WPARAM wp; LPARAM lp; };
static std::mutex threadedInputMutex;
static std::deque<QueuedWindowInput> threadedInputQueue;
static bool replayingWindowInput = false; // Only touched on the render thread.
static DWORD renderThread = 0;
static GetWindowFn getWin32Window = nullptr;
static void* swapTarget = nullptr;
static HWND hwnd = nullptr;
static const UINT attackPulseMessage=RegisterWindowMessageW(L"Nova.AutoClicker.MousePulse.v1");
static std::atomic<unsigned> attackPulseGeneration{1};
static std::atomic<bool> attackPulsePending{false};

void CancelAttackMousePulse() {
    attackPulseGeneration.fetch_add(1);
    attackPulsePending.store(false);
}

bool QueueAttackMousePulse() {
    if(!compat::IsLegacyLunar() || !attackPulseMessage || !hwnd || GetForegroundWindow()!=hwnd ||
        nova::clickgui::CapturesInput() || nova::java::ScreenOpen() || !(GetAsyncKeyState(VK_LBUTTON)&0x8000))return false;
    if(attackPulsePending.exchange(true))return false;
    const unsigned generation=attackPulseGeneration.load();
    if(PostMessageW(hwnd,attackPulseMessage,generation,0))return true;
    attackPulsePending.store(false);
    return false;
}
static void* glfwWindow = nullptr;
static bool imguiContext = false;
static bool imguiPlatform = false;
static bool imguiRenderer = false;
static bool installed = false;
static std::atomic<bool> teardownDone{false};
static std::atomic<bool> guardTripped{false};
static std::atomic<bool> rendererUnusable{false};
static std::atomic<bool> firstFrame{false};

constexpr int kGlfwCursor = 0x00033001;
constexpr int kGlfwCursorNormal = 0x00034001;
constexpr int kGlfwCursorDisabled = 0x00034003;

static void TripGuard(const char* where) {
    if (guardTripped.exchange(true, std::memory_order_acq_rel))
        return;
    LogTo("Base: a C++ exception escaped %s; the overlay disables itself to keep the game alive",
          where);
    OpenCircuit();
    ReportBootstrapFailed("The render or input callback raised an exception");
    RequestUnload();
}

using SetInputModeFn = void (__cdecl*)(void*, int, int);
using GetInputModeFn = int (__cdecl*)(void*, int);
static SetInputModeFn setInputMode = nullptr;
static GetInputModeFn getInputMode = nullptr;
static int gameCursorMode = kGlfwCursorDisabled;
static bool cursorTaken = false;

struct GlStateGuard {
    GLint program = 0, texture = 0, arrayBuffer = 0, vertexArray = 0;
    GLint activeTexture = 0, drawFramebuffer = 0, readFramebuffer = 0;
    GLint viewport[4]{};
    GLint scissorBox[4]{};
    GLboolean blend = GL_FALSE, cull = GL_FALSE, depth = GL_FALSE, scissor = GL_FALSE;
    GlStateGuard() {
        glGetIntegerv(GL_CURRENT_PROGRAM, &program);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &arrayBuffer);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vertexArray);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFramebuffer);
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFramebuffer);
        glGetIntegerv(GL_VIEWPORT, viewport);
        glGetIntegerv(GL_SCISSOR_BOX, scissorBox);
        blend = glIsEnabled(GL_BLEND); cull = glIsEnabled(GL_CULL_FACE);
        depth = glIsEnabled(GL_DEPTH_TEST); scissor = glIsEnabled(GL_SCISSOR_TEST);
    }
    ~GlStateGuard() {
        if (blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
        if (cull) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
        if (depth) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
        if (scissor) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
        glUseProgram(program);
        glActiveTexture(activeTexture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glBindBuffer(GL_ARRAY_BUFFER, arrayBuffer);
        glBindVertexArray(vertexArray);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, drawFramebuffer);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, readFramebuffer);
        glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
        glScissor(scissorBox[0], scissorBox[1], scissorBox[2], scissorBox[3]);
    }
};

static bool IsGameInput(UINT message) {
    switch (message) {
        case WM_KEYDOWN: case WM_CHAR: case WM_DEADCHAR: case WM_UNICHAR:
        case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
        case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK:
        case WM_MOUSEMOVE: case WM_NCMOUSEMOVE:
        case WM_MOUSEWHEEL: case WM_MOUSEHWHEEL:
        case WM_INPUT:
            return true;
        default:
            return false;
    }
}

static int ButtonOf(UINT message, WPARAM wp) {
    switch (message) {
        case WM_LBUTTONDOWN: case WM_LBUTTONUP: case WM_LBUTTONDBLCLK: return 0;
        case WM_RBUTTONDOWN: case WM_RBUTTONUP: case WM_RBUTTONDBLCLK: return 1;
        case WM_MBUTTONDOWN: case WM_MBUTTONUP: case WM_MBUTTONDBLCLK: return 2;
        case WM_XBUTTONDOWN: case WM_XBUTTONUP: case WM_XBUTTONDBLCLK:
            return ((wp >> 16) & 0xFFFF) == XBUTTON1 ? 3 : 4;
        default: return -1;
    }
}

static bool IsButtonDownMessage(UINT message) {
    return message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN ||
           message == WM_MBUTTONDOWN || message == WM_XBUTTONDOWN ||
           message == WM_LBUTTONDBLCLK || message == WM_RBUTTONDBLCLK ||
           message == WM_MBUTTONDBLCLK || message == WM_XBUTTONDBLCLK;
}

static int KeyOfMessage(WPARAM wp, LPARAM lp) {
    const auto scan = static_cast<UINT>((static_cast<unsigned long long>(lp) >> 16) & 0xFF);
    const bool extended = (lp & (1ll << 24)) != 0;

    switch (static_cast<int>(wp)) {
        case VK_SHIFT: {
            const UINT resolved = MapVirtualKeyW(scan, MAPVK_VSC_TO_VK_EX);
            return resolved == 0 ? VK_SHIFT : static_cast<int>(resolved);
        }
        case VK_CONTROL:
            return extended ? VK_RCONTROL : VK_LCONTROL;
        case VK_MENU:
            return extended ? VK_RMENU : VK_LMENU;
        default:
            return static_cast<int>(wp);
    }
}

static bool DispatchToBind(UINT msg, WPARAM wp, LPARAM lp) {
    using nova::bind::InputManager;

    switch (msg) {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN: {
            if ((lp & (1 << 30)) != 0)
                return true;
            return InputManager::Get().onKey(KeyOfMessage(wp, lp), true);
        }
        case WM_KEYUP:
        case WM_SYSKEYUP:
            return InputManager::Get().onKey(KeyOfMessage(wp, lp), false);
        case WM_CHAR:
        case WM_SYSCHAR:
            return InputManager::Get().onChar(static_cast<char>(wp & 0xFF));
        case WM_MOUSEWHEEL:
            return InputManager::Get().onMouseScroll(
                0.0f, static_cast<float>(GET_WHEEL_DELTA_WPARAM(wp)) / 120.0f);
        case WM_MOUSEHWHEEL:
            return InputManager::Get().onMouseScroll(
                static_cast<float>(GET_WHEEL_DELTA_WPARAM(wp)) / 120.0f, 0.0f);
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_XBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        case WM_XBUTTONUP: {
            const int button = ButtonOf(msg, wp);
            return button < 0 ||
                   InputManager::Get().onMouseButton(button, IsButtonDownMessage(msg));
        }
        default:
            return true;
    }
}

static LRESULT PassToGame(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (replayingWindowInput) return 0; // Original message was already handled on its owner thread.
    return DefSubclassProc(hwnd, msg, wp, lp);
}

static LRESULT HandleWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if(attackPulseMessage && msg==attackPulseMessage) {
        if(wp!=attackPulseGeneration.load() || !attackPulsePending.exchange(false))return 0;
        // Revalidate on delivery: never replay into a GUI, another window, or
        // after the physical hold ended. Bypass Nova's physical keybind routing.
        if(guardTripped.load() || teardownDone.load() || !compat::IsLegacyLunar() || GetForegroundWindow()!=hwnd ||
            nova::clickgui::CapturesInput() || nova::java::ScreenOpen() ||
            !nova::java::MouseGrabbed() || !(GetAsyncKeyState(VK_LBUTTON)&0x8000))return 0;
        POINT cursor{};
        if(!GetCursorPos(&cursor) || !ScreenToClient(hwnd,&cursor))return 0;
        WPARAM buttons=0;
        if(GetAsyncKeyState(VK_RBUTTON)&0x8000)buttons|=MK_RBUTTON;
        if(GetAsyncKeyState(VK_MBUTTON)&0x8000)buttons|=MK_MBUTTON;
        if(GetAsyncKeyState(VK_SHIFT)&0x8000)buttons|=MK_SHIFT;
        if(GetAsyncKeyState(VK_CONTROL)&0x8000)buttons|=MK_CONTROL;
        const LPARAM point=MAKELPARAM(cursor.x,cursor.y);
        // Delivery is asynchronous: a vanilla air swing may have restored the
        // miss cooldown since AutoClicker queued this pulse.
        nova::java::ClearMissTime();
        PassToGame(hwnd,WM_LBUTTONUP,buttons,point);
        PassToGame(hwnd,WM_LBUTTONDOWN,buttons|MK_LBUTTON,point);
        return 0;
    }
    if (imguiPlatform && nova::clickgui::IsOpen() &&
        nova::clickgui::GetMode() == nova::clickgui::Mode::Panel &&
        msg == WM_KEYDOWN && wp == VK_ESCAPE) {
        nova::bind::BindCapture::Get().Cancel();
        ImGui::ClosePopupsOverWindow(nullptr, false);
        ImGui::ClearActiveID();
        nova::clickgui::Close();
        return 0;
    }
    const bool taken = !DispatchToBind(msg, wp, lp);

    if (imguiPlatform && nova::clickgui::CapturesInput()) {
        const LRESULT consumed = ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp);
        if (msg == WM_KILLFOCUS || msg == WM_SETFOCUS) {
            const LRESULT result = PassToGame(hwnd, msg, wp, lp);
            if (msg == WM_KILLFOCUS)
                return PassToGame(hwnd, WM_SETFOCUS, 0, 0);
            return result;
        }
        if (msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP || msg == WM_SYSCHAR) {
            return DefWindowProcW(hwnd, msg, wp, lp);
        }
        if (msg == WM_KEYDOWN && wp == VK_ESCAPE) {
            if (!taken && !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId |
                                                          ImGuiPopupFlags_AnyPopupLevel))
                nova::clickgui::Close();
            return 0;
        }
        if (IsGameInput(msg)) {
            return msg == WM_INPUT ? DefWindowProcW(hwnd, msg, wp, lp) : 0;
        }
        if (taken)
            return 0;
        if (consumed)
            return consumed;
    } else if (taken) {
        return 0;
    }
    if (msg == WM_KEYDOWN || msg == WM_KEYUP || msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP) {
        nova::event::KeyboardEvent event{static_cast<int>(wp), static_cast<int>((lp >> 16) & 0xFF),
                                         msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN,
                                         (lp & (1 << 30)) != 0};
        if (nova::event::EventBus::Get().Post(event))
            return 0;
    } else if (const int button = ButtonOf(msg, wp); button >= 0) {
        nova::event::MouseButtonEvent event{button, IsButtonDownMessage(msg)};
        if (nova::event::EventBus::Get().Post(event))
            return 0;
    }
    return PassToGame(hwnd, msg, wp, lp);
}

static LRESULT CALLBACK HookWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
                                    UINT_PTR subclassId, DWORD_PTR binding) {
    if (nova::module::imefix::WindowMessage(hwnd, msg, wp, lp)) return 0;
    if(nova::chrome::Focused() && (msg==WM_KEYDOWN||msg==WM_SYSKEYDOWN||msg==WM_KEYUP||msg==WM_SYSKEYUP) &&
        nova::bind::unload::Input(KeyOfMessage(wp,lp),msg==WM_KEYDOWN||msg==WM_SYSKEYDOWN)){
        RequestUnload();return 0;
    }
    if(nova::ui::workspace::WindowMessage(msg,wp,lp))return 0;
    const bool browserConsumed=nova::clickgui::CapturesInput()&&nova::chrome::WindowMessage(hwnd,msg,wp,lp);
    if(browserConsumed&&msg!=WM_KEYUP&&msg!=WM_SYSKEYUP)return 0;
    if (threadedInput.load(std::memory_order_acquire)) {
        if (msg == WM_NCDESTROY) {
            reinterpret_cast<Win32Surface*>(binding)->Destroyed(hwnd);
            RemoveWindowSubclass(hwnd, HookWndProc, subclassId);
            return DefSubclassProc(hwnd, msg, wp, lp);
        }
        const bool queueable = (msg >= WM_KEYFIRST && msg <= WM_KEYLAST) ||
            (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST) || msg == WM_SETFOCUS || msg == WM_KILLFOCUS;
        const bool accepting = threadedAccept.load(std::memory_order_acquire);
        if (accepting && queueable) {
            std::lock_guard lock(threadedInputMutex);
            if (msg == WM_MOUSEMOVE && !threadedInputQueue.empty() && threadedInputQueue.back().message == msg)
                threadedInputQueue.back() = {hwnd,msg,wp,lp};
            else if (threadedInputQueue.size() < 4096)
                threadedInputQueue.push_back({hwnd,msg,wp,lp});
        }
        if (accepting && threadedCapture.load(std::memory_order_acquire) && IsGameInput(msg))
            return msg == WM_INPUT ? DefWindowProcW(hwnd,msg,wp,lp) : 0;
        return DefSubclassProc(hwnd,msg,wp,lp);
    }
    if (msg == WM_NCDESTROY) {
        CancelAttackMousePulse();
        reinterpret_cast<Win32Surface*>(binding)->Destroyed(hwnd);
        RemoveWindowSubclass(hwnd, HookWndProc, subclassId);
        return PassToGame(hwnd, msg, wp, lp);
    }
    try {
        return HandleWndProc(hwnd, msg, wp, lp);
    } catch (const std::exception& error) {
        LogTo("Base: the window procedure raised '%s'", error.what());
        TripGuard("the window procedure");
    } catch (...) {
        TripGuard("the window procedure");
    }
    return PassToGame(hwnd, msg, wp, lp);
}

static void UpdateInputCapture() {
    if (compat::Current().renderer == compat::RenderBackend::Lwjgl2 || compat::UsesModernRuntime()) {
        const bool open = nova::clickgui::CapturesInput();
        if (open || cursorTaken) {
            if (compat::CaptureMouse(open)) cursorTaken = open;
        }
        return;
    }
    if (!glfwWindow || !setInputMode)
        return;
    const bool open = nova::clickgui::CapturesInput();

    if (open) {
        const bool wasOpen = cursorTaken;
        if (!wasOpen) {
            cursorTaken = true;
            gameCursorMode = getInputMode ? getInputMode(glfwWindow, kGlfwCursor) : kGlfwCursorDisabled;
        }
        if (!nova::java::ReleaseMouse())
            setInputMode(glfwWindow, kGlfwCursor, kGlfwCursorNormal);
        if (!wasOpen)
            LogTo("Base: ClickGUI opened; cursor released to the system");
        return;
    }

    if (!cursorTaken)
        return;
    cursorTaken = false;
    if (!nova::java::GrabMouse())
        setInputMode(glfwWindow, kGlfwCursor, gameCursorMode);
    LogTo("Base: ClickGUI closed; cursor handed back to the game");
}

static void SyncFontAtlas() {
    ImFontAtlas* atlas = ImGui::GetIO().Fonts;
    ImDrawListSharedData* shared = ImGui::GetDrawListSharedData();
    if (atlas == nullptr || shared == nullptr)
        return;
    if (shared->FontAtlas == atlas &&
        shared->TexUvWhitePixel.x == atlas->TexUvWhitePixel.x &&
        shared->TexUvWhitePixel.y == atlas->TexUvWhitePixel.y)
        return;

    shared->FontAtlas = atlas;
    shared->TexUvWhitePixel = atlas->TexUvWhitePixel;
    shared->TexUvLines = atlas->TexUvLines;
    static std::atomic<bool> reported{false};
    if (!reported.exchange(true, std::memory_order_acq_rel))
        LogTo("Base: the draw lists were bound to a stale font atlas; solid fills were re-synced");
}

static void LogFontAtlas(const ImDrawData* data) {
    static int logged = 0;
    static int lastTexWidth = 0;
    static int lastTexHeight = 0;
    if (data == nullptr || data->CmdListsCount <= 0)
        return;
    const ImDrawList* list = data->CmdLists[0];
    if (list == nullptr || list->CmdBuffer.Size <= 0 || list->VtxBuffer.Size <= 0)
        return;

    const ImFontAtlas* atlas = ImGui::GetIO().Fonts;
    const int texWidth = atlas->TexData != nullptr ? atlas->TexData->Width : -1;
    const int texHeight = atlas->TexData != nullptr ? atlas->TexData->Height : -1;
    if (logged > 0 && texWidth == lastTexWidth && texHeight == lastTexHeight)
        return;
    ++logged;
    lastTexWidth = texWidth;
    lastTexHeight = texHeight;

    const ImDrawListSharedData* shared = ImGui::GetDrawListSharedData();
    const ImDrawVert& vtx = list->VtxBuffer[0];
    LogTo("Base: atlas tex=%dx%d list=%d white=(%.5f,%.5f) shared=(%.5f,%.5f) bound=%d "
          "font=%p size=%.1f wbg=0x%08X vcol=0x%08X vuv=(%.5f,%.5f) cmds=%d",
          texWidth, texHeight, atlas->TexList.Size,
          atlas->TexUvWhitePixel.x, atlas->TexUvWhitePixel.y, shared->TexUvWhitePixel.x,
          shared->TexUvWhitePixel.y, shared->FontAtlas == atlas ? 1 : 0,
          static_cast<const void*>(ImGui::GetFont()), ImGui::GetFontSize(),
          static_cast<unsigned>(ImGui::GetColorU32(ImGuiCol_WindowBg)),
          static_cast<unsigned>(vtx.col), vtx.uv.x, vtx.uv.y, list->CmdBuffer.Size);
}

static bool RetireOpenGLRenderer() {
    const bool restored = glSurface.Retire([](bool contextAvailable) {
        ui::blur::ResetOpenGLContext(contextAvailable);
        if (imguiRenderer) ImGui_ImplOpenGL3_Shutdown(!contextAvailable);
        imguiRenderer = false;
    });
    gl::ResetPointers();
    return restored;
}

static void TeardownOnRenderThread() {
    if (threadedInput.load()) {
        threadedAccept.store(false);
        threadedCapture.store(false);
        // Keep the DLL and ImGui context alive until the window thread removes
        // its callback. Never block that thread against the game's render loop.
        if (!win32Surface.Reset()) return;
        std::lock_guard lock(threadedInputMutex);
        threadedInputQueue.clear();
    }
    CancelAttackMousePulse();
    if (compat::Current().renderer == compat::RenderBackend::Lwjgl2 || compat::UsesModernRuntime()) {
        compat::RestoreMouse();
        cursorTaken = false;
    }
    if (cursorTaken && glfwWindow && setInputMode) {
        cursorTaken = false;
        if (!nova::java::GrabMouse())
            setInputMode(glfwWindow, kGlfwCursor, gameCursorMode);
    }
    try {
        nova::config::ConfigManager& configs = nova::config::ConfigManager::Get();
        configs.SaveForUnload();
        LogTo("Base: unload config: %s", configs.Status().c_str());
    } catch (...) {
        LogTo("Base: the config could not be saved before the overlay shut down");
    }
    nova::module::ModuleManager::Get().SetAllEnabled(false);
    nova::module::ModuleManager::Get().Tick();
    if (compat::mc263::IsVulkan()) vkoverlay::ShutdownImGui();
    else RetireOpenGLRenderer();
    imguiRenderer = false;
    if (imguiContext) ui::skins::Release();
    if (imguiContext) ui::music::ReleaseCovers();
    if (imguiContext) ui::browser::ReleaseTextures();
    win32Surface.Reset();
    imguiPlatform = false;
    if (imguiContext) { ImGui::DestroyContext(); imguiContext = false; }
    ResetFatalMenuFonts();
    if (swapTarget) MH_DisableHook(swapTarget);
    if (compat::mc263::IsVulkan()) vkoverlay::DisableHooks();
    teardownDone.store(true, std::memory_order_release);
}

static void RunFrame(void* handle) {
    const bool vulkan = compat::mc263::IsVulkan();
    // This is a window-thread capability, not a choice of Minecraft JNI ABI.
    // Forge keeps its existing game adapter but shares the asynchronous input path.
    const bool allowThreadedInput = compat::UsesGlfwModernRuntime() ||
        compat::Current().id == NovaClient_Forge1201;
    const HWND nextWindow = compat::Current().renderer != compat::RenderBackend::Glfw
        ? static_cast<HWND>(handle)
        : (getWin32Window ? getWin32Window(handle) : nullptr);
    // Ignore transient/auxiliary swaps, but do not pin the first HGLRC: F11 can
    // replace either the window, the context, or both on the game render thread.
    if (!IsWindow(nextWindow)) return;
    if (!vulkan && (!wglGetCurrentContext() ||
        WindowFromDC(wglGetCurrentDC()) != nextWindow)) return;
    if (!vulkan) {
        if (renderThread && renderThread != GetCurrentThreadId()) return;
        renderThread = GetCurrentThreadId();
        if (imguiRenderer && glSurface.context != wglGetCurrentContext()) {
            LogTo("Base: OpenGL context changed (%p -> %p); rebuilding overlay resources",
                  glSurface.context, wglGetCurrentContext());
            if (!RetireOpenGLRenderer()) {
                LogTo("Base: could not restore the game's OpenGL context after resource cleanup");
                OpenCircuit(); RequestUnload(); return;
            }
        }
        glSurface.Capture();
    }
    if (imguiContext && hwnd != nextWindow)
        LogTo("Base: overlay window changed (%p -> %p); rebinding input and display size", hwnd, nextWindow);
    if (glfwWindow != handle && cursorTaken) gameCursorMode = kGlfwCursorDisabled;
    glfwWindow = handle;
    hwnd = nextWindow;
    if (allowThreadedInput) {
        threadedInput.store(GetWindowThreadProcessId(hwnd, nullptr) != GetCurrentThreadId(), std::memory_order_release);
        if (threadedInput.load()) {
            threadedCapture.store(imguiPlatform && nova::clickgui::CapturesInput(), std::memory_order_release);
            threadedAccept.store(!IsCircuitOpen(), std::memory_order_release);
            if (imguiPlatform && !IsCircuitOpen()) {
                std::deque<QueuedWindowInput> messages;
                { std::lock_guard lock(threadedInputMutex); messages.swap(threadedInputQueue); }
                struct ReplayScope { ReplayScope(){replayingWindowInput=true;} ~ReplayScope(){replayingWindowInput=false;} } replay;
                for (const auto& input : messages)
                    if (input.window == hwnd) HandleWndProc(hwnd,input.message,input.wp,input.lp);
            }
        }
    }
    if (IsCircuitOpen()) {
        if (!teardownDone.load(std::memory_order_acquire))
            TeardownOnRenderThread();
    } else {
        nova::input::Update();
        if (nova::bind::unload::Poll()) {
            LogTo("Base: unload binding pressed; beginning full unload");
            OpenCircuit();
            RequestUnload();
            return;
        }
        nova::module::ModuleManager::Get().Tick();
        if (nova::compat::IsLegacyLunar()) nova::compat::lunar::ValidateWorld();
        if (!imguiContext) {
            if (hwnd && ImGui::CreateContext()) {
                imguiContext = true;
                ImGuiIO& io = ImGui::GetIO();
                io.IniFilename = nullptr;
                InitializeFatalMenuFonts();
                io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
                io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
                ImGui::StyleColorsDark();
                ImGui::GetStyle().Colors[ImGuiCol_NavCursor] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
                const float displayScale = ImGui_ImplWin32_GetDpiScaleForHwnd(hwnd);
                ImGuiStyle& style = ImGui::GetStyle();
                style.ScaleAllSizes(displayScale);
                style.FontScaleDpi = displayScale;
                imguiPlatform = win32Surface.Bind(hwnd, HookWndProc, allowThreadedInput);
                // Use the backend's default GLSL selection for this context.
                imguiRenderer = vulkan ? vkoverlay::InitializeImGui() : ImGui_ImplOpenGL3_Init(nullptr);
                LogTo("Base: %s ClickGUI hook initialized (hwnd=%p)", vulkan ? "Vulkan" : "OpenGL", hwnd);
                if (!vulkan) {
                const auto* glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
                const auto* glslVersion =
                    reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));
                LogTo("Base: context GL='%s' GLSL='%s'",
                      glVersion ? glVersion : "?", glslVersion ? glslVersion : "?");

                // Build the renderer's program now, while the failure is still a
                // once-per-session event worth a log line. Left to the frame loop,
                // a build that cannot succeed is retried on every frame, and each
                // attempt that fails on a shader compile returns without deleting
                // the shader it created -- an unbounded GL object leak at frame
                // rate, which is one way to bring a driver down shortly after
                // injection. If the program cannot be built the overlay stands
                // down instead.
                if (imguiRenderer && !ImGui_ImplOpenGL3_CreateDeviceObjects()) {
                    rendererUnusable.store(true, std::memory_order_release);
                    LogTo("Base: ImGui could not build its GL program on this context; the "
                          "overlay is disabled rather than retried every frame");
                }
                }
                if ((!imguiPlatform && !win32Surface.Pending()) || !imguiRenderer || rendererUnusable.load()) {
                    LogTo("Base: overlay init platform=%d pending=%d renderer=%d unusable=%d windowThread=%lu renderThread=%lu error=%lu",
                        imguiPlatform,win32Surface.Pending(),imguiRenderer,rendererUnusable.load(),
                        GetWindowThreadProcessId(hwnd,nullptr),GetCurrentThreadId(),win32Surface.Error());
                    ReportBootstrapFailed("The overlay renderer could not initialize");
                    OpenCircuit();
                    RequestUnload();
                }
            }
        } else {
            imguiPlatform = win32Surface.Bind(hwnd, HookWndProc, allowThreadedInput);
            if (!imguiPlatform) {
                if (allowThreadedInput && !win32Surface.Pending()) {
                    LogTo("Base: window input binding failed; windowThread=%lu renderThread=%lu error=%lu",
                        GetWindowThreadProcessId(hwnd,nullptr),GetCurrentThreadId(),win32Surface.Error());
                    ReportBootstrapFailed("The client window input could not initialize");
                    OpenCircuit(); RequestUnload();
                }
                return;
            }
            if (!imguiRenderer && !rendererUnusable.load()) {
                imguiRenderer = vulkan ? vkoverlay::InitializeImGui() : ImGui_ImplOpenGL3_Init(nullptr);
                if (!imguiRenderer || (!vulkan && !ImGui_ImplOpenGL3_CreateDeviceObjects())) {
                    rendererUnusable.store(true);
                    ReportBootstrapFailed("The overlay could not rebuild its renderer");
                    OpenCircuit(); RequestUnload(); return;
                }
                LogTo("Base: %s overlay renderer restored; UI state retained", vulkan ? "Vulkan" : "OpenGL");
            }
            UpdateInputCapture();
            RECT clientRect{};
            GetClientRect(hwnd, &clientRect);
            const bool drawable = imguiPlatform && imguiRenderer &&
                                  !rendererUnusable.load(std::memory_order_acquire) &&
                                  !IsIconic(hwnd) && clientRect.right > 0 && clientRect.bottom > 0;
            static int frameLog = 0;
            const bool logFrame = drawable && frameLog < 10;
            if (logFrame) {
                ++frameLog;
                LogTo("Base: frame %d platform=%d renderer=%d iconic=%d hwnd=%p open=%d mode=%d",
                      frameLog, imguiPlatform, imguiRenderer, IsIconic(hwnd), hwnd,
                      nova::clickgui::IsOpen(), static_cast<int>(nova::clickgui::GetMode()));
            }
            if (drawable) {
                std::optional<GlStateGuard> state;
                if (!vulkan) state.emplace();
                std::optional<PixelUnpackGuard> unpack;
                // CPU texture uploads must not inherit the game's PBO or pixel
                // offsets. Forge uses the same dynamic font atlas as other GL
                // clients; stale skip rows/pixels can make the driver read past it.
                if (!vulkan) unpack.emplace();
                // RenderPearl may leave its blit framebuffer bound after presenting.
                if (compat::UsesModernRuntime() && !vulkan) glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
                if (vulkan) vkoverlay::NewFrame();
                else ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplWin32_NewFrame();
                ImGui::NewFrame();
                if (!vulkan) {
                    const auto& io = ImGui::GetIO();
                    ui::blur::RenderOpenGL(ui::blur::GetOptions(),
                        static_cast<int>(io.DisplaySize.x * io.DisplayFramebufferScale.x),
                        static_cast<int>(io.DisplaySize.y * io.DisplayFramebufferScale.y));
                }
                SyncFontAtlas();
                nova::module::ModuleManager::Get().RenderOverlays();
                nova::clickgui::Render();
                if (nova::ui::WindowPlacementsNeedFlush())
                    nova::config::ConfigManager::Get().SaveCurrent(false);
                ImGui::Render();
                ImDrawData* data = ImGui::GetDrawData();
                LogFontAtlas(data);
                if (logFrame) {
                    const ImGuiIO& io = ImGui::GetIO();
                    LogTo("Base: drew cmd=%d vtx=%d idx=%d display=(%.0f,%.0f) framebuffer=(%.0f,%.0f)",
                          data ? data->CmdListsCount : -1, data ? data->TotalVtxCount : -1,
                          data ? data->TotalIdxCount : -1, io.DisplaySize.x, io.DisplaySize.y,
                          io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
                }
                if (vulkan) vkoverlay::Render(data);
                else {
                    ImGui_ImplOpenGL3_RenderDrawData(data);
                    firstFrame.store(true, std::memory_order_release);
                    ReportBootstrapReady();
                }
            } else {
                static bool noted = false;
                if (!noted) {
                    noted = true;
                    LogTo("Base: rendering skipped (platform=%d renderer=%d iconic=%d hwnd=%p)",
                          imguiPlatform, imguiRenderer, IsIconic(hwnd), hwnd);
                }
            }
        }
    }
}

static void __cdecl HookSwap(void* handle) {
    const SwapFn nextSwap = originalSwap;
    try {
        RunFrame(handle);
    } catch (const std::exception& error) {
        LogTo("Base: the render hook raised '%s'", error.what());
        TripGuard("the render hook");
    } catch (...) {
        TripGuard("the render hook");
    }
    if (nextSwap) nextSwap(handle);
}

static void JNICALL HookLwjglSwap(JNIEnv* env, jclass type, jobject peer) {
    const LwjglSwapFn next = originalLwjglSwap;
    const HGLRC context = wglGetCurrentContext();
    const HWND window = WindowFromDC(wglGetCurrentDC());
    if (context && window) {
        try {
            RunFrame(window);
        } catch (const std::exception& error) {
            LogTo("Base: LWJGL2 frame raised '%s'", error.what());
            TripGuard("LWJGL2 render hook");
        } catch (...) {
            TripGuard("LWJGL2 render hook");
        }
    }
    if (next) next(env, type, peer);
}

static bool __cdecl HookSdlSwap(void* handle) {
    const SdlSwapFn next = originalSdlSwap;
    if (handle != compat::mc263::WindowHandle()) return next ? next(handle) : false;
    const HGLRC context = wglGetCurrentContext();
    const HWND window = WindowFromDC(wglGetCurrentDC());
    if (context && window) {
        try { RunFrame(window); }
        catch (const std::exception& error) {
            LogTo("Base: SDL frame raised '%s'", error.what());
            TripGuard("SDL render hook");
        }
        catch (...) { TripGuard("SDL render hook"); }
    }
    return next ? next(handle) : false;
}

bool Install() {
    if (compat::mc263::IsVulkan()) {
        vkoverlay::Handles handles{};
        if (!compat::mc263::VulkanHandles(&handles)) return false;
        HMODULE sdl = GetModuleHandleW(L"SDL3.dll");
        if (!sdl) return false;
        using PropertiesFn = unsigned (__cdecl*)(void*);
        using PointerFn = void* (__cdecl*)(unsigned, const char*, void*);
        auto properties = reinterpret_cast<PropertiesFn>(GetProcAddress(sdl, "SDL_GetWindowProperties"));
        auto pointer = reinterpret_cast<PointerFn>(GetProcAddress(sdl, "SDL_GetPointerProperty"));
        if (!properties || !pointer) return false;
        auto nativeWindow = pointer(properties(compat::mc263::WindowHandle()), "SDL.window.win32.hwnd", nullptr);
        installed = vkoverlay::Install(handles, nativeWindow,{VulkanFrame,VulkanRendererLost,VulkanFrameSubmitted});
        return installed;
    }
    if (compat::Current().renderer == compat::RenderBackend::Sdl) {
        HMODULE sdl = GetModuleHandleW(L"SDL3.dll");
        if (!sdl) return false;
        auto swap = reinterpret_cast<void*>(GetProcAddress(sdl, "SDL_GL_SwapWindow"));
        if (!swap) return false;
        const MH_STATUS status = MH_Initialize();
        if (status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED) return false;
        if (MH_CreateHook(swap, &HookSdlSwap, reinterpret_cast<void**>(&originalSdlSwap)) != MH_OK) return false;
        swapTarget = swap;
        if (MH_EnableHook(swap) != MH_OK) {
            MH_RemoveHook(swap); swapTarget = nullptr; return false;
        }
        installed = true;
        LogTo("Base: Minecraft 26.3 SDL3/OpenGL swap hook installed; waiting for first frame");
        return true;
    }
    if (compat::Current().renderer == compat::RenderBackend::Lwjgl2) {
        HMODULE lwjgl = GetModuleHandleA("lwjgl64.dll");
        if (!lwjgl) return false;
        auto swap = reinterpret_cast<void*>(GetProcAddress(lwjgl,
            "Java_org_lwjgl_opengl_WindowsContextImplementation_nSwapBuffers"));
        if (!swap) return false;
        const MH_STATUS status = MH_Initialize();
        if (status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED) return false;
        if (MH_CreateHook(swap, &HookLwjglSwap, reinterpret_cast<void**>(&originalLwjglSwap)) != MH_OK) return false;
        swapTarget = swap;
        if (MH_EnableHook(swap) != MH_OK) { MH_RemoveHook(swap); swapTarget = nullptr; return false; }
        installed = true;
        LogTo("Base: LWJGL2 swap hook installed");
        return true;
    }
    HMODULE glfw = GetModuleHandleA("glfw.dll");
    if (!glfw) { LogTo("Base: GLFW module not loaded; OpenGL hook deferred"); return false; }
    auto swap = reinterpret_cast<void*>(GetProcAddress(glfw, "glfwSwapBuffers"));
    getWin32Window = reinterpret_cast<GetWindowFn>(GetProcAddress(glfw, "glfwGetWin32Window"));
    setInputMode = reinterpret_cast<SetInputModeFn>(GetProcAddress(glfw, "glfwSetInputMode"));
    getInputMode = reinterpret_cast<GetInputModeFn>(GetProcAddress(glfw, "glfwGetInputMode"));
    if (!swap || !getWin32Window || !setInputMode || !getInputMode) {
        LogTo("Base: GLFW exports unavailable"); return false;
    }
    const MH_STATUS initStatus = MH_Initialize();
    if (initStatus != MH_OK && initStatus != MH_ERROR_ALREADY_INITIALIZED) return false;
    if (MH_CreateHook(swap, &HookSwap, reinterpret_cast<void**>(&originalSwap)) != MH_OK) return false;
    if (MH_EnableHook(swap) != MH_OK) return false;
    swapTarget = swap;
    installed = true;
    LogTo("Base: GLFW swap hook installed");
    return true;
}

bool WaitForTeardown(unsigned timeoutMs) {
    if (!installed)
        return true;
    const ULONGLONG deadline = GetTickCount64() + timeoutMs;
    for (;;) {
        if (teardownDone.load(std::memory_order_acquire))
            return true;
        if (GetTickCount64() >= deadline)
            return false;
        Sleep(10);
    }
}

bool WaitForFirstFrame(unsigned timeoutMs) {
    const ULONGLONG deadline = GetTickCount64() + timeoutMs;
    do {
        if (IsCircuitOpen()) return false;
        if (firstFrame.load(std::memory_order_acquire)) return true;
        Sleep(10);
    } while (GetTickCount64() < deadline);
    return false;
}

void FinalizeHooks() {
    if (compat::mc263::IsVulkan()) vkoverlay::Finalize();
    MH_Uninitialize();
    originalSwap = nullptr;
    originalLwjglSwap = nullptr;
    originalSdlSwap = nullptr;
    swapTarget = nullptr;
    installed = false;
}
void VulkanFrame(void* window) { RunFrame(window); }
void VulkanRendererLost() { imguiRenderer = false; }
void VulkanFrameSubmitted() {
    if (IsCircuitOpen()) return;
    firstFrame.store(true, std::memory_order_release);
    ReportBootstrapReady();
}
}
