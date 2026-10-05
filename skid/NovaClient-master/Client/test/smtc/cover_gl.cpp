// Hidden, independent WGL context; never attaches to the game.
#include <windows.h>
#include <GL/gl.h>
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "ui/hud/musichud/CoverTexture.h"
#include "imgui_internal.h"
#include <cstdio>
#include <stdexcept>

void Check(bool pass,const char* message){if(!pass)throw std::runtime_error(message);}
int main(){
    try{
        WNDCLASSW wc{};wc.lpfnWndProc=DefWindowProcW;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"NovaSkinTest";wc.style=CS_OWNDC;
        RegisterClassW(&wc);
        auto window=CreateWindowW(wc.lpszClassName,L"Nova skin test",WS_POPUP,0,0,128,128,nullptr,nullptr,wc.hInstance,nullptr);
        Check(window!=nullptr,"hidden window");auto dc=GetDC(window);
        PIXELFORMATDESCRIPTOR pfd{};pfd.nSize=sizeof(pfd);pfd.nVersion=1;pfd.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;pfd.iPixelType=PFD_TYPE_RGBA;pfd.cColorBits=32;
        Check(SetPixelFormat(dc,ChoosePixelFormat(dc,&pfd),&pfd)!=FALSE,"pixel format");
        auto context=wglCreateContext(dc);Check(context&&wglMakeCurrent(dc,context),"GL context");
        std::printf("OpenGL: %s / %s\n",glGetString(GL_RENDERER),glGetString(GL_VERSION));
        ImGui::CreateContext();ImGui::GetIO().IniFilename=nullptr;
        Check(ImGui_ImplOpenGL3_Init("#version 130"),"GL backend");
        auto art=std::make_shared<nova::smtc::Artwork>();
        art->width=32; art->height=16; art->version=1; art->rgba.assign(512,IM_COL32(0,255,0,255));
        for(int cycle=0;cycle<2;++cycle){
            for(int frame=0;frame<90;++frame){
                ImGui::GetIO().DisplaySize=ImVec2(128,128);ImGui::GetIO().DeltaTime=1.f/60;
                ImGui_ImplOpenGL3_NewFrame();ImGui::NewFrame();
                if (frame > 0) art->version = cycle == 0 ? art->version + 1 : 1 + frame % 2;
                nova::ui::music::RetainCovers(art->version,0);
                Check(nova::ui::music::DrawCover(ImGui::GetBackgroundDrawList(),art,ImVec2(10,10),ImVec2(40,40),4),"bounded cache continues accepting covers");
                Check(ImGui::GetCurrentContext()->UserTextures.Size <= 8,"bounded GPU retirement queue");
                ImGui::Render();glViewport(0,0,128,128);glClearColor(0,0,1,1);glClear(GL_COLOR_BUFFER_BIT);
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());glFinish();
                unsigned char pixel[4]{};glReadPixels(20,108,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
                Check(pixel[0]<5&&pixel[1]>245&&pixel[2]<5,"skin GPU pixels");
                glReadPixels(90,38,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);Check(pixel[0]<5&&pixel[1]<5&&pixel[2]>245,"preserved background");
                Check(glGetError()==GL_NO_ERROR,"GL errors");
            }
            ImGui_ImplOpenGL3_Shutdown();
            if(cycle==0)Check(ImGui_ImplOpenGL3_Init("#version 130"),"backend recreation");
        }
        nova::ui::music::ReleaseCovers();
        Check(ImGui::GetCurrentContext()->UserTextures.Size==0,"cover registrations drained");ImGui::DestroyContext();
        wglMakeCurrent(nullptr,nullptr);wglDeleteContext(context);ReleaseDC(window,dc);DestroyWindow(window);
        std::puts("PASS: OpenGL cover upload, pixel readback, 180 cover replacements, bounded retirement and backend recreation");return 0;
    }catch(const std::exception& e){std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;}
}
