#include "ui/backend/GlExt.h"
#include "ui/backend/blur/Blur.h"
#include "ui/backend/blur/Gaussian.h"
#include "ui/overlay/HudBackground.h"
#include "ui/clickgui/panel/ClientSettings.h"
#include "ui/clickgui/panel/ReferenceStyle.h"
#include "ui/clickgui/browser/BrowserBackdrop.h"
#include "imgui_impl_opengl3.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <stdexcept>

namespace {
void Check(bool ok,const char* what){if(!ok)throw std::runtime_error(what);}
void Setting(const char* name,double value) {
    for(const auto& row:nova::menu::panel::ClientSettings::Get().rows())if(std::string_view(row.getName())==name) {
        if(auto* p=dynamic_cast<Properties::NumberProp*>(row.getProp()))p->set(value);
        else if(auto* p=dynamic_cast<Properties::BoolProperty*>(row.getProp()))p->set(value!=0);
        return;
    }
    throw std::runtime_error("Missing production setting");
}
std::array<int,3> Pixel(int x,int y) {
    unsigned char p[4]{};glReadPixels(x,127-y,1,1,GL_RGBA,GL_UNSIGNED_BYTE,p);return {p[0],p[1],p[2]};
}
void Draw(bool capture,float visibility=1,bool red=false) {
    glDisable(GL_SCISSOR_TEST);glViewport(0,0,128,128);
    glClearColor(red?1.f:0.f,0,0,0);glClear(GL_COLOR_BUFFER_BIT);
    if(!red){glEnable(GL_SCISSOR_TEST);glScissor(64,0,64,128);glClearColor(1,1,1,0);glClear(GL_COLOR_BUFFER_BIT);glDisable(GL_SCISSOR_TEST);}
    if(capture)nova::ui::blur::RenderOpenGL(nova::ui::blur::GetOptions(),128,128);
    ImGui::GetIO().DisplaySize={128,128};ImGui::GetIO().DeltaTime=1.f/60;
    ImGui_ImplOpenGL3_NewFrame();ImGui::NewFrame();
    nova::ui::hud::Background(ImGui::GetBackgroundDrawList(),{16,16},{112,112},IM_COL32(20,20,20,204),16,0,visibility);
    ImGui::Render();ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());glFinish();
    Check(glGetError()==GL_NO_ERROR,"GL error in actual HUD background");
}
void DrawLayers(bool nested=false,bool reversed=false,bool panel=false) {
    glDisable(GL_SCISSOR_TEST);glViewport(0,0,128,128);
    glClearColor(0,1,0,1);glClear(GL_COLOR_BUFFER_BIT);
    nova::ui::blur::RenderOpenGL(nova::ui::blur::GetOptions(),128,128);
    ImGui::GetIO().DisplaySize={128,128};ImGui::GetIO().DeltaTime=1.f/60;
    ImGui_ImplOpenGL3_NewFrame();ImGui::NewFrame();
    auto* lower=ImGui::GetBackgroundDrawList();
    lower->AddRectFilled({0,0},{64,128},reversed?IM_COL32(0,0,255,255):IM_COL32(255,0,0,255));
    lower->AddRectFilled({64,0},{128,128},reversed?IM_COL32(255,0,0,255):IM_COL32(0,0,255,255));
    ImGui::PushStyleColor(ImGuiCol_PopupBg,0u);
    ImGui::SetNextWindowPos({16,16});ImGui::SetNextWindowSize({96,96});
    ImGui::OpenPopup("LayerRegression");
    const auto flags=ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoTitleBar;
    const bool open=panel?ImGui::BeginPopup("LayerRegression",flags|ImGuiWindowFlags_NoBackground):
        nova::ui::blur::BeginPopup("LayerRegression",flags);
    if(open) {
        if(panel)nova::menu::panel::reference::PopupBackdrop(ImGui::GetWindowDrawList(),ImGui::GetWindowPos(),ImGui::GetWindowSize());
        if(nested) {
            ImGui::GetWindowDrawList()->AddRectFilled({32,48},{64,104},IM_COL32(0,255,0,255));
            ImGui::SetNextWindowPos({48,64});ImGui::SetNextWindowSize({48,32});
            ImGui::OpenPopup("NestedLayerRegression");
            if(nova::ui::blur::BeginPopup("NestedLayerRegression",ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoTitleBar))ImGui::EndPopup();
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleColor();
    // The world texture must remain green even after all live UI captures.
    nova::ui::blur::Background(ImGui::GetForegroundDrawList(),{0,0},{8,8},0u);
    ImGui::Render();ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());glFinish();
    Check(glGetError()==GL_NO_ERROR,"GL error in layered popup background");
}
}
int TestGaussianHudBlur() {
    try {
        Setting("Blur",1);Setting("Blur Strength",3);Setting("Blur FPS",5);
        std::array<int,3> edgeValues{};int index=0;
        for(int alpha:{0,50,100}) {
            nova::ui::blur::ReleaseOpenGL();Setting("HUD Background Alpha",alpha);Draw(true);
            const auto edge=Pixel(63,80);edgeValues[index++]=edge[0];
            Check(edge[0]>20 && edge[0]<160,"Blur must remain at 0/50/100 percent HUD alpha");
            // Reuse the captured black/white backdrop over a new red scene. If
            // master alpha fades blur, the red component leaks through here.
            Draw(false,1,true);const auto cached=Pixel(63,80);
            Check(cached==edge,"HUD background alpha leaks the unblurred scene");
            Draw(false,0,true);Check(Pixel(63,80)==std::array<int,3>{255,0,0},"Hidden HUD retains blur");
            Draw(false,.5f,true);const auto faded=Pixel(63,80);
            Check(std::abs(faded[0]-(255+edge[0])/2)<=2 && std::abs(faded[1]-edge[1]/2)<=2,"Animation must fade the whole glass");
        }
        Check(edgeValues[0]>edgeValues[1] && edgeValues[1]>edgeValues[2],"Master alpha must change tint");
        Setting("Blur",0);Setting("HUD Background Alpha",0);Draw(true,1,true);
        Check(Pixel(63,80)==std::array<int,3>{255,0,0},"Disabled blur with zero alpha must be transparent");
        Setting("Blur",1);nova::ui::blur::ReleaseOpenGL();
        DrawLayers();DrawLayers();const auto first=Pixel(60,80);
        Check(first[0]>100 && first[2]>30 && first[1]<5,"Popup must blur lower UI, not replace it with the world");
        Check(Pixel(4,4)==std::array<int,3>{0,255,0},"Layer capture polluted the world cache");
        DrawLayers(false,true);const auto changed=Pixel(60,80);
        Check(changed[2]>100 && changed[0]>30 && changed[1]<5,"Layer capture must refresh independently of Blur FPS");
        DrawLayers(true);DrawLayers(true);const auto nested=Pixel(60,80);
        Check(nested[1]>100 && nested[2]>20,"Nested popup must include its parent UI in the blur");
        Check(Pixel(4,4)==std::array<int,3>{0,255,0},"Nested popup polluted the world cache");
        DrawLayers(false,false,true);const auto panel=Pixel(60,80);
        Check(panel[0]>panel[1]+8 && panel[2]>panel[1]+5,"Panel styled popup bypassed lower UI capture");
        Setting("Blur",0);DrawLayers();
        Check(Pixel(60,80)==std::array<int,3>{255,0,0},"Disabled layered blur must preserve lower UI");
        // Verify the browser CPU convolution against an independent impulse
        // response, including normalized energy and strength changes.
        nova::chrome::Frame source;source.width=source.height=65;source.sequence=1;
        source.rgba.assign(65*65*4,0);
        for(int y=0;y<65;++y)for(int x=0;x<65;++x) {
            const auto at=(y*65+x)*4;source.rgba[at+3]=255;
            if(x==32)source.rgba[at]=source.rgba[at+1]=source.rgba[at+2]=255;
        }
        nova::ui::browser::BrowserBackdrop browser;
        for(int strength:{1,3,6}) {
            const auto& result=browser.Update(source,{true,strength,30});
            const nova::ui::blur::GaussianKernel kernel(strength);
            for(int x=0;x<65;++x) {
                const int offset=std::abs(x-32);
                const int expected=offset<=kernel.radius?int(std::lround(255*kernel.weights[offset])):0;
                Check(std::abs(int(result.rgba[(32*65+x)*4])-expected)<=1,"Browser impulse does not match Gaussian");
            }
        }
        puts("PASS: HUD alpha, fade, popup/nested UI blur, independent layer refresh, world cache isolation, blur-off, browser Gaussian");return 0;
    }catch(const std::exception& error){fprintf(stderr,"FAIL: %s\n",error.what());return 1;}
}
