#define NOMINMAX
#include <Windows.h>
#include <GL/gl.h>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <vector>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_opengl3.h"
#include "ui/clickgui/Fonts.h"
#include "ui/hud/cswin/CsWin.h"

// Hidden native preview of the production renderer. No JVM or game injection.
int main() {
    WNDCLASSW wc{};wc.lpfnWndProc=DefWindowProcW;wc.hInstance=GetModuleHandleW(nullptr);
    wc.lpszClassName=L"NovaCSWinPreview";wc.style=CS_OWNDC;RegisterClassW(&wc);
    RECT bounds{0,0,1440,900};AdjustWindowRect(&bounds,WS_OVERLAPPEDWINDOW,FALSE);
    HWND window=CreateWindowW(wc.lpszClassName,L"CSWin preview",WS_OVERLAPPEDWINDOW,0,0,
        bounds.right-bounds.left,bounds.bottom-bounds.top,nullptr,nullptr,wc.hInstance,nullptr);
    if(!window)return 1;
    auto dc=GetDC(window);PIXELFORMATDESCRIPTOR pfd{sizeof(pfd),1};
    pfd.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;pfd.iPixelType=PFD_TYPE_RGBA;pfd.cColorBits=32;
    if(!SetPixelFormat(dc,ChoosePixelFormat(dc,&pfd),&pfd))return 2;
    auto gl=wglCreateContext(dc);if(!gl || !wglMakeCurrent(dc,gl))return 3;
    ImGui::CreateContext();ImGui::GetIO().IniFilename=nullptr;InitializeFatalMenuFonts();
    if(!ImGui_ImplWin32_Init(window)||!ImGui_ImplOpenGL3_Init())return 4;
    nova::ui::cswin::Snapshot player;player.name="NovaPlayer";
    std::vector<std::uint32_t> pixels(64,IM_COL32(187,137,105,255));
    for(int x=0;x<8;++x){pixels[x]=IM_COL32(60,42,29,255);pixels[8+x]=IM_COL32(60,42,29,255);}
    pixels[26]=pixels[29]=IM_COL32(108,151,215,255);
    player.portrait.texture=nova::ui::skins::Store("cswin-preview",8,8,std::move(pixels));player.portrait.nearest=true;
    struct Moment {double time,duration;};
    const Moment times[]{{0,12.016},{.2,12.016},{.6,12.016},{1.6,12.016},{5,12.016},{11.55,12.016},{11.9,12.016},{12.016,12.016},
        {0,6.3},{.2,6.3},{1,6.3},{3,6.3},{5.9,6.3},{6.2,6.3},{6.3,6.3}};
    int frame=0;
    for(auto [time,duration]:times) {
        player.style=duration<10?nova::ui::cswin::Style::CT:nova::ui::cswin::Style::T;
        ImGui_ImplOpenGL3_NewFrame();ImGui_ImplWin32_NewFrame();ImGui::NewFrame();
        nova::ui::cswin::Draw(time,player,duration);ImGui::Render();auto* draws=ImGui::GetDrawData();
        if((time==0 || time>=duration) && draws->TotalVtxCount!=0)return 5;
        if(time>0 && time<duration && draws->TotalVtxCount==0)return 6;
        for(auto* list:draws->CmdLists)for(const auto& v:list->VtxBuffer)
            if(!std::isfinite(v.pos.x)||!std::isfinite(v.pos.y))return 7;
        const int w=int(ImGui::GetIO().DisplaySize.x),h=int(ImGui::GetIO().DisplaySize.y);
        glViewport(0,0,w,h);glClearColor(.10f,.13f,.17f,1);glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(draws);
        const int stride=(w*3+3)&~3;std::vector<unsigned char> rgb(size_t(stride)*h);
        glPixelStorei(GL_PACK_ALIGNMENT,4);glReadPixels(0,0,w,h,0x80E0,GL_UNSIGNED_BYTE,rgb.data());
        BITMAPFILEHEADER file{};file.bfType=0x4d42;file.bfOffBits=sizeof(file)+sizeof(BITMAPINFOHEADER);file.bfSize=file.bfOffBits+DWORD(rgb.size());
        BITMAPINFOHEADER info{};info.biSize=sizeof(info);info.biWidth=w;info.biHeight=h;info.biPlanes=1;info.biBitCount=24;
        std::ofstream shot("cswin-"+std::to_string(frame++)+".bmp",std::ios::binary);
        shot.write(reinterpret_cast<char*>(&file),sizeof(file));shot.write(reinterpret_cast<char*>(&info),sizeof(info));shot.write(reinterpret_cast<char*>(rgb.data()),rgb.size());
        if(glGetError()!=GL_NO_ERROR)return 8;
        SwapBuffers(dc);
    }
    ImGui_ImplOpenGL3_Shutdown();ImGui_ImplWin32_Shutdown();nova::ui::skins::Release();
    ImGui::DestroyContext();ResetFatalMenuFonts();wglMakeCurrent(nullptr,nullptr);wglDeleteContext(gl);
    ReleaseDC(window,dc);DestroyWindow(window);
    puts("PASS: CSWin entry, presentation, exit and completion; finite ImGui geometry and OpenGL rendering");
}
