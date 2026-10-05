#include "ui/hud/playerlist/PlayerListHud.h"
#include <format>
#include "ui/clickgui/Fonts.h"
#include "ui/overlay/ClientOverlay.h"
#include "ui/overlay/HudBackground.h"
#include "ui/text/ImTextRenderer.h"

namespace nova::ui::playerlist {
namespace {
constexpr ImU32 kBackground = 0xCC331914, kText = 0xFFFFEAE5;
// Trim only at UTF-8 boundaries; the normal HUD renderer retains name formatting.
std::string Fit(std::string name, float width) {
    if (text::TextWidth(name.c_str()) <= width) return name;
    while (!name.empty()) {
        auto i = name.size()-1;
        while (i && (static_cast<unsigned char>(name[i]) & 0xC0) == 0x80) --i;
        name.resize(i);
        if (text::TextWidth((name+"...").c_str()) <= width) break;
    }
    return name+"...";
}
}

struct PlayerListHud::Impl final : overlay::ClientOverlay {
    explicit Impl(Settings& settings) : settings(settings) {}
    Settings& settings;
    Model model;
    double lastFrame = -1;
    float delta = 0;
    bool editing = false;
    const char* OverlayId() const override { return "overlays.playerlist"; }
    const char* OverlayName() const override { return "Player List"; }
    bool IsVisible() const override { return editing || !model.cards.empty(); }
    ImVec2 DefaultPosition() const override { return {.03f,.28f}; }
    float FontSize() const { return kOverlayTextSize * static_cast<float>(settings.size.get()/100.); }
    float ContentWidth() const override {
        ImGui::PushFont(Medium12,FontSize());
        const float width = ImGui::GetFontSize()*10.f;
        ImGui::PopFont(); return width;
    }
    void OnDraw(const Placement& placement, ImDrawList* draws) override {
        ImGui::PushFont(Medium12,FontSize());
        const float font = ImGui::GetFontSize(), pad = std::round(font*.26f);
        const float gap = std::round(font*.42f), rowGap = std::round(font*.32f);
        const float height = std::max(font*2.21f, text::TextHeight()*1.72f+pad*2+font*.1f);
        const float width = font*10.f, rounding = std::round(font*.42f);
        const bool right = placement.offset.x < 0, up = placement.offset.y < 0;
        const float left = right ? placement.max.x-width : placement.min.x;
        const float anchor = up ? placement.upY : placement.downY;
        const float follow = 1.f-std::exp(-10.f*delta);
        float cursor = 0;
        for (auto& card : model.cards) {
            const float reveal = Ease(card.progress);
            const float wanted = up ? -cursor-height : cursor;
            if (!card.placed) { card.y = wanted; card.placed = true; }
            else card.y += (wanted-card.y)*follow;
            cursor += (height+rowGap)*reveal;
            const float top = anchor+card.y;
            if (reveal <= .001f || top+height < 0 || top > ImGui::GetIO().DisplaySize.y) continue;
            const float revealLeft = right ? left+width*(1-reveal) : left;
            draws->PushClipRect({revealLeft,top},{revealLeft+width*reveal,top+height},true);
            hud::Background(draws,{left,top},{left+height,top+height},kBackground,rounding);
            const float info = left+height+gap;
            hud::Background(draws,{info,top},{left+width,top+height},kBackground,rounding);
            const float face = std::round(font*1.58f), inset = (height-face)*.5f;
            if (card.snapshot.portrait.texture)
                DrawPortrait(draws,card.snapshot.portrait,{left+inset,top+inset},
                    {left+inset+face,top+inset+face},IM_COL32_WHITE,font*.18f);
            else {
                const auto p = text::CenterText("?",left+height*.5f,top+height*.5f,true,true);
                text::DrawText(draws,"?",p.x,p.y,kText,settings.shadow.get());
            }
            const float teamSpace = card.snapshot.teammate ? font*.8f : 0;
            const auto name = Fit(card.snapshot.name,width-height-gap-pad*2-teamSpace);
            text::DrawText(draws,name.c_str(),info+pad,top+pad,kText,settings.shadow.get());
            if (card.snapshot.teammate) {
                // Shared-color paired dots mark a teammate without adding a loud badge.
                const float x = left+width-pad-font*.25f, y = top+pad+font*.45f;
                draws->AddCircleFilled({x-font*.22f,y},font*.1f,kText);
                draws->AddCircleFilled({x+font*.08f,y},font*.1f,kText);
            }
            const float line = text::TextHeight();
            ImGui::PushFont(Medium12,FontSize()*.72f);
            auto digitWidth = [] {
                float result = text::TextWidth(".");
                for (char c='0'; c<='9'; ++c) { const char value[]{c,0}; result=std::max(result,text::TextWidth(value)); }
                return result;
            };
            float cell = digitWidth();
            const float healthCells = static_cast<float>(std::max<std::size_t>(4,card.healthText.current.size()));
            const float distanceCells = static_cast<float>(std::max<std::size_t>(2,card.distanceText.current.size()));
            const float available = width-height-gap-pad*2-(settings.direction.get()?font*.85f:0);
            const float needed = (healthCells+distanceCells)*cell+text::TextWidth(" HP")+text::TextWidth(" m")+font*.3f;
            // Keep large health/distance values inside the existing HUD size.
            if (needed>available) {
                ImGui::PopFont(); ImGui::PushFont(Medium12,FontSize()*.72f*available/needed);
                cell=digitWidth();
            }
            const float y = top+pad+line+font*.1f;
            const float valueGap = ImGui::GetFontSize()*(.3f/.72f);
            const float distanceX = info+pad+healthCells*cell+text::TextWidth(" HP")+valueGap;
            card.healthText.Draw(draws,info+pad,y,cell,kText,settings.shadow.get());
            text::DrawText(draws," HP",info+pad+healthCells*cell,y,kText,settings.shadow.get());
            card.distanceText.Draw(draws,distanceX,y,cell,kText,settings.shadow.get());
            text::DrawText(draws," m",distanceX+distanceCells*cell,y,kText,settings.shadow.get());
            if (settings.direction.get()) {
                const float cx = left+width-pad-font*.35f, cy = y+text::TextHeight()*.5f;
                const double angle = card.angle*3.141592653589793/180.;
                auto point = [&](float x,float y) { return ImVec2(cx+float(x*std::cos(angle)-y*std::sin(angle)),
                    cy+float(x*std::sin(angle)+y*std::cos(angle))); };
                const float radius = font*.3f;
                draws->AddTriangleFilled(point(0,-radius),point(-radius*.65f,radius*.6f),point(radius*.65f,radius*.6f),kText);
            }
            ImGui::PopFont();
            draws->PopClipRect();
        }
        ImGui::PopFont();
    }
};
PlayerListHud::PlayerListHud() : impl_(std::make_unique<Impl>(settings_)) {}
PlayerListHud::~PlayerListHud() = default;
void PlayerListHud::Draw(bool enabled,bool editing,std::span<const Snapshot> entries,std::uint64_t generation,float yaw) {
    if (!ImGui::GetCurrentContext() || !ImGui::GetFont()) return;
    const double now = ImGui::GetTime();
    impl_->delta = impl_->lastFrame < 0 ? 0 : static_cast<float>(std::clamp(now-impl_->lastFrame,0.,.1));
    impl_->lastFrame = now;
    impl_->editing = enabled && editing;
    impl_->model.Update(entries,enabled,generation,now,impl_->delta,std::isfinite(yaw)?yaw:0);
    for (auto& card:impl_->model.cards) card.UpdateNumbers(impl_->delta,settings_.numberRolling.get());
    impl_->Render(impl_->editing);
}
}
