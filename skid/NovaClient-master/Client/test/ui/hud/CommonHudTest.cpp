// Actual shared HUD views rendered without Module, Minecraft, JNI or a media player.
#define NOMINMAX
#include <windows.h>
#include <GL/gl.h>
#include <cassert>
#include <cstdio>
#include <cmath>
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "ui/clickgui/Fonts.h"
#include "ui/hud/arraylist/ArrayListHud.h"
#include "ui/hud/autoblockhud/AutoBlockHud.h"
#include "ui/hud/compass/CompassHud.h"
#include "ui/hud/bedplates/BedPlateHud.h"
#include "ui/hud/itemesp/ItemLabel.h"
#include "ui/hud/world/ProjectedBox.h"
#include "ui/hud/esp2d/Esp2DView.h"
#include "ui/hud/nametags/NameTagView.h"
#include "ui/hud/watermark/WatermarkHud.h"
#include "ui/hud/targethud/TargetHud.h"
#include "ui/hud/breaker/BreakerHud.h"
#include "ui/hud/blockin/BlockInHud.h"
#include "ui/hud/world/ScreenGeometry.h"
#include "ui/notifications/NotificationHud.h"
#include "ui/notifications/Notifications.h"
#include "ui/overlay/HudBackground.h"

struct Rect { ImVec2 min,max; };
static std::vector<Rect> backgrounds;
namespace nova::ui::hud {
ImU32 BackgroundColor(ImU32 color) { return color; }
void Background(ImDrawList* d,ImVec2 a,ImVec2 b,ImU32 color,float rounding,ImDrawFlags flags,float) {
    if(color==0xCC331914)backgrounds.push_back({a,b});
    d->AddRectFilled(a,b,color,rounding,flags);
}
}

