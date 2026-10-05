#define NOMINMAX
#include <windows.h>
#include <GL/gl.h>
#include <array>
#include <cassert>
#include <cstdio>
#include <cmath>
#include <filesystem>
#include <fstream>
#include "core/Instance.h"
#include "ui/clickgui/panel/PanelClickGui.h"
#include "ui/clickgui/panel/PageLayout.h"
#include "contracts/AccountService.h"
#include "proxy/ProxyService.h"
#include "ui/skin/SkinTextures.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_opengl3.h"
#include "config/HostConfig.h"
#include "config/ConfigManager.h"
#include "config/PropertyCodec.h"
#include "bind/InputManager.h"
#include "ui/clickgui/ClickGui.h"
#include "chrome/BrowserService.h"
#include "ui/clickgui/browser/BrowserPage.h"
#include "ui/workspace/ForegroundWorkspace.h"
#include "ui/workspace/PanelSnapshot.h"
#include "config/BrowserSettings.h"
#include "ui/clickgui/FeatureTree.h"
#include "ui/clickgui/Fonts.h"
#include "ui/clickgui/panel/ClientSettings.h"
#include "ui/hud/arraylist/ArrayListHud.h"
#include "ui/hud/autoblockhud/AutoBlockHud.h"
#include "ui/hud/watermark/WatermarkHud.h"
#include "ui/hud/targethud/TargetHud.h"
#include "ui/hud/nametags/NameTagView.h"
#include "ui/hud/esp2d/Esp2DView.h"
#include "ui/hud/bedplates/BedPlateHud.h"
#include "ui/hud/breaker/BreakerHud.h"
#include "ui/hud/blockin/BlockInHud.h"
#include "ui/hud/itemesp/ItemLabel.h"
#include "ui/hud/compass/CompassHud.h"
#include "ui/hud/world/ProjectedBox.h"
#include "ui/hud/world/ScreenGeometry.h"
#include "ui/hud/musichud/MusicHud.h"
#include "ui/hud/musichud/CoverTexture.h"
#include "ui/backend/blur/Blur.h"
#include "ui/notifications/NotificationHud.h"
#include "ui/overlay/WindowPlacement.h"
#include "smtc/SmtcService.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND,UINT,WPARAM,LPARAM);
namespace {
LRESULT CALLBACK WindowProc(HWND window,UINT message,WPARAM w,LPARAM l) {
    if(nova::ui::workspace::WindowMessage(message,w,l))return 0;
    if(nova::clickgui::CapturesInput() && nova::chrome::WindowMessage(window,message,w,l))return 0;
    if(message==WM_DESTROY) {PostQuitMessage(0);return 0;}
    if(message==WM_KEYDOWN && !(l&(1LL<<30))) {
        if(w==VK_INSERT)nova::clickgui::Toggle();
        if(w==VK_F2)nova::clickgui::SetMode(nova::clickgui::GetMode()==nova::clickgui::Mode::Panel?
            nova::clickgui::Mode::Classic:nova::clickgui::Mode::Panel);
        if(w==VK_F3) {
            auto& theme=nova::menu::panel::ClientSettings::Get().theme();theme.set(1-theme.get());
        }
    }
    if(message==WM_KEYDOWN || message==WM_KEYUP)nova::bind::InputManager::Get().onKey(static_cast<int>(w),message==WM_KEYDOWN);
    if(ImGui_ImplWin32_WndProcHandler(window,message,w,l))return 1;
    return DefWindowProcW(window,message,w,l);
}
void SaveFeatures(nova::json::Value& root) {
    auto features=nova::json::Value::Object();
    for(const auto& feature:nova::menu::Features().features)features[feature.id]=nova::config::SaveRows(feature.settings);
    root["features"]=std::move(features);
}
void LoadFeatures(const nova::json::Value& root) {
    if(const auto* features=root.Find("features"))for(const auto& feature:nova::menu::Features().features)
        if(const auto* row=features->Find(feature.id))nova::config::LoadRows(*row,feature.settings);
}
void Place(const char* id,double x,double y,nova::json::Value& placements) {
    auto entry=nova::json::Value::Object();entry["id"]=id;entry["x"]=x;entry["y"]=y;entry["moved"]=true;
    placements.Push(std::move(entry));
}
}
int TestGaussianHudBlur();
int main(int argc,char** argv) {
    const bool proxySmoke=argc>1 && std::string_view(argv[1])=="--proxy-smoke";
    const bool accountsSmoke=argc>1 && std::string_view(argv[1])=="--accounts-smoke";
    const bool browserSettingsSmoke=argc>1 && std::string_view(argv[1])=="--browser-settings-smoke";
    const bool workspaceSmoke=argc>1 && std::string_view(argv[1])=="--workspace-smoke";
    const bool browserSmoke=workspaceSmoke || browserSettingsSmoke || (argc>1 && std::string_view(argv[1])=="--browser-smoke");
    const auto savedPlacement=nova::config::LoadBrowserPlacement();
    const bool smoke=proxySmoke || accountsSmoke || browserSmoke || (argc>1 && std::string_view(argv[1])=="--smoke");
    _set_error_mode(_OUT_TO_STDERR);SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    WNDCLASSW wc{};wc.lpfnWndProc=WindowProc;wc.hInstance=GetModuleHandleW(nullptr);
    wc.lpszClassName=L"NovaUiPreview";wc.style=CS_OWNDC;RegisterClassW(&wc);
    const auto window=CreateWindowW(wc.lpszClassName,L"NOVA UI Preview | Insert: menu | F2: Panel/Classic | F3: theme",
        WS_OVERLAPPEDWINDOW,50,50,1440,1000,nullptr,nullptr,wc.hInstance,nullptr);
    if(!window)return 1;
    const auto dc=GetDC(window);
    PIXELFORMATDESCRIPTOR pfd{sizeof(pfd),1};pfd.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
    pfd.iPixelType=PFD_TYPE_RGBA;pfd.cColorBits=32;
    if(!SetPixelFormat(dc,ChoosePixelFormat(dc,&pfd),&pfd))return 2;
    const auto gl=wglCreateContext(dc);if(!gl || !wglMakeCurrent(dc,gl))return 3;
    ImGui::CreateContext();ImGui::GetIO().IniFilename=nullptr;InitializeFatalMenuFonts();
    if(!ImGui_ImplWin32_Init(window) || !ImGui_ImplOpenGL3_Init())return 4;
    if(argc>1 && std::string_view(argv[1])=="--blur-only") {
        const int result=TestGaussianHudBlur();
        nova::ui::blur::ReleaseOpenGL();ImGui_ImplOpenGL3_Shutdown();ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();ResetFatalMenuFonts();wglMakeCurrent(nullptr,nullptr);wglDeleteContext(gl);
        ReleaseDC(window,dc);DestroyWindow(window);return result;
    }
    nova::ui::arraylist::ArrayListHud array;
    nova::ui::autoblockhud::AutoBlockHud block;
    nova::ui::watermark::WatermarkHud watermark;
    nova::ui::targethud::TargetHud target;
    nova::ui::NotificationHud notifications;
    nova::ui::music::MusicHud music;
    nova::ui::BreakerHud breaker;
    nova::ui::BlockInHud blockIn;
    const char* names[]={"ArrayList","AutoBlockHUD","Watermark","TargetHUD","NameTags","2DESP",
        "ChestESP","ItemESP","BedPlates","Breaker","BlockIn","Compass","MusicHUD","Notifications","Backtrack"};
    std::array<Properties::BoolProperty,15> enabled{true,true,true,true,true,true,true,true,true,true,true,true,true,true,true};
    nova::ui::FeatureCatalog catalog;
    catalog.categories.push_back({"display","Display",u8"",nova::ui::IconFace::Fatal,"Display",u8"\uf03e",0});
    if(accountsSmoke||proxySmoke)for(const char* category:{"Combat","Movement","Player","World"})catalog.categories.push_back({category,category,u8"",nova::ui::IconFace::Fatal,category,u8"\uf007",0});
    auto add=[&](size_t i,std::vector<Properties::PropDesc> rows={}) {
        rows.insert(rows.begin(),{"Enabled",&enabled[i]});
        catalog.features.push_back({names[i],names[i],"display","General","HUD",true,&enabled[i],nullptr,std::move(rows)});
    };
    add(0,array.Options().Rows());add(1,block.Options().Rows());add(2,watermark.Options().Rows());add(3,target.Options().Rows());
    for(size_t i=4;i<12;++i)add(i);
    add(12,music.Settings().Rows());add(13,notifications.Settings().Rows());add(14);
    if(!nova::menu::InstallFeatureCatalog(std::move(catalog)))return 5;
    auto directory=std::filesystem::temp_directory_path()/"NovaUiPreview";
    if(smoke)directory/=std::to_string(GetCurrentProcessId());
    if(!nova::config::InstallHostConfig({directory,SaveFeatures,LoadFeatures}))return 6;
    nova::config::SetClientSettings(&nova::menu::panel::ClientSettings::Get().rows());
    nova::config::ConfigManager::Get().Init();
    if(proxySmoke){nova::proxy::SetSupported(true);nova::proxy::Config cfg;cfg.host="127.0.0.1";cfg.port=1080;
        cfg.authentication=true;cfg.username="NovaUser";cfg.password="preview-password";std::string error;nova::proxy::Apply(cfg,error);
        for(auto& value:enabled)value.set(false);
    }
    auto placements=nova::json::Value::Array();
    Place("overlays.arraylist",.83,.06,placements);Place("overlays.targethud",.03,.18,placements);
    Place("overlays.autoblockhud",.03,.33,placements);Place("overlays.musichud",.03,.50,placements);
    nova::ui::LoadWindowPlacements(placements);
    if(smoke&&!proxySmoke) {
        // Real config service and atomic disk publication, without game adapters.
        if(!nova::config::ConfigManager::Get().SaveAs("smoke"))return 7;
        enabled[0].set(false);
        if(!nova::config::ConfigManager::Get().LoadByName("smoke") || !enabled[0].get())return 8;
    }
    nova::notifications::Post("Independent UI ready",nova::notifications::Type::Success);
    const auto portrait=nova::ui::Portrait{nova::ui::skins::Store("preview/avatar",16,16,std::vector<std::uint32_t>(256,0xffdd9977))};
    if(!smoke)ShowWindow(window,SW_SHOW);
    int result=0;
    bool browserNavigated=false,browserCaptured=false;int browserReadyFrame=-1;
    ImVec2 dragStart{},sizeStart{},resizePoint{};
    // Opening the page must start Chromium without a separate launch action.
    if(browserSmoke)nova::clickgui::OpenBrowser();
    nova::menu::panel::PanelClickGui accountPanel;
    if(proxySmoke)accountPanel.layout().changePage(nova::menu::panel::PageLayout::ProxyPage);
    if(accountsSmoke) {
        accountPanel.layout().changePage(nova::menu::panel::PageLayout::AccountsPage);
        nova::accounts::Snapshot view;view.supported=true;view.currentName="NovaPlayer";view.status.clear();view.clientId.clear();view.hasHidden=true;
        view.rows={{"lunar:demo","NovaPlayer","",nova::accounts::Source::Lunar,true},
            {"offline:demo","OfflinePlayer","",nova::accounts::Source::Offline},
            {"token:demo","TokenPlayer","",nova::accounts::Source::Token}};
        // Small skin-face fixture exercises the production texture/portrait path.
        std::vector<std::uint32_t> face(64,IM_COL32(183,132,99,255));
        for(int y=0;y<2;++y)for(int x=0;x<8;++x)face[y*8+x]=IM_COL32(55,35,23,255);
        face[26]=face[29]=IM_COL32(80,73,130,255);
        for(int x=2;x<6;++x)face[6*8+x]=IM_COL32(83,50,35,255);
        const auto head=nova::ui::skins::Store("account-preview",8,8,std::move(face));
        for(auto& row:view.rows)row.portrait=head;
        nova::accounts::Publish(std::move(view));
        for(auto& value:enabled)value.set(false);
        nova::menu::panel::ClientSettings::Get().theme().set(1);
    }
    if(browserSettingsSmoke)for(const auto& row:nova::menu::panel::ClientSettings::Get().rows())
        if(std::string_view(row.getName())=="Blur")static_cast<Properties::BoolProperty*>(row.getProp())->set(true);
    for(int frame=0;;++frame) {
        MSG msg;bool quit=false;
        while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)) {if(msg.message==WM_QUIT)quit=true;TranslateMessage(&msg);DispatchMessageW(&msg);}
        if(quit || (smoke && !browserSmoke && frame>=150))break;
        if(browserSmoke && frame>=2000){result=11;break;}
        if(browserSmoke){if(!workspaceSmoke)nova::clickgui::SetOpen(true);nova::clickgui::SetMode(nova::clickgui::Mode::Panel);}
        else if(smoke) {
            nova::clickgui::SetOpen(frame>=30 && frame<130);
            nova::clickgui::SetMode(frame>=80?nova::clickgui::Mode::Classic:nova::clickgui::Mode::Panel);
            nova::menu::panel::ClientSettings::Get().theme().set(frame>=105?0:1);
        }
        ImGui_ImplOpenGL3_NewFrame();ImGui_ImplWin32_NewFrame();
        if(workspaceSmoke&&browserReadyFrame>=0){
            const int elapsed=frame-browserReadyFrame;auto& io=ImGui::GetIO();
            auto* panel=ImGui::FindWindowByName("PanelClickGui");auto* floating=ImGui::FindWindowByName("##ChromeForeground");
            if((elapsed==60||elapsed==61)&&panel){io.AddMousePosEvent(panel->Pos.x+716,panel->Pos.y+31);io.AddMouseButtonEvent(0,elapsed==60);}
            if(elapsed==112&&floating){dragStart=floating->Pos;io.AddMousePosEvent(dragStart.x+150,dragStart.y+16);io.AddMouseButtonEvent(0,true);}
            if(elapsed==114){io.AddMousePosEvent(dragStart.x+195,dragStart.y+36);}
            if(elapsed==116)io.AddMouseButtonEvent(0,false);
            if(elapsed==132&&floating){sizeStart=floating->Size;resizePoint=floating->Pos+floating->Size-ImVec2(3,3);io.AddMousePosEvent(resizePoint.x,resizePoint.y);io.AddMouseButtonEvent(0,true);}
            if(elapsed==134)io.AddMousePosEvent(resizePoint.x-70,resizePoint.y-40);
            if(elapsed==136)io.AddMouseButtonEvent(0,false);
            if((elapsed==180||elapsed==181)&&floating){io.AddMousePosEvent(floating->Pos.x+floating->Size.x-84,floating->Pos.y+16);io.AddMouseButtonEvent(0,elapsed==180);}
            if(elapsed==225)nova::clickgui::SetOpen(true);
            if(elapsed==260||elapsed==261){io.AddMousePosEvent(60,io.DisplaySize.y*.5f);io.AddMouseButtonEvent(0,elapsed==260);}
            if((elapsed==305||elapsed==306||elapsed==330||elapsed==331)&&floating){io.AddMousePosEvent(floating->Pos.x+floating->Size.x-54,floating->Pos.y+16);io.AddMouseButtonEvent(0,elapsed==305||elapsed==330);}
            if((elapsed==310||elapsed==311)&&floating){io.AddMousePosEvent(floating->Pos.x+floating->Size.x-60,floating->Pos.y+95);io.AddMouseButtonEvent(0,elapsed==310);}
            if(elapsed==355){nova::ui::workspace::WindowMessage(WM_KEYDOWN,VK_ESCAPE,0);nova::ui::workspace::WindowMessage(WM_KEYUP,VK_ESCAPE,0);}
            if((elapsed==460||elapsed==461)&&panel){io.AddMousePosEvent(panel->Pos.x+141,panel->Pos.y+28);io.AddMouseButtonEvent(0,elapsed==460);}
            if(elapsed==520)nova::clickgui::SetOpen(true);
            if(elapsed==620||elapsed==621){io.AddMousePosEvent(60,io.DisplaySize.y*.5f);io.AddMouseButtonEvent(0,elapsed==620);}
            if(elapsed==770)nova::clickgui::SetOpen(true);
            if(elapsed==835||elapsed==930)nova::clickgui::Close();
            if(elapsed==885||elapsed==980){
                for(const auto& row:nova::menu::panel::ClientSettings::Get().rows())
                    if(std::string_view(row.getName())=="Stage Manager")
                        static_cast<Properties::BoolProperty*>(row.getProp())->set(elapsed==980);
            }
            if(elapsed==900)nova::clickgui::SetOpen(true);
        }
        if(browserSettingsSmoke&&browserReadyFrame>=0){
            const int elapsed=frame-browserReadyFrame;
            if(elapsed==35||elapsed==36){
                if(auto* panel=ImGui::FindWindowByName("PanelClickGui")){
                    const float scale=nova::menu::panel::ClientSettings::Get().guiScale();
                    auto& io=ImGui::GetIO();io.AddMousePosEvent(panel->Pos.x+684*scale,panel->Pos.y+69*scale);
                    io.AddMouseButtonEvent(0,elapsed==35);
                }
            }
            if(elapsed==80)nova::menu::panel::ClientSettings::Get().theme().set(0);
        }
        if(accountsSmoke) {
            auto* panel=ImGui::FindWindowByName("PanelClickGui");
            if(panel&&(frame==40||frame==41)){
                auto& io=ImGui::GetIO();io.AddMousePosEvent(panel->Pos.x+452,panel->Pos.y+516);io.AddMouseButtonEvent(0,frame==40);
            }
            if(frame==50)ImGui::GetIO().AddInputCharactersUTF8("PreviewPlayer");
            if(frame==78||frame==79)ImGui::GetIO().AddKeyEvent(ImGuiKey_Escape,frame==78);
            if(panel&&(frame==90||frame==91||frame==120||frame==121||frame==130||frame==131)) {
                const float x=frame<100?643.f:frame<130?261.f:452.f;
                auto& io=ImGui::GetIO();io.AddMousePosEvent(panel->Pos.x+x,panel->Pos.y+516);
                io.AddMouseButtonEvent(0,frame==90||frame==120||frame==130);
            }
            if(frame==95)ImGui::GetIO().AddInputCharactersUTF8("secret-preview-token");
            if(frame==136)ImGui::GetIO().AddInputCharactersUTF8("BlockedPlayer");
            if((frame==101||frame==102||frame==140||frame==141)&&!GImGui->OpenPopupStack.empty()) {
                if(auto* popup=GImGui->OpenPopupStack.back().Window) {
                    auto& io=ImGui::GetIO();io.AddMousePosEvent(popup->Pos.x+90,popup->Pos.y+popup->Size.y-29);io.AddMouseButtonEvent(0,frame==101||frame==140);
                }
            }
            if(frame==105||frame==106||frame==145||frame==146)ImGui::GetIO().AddKeyEvent(ImGuiKey_Escape,frame==105||frame==145);
            if(frame==126){auto view=nova::accounts::Read();view.inWorld=true;nova::accounts::Publish(std::move(view));}
        }
        ImGui::NewFrame();
        if(accountsSmoke && frame==90)nova::menu::panel::ClientSettings::Get().theme().set(0);
        if(workspaceSmoke&&browserReadyFrame>=0){
            const int elapsed=frame-browserReadyFrame;
            if(elapsed==322)ImGui::ClosePopupsOverWindow(nullptr,false);
            if(elapsed==570)nova::ui::workspace::Detach();
            if(elapsed==670)nova::ui::workspace::Restore();
            if(elapsed==720)nova::ui::workspace::Shelve();
        }
        if(workspaceSmoke&&browserReadyFrame>=0&&frame-browserReadyFrame==400)nova::ui::workspace::Dock();
        auto* draw=ImGui::GetBackgroundDrawList();const auto display=ImGui::GetIO().DisplaySize;
        // Publish the backdrop before drawing any UI, so blur never samples HUDs.
        glViewport(0,0,static_cast<int>(display.x),static_cast<int>(display.y));
        glEnable(GL_SCISSOR_TEST);
        for(int y=0;y<display.y;y+=48)for(int x=0;x<display.x;x+=48) {
            const float shade=((x/48+y/48)%2)?1.f:.65f;
            glScissor(x,y,48,48);glClearColor(.16f*shade,.21f*shade,.28f*shade,1);glClear(GL_COLOR_BUFFER_BIT);
        }
        glDisable(GL_SCISSOR_TEST);
        nova::ui::blur::RenderOpenGL(nova::ui::blur::GetOptions(),static_cast<int>(display.x),static_cast<int>(display.y));
        const bool editing=nova::clickgui::IsOpen();
        std::vector<nova::ui::arraylist::Entry> entries;
        for(size_t i=0;i<enabled.size();++i)entries.push_back({names[i],names[i],"Preview",enabled[i].get(),i==0});
        array.Draw(enabled[0].get(),editing,entries);
        const float pulse=static_cast<float>(std::sin(ImGui::GetTime()));
        block.Draw(enabled[1].get(),editing,{true,75+pulse*20,portrait});
        watermark.Draw(enabled[2].get(),{"Preview","Local preview","0 / 64 / 0","Native",12,4.2,20});
        const std::array<nova::ui::targethud::Snapshot,1> targets{{{true,1,"Sample target",14+pulse*3,2,20,3,2,portrait}}};
        if(enabled[3].get())target.Draw(editing,targets);
        if(enabled[4].get()) {
            nova::ui::nametags::DrawDefault({display.x*.6f,180},1,"Default","20+2",true,{});
            nova::ui::nametags::DrawClassic({display.x*.6f,230},2,{{"C",IM_COL32_WHITE,true},{"lassic",0xff88ff88}},{});
        }
        if(enabled[5].get())nova::ui::esp2d::Draw({display.x*.55,280,display.x*.6,430},1,
            nova::ui::esp2d::BoxMode::Normal,nova::ui::esp2d::HealthMode::Health,14,20,10,true);
        const nova::ui::ProjectedBox box{{{720,520},{800,520},{720,450},{800,450},{745,540},{825,540},{745,470},{825,470}}};
        if(enabled[6].get())nova::ui::DrawFilledBox(draw,box,0x5500ff00);
        if(enabled[7].get())nova::ui::DrawItemLabel({600,500},16,.03,64,IM_COL32_WHITE,true);
        if(enabled[8].get())nova::ui::DrawBedPlate({{1000,430},"Wool",12,4},{});
        breaker.Advance((pulse+1)*.5f,ImGui::GetIO().DeltaTime);
        if(enabled[9].get()) {breaker.DrawProgress(88);nova::ui::BreakerHud::DrawBox(box,{true,true,true,true,true,true,true,true},true);}
        if(enabled[10].get())blockIn.Draw(5,true,ImGui::GetTime()*1000);
        if(enabled[11].get())nova::ui::DrawCompass(pulse);
        if(enabled[14].get())nova::ui::DrawGeometry({{{{900,500},{960,500},{960,600},{900,600}}},{{{{900,500},{960,600}}}}},0x23ffeae5,0xbeffeae5,1.5f);
        // Editing supplies the existing SMTC demo and lyric preview without a player.
        // Smoke leaves media disabled to avoid starting WinRT/background reads.
        if(smoke&&!accountsSmoke)music.DrawPreview();
        else {music.SetEnabled(enabled[12].get());music.Draw(editing);}
        static ImVec2 beforeOffscreenTest{};
        if(smoke&&!browserSmoke&&!accountsSmoke){
            if(auto* panel=ImGui::FindWindowByName("PanelClickGui")){
                if(frame==70){beforeOffscreenTest=panel->Pos;ImGui::SetWindowPos("PanelClickGui",display-ImVec2(100,100));}
                if(frame==72)ImGui::SetWindowPos("PanelClickGui",ImVec2(-400,-300));
                if(frame==74)ImGui::SetWindowPos("PanelClickGui",beforeOffscreenTest);
            }
        }
        notifications.Draw(enabled[13].get());if(accountsSmoke||proxySmoke)accountPanel.render();else nova::clickgui::Render();
        if(accountsSmoke&&frame==65&&GImGui->OpenPopupStack.empty()){puts("FAIL: account popup did not open");result=18;break;}
        if(accountsSmoke&&frame==85&&!GImGui->OpenPopupStack.empty()){puts("FAIL: account popup reopens after Escape");result=18;break;}
        if(accountsSmoke&&(frame==100||frame==135)&&GImGui->OpenPopupStack.empty()){puts("FAIL: token/offline button did not open a popup");result=18;break;}
        if(accountsSmoke&&frame==100&&GImGui->InputTextState.TextLen!=20){puts("FAIL: token popup did not retain typed input");result=18;break;}
        if(accountsSmoke&&frame==104){
            nova::accounts::Request request{nova::accounts::Action::Select,{}};
            if(!nova::accounts::Take(request)||request.action!=nova::accounts::Action::Token||request.value!="secret-preview-token"){
                puts("FAIL: filled Token Continue button did not submit");result=18;break;
            }
            if(!GImGui->OpenPopupStack.empty()){puts("FAIL: token popup did not close after submission");result=18;break;}
            puts("PASS: token text input and Continue submit the complete request");
        }
        if(accountsSmoke&&frame==144){
            nova::accounts::Request request{nova::accounts::Action::Select,{}};
            if(nova::accounts::Take(request)||GImGui->OpenPopupStack.empty()){
                puts("FAIL: account submission must stay blocked in a world");result=18;break;
            }
            puts("PASS: filled account form stays blocked in a world");
        }
        if(accountsSmoke&&frame==125){
            nova::accounts::Request request{nova::accounts::Action::Select,{}};
            if(!nova::accounts::Take(request)||request.action!=nova::accounts::Action::Microsoft){puts("FAIL: Microsoft button did not submit");result=18;break;}
            if(!GImGui->OpenPopupStack.empty()){puts("FAIL: Microsoft button opened a setup popup");result=18;break;}
        }
        if(smoke&&!browserSmoke&&!accountsSmoke&&(frame==71||frame==73)){
            auto* panel=ImGui::FindWindowByName("PanelClickGui");
            bool fullBackground=false,offscreenChild=false;
            if(panel){
                for(const auto& cmd:panel->DrawList->CmdBuffer)
                    if(cmd.ElemCount&&cmd.ClipRect.x<=panel->Pos.x&&cmd.ClipRect.y<=panel->Pos.y&&
                       cmd.ClipRect.z>=panel->Pos.x+panel->Size.x&&cmd.ClipRect.w>=panel->Pos.y+panel->Size.y)fullBackground=true;
                for(auto* child:GImGui->Windows)if(child!=panel&&child->Active&&child->RootWindow==panel)
                    for(const auto& cmd:child->DrawList->CmdBuffer)
                        if(cmd.ElemCount&&(cmd.ClipRect.x<0||cmd.ClipRect.y<0||cmd.ClipRect.z>display.x||cmd.ClipRect.w>display.y))offscreenChild=true;
            }
            if(!fullBackground||!offscreenChild){puts("FAIL: offscreen panel snapshot loses background or children");result=16;break;}
            puts("PASS: offscreen panel retains full background and child content");
        }
        ImGui::Render();glViewport(0,0,static_cast<int>(display.x),static_cast<int>(display.y));
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        if((accountsSmoke&&(frame==65||frame==115))||(proxySmoke&&frame==60)) {
            const int w=int(display.x),h=int(display.y),stride=(w*3+3)&~3;std::vector<unsigned char> pixels(size_t(stride)*h);
            glPixelStorei(GL_PACK_ALIGNMENT,4);glReadPixels(0,0,w,h,0x80E0,GL_UNSIGNED_BYTE,pixels.data());
            BITMAPFILEHEADER file{};file.bfType=0x4D42;file.bfOffBits=sizeof(file)+sizeof(BITMAPINFOHEADER);file.bfSize=file.bfOffBits+DWORD(pixels.size());
            BITMAPINFOHEADER info{};info.biSize=sizeof(info);info.biWidth=w;info.biHeight=h;info.biPlanes=1;info.biBitCount=24;
            std::ofstream shot(proxySmoke?"proxy-page.bmp":frame==65?"accounts-dark.bmp":"accounts-light.bmp",std::ios::binary);shot.write(reinterpret_cast<char*>(&file),sizeof(file));shot.write(reinterpret_cast<char*>(&info),sizeof(info));shot.write(reinterpret_cast<char*>(pixels.data()),pixels.size());
            if(proxySmoke){puts("PASS: Proxy page rendered with authenticated configuration");break;}
        }
        if(smoke&&frame==30){
            auto* panel=ImGui::FindWindowByName("PanelClickGui");
            if(!panel){puts("FAIL: snapshot regression requires panel");result=15;break;}
            nova::ui::workspace::PanelSnapshot snapshot;
            snapshot.capture(panel);snapshot.freeze();
            const int textures=ImGui::GetPlatformIO().Textures.Size;
            for(int i=0;i<20;++i)snapshot.freeze();
            const bool unchanged=ImGui::GetPlatformIO().Textures.Size==textures;
            // These private test textures have not been submitted to a backend.
            snapshot.release();
            if(!unchanged){puts("FAIL: repeated freeze replaces snapshot textures");result=15;break;}
            puts("PASS: repeated snapshot freeze preserves textures across rapid reversals");
        }
        if(browserSmoke){
            const auto browser=nova::chrome::Read();
            if(!browser.error.empty()){printf("Browser: %s\n",browser.error.c_str());result=12;break;}
            if(browser.status.ready&&!browserNavigated){nova::chrome::Navigate(L"http://127.0.0.1:18795/test.html");browserNavigated=true;}
            if(browser.frame&&std::wstring(browser.status.tabs[0].title)==L"Nova Browser Test"&&browserReadyFrame<0)browserReadyFrame=frame;
            const int elapsed=frame-browserReadyFrame;
            if(workspaceSmoke&&browserReadyFrame>=0){
                auto fail=[&](const char* msg){printf("FAIL: %s\n",msg);result=14;};
                const auto state=nova::ui::workspace::Current();auto* floating=ImGui::FindWindowByName("##ChromeForeground");
                if(elapsed==110&&(state!=nova::ui::workspace::State::Foreground||nova::clickgui::IsOpen()||!floating||!floating->Active))fail("detached browser must render with ClickGUI closed");
                if(elapsed==125&&(!floating||floating->Pos.x<=dragStart.x+20))fail("title dragging moves independent window");
                if(elapsed==170&&(!floating||floating->Size.x>=sizeStart.x-30))fail("corner resizing changes independent viewport");
                if(elapsed==220&&(state!=nova::ui::workspace::State::Shelved||nova::clickgui::CapturesInput()))fail("shelf must release game input");
                if(elapsed==300&&(state!=nova::ui::workspace::State::Foreground||!nova::clickgui::CapturesInput()))fail("thumbnail restores input and window");
                if(elapsed==320&&(!floating||floating->Size.y<display.y-30))fail("maximize fills work area");
                if(elapsed==320){
                    if(GImGui->OpenPopupStack.empty())fail("maximized settings popup opens");
                    else {
                        auto* popup=GImGui->OpenPopupStack.back().Window;
                        const auto* data=ImGui::GetDrawData();int parentIndex=-1,popupIndex=-1;
                        for(int i=0;i<data->CmdListsCount;++i){if(data->CmdLists[i]==floating->DrawList)parentIndex=i;if(popup&&data->CmdLists[i]==popup->DrawList)popupIndex=i;}
                        if(parentIndex<0||popupIndex<=parentIndex)fail("settings popup must render above maximized browser");
                    }
                }
                if(elapsed==345&&(!floating||floating->Size.x>=sizeStart.x-30))fail("restore retains resized bounds");
                if(elapsed==390&&(state!=nova::ui::workspace::State::Shelved||nova::clickgui::CapturesInput()))fail("Escape shelves foreground and releases input");
                if(elapsed==440&&(state!=nova::ui::workspace::State::Embedded||!nova::clickgui::IsOpen()||std::wstring(browser.status.tabs[0].title)!=L"Nova Browser Test"))fail("docking preserves session");
                if(elapsed==880&&(!nova::ui::workspace::PanelShelved()||nova::clickgui::CapturesInput()))fail("closing ClickGUI automatically shelves and releases input");
                if((elapsed==895||elapsed==970)&&(nova::ui::workspace::PanelHidden()||nova::ui::workspace::Detached()||nova::clickgui::CapturesInput()))fail("disabled Stage Manager removes shelves and restores normal closing");
                if(elapsed==510&&(!nova::ui::workspace::PanelShelved()||nova::clickgui::CapturesInput()))fail("ClickGUI shelf button releases input");
                if((elapsed==560||elapsed==660||elapsed==815)&&(!nova::clickgui::IsOpen()||nova::ui::workspace::PanelHidden()))fail("ClickGUI restores from shortcut or thumbnail");
                if(elapsed==660&&state!=nova::ui::workspace::State::Shelved)fail("restoring ClickGUI shelves Chrome");
                if(elapsed==760){
                    auto* panelCard=ImGui::FindWindowByName("##PanelStageStrip");auto* chromeCard=ImGui::FindWindowByName("##ChromeStageStrip");
                    if(!panelCard||!chromeCard||!panelCard->Active||!chromeCard->Active||panelCard->Pos.y<=chromeCard->Pos.y+chromeCard->Size.y||nova::clickgui::CapturesInput())fail("two shelf cards have separate bounds and release input");
                }
                if(result)break;
            }
            if(browserReadyFrame>=0&&(elapsed==30||(browserSettingsSmoke&&(elapsed==65||elapsed==100))||(workspaceSmoke&&(elapsed==170||elapsed==220||elapsed==300||elapsed==320||elapsed==510||elapsed==760)))){
                if(browserSettingsSmoke&&elapsed>30&&(GImGui->OpenPopupStack.empty()||!nova::ui::blur::Available())){puts("FAIL: browser settings or blur unavailable");result=13;break;}
                const int w=int(display.x),h=int(display.y),stride=(w*3+3)&~3;std::vector<unsigned char> pixels(size_t(stride)*h);
                glPixelStorei(GL_PACK_ALIGNMENT,4);glReadPixels(0,0,w,h,0x80E0,GL_UNSIGNED_BYTE,pixels.data());
                BITMAPFILEHEADER file{};file.bfType=0x4D42;file.bfOffBits=sizeof(file)+sizeof(BITMAPINFOHEADER);file.bfSize=file.bfOffBits+DWORD(pixels.size());
                BITMAPINFOHEADER info{};info.biSize=sizeof(info);info.biWidth=w;info.biHeight=h;info.biPlanes=1;info.biBitCount=24;
                const char* name=elapsed==320?"workspace-maximized-popup.bmp":elapsed==510?"workspace-panel-shelf.bmp":elapsed==760?"workspace-two-shelves.bmp":elapsed==170?"workspace-floating.bmp":elapsed==220?"workspace-shelf.bmp":elapsed==300?"workspace-restored.bmp":elapsed==65?"browser-settings-dark.bmp":elapsed==100?"browser-settings-light.bmp":"browser-preview.bmp";
                std::ofstream shot(name,std::ios::binary);shot.write(reinterpret_cast<char*>(&file),sizeof(file));shot.write(reinterpret_cast<char*>(&info),sizeof(info));shot.write(reinterpret_cast<char*>(pixels.data()),pixels.size());browserCaptured=true;
                if(elapsed==30){ImGui_ImplOpenGL3_Shutdown();ImGui_ImplOpenGL3_Init();}
            }
            if(browserCaptured&&elapsed>(workspaceSmoke?990:browserSettingsSmoke?105:60)){puts("PASS: Chromium in shared Panel UI and OpenGL texture recreation");if(browserSettingsSmoke)puts("PASS: browser settings popup, dark/light screenshots");if(workspaceSmoke)puts("PASS: detach, drag, resize, maximize, popup layering, ClickGUI shelf/shortcut/thumbnail, two shelves, close-to-shelf, Stage Manager toggle, Escape, dock and session continuity");break;}
        }
        if(glGetError()!=GL_NO_ERROR) {result=9;break;}
        SwapBuffers(dc);Sleep(10);
    }
    music.SetEnabled(false);nova::smtc::BeginShutdown();
    nova::chrome::Shutdown();
    if(!nova::smtc::WaitStopped(5000))result=10;
    nova::ui::blur::ReleaseOpenGL();
    ImGui_ImplOpenGL3_Shutdown();ImGui_ImplWin32_Shutdown();nova::ui::skins::Release();nova::ui::music::ReleaseCovers();
    nova::ui::browser::ReleaseTextures();
    ImGui::DestroyContext();ResetFatalMenuFonts();wglMakeCurrent(nullptr,nullptr);wglDeleteContext(gl);
    ReleaseDC(window,dc);DestroyWindow(window);
    if(workspaceSmoke)nova::config::SaveBrowserPlacement(savedPlacement);
    if(!result&&accountsSmoke)puts("PASS: Accounts dark/light portraits, all three footer actions, in-world popup, Escape closes without reopening");
    if(!result)puts("PASS: native shared UI/HUD preview, Panel/Classic, light/dark, config round trip; no Minecraft/JVM");
    return result;
}
