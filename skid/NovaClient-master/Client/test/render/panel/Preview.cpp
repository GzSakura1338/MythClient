// Production Panel ClickGUI rendered into a hidden OpenGL window. No game process.
#define NOMINMAX
#include <windows.h>
#include <GL/gl.h>
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_opengl3.h"
#include "ui/clickgui/Fonts.h"
#include "ui/clickgui/panel/PanelClickGui.h"
#include "ui/clickgui/panel/PageLayout.h"
#include "ui/clickgui/panel/ClientSettings.h"
#include "ui/clickgui/panel/Widgets.h"
#include "ui/backend/blur/Blur.h"
#include "minecraft/modules/ModuleManager.h"
#include "ui/hud/musichud/SourceProperty.h"
#include "ui/clickgui/FeatureTree.h"
#include "config/ConfigManager.h"
#include "smtc/SmtcService.h"

namespace nova::java { bool ScreenOpen(){return false;} }
namespace nova::input { bool IsProcessForeground(DWORD){return true;} }
namespace nova::clickgui { bool IsOpen(){return true;} }
namespace nova::ui { void PlaceWindowCentered(const char*,float width,float){ImGui::SetNextWindowPos({std::min(100.f,1200.f-width-8.f),90},ImGuiCond_Once);} void CaptureWindow(const char*){} }
namespace nova::smtc { std::shared_ptr<const MediaSnapshot> ReadSnapshot(){auto p=std::make_shared<MediaSnapshot>();p->sources={{"cloudmusic.exe","NetEase Cloud Music"},{"test-player","Music player"}};return p;} }
// Configuration service double: verifies UI actions without touching user configs.
namespace nova::config {
ConfigManager& ConfigManager::Get(){static ConfigManager manager;return manager;}
Config::Config(std::string n,std::filesystem::path p):name_(std::move(n)),file_(std::move(p)){}
std::string SanitizeConfigName(std::string_view s){return std::string(s);}
void ConfigManager::Refresh() { if(configs_.empty()){configs_.emplace_back("default","");configs_.emplace_back("pvp","");} }
bool ConfigManager::SaveCurrent(bool){return true;}
bool ConfigManager::SaveAs(std::string_view n){configs_.emplace_back(std::string(n),"");current_=n;return true;}
bool ConfigManager::LoadByName(std::string_view n){current_=n;return true;}
bool ConfigManager::Rename(std::string_view a,std::string_view b){for(auto& c:configs_)if(c.name()==a)c=Config(std::string(b),"");return true;}
bool ConfigManager::Delete(std::string_view n){std::erase_if(configs_,[&](const auto& c){return c.name()==n;});return true;}
bool ConfigManager::Duplicate(std::string_view n){return SaveAs(std::string(n)+" copy");}
void ConfigManager::OpenFolder()const{}
}

class Sample final : public nova::module::Module {
public:
    Properties::StringProperty text{"PlayerName"};
    nova::ui::music::SourceProperty source;
    Sample(std::string name,nova::module::Category cat,std::string group):Module(name,cat,"General",group){
        if (name == "KeepSprint") {
            addNumber("Horizontal speed",0,1000,1,670,{},[](double v){return std::format("{:.0f} deg/s",v);});
            addNumber("Vertical speed",0,1000,1,320,{},[](double v){return std::format("{:.0f} deg/s",v);});
            addNumber("Hit Slow Down During Velocity",0,1,.05,.6);
            addNumber("Hit Slow Down Normal",0,1,.05,.6);
            addNumber("Buffer Decrease",.1,10,.1,1);
            addNumber("Max Buffer",1,10,1,5);
            addBool("Velocity Hit Sprint",false);
            addBool("Normal Hit Sprint",false);
            addBool("Buffer Abuse",true);
            addBool("Only In Air",false);
            return;
        }
        addMode("Mode",0,{"Default","Classic","Watchdog"});
        addNumber("Range",1,10,.1,3,{},[](double v){return std::format("{:.1f}",v);});
        addBool("Players",true,{subBool("Invisible",false),subNumber("Distance",1,100,1,50)});
        addBool("Animals",false);
        addColor(0xffff7e4b,true);
        addSettings({Properties::PropDesc("Names",&text),Properties::PropDesc("Media source",&source)});
        addSettings({Properties::PropDesc("Sneaking",nullptr,{subBool("Sneak option",false)}),
            Properties::PropDesc("Timing",nullptr,{subBool("Timing option",true)})});
    }
};

