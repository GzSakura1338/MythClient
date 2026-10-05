#include "ui/backend/Win32Surface.h"
#include <thread>
#include <cstdio>
#include <cassert>
#include <atomic>

static std::atomic<unsigned> overlayVisits{0};
static LRESULT CALLBACK Overlay(HWND h, UINT m, WPARAM w, LPARAM l, UINT_PTR id, DWORD_PTR data) {
    overlayVisits.fetch_add(1);
    if(m==WM_NCDESTROY){reinterpret_cast<nova::glhook::Win32Surface*>(data)->Destroyed(h);RemoveWindowSubclass(h,Overlay,id);}
    return DefSubclassProc(h,m,w,l);
}
static LRESULT CALLBACK Foreign(HWND h, UINT m, WPARAM w, LPARAM l, UINT_PTR, DWORD_PTR) {
    return DefSubclassProc(h,m,w,l);
}
int main() {
    WNDCLASSW wc{};wc.lpfnWndProc=DefWindowProcW;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"NovaThreadedSurfaceTest";
    assert(RegisterClassW(&wc));
    HWND window=CreateWindowW(wc.lpszClassName,L"Hidden test",WS_OVERLAPPEDWINDOW,0,0,400,300,nullptr,nullptr,wc.hInstance,nullptr);
    assert(window);
    assert(SetWindowSubclass(window,Foreign,99,0));
    auto* context=ImGui::CreateContext();
    bool bound=false;
    std::atomic<bool> done=false;
    std::thread render([&]{
        nova::glhook::Win32Surface surface;
        assert(!surface.Bind(window,Overlay)); // Legacy path still refuses cross-thread subclassing.
        const bool initialBind=surface.Bind(window,Overlay,true);
        // Shader creation can take long enough for subclass installation to
        // finish before OverlayHost checks the original Bind result.
        for(int i=0;i<300 && overlayVisits.load()==0;++i){SendNotifyMessageW(window,WM_NULL,0,0);Sleep(1);}
        if(overlayVisits.load()==0)ExitProcess(5);
        if(!initialBind && !surface.Pending()){
            std::puts("FAIL: completed subclass is misreported as a platform initialization failure");
            std::fflush(stdout);ExitProcess(6);
        }
        for(int cycle=0;cycle<8;++cycle){
        bound=false;
        for(int i=0;i<300&&!bound;++i){bound=surface.Bind(window,Overlay,true);Sleep(1);}
        if(!bound){std::printf("Attach failed: pending=%d error=%lu\n",surface.Pending(),surface.Error());std::fflush(stdout);ExitProcess(2);}
        bool removed=false;
        for(int i=0;i<300&&!removed;++i){removed=surface.Reset();Sleep(1);}
        if(!removed){std::printf("Detach failed: pending=%d error=%lu\n",surface.Pending(),surface.Error());std::fflush(stdout);ExitProcess(3);}
        }
        // Cancel while installation is queued, then ensure the callback is gone.
        surface.Bind(window,Overlay,true);
        bool removed=false;
        for(int i=0;i<300&&!removed;++i){removed=surface.Reset();Sleep(1);}
        if(!removed)ExitProcess(4);
        done=true;
    });
    while(!done){MSG message;while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);}Sleep(1);}
    render.join();
    DWORD_PTR data=0;
    assert(GetWindowSubclass(window,Foreign,99,&data));
    std::printf("Cross-thread completion-before-readiness-check, repeated install/remove/cancel PASS; foreign subclass and legacy behavior retained\n");
    ImGui::DestroyContext(context);DestroyWindow(window);
    return bound?0:1;
}
