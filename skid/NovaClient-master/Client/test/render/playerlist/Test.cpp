#define NOMINMAX
#include <windows.h>
#include <GL/gl.h>
#include <cassert>
#include <cstdio>
#include <fstream>
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "ui/clickgui/Fonts.h"
#include "ui/hud/playerlist/PlayerListHud.h"
#include "ui/overlay/HudBackground.h"

namespace mock { bool right = false; GLuint skin = 0; }
namespace nova::ui {
void PlaceWindow(const char*,float,float) { ImGui::SetNextWindowPos(mock::right?ImVec2(560,330):ImVec2(40,35),ImGuiCond_Always); }
void CaptureWindow(const char*) {}
namespace skins { ImTextureRef Resolve(std::uint64_t) { return ImTextureRef(static_cast<ImTextureID>(mock::skin)); } }
// Isolate the production card/layout renderer; this host has no scene blur source.
namespace hud {
void Background(ImDrawList* list,ImVec2 min,ImVec2 max,ImU32 color,float rounding,ImDrawFlags flags,float) {
    list->AddRectFilled(min,max,color,rounding,flags);
}
}
}
void ModelTests() {
    using namespace nova::ui::playerlist;
    assert(InRange(300,false,48) && InRange(48,true,48) && !InRange(48.1,true,48));
    assert(!InRange(NAN,false,48) && !InRange(-1,false,48));
    std::vector<Snapshot> entries{{1,"A",10.8f},{2,"B",10.1f},{3,"C",5}};
    Selection selection;
    selection.Update(entries,3,0);
    assert(entries[0].id==3 && entries[1].id==2 && entries[2].id==1);
    for (int i=0;i<100;++i) {
        entries={{1,"A",float(5+i%5)},{2,"B",float(4+i%3)},{3,"C",10}};
        selection.Update(entries,3,i*.05);
        assert(entries[0].id==3 && entries[1].id==2 && entries[2].id==1);
    }
    auto crowd = [] { return std::vector<Snapshot>{{1,"A",20},{2,"B",10},{3,"C",12},{4,"D",5}}; };
    entries=crowd();selection.Update(entries,3,6);assert(entries[2].id==1);
    entries=crowd();selection.Update(entries,3,6.7);assert(entries[2].id==1);
    entries=crowd();selection.Update(entries,3,6.9);assert(entries[2].id==4);
    // A marginally closer excluded player must not churn the full list.
    for(int i=0;i<100;++i) {
        entries=crowd();entries[0].distance=11.5f;
        selection.Update(entries,3,7+i*.05);
        assert(entries[0].id==3 && entries[1].id==2 && entries[2].id==4);
    }
    entries={{2,"B",10},{4,"D",5},{5,"E",8}};selection.Update(entries,3,13);
    assert(entries[0].id==2 && entries[1].id==4 && entries[2].id==5);
    selection.Reset();selection.Update(entries,2,14);assert(entries.size()==2 && entries[0].id==4 && entries[1].id==5);
    Card numbers;numbers.snapshot.health=20;numbers.snapshot.distance=9.4f;
    numbers.UpdateNumbers(.016f,true);
    assert(numbers.healthText.current=="20.0" && numbers.distanceText.current=="9" && numbers.distanceText.progress==1);
    numbers.snapshot.health=9.5f;numbers.snapshot.distance=10.2f;numbers.UpdateNumbers(.016f,true);
    assert(numbers.healthText.previous=="20.0" && numbers.healthText.current=="9.5" && numbers.healthText.progress==0);
    assert(numbers.distanceText.previous=="9" && numbers.distanceText.current=="10");
    numbers.snapshot.health=15;numbers.snapshot.distance=8.1f;numbers.UpdateNumbers(.016f,false);
    assert(numbers.healthText.current=="15.0" && numbers.distanceText.current=="8" && numbers.healthText.progress==1);
    numbers.snapshot.health=-1;numbers.UpdateNumbers(.016f,true);assert(numbers.healthText.current=="--" && numbers.healthText.progress==1);
    assert(std::abs(FollowAngle(179,-179,.5f)-180)<.001f);
    entries={{1,"A",10.8f},{2,"B",10.1f},{3,"C",5}};
    Model model;
    model.Update(entries,true,1,0,.1f,0); assert(model.cards.size()==3);
    model.Update(entries,true,1,.1,.1f,0); assert(model.cards[0].progress==1);
    model.Update({},true,1,.2,.1f,0); assert(model.cards[0].progress==1);
    model.Update({},true,1,.35,.1f,0); assert(model.cards[0].progress<1);
    const float leaving=model.cards[0].progress;
    model.Update(entries,true,1,.4,.05f,0);
    assert(model.cards.size()==3 && model.cards[0].progress>leaving);
    for(int i=0;i<5;++i)model.Update({},false,1,1+i*.1,.1f,0);
    assert(model.cards.empty());
    model.Update(entries,true,1,2,.1f,0);
    std::vector<Snapshot> next{{1,"New world",30}};
    model.Update(next,true,2,2.1,.01f,0);
    assert(model.cards.size()==1 && model.cards[0].snapshot.name=="New world" && model.cards[0].progress<.1f);
    std::puts("PASS: stable crowd slots, sustained replacement, boundary hysteresis, range, numeric rolling/snap, angle wrap, grace/reappearance and world reset");
}
void Save(const char* name) {
    std::vector<unsigned char> pixels(900*600*3);
    glReadPixels(0,0,900,600,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
    std::ofstream out(name,std::ios::binary); out<<"P6\n900 600\n255\n";
    for(int y=599;y>=0;--y)out.write(reinterpret_cast<char*>(pixels.data()+y*900*3),900*3);
}
int main() {
    ModelTests();
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    WNDCLASSW wc{}; wc.lpfnWndProc=DefWindowProcW; wc.hInstance=GetModuleHandleW(nullptr);
    wc.lpszClassName=L"NovaPlayerListPreview";wc.style=CS_OWNDC;
    RegisterClassW(&wc);
    HWND window=CreateWindowW(wc.lpszClassName,L"Hidden preview",WS_POPUP,0,0,900,600,nullptr,nullptr,wc.hInstance,nullptr);assert(window);
    HDC dc=GetDC(window);PIXELFORMATDESCRIPTOR pfd{sizeof(pfd),1};
    pfd.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;pfd.iPixelType=PFD_TYPE_RGBA;pfd.cColorBits=32;
    assert(SetPixelFormat(dc,ChoosePixelFormat(dc,&pfd),&pfd));
    HGLRC context=wglCreateContext(dc);assert(wglMakeCurrent(dc,context));
    ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.DisplaySize={900,600};io.DeltaTime=1.f/60;
    InitializeFatalMenuFonts();assert(ImGui_ImplOpenGL3_Init());
    unsigned char face[8*8*4];
    for(int y=0;y<8;++y)for(int x=0;x<8;++x){const int i=(y*8+x)*4;
        face[i]=y<2?75:194;face[i+1]=y<2?51:141;face[i+2]=y<2?35:104;face[i+3]=255;
        if(y==4&&(x==2||x==5)){face[i]=80;face[i+1]=150;face[i+2]=220;}}
    glGenTextures(1,&mock::skin);glBindTexture(GL_TEXTURE_2D,mock::skin);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,8,8,0,GL_RGBA,GL_UNSIGNED_BYTE,face);
    {
        using namespace nova::ui::playerlist;
        PlayerListHud hud;
        std::vector<Snapshot> entries{{1,"Sakura",3.2f,45},{2,"LongPlayerName123456",12.4f,-70,true},{3,"附近的玩家",48.f,179}};
        for(auto& entry:entries){entry.portrait.texture=1;entry.portrait.nearest=true;entry.health=20;}
        entries[1].health=9.5f;entries[2].health=-1;
        for(int scenario=0;scenario<4;++scenario) {
            mock::right=scenario==2;hud.Options().size.set(scenario==1?150:100);
            hud.Options().numberRolling.set(scenario!=2);
            for(int frame=0;frame<70;++frame) {
                if(scenario==3 && frame==65) { entries[0].health=9.5f;entries[0].distance=10;entries[1].health=20;entries[1].distance=101; }
                ImGui_ImplOpenGL3_NewFrame();ImGui::NewFrame();
                hud.Draw(true,scenario==2,entries,1,0);
                ImGui::Render();if(frame>2)assert(ImGui::GetDrawData()->TotalVtxCount>0);
                for(int n=0;n<ImGui::GetDrawData()->CmdListsCount;++n)for(auto& v:ImGui::GetDrawData()->CmdLists[n]->VtxBuffer)
                    assert(std::isfinite(v.pos.x)&&std::isfinite(v.pos.y));
                glViewport(0,0,900,600);glClearColor(.13f,.17f,.20f,1);glClear(GL_COLOR_BUFFER_BIT);
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());glFinish();
            }
            Save(scenario==0?"normal.ppm":scenario==1?"scaled.ppm":scenario==2?"editing.ppm":"rolling.ppm");
        }
        for(int frame=0;frame<40;++frame){ImGui_ImplOpenGL3_NewFrame();ImGui::NewFrame();hud.Draw(false,false,{},1,0);ImGui::Render();}
        assert(ImGui::GetDrawData()->TotalVtxCount==0);
    }
    ImGui_ImplOpenGL3_Shutdown();ImGui::DestroyContext();
    glDeleteTextures(1,&mock::skin);wglMakeCurrent(nullptr,nullptr);wglDeleteContext(context);ReleaseDC(window,dc);DestroyWindow(window);
    std::puts("PASS: actual ImGui Player List at 100/150 percent, multilingual/long names, right/up edit title and complete close animation");
}