void Save(const char* name,int width,int height){
    std::vector<unsigned char> pixels(width*height*3);glReadPixels(0,0,width,height,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
    std::ofstream out(std::string(name)+".ppm",std::ios::binary);out<<"P6\n"<<width<<" "<<height<<"\n255\n";
    for(int y=height-1;y>=0;--y)out.write(reinterpret_cast<char*>(pixels.data()+y*width*3),width*3);
}
int main(int argc,char** argv){
    _set_error_mode(_OUT_TO_STDERR);
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    WNDCLASSW wc{};wc.lpfnWndProc=DefWindowProcW;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"NovaPanelPreview";wc.style=CS_OWNDC;
    RegisterClassW(&wc);HWND window=CreateWindowW(wc.lpszClassName,L"Hidden preview",WS_POPUP,0,0,1200,1000,nullptr,nullptr,wc.hInstance,nullptr);assert(window);
    HDC dc=GetDC(window);PIXELFORMATDESCRIPTOR pfd{sizeof(pfd),1};pfd.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;pfd.iPixelType=PFD_TYPE_RGBA;pfd.cColorBits=32;
    int fmt=ChoosePixelFormat(dc,&pfd);assert(SetPixelFormat(dc,fmt,&pfd));HGLRC rc=wglCreateContext(dc);assert(wglMakeCurrent(dc,rc));
    ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.DisplaySize={1200,1000};io.DeltaTime=1.f/60;
    InitializeFatalMenuFonts();assert(ImGui_ImplOpenGL3_Init());
    auto& settings=nova::menu::panel::ClientSettings::Get();settings.theme().set(1);
    if(argc>1)settings.theme().set(std::atoi(argv[1]));
    if(argc>2)for(auto& row:settings.rows())if(std::string_view(row.getName())=="Size")static_cast<Properties::NumberProp*>(row.getProp())->set(std::atof(argv[2]));
    auto& manager=nova::module::ModuleManager::Get();
    for(int cat=0;cat<5;++cat)for(int i=0;i<20;++i)manager.Create<Sample>(i==0?"KillAura":i==1?"AutoBlock":i==2?"Aim":std::format("Module {}",i),static_cast<nova::module::Category>(cat),cat==0?"General":i<6?"Combat":i<12?"Target":"General");
    auto& clippingFixture=manager.Create<Sample>("KeepSprint",nova::module::Category::Combat,"General");
    nova::ui::FeatureCatalog catalog;
    constexpr const char* names[]={"Combat","Player","Movement","Render","Miscellaneous"};
    constexpr const char8_t* icons[]={u8"\uf05b",u8"\uf007",u8"\uf8cc",u8"\uf03e",u8"\uf1de"};
    constexpr int order[]={0,2,1,3,4};
    for(int cat=0;cat<5;++cat)catalog.categories.push_back({names[cat],names[cat],u8"",nova::ui::IconFace::Fatal,names[cat],icons[cat],order[cat]});
    for(const auto& m:manager.All()) {
        const auto cat=static_cast<int>(m->category());
        catalog.features.push_back({std::format("{}.{}",cat,m->name()),m->name(),names[cat],m->page(),m->subpage(),false,
            m->enabledProperty(),&m->hiddenInArrayListProperty(),m->settings()});
    }
    assert(nova::menu::InstallFeatureCatalog(std::move(catalog)));
    nova::config::ConfigManager::Get().Refresh();
    nova::menu::panel::PanelClickGui gui;
    auto frame=[&]{
        ImGui_ImplOpenGL3_NewFrame();ImGui::NewFrame();
        auto* bg=ImGui::GetBackgroundDrawList();bg->AddRectFilledMultiColor({0,0},io.DisplaySize,IM_COL32(35,48,67,255),IM_COL32(20,30,45,255),IM_COL32(60,77,95,255),IM_COL32(32,41,62,255));
        gui.render();ImGui::Render();glViewport(0,0,1200,1000);glClearColor(.04f,.05f,.08f,1);glClear(GL_COLOR_BUFFER_BIT);ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());glFinish();
    };
    for(int i=0;i<60;++i)frame();Save("panel",1200,1000);
    std::ofstream layout("windows.txt");for(auto* w:GImGui->Windows)if(w->Active)layout<<w->Name<<" "<<w->Pos.x<<","<<w->Pos.y<<" "<<w->Size.x<<","<<w->Size.y<<"\n";
    auto settle=[&](int n=36){for(int i=0;i<n;++i)frame();};
    const float scale=settings.guiScale();
    const auto base=ImGui::FindWindowByName("PanelClickGui")->Pos;
    auto click=[&](ImVec2 p,int button=0){io.AddMousePosEvent(p.x,p.y);frame();io.AddMouseButtonEvent(button,true);frame();io.AddMouseButtonEvent(button,false);frame();};
    auto row=[&](const char* label){
        const std::string match=std::string("/")+label+"###row_";
        ImGuiWindow* found=nullptr;
        for(auto* w:GImGui->Windows)if(w->Active&&std::string_view(w->Name).find(match)!=std::string_view::npos)found=w;
        assert(found);return found;
    };
    auto control=[&](const char* label,float inset=16.f){auto* w=row(label);return w->Pos+ImVec2(w->Size.x-(inset+7.f)*scale,18.5f*scale);};
    // One large General group must occupy both columns, with aligned top rows.
    assert(row("Module 11")->Pos.x>row("KillAura")->Pos.x+200*scale);
    assert(std::abs(row("Module 11")->Pos.y-row("KillAura")->Pos.y)<1);
    assert(row("KeepSprint")->ClipRect.Max.y>=row("KeepSprint")->Pos.y+36*scale);
    auto popup=[&](){assert(GImGui->OpenPopupStack.Size);auto* w=GImGui->OpenPopupStack.back().Window;assert(w&&w->Active);return w;};
    bool reopenModule=true;
    auto dismiss=[&](){click({20,20});settle();assert(GImGui->OpenPopupStack.empty());if(reopenModule){click(control("KillAura",10));settle();}};
    auto* selected=manager.All().front().get();assert(selected);
    auto prop=[&](const char* label){for(const auto& p:selected->settings())if(std::string_view(p.getName())==label)return p.getProp();return (Properties::AbstractProperty*)nullptr;};
    auto* enabled=dynamic_cast<Properties::BoolProperty*>(prop("Enabled"));assert(enabled&&!enabled->get());
    click(control("KillAura",38));settle();assert(enabled->get());
    click(control("KillAura",10));settle();assert(row("Mode"));
    // Drag from popup padding; automatic placement must not reset the position.
    const ImVec2 beforeDrag=popup()->Pos;
    const ImVec2 dragStart=beforeDrag+ImVec2(80*scale,5*scale);
    io.AddMousePosEvent(dragStart.x,dragStart.y);frame();
    io.AddMouseButtonEvent(0,true);frame();
    io.AddMousePosEvent(dragStart.x+24*scale,dragStart.y+18*scale);frame();frame();
    io.AddMouseButtonEvent(0,false);frame();settle();
    assert(popup()->Pos.x>beforeDrag.x+20*scale);
    assert(popup()->Pos.y>beforeDrag.y+14*scale);
    auto numericHover=control("Range",20);
    io.AddMousePosEvent(numericHover.x,numericHover.y);settle();Save("numeric-tooltip",1200,1000);
    click(control("Mode",70));settle();Save("selector-open",1200,1000);
    auto* mode=dynamic_cast<Properties::ModeProperty*>(prop("Mode"));assert(mode);
    click(popup()->Pos+ImVec2(55*scale,52*scale));assert(mode->get()==1);
    frame();assert(ImGui::GetForegroundDrawList()->VtxBuffer.Size>0); // Closing geometry survives native popup dismissal.
    Save("selector-closing",1200,1000);settle(60);assert(ImGui::GetForegroundDrawList()->VtxBuffer.empty());
    auto* range=dynamic_cast<Properties::NumberProp*>(prop("Range"));assert(range);
    click(control("Range",72));assert(range->get()>7&&range->get()<8.5);
    click(control("Range",20));settle(3);io.AddKeyEvent(ImGuiMod_Ctrl,true);io.AddKeyEvent(ImGuiKey_A,true);frame();io.AddKeyEvent(ImGuiKey_A,false);io.AddKeyEvent(ImGuiMod_Ctrl,false);frame();io.AddInputCharactersUTF8("4.2");frame();io.AddKeyEvent(ImGuiKey_Enter,true);frame();io.AddKeyEvent(ImGuiKey_Enter,false);frame();fprintf(stderr,"numeric value: %.3f\n",range->get());assert(std::abs(range->get()-4.2)<.11);
    auto editNumber=[&](const char* value){
        click(control("Range",20));settle(3);
        io.AddKeyEvent(ImGuiMod_Ctrl,true);io.AddKeyEvent(ImGuiKey_A,true);frame();
        io.AddKeyEvent(ImGuiKey_A,false);io.AddKeyEvent(ImGuiMod_Ctrl,false);frame();
        io.AddInputCharactersUTF8(value);frame();
    };
    editNumber("6.3");Save("numeric-edit",1200,1000);
    click({20,20});frame();assert(std::abs(range->get()-6.3)<.01);
    settle();click(control("KillAura",10));settle();
    editNumber("1e");io.AddKeyEvent(ImGuiKey_Enter,true);frame();io.AddKeyEvent(ImGuiKey_Enter,false);frame();
    assert(std::abs(range->get()-6.3)<.01); // Reject incomplete numeric input.
    click(control("Players",10));settle();Save("settings",1200,1000);assert(row("Invisible"));
    click(control("Invisible"));dismiss();
    auto* color=dynamic_cast<Properties::ColorProperty*>(prop("Static Color"));assert(color);const auto oldSat=color->s();
    click(control("Static Color",9));settle();Save("color",1200,1000);
    click(popup()->Pos+ImVec2(72*scale,80*scale));assert(std::abs(color->s()-oldSat)>.01f);dismiss();
    click(control("Media source",70));settle();Save("sources",1200,1000);
    click(popup()->Pos+ImVec2(70*scale,52*scale));settle();assert(static_cast<Sample*>(selected)->source.get()=="cloudmusic.exe");
    click(control("Names",70));settle(3);io.AddKeyEvent(ImGuiMod_Ctrl,true);io.AddKeyEvent(ImGuiKey_A,true);frame();io.AddKeyEvent(ImGuiKey_A,false);io.AddKeyEvent(ImGuiMod_Ctrl,false);frame();io.AddInputCharactersUTF8("Nova Preview");frame();fprintf(stderr,"text value: %s\n",static_cast<Sample*>(selected)->text.get().c_str());assert(static_cast<Sample*>(selected)->text.get()=="Nova Preview");
    click(control("Bind",7));settle();Save("bind",1200,1000);assert(popup());dismiss();
    // Multiple property-less groups must own separate windows and popup IDs.
    assert(row("Sneaking")!=row("Timing"));
    click(control("Sneaking",10));settle();assert(row("Sneak option"));
    click(control("Sneak option"));dismiss();
    click(control("Timing",10));settle();assert(row("Timing option"));dismiss();
    reopenModule=false;dismiss();Save("modules",1200,1000);
    auto* ab=static_cast<Properties::BoolProperty*>(manager.Find("AutoBlock")->enabledProperty());
    const bool previousAb=ab->get();
    click(control("AutoBlock",38));settle();assert(ab->get()!=previousAb);
    click(control("KillAura",70),1);settle();assert(row("Hide in ArrayList"));
    click(control("Hide in ArrayList"));assert(selected->hiddenInArrayList());dismiss();
    click({base.x+255*scale,base.y+29*scale});settle();Save("config",1200,1000);assert(popup());dismiss();
    click({base.x+80*scale,base.y+550*scale});settle();Save("account",1200,1000);assert(popup());dismiss();
    const ImVec2 searchButton=base+ImVec2(719*scale,28*scale);
    click(searchButton);settle();assert(gui.isSearching());Save("search-hint",1200,1000);
    io.AddInputCharactersUTF8("aUtObLoCk");frame();settle();
    assert(gui.search()=="aUtObLoCk");Save("search",1200,1000);
    assert(row("AutoBlock"));
    click(control("AutoBlock",10));settle();assert(row("Mode"));dismiss();
    // Filtering keeps the actual module toggles and their own settings available.
    assert(gui.search()=="aUtObLoCk");
    click(searchButton);settle();assert(gui.search().empty());
    click(searchButton);settle(3);
    io.AddInputCharactersUTF8("KillAura");frame();assert(gui.search()=="KillAura");
    // Clicking the already selected category must also leave search mode.
    click(base+ImVec2(80*scale,99*scale));settle();
    assert(!gui.isSearching()&&gui.search().empty());
    click(searchButton);click(searchButton);settle();
    assert(!gui.isSearching()&&gui.search().empty());
    for(int page:{1,2,3,4,nova::menu::panel::PageLayout::ClientPage}){gui.layout().changePage(page);settle();assert(gui.layout().page()==page);}
    gui.layout().changePage(0);settle();Save("verified",1200,1000);
    gui.setSearch("KeepSprint");settle();click(control("KeepSprint",10));settle();
    auto at=control("Normal Hit Sprint");io.AddMousePosEvent(at.x,at.y);settle();
    for(const char* label:{"Bind","Normal Hit Sprint"}) {
        const auto* w=row(label);
        for(const auto& vertex:w->DrawList->VtxBuffer)
            if((vertex.col&IM_COL32_A_MASK)!=0)assert(vertex.pos.x<=w->ClipRect.Max.x+.5f);
    }
    Save("clipping",1200,1000);
    dismiss();gui.setSearch({});settle();
    // The real popup path must sample the published backdrop, including nested settings.
    GLuint backdrop=0;glGenTextures(1,&backdrop);glBindTexture(GL_TEXTURE_2D,backdrop);
    const unsigned char pixels[]={42,55,70,255};
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    nova::ui::blur::Publish(static_cast<ImTextureID>(backdrop));
    for(auto& setting:settings.rows())if(std::string_view(setting.getName())=="Blur")
        static_cast<Properties::BoolProperty*>(setting.getProp())->set(true);
    auto hasBackdrop=[&](ImGuiWindow* window){for(const auto& cmd:window->DrawList->CmdBuffer)if(cmd.ElemCount&&cmd.GetTexID()==static_cast<ImTextureID>(backdrop))return true;return false;};
    click(control("KillAura",10));settle();assert(hasBackdrop(popup()));
    click(control("Players",10));settle();assert(hasBackdrop(popup()));Save("popup-blur",1200,1000);
    dismiss();settle(60);nova::ui::blur::Publish(0);glDeleteTextures(1,&backdrop);
    for(auto& setting:settings.rows())if(std::string_view(setting.getName())=="Blur")
        static_cast<Properties::BoolProperty*>(setting.getProp())->set(false);
    gui.layout().changePage(nova::menu::panel::PageLayout::ClientPage);settle();
    assert(settings.clientName()=="NOVA");
    click(control("Custom Client Name"));settle();
    assert(settings.clientName()=="NOVA"); // Enabled but empty still uses the default.
    click(control("Client Name",70));io.AddInputCharactersUTF8("IRIDIUM");frame();
    assert(settings.clientName()=="IRIDIUM");Save("custom-name",1200,1000);
    click(control("Custom Client Name"));settle();assert(settings.clientName()=="NOVA");
    click(control("Custom Client Name"));settle();assert(settings.clientName()=="IRIDIUM");
    for(auto& setting:settings.rows())if(std::string_view(setting.getName())=="Client Name") {
        auto* name=static_cast<Properties::StringProperty*>(setting.getProp());
        name->set("   \t");assert(settings.clientName()=="NOVA");
        name->set("A Long Custom Client Name");settle();Save("custom-name-long",1200,1000);
        name->set("");assert(settings.clientName()=="NOVA");
    }
    // Hidden scrollbars must not disable wheel scrolling on long search results.
    gui.layout().changePage(0);gui.setSearch("Module");settle();
    ImGuiWindow* column=nullptr;
    for(auto* w:GImGui->Windows)if(w->Active && std::string_view(w->Name).find("/module-column_")!=std::string_view::npos && w->ScrollMax.y>0)column=w;
    assert(column && !column->ScrollbarY && !column->ScrollbarX);
    io.AddMousePosEvent(column->Pos.x+40*scale,column->Pos.y+40*scale);settle();
    io.AddMouseWheelEvent(0,-3);settle();assert(column->Scroll.y>0);
    gui.setSearch({});settle();
    // Exercise context recreation with the same production controller, as fullscreen toggles can do.
    ImGui_ImplOpenGL3_Shutdown();ImGui::DestroyContext();ResetFatalMenuFonts();
    ImGui::CreateContext();ImGui::GetIO().IniFilename=nullptr;ImGui::GetIO().DisplaySize={1200,1000};ImGui::GetIO().DeltaTime=1.f/60;
    InitializeFatalMenuFonts();assert(ImGui_ImplOpenGL3_Init());
    for(int i=0;i<60;++i){ImGui_ImplOpenGL3_NewFrame();ImGui::NewFrame();gui.render();ImGui::Render();ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());}
    ImGui_ImplOpenGL3_Shutdown();ImGui::DestroyContext();ResetFatalMenuFonts();wglMakeCurrent(nullptr,nullptr);wglDeleteContext(rc);ReleaseDC(window,dc);DestroyWindow(window);
    puts("PASS: settings bindings, popup close motion, navigation, sources, search, scaling and context recreation");
}
