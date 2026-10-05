#include "ui/clickgui/panel/PageLayout.h"
#include "ui/clickgui/browser/BrowserPage.h"
#include "ui/clickgui/browser/BrowserIcon.h"
#include "ui/clickgui/accounts/AccountPage.h"
#include "ui/clickgui/proxy/ProxyPage.h"
#include "ui/workspace/ForegroundWorkspace.h"
#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdlib>
#include "imgui_internal.h"
#include "ui/clickgui/Fonts.h"
#include "ui/clickgui/FeatureTree.h"
#include "ui/clickgui/panel/PanelClickGui.h"
#include "ui/clickgui/panel/ClientSettings.h"
#include "ui/clickgui/panel/Widgets.h"
#include "ui/clickgui/panel/ReferenceStyle.h"

namespace nova::menu::panel {
namespace {
bool Contains(std::string_view text, std::string_view pattern) {
    return std::search(text.begin(),text.end(),pattern.begin(),pattern.end(),[](unsigned char a,unsigned char b){return std::tolower(a)==std::tolower(b);})!=text.end();
}
void Label(ImDrawList* d, ImVec2 p, ImU32 c, const char* text,float size) {
    PushText(size);d->AddText(p,c,text);PopFont();
}
}
PageLayout::PageLayout(PanelClickGui& gui):gui_(gui) {
    const char* name=std::getenv("USERNAME");userName_=name&&*name?name:"Nova User";
}
PageLayout::Page PageLayout::route(const ui::Feature& feature) {
    const auto& categories = Features().categories;
    for (size_t i = 0; i < categories.size(); ++i)
        if (categories[i].id == feature.category) return static_cast<Page>(i);
    return ClientPage;
}
std::vector<const ui::Feature*> PageLayout::choices() const {
    std::vector<const ui::Feature*> out;
    for(const auto& feature:Features().features) {
        if(!gui_.search().empty()) {if(Contains(feature.name,gui_.search()))out.push_back(&feature);}
        else if(route(feature)==page_)out.push_back(&feature);
    }
    return out;
}
void PageLayout::changePage(Page page) {
    if(page==BrowserPage&&ui::workspace::Detached()){ui::workspace::Restore();return;}
    gui_.setSearching(false);
    if(page_==page)return;
    page_=page;gui_.pageChanged();gui_.setSearch({});gui_.setSearching(false);
}
void PageLayout::navigation(ImVec2 b) {
    const auto& t=gui_.colors();auto* d=ImGui::GetWindowDrawList();
    auto nav=[&](int id,const char8_t* icon,const char* name,float y,Page page,bool selected,float indent=0.f){
        auto* d=ImGui::GetWindowDrawList();
        const auto p=b+ImVec2(Scale(7+indent),Scale(y));ImGui::PushID(id);ImGui::SetCursorScreenPos(p);
        if(ImGui::InvisibleButton("nav",{Scale(140-indent),Scale(30)}))changePage(page);
        const float r=Motion(ImGui::GetItemID()^0x7771u,selected?1.f:ImGui::IsItemHovered()?.48f:0.f);
        d->AddRectFilled(p,p+ImVec2(Scale(140-indent),Scale(30)),color::Interpolate(color::ReAlpha(t.categoryPanelBgColor,0),t.categoryHighlightColor,r),Scale(6));
        const ImU32 iconColor=color::Interpolate(IM_COL32(137,142,153,255),IM_COL32(82,141,255,255),r);
        if(page==BrowserPage)ui::browser::DrawChromeIcon(d,p+ImVec2(Scale(16.5f),Scale(15)),Scale(6.5f),iconColor);
        else {PushPanelIcon();d->AddText(p+ImVec2(Scale(10),(Scale(30)-TextHeight())*.5f),iconColor,Icon(icon));PopFont();}
        PushText(kTextBody);d->AddText(p+ImVec2(Scale(31),(Scale(30)-TextHeight())*.5f),color::Interpolate(IM_COL32(145,149,159,255),t.textHighlightColor,r),name);PopFont();
        ImGui::PopID();
    };
    Label(d,b+ImVec2(Scale(16),Scale(67)),t.secondaryColor,"MODULES",10);
    const auto& categories = Features().categories;
    std::vector<Page> order;
    for(size_t i=0;i<categories.size();++i)order.push_back(static_cast<Page>(i));
    std::stable_sort(order.begin(),order.end(),[&](Page a,Page c){return categories[a].panelOrder<categories[c].panelOrder;});
    // Preserve the five-row Minecraft layout; additional host categories scroll
    // within the navigation area instead of overlapping Client/account controls.
    const float navHeight=std::min(180.f,36.f*static_cast<float>(order.size()));
    if(order.size()<=5) {
        for(size_t i=0;i<order.size();++i) {
            const auto index=order[i];const auto& category=categories[index];
            nav(index,category.panelIcon.empty()?u8"\uf1de":category.panelIcon.c_str(),
                category.panelName.empty()?category.name.c_str():category.panelName.c_str(),
                84+36*static_cast<float>(i),index,page_==index);
        }
    } else {
        ImGui::SetCursorScreenPos(b+ImVec2(0,Scale(84)));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0,0});
        ImGui::BeginChild("category-navigation",{Scale(154),Scale(navHeight)},ImGuiChildFlags_None,ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoScrollbar);
        const float scroll=ImGui::GetScrollY()/Scale(1);
        for(size_t i=0;i<order.size();++i) {
            const auto index=order[i];const auto& category=categories[index];
            nav(index,category.panelIcon.empty()?u8"\uf1de":category.panelIcon.c_str(),
                category.panelName.empty()?category.name.c_str():category.panelName.c_str(),
                84+36*static_cast<float>(i)-scroll,index,page_==index);
        }
        ImGui::EndChild();
        ImGui::PopStyleVar();
    }
    Label(d,b+ImVec2(Scale(16),Scale(97+navHeight)),t.secondaryColor,"CLIENT",10);
    nav(-1,u8"\uf013","Settings",111+navHeight,ClientPage,page_==ClientPage);
    Label(d,b+ImVec2(Scale(16),Scale(147+navHeight)),t.secondaryColor,"BROWSER",10);
    nav(-2,u8"\uf0ac","Chrome",161+navHeight,BrowserPage,page_==BrowserPage);
    Label(d,b+ImVec2(Scale(16),Scale(197+navHeight)),t.secondaryColor,"ACCOUNTS",10);
    nav(-3,u8"\uf007","Accounts",211+navHeight,AccountsPage,page_==AccountsPage);
    Label(d,b+ImVec2(Scale(16),Scale(247+navHeight)),t.secondaryColor,"PROXY",10);
    nav(-4,u8"\uf0ac","Proxy",261+navHeight,ProxyPage,page_==ProxyPage);
}
void PageLayout::rebuild() {
    const bool client=page_==ClientPage && gui_.search().empty();
    const auto modules=client?std::vector<const ui::Feature*>{}:choices();
    if(modules==shown_ && client==builtClient_ && !groups_.empty())return;
    shown_=modules;builtClient_=client;groups_.clear();
    if(client) {
        groups_.push_back(Group{"Client"});
        for(const auto& row:ClientSettings::Get().rows())
            groups_.back().rows.push_back(std::make_unique<PropertyComponent>(gui_,row));
    } else for(auto* m:shown_) {
        const std::string name=m->hud?"HUD":m->subpage.empty()?"General":m->subpage;
        auto group=std::find_if(groups_.begin(),groups_.end(),[&](const Group& g){return g.name==name;});
        if(group==groups_.end()){groups_.push_back(Group{name});group=std::prev(groups_.end());}
        group->rows.push_back(std::make_unique<PropertyComponent>(gui_,FeatureToggleRow(*m)));
    }
    // A category may place every module in General. Split large groups so one
    // column cannot overflow while the other stays empty.
    if(!client) {
        size_t total=0;
        for(const auto& group:groups_)total+=group.rows.size();
        const size_t limit=std::max(size_t(1),(total+1)/2);
        std::vector<Group> balanced;
        size_t placed=0;
        for(auto& group:groups_) {
            for(size_t start=0;start<group.rows.size();) {
                Group part{group.name};
                part.column=placed<limit?0:1;
                const size_t count=std::min(group.rows.size()-start,part.column==0?limit-placed:group.rows.size()-start);
                for(size_t i=start;i<start+count;++i)
                    part.rows.push_back(std::move(group.rows[i]));
                balanced.push_back(std::move(part));
                start+=count;placed+=count;
            }
        }
        groups_=std::move(balanced);
    }
}
void PageLayout::card(Group& group,float width) {
    const auto& t=gui_.colors();auto* d=ImGui::GetWindowDrawList();
    const auto count=std::count_if(group.rows.begin(),group.rows.end(),[](const auto& row){return row->isVisible();});
    if(!count)return;
    const auto p=ImGui::GetCursorScreenPos()+ImVec2(0,Scale(20));
    const float height=Scale(float(count)*37);
    reference::Card(d,p,{width,height},group.name.c_str());
    ImGui::PushID(group.name.c_str());
    ImGui::SetCursorScreenPos(p+ImVec2(Scale(13),0));
    ImGui::BeginChild("card",{width-Scale(19),height},ImGuiChildFlags_None,ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
    size_t shown=0;
    for(auto& row:group.rows)if(row->isVisible()) {
        if(shown){auto at=ImGui::GetCursorScreenPos();ImGui::GetWindowDrawList()->AddLine(at,at+ImVec2(width-Scale(26),0),t.splitColor,Scale(1));}
        row->render();++shown;
    }
    ImGui::EndChild();ImGui::PopID();
    ImGui::SetCursorScreenPos({p.x,p.y+height+Scale(12)});
    ImGui::Dummy({0,0});
}
void PageLayout::content(ImVec2 b) {
    if(page_!=ProxyPage)ui::proxy::Hide();
    if(page_!=AccountsPage)ui::accounts::Hide();
    if(page_==BrowserPage){ui::browser::Draw(gui_,b);return;}
    ui::browser::Hide();
    if(page_==AccountsPage){ui::accounts::Draw(gui_,b);return;}
    if(page_==ProxyPage){ui::proxy::Draw(gui_,b);return;}
    rebuild();
    ImGui::SetCursorScreenPos(b+ImVec2(Scale(158),Scale(56)));
    ImGui::BeginChild("content",{Scale(590),Scale(520)},ImGuiChildFlags_None,ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoScrollbar);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,{0,0});
    if(groups_.empty())Label(ImGui::GetWindowDrawList(),b+ImVec2(Scale(180),Scale(88)),gui_.colors().secondaryColor,"No matching modules",14);
    const int columns=builtClient_?1:2;
    for(int column=0;column<columns;++column) {
        ImGui::PushID(column);
        ImGui::SetCursorScreenPos(b+ImVec2(Scale(167+290*column),Scale(64)));
        ImGui::BeginChild("module-column",{Scale(builtClient_?568:278),Scale(497)},ImGuiChildFlags_None,ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoScrollbar);
        // Window IDs survive page changes; reset stale scroll when their contents change.
        const auto key=ImGui::GetID("##list-generation");
        const ImGuiID generation=ImHashStr(gui_.search().c_str(),0,static_cast<ImGuiID>(page_)+1);
        auto* storage=ImGui::GetStateStorage();
        if(storage->GetInt(key)!=static_cast<int>(generation)){ImGui::SetScrollY(0);storage->SetInt(key,static_cast<int>(generation));}
        for(auto& group:groups_)if(group.column==column)card(group,ImGui::GetWindowWidth());
        ImGui::EndChild();ImGui::PopID();
    }
    ImGui::PopStyleVar();
    reference::PageReveal(gui_.pageReveal());
    ImGui::EndChild();
}
void PageLayout::account(ImVec2 b) {
    const auto& t=gui_.colors();auto* d=ImGui::GetWindowDrawList();const auto p=b+ImVec2(Scale(7),Scale(531));
    ImGui::SetCursorScreenPos(p);if(ImGui::InvisibleButton("account",{Scale(140),Scale(38)}))ImGui::OpenPopup("account-popup");
    const float r=Motion(ImGui::GetItemID()^0x1932u,ImGui::IsPopupOpen("account-popup")?1.f:ImGui::IsItemHovered()?.5f:0.f);
    d->AddRectFilled(p,p+ImVec2(Scale(140),Scale(38)),color::ReAlpha(t.categoryHighlightColor,int(235*r)),Scale(6));
    d->AddCircleFilled(p+ImVec2(Scale(21),Scale(19)),Scale(14),t.clientIconBgColor);
    const auto brand=ClientSettings::Get().clientName();
    size_t initialEnd=1;
    while(initialEnd<brand.size()&&(static_cast<unsigned char>(brand[initialEnd])&0xc0)==0x80)++initialEnd;
    const auto initial=brand.substr(0,initialEnd);
    PushText(14);d->AddText(CenterText(initial.c_str(),p.x+Scale(21),p.y+Scale(19),true,true),t.clientIconColor,initial.c_str());PopFont();
    d->PushClipRect(p,p+ImVec2(Scale(127),Scale(38)),true);
    Label(d,p+ImVec2(Scale(43),Scale(4)),t.clientNameColor,userName_.c_str(),14);
    Label(d,p+ImVec2(Scale(43),Scale(20)),t.secondaryColor,(ClientSettings::Get().clientName()+" Client").c_str(),12);d->PopClipRect();Chevron(d,p+ImVec2(Scale(132),Scale(15)),t.mainColor);
    ImGui::SetNextWindowPos(b+ImVec2(Scale(151),Scale(380)),ImGuiCond_Appearing);
    ImGui::SetNextWindowSize({Scale(218),Scale(181)});
    if(BeginPanelPopup(t,"account-popup",0,PopupMotion::Account)){
        auto pos=ImGui::GetWindowPos();auto* draw=ImGui::GetWindowDrawList();
        draw->AddCircleFilled(pos+ImVec2(Scale(35),Scale(34)),Scale(18),t.clientIconBgColor);
        Label(draw,pos+ImVec2(Scale(64),Scale(15)),t.clientNameColor,userName_.c_str(),15);
        Label(draw,pos+ImVec2(Scale(64),Scale(34)),t.iconHighlightColor,(ClientSettings::Get().clientName()+" settings").c_str(),14);
        ImGui::SetCursorPos({Scale(4),Scale(66)});
        if(Choice(t,"settings","Client settings",page_==ClientPage,Scale(210))){changePage(ClientPage);ImGui::CloseCurrentPopup();}
        if(Choice(t,"theme","Light theme",ClientSettings::Get().themeValue()==Theme::Light,Scale(210)))ClientSettings::Get().theme().set(ClientSettings::Get().themeValue()==Theme::Light?1:0);
        if(Choice(t,"scale","Menu scale",false,Scale(210))){changePage(ClientPage);ImGui::CloseCurrentPopup();}
        EndPanelPopup();
    }
}
}