int main() {
    _set_error_mode(_OUT_TO_STDERR);
    WNDCLASSW wc{};wc.lpfnWndProc=DefWindowProcW;wc.hInstance=GetModuleHandleW(nullptr);
    wc.lpszClassName=L"NovaCommonHudTest";wc.style=CS_OWNDC;RegisterClassW(&wc);
    const auto window=CreateWindowW(wc.lpszClassName,L"Hidden HUD test",WS_POPUP,0,0,960,540,nullptr,nullptr,wc.hInstance,nullptr);
    assert(window);const auto dc=GetDC(window);
    PIXELFORMATDESCRIPTOR pfd{sizeof(pfd),1};pfd.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
    pfd.iPixelType=PFD_TYPE_RGBA;pfd.cColorBits=32;
    assert(SetPixelFormat(dc,ChoosePixelFormat(dc,&pfd),&pfd));
    const auto gl=wglCreateContext(dc);assert(gl&&wglMakeCurrent(dc,gl));
    ImGui::CreateContext();ImGui::GetIO().IniFilename=nullptr;InitializeFatalMenuFonts();assert(ImGui_ImplOpenGL3_Init());
    auto frame=[&](auto draw) {
        auto& io=ImGui::GetIO();io.DisplaySize={960,540};io.DeltaTime=1.f/60;
        ImGui_ImplOpenGL3_NewFrame();ImGui::NewFrame();backgrounds.clear();draw();ImGui::Render();
        glViewport(0,0,960,540);glClear(GL_COLOR_BUFFER_BIT);ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        assert(glGetError()==GL_NO_ERROR);
    };
    {
        nova::ui::arraylist::ArrayListHud list;
        auto& settings=list.Options();
        const auto rows=settings.ConfigRows();
        assert(std::string_view(rows[0].getName())=="Animations");
        assert(rows[1].getProp()==&settings.size);
        assert(std::string_view(rows[2].getName())=="HorizontalPadding");
        int colorModes=0;
        for(const auto& row:rows)if(std::string_view(row.getName())=="Type")++colorModes;
        assert(colorModes==3); // Flat Type/Type#2/Type#3 serialization is retained.
        std::vector<nova::ui::arraylist::Entry> entries{
            {"first","Long feature","Mode",true,false},{"second","Short","",true,false}};
        settings.animations.set(false);
        frame([&]{list.Draw(true,false,entries);});
        assert(backgrounds.size()==2);
        assert(std::abs(backgrounds[1].min.y-backgrounds[0].max.y)<.1f);
        const auto initialWidth=backgrounds[0].max.x-backgrounds[0].min.x;
        settings.size.set(150);
        frame([&]{list.Draw(true,false,entries);});
        assert(backgrounds[0].max.x-backgrounds[0].min.x>initialWidth*1.4f);
        entries[0].hidden=true;frame([&]{list.Draw(true,false,entries);});assert(backgrounds.size()==1);
        entries[0].hidden=false;settings.animations.set(true);
        for(int i=0;i<240;++i)frame([&]{list.Draw(true,false,entries);});
        assert(backgrounds.size()==2);
        entries.erase(entries.begin()); // The view must not retain an entry pointer.
        frame([&]{list.Draw(true,false,entries);});assert(backgrounds.size()==2);
        for(int i=0;i<400;++i) {
            frame([&]{list.Draw(true,false,entries);});
            if(backgrounds.size()==2)assert(backgrounds[1].min.y<=backgrounds[0].max.y+.1f);
        }
        assert(backgrounds.size()==1);
        settings.animations.set(false);frame([&]{list.Draw(false,false,{});});assert(backgrounds.empty());
    }
    {
        nova::ui::autoblockhud::AutoBlockHud block;
        nova::ui::autoblockhud::Snapshot snapshot;
        snapshot.active=true;snapshot.percent=75;
        snapshot.portrait.texture=nova::ui::skins::Store("fixture/avatar",16,16,std::vector<std::uint32_t>(256,0xff88aaff));
        for(int i=0;i<40;++i)frame([&]{block.Draw(true,false,snapshot);});
        assert(backgrounds.size()==1);
        assert(backgrounds[0].max.x-backgrounds[0].min.x>=199);
        snapshot.active=false;
        for(int i=0;i<50;++i)frame([&]{block.Draw(true,false,snapshot);});assert(backgrounds.empty());
        for(int i=0;i<40;++i)frame([&]{block.Draw(true,true,snapshot);});assert(backgrounds.size()==1);
        for(int i=0;i<50;++i)frame([&]{block.Draw(false,false,snapshot);});assert(backgrounds.empty());
    }
    frame([&]{nova::ui::DrawCompass(0);assert(!ImGui::GetBackgroundDrawList()->VtxBuffer.empty());});
    frame([&]{
        nova::ui::DrawBedPlate({{400,300},"Wool",25,4},{});
        assert(backgrounds.size()==1);
        assert(std::abs((backgrounds[0].min.x+backgrounds[0].max.x)*.5f-400)<.1f);
        nova::ui::DrawItemLabel({500,300},14,.03,64,IM_COL32_WHITE,true);
        nova::ui::ProjectedBox box{{{100,300},{130,300},{100,200},{130,200},{120,320},{150,320},{120,220},{150,220}}};
        nova::ui::DrawFilledBox(ImGui::GetBackgroundDrawList(),box,0x4400ff00);
        nova::ui::esp2d::Draw({600,200,650,350},1,nova::ui::esp2d::BoxMode::CS,
            nova::ui::esp2d::HealthMode::Gradient,12,20,10,true);
    });
    frame([&]{
        nova::ui::nametags::DrawDefault({480,300},1,"Player","20+2",true,{});
        assert(backgrounds.size()==3);
        assert(std::abs((backgrounds.front().min.x+backgrounds.back().max.x)*.5f-480)<.1f);
        nova::ui::nametags::DrawClassic({480,220},2,{{"A",IM_COL32_WHITE,true,true,true,true},{"B",0xff00ff00}},{});
    });
    {
        nova::ui::watermark::WatermarkHud watermark;
        nova::ui::watermark::Snapshot data;
        data.player="Player";data.server="local";data.tps=20;data.ping=30;
        watermark.Options().player.set(true);watermark.Options().tps.set(true);
        for(int i=0;i<40;++i)frame([&]{watermark.Draw(true,data);});
        assert(backgrounds.size()==1);
        assert(std::abs(backgrounds[0].min.x-12)<.1f);
        data.tps=17.5;
        for(int i=0;i<40;++i)frame([&]{watermark.Draw(true,data);});
        for(int i=0;i<40;++i)frame([&]{watermark.Draw(false,data);});assert(backgrounds.empty());
    }
    {
        nova::ui::targethud::TargetHud target;
        std::vector<nova::ui::targethud::Snapshot> data{{true,1,"First",16,2,20,3,0,{}},
            {true,2,"Second",8,0,20,4,5,{}}};
        // The production clock is monotonic wall time; pace this short animation check.
        for(int i=0;i<24;++i) {Sleep(10);frame([&]{target.Draw(false,data);});}
        assert(backgrounds.size()==2);
        data.erase(data.begin());data[0].health=4;
        for(int i=0;i<70;++i) {Sleep(10);frame([&]{target.Draw(false,data);});}
        assert(backgrounds.size()==1);
        data.clear();
        for(int i=0;i<70;++i) {Sleep(10);frame([&]{target.Draw(false,data);});}
        assert(backgrounds.empty());
    }
    {
        nova::ui::NotificationHud notifications;
        nova::notifications::Post("Shared notification",nova::notifications::Type::Success);
        for(int i=0;i<24;++i)frame([&]{notifications.Draw(true);});
        assert(backgrounds.size()==1);
        assert(std::abs((backgrounds[0].min.x+backgrounds[0].max.x)*.5f-480)<.1f);
        assert(backgrounds[0].min.y>270);
        for(int i=0;i<30;++i)frame([&]{notifications.Draw(false);});assert(backgrounds.empty());
        notifications.Settings().configs.set(false);
        nova::notifications::Post("Filtered config",nova::notifications::Type::Info,nova::notifications::Source::Config);
        frame([&]{notifications.Draw(true);});assert(backgrounds.empty());
    }
    {
        nova::ui::BreakerHud breaker;
        assert(std::abs(breaker.Advance(1,.025f)-.5f)<.001f);
        assert(std::abs(breaker.Advance(1,.025f)-1)<.001f);
        assert(std::abs(breaker.Advance(0,.025f)-.5f)<.001f);
        frame([&]{breaker.DrawProgress(88);});
        assert(backgrounds.size()==1);
        assert(std::abs((backgrounds[0].min.x+backgrounds[0].max.x)*.5f-480)<.1f);
        assert(std::abs(backgrounds[0].max.y-(540-88-28))<.1f);
        nova::ui::BlockInHud block;
        frame([&]{block.Draw(5,true,1000);});
        frame([&]{block.Draw(5,true,1050);});
        assert(ImGui::GetDrawData()->TotalVtxCount>0);
        frame([&]{block.Draw(5,false,1060);});
        assert(ImGui::GetDrawData()->TotalVtxCount==0);
    }
    ImGui_ImplOpenGL3_Shutdown();nova::ui::skins::Release();ImGui::DestroyContext();ResetFatalMenuFonts();
    wglMakeCurrent(nullptr,nullptr);wglDeleteContext(gl);ReleaseDC(window,dc);DestroyWindow(window);
    puts("PASS: shared HUDs, watermark, target cards, name tags, ESP, bed plates, breaker, BlockIn and notifications; no game dependencies");
}
