#include "ui/hud/musichud/MusicHud.h"
#include "ui/hud/musichud/CoverTexture.h"
#include "ui/hud/musichud/SourcePicker.h"
#include "ui/hud/lyrics/LyricsOverlay.h"
#include "smtc/SmtcService.h"
#include "ui/clickgui/Fonts.h"
#include "ui/backend/blur/Blur.h"
#include "ui/overlay/ClientOverlay.h"
#include "ui/overlay/HudBackground.h"
#include "ui/text/ImTextRenderer.h"
#include "utils/Color.h"
#include <format>
#include "imgui_internal.h"

namespace nova::ui::music {
namespace {
float Ease(float t) { return 1.f - std::pow(1.f - std::clamp(t, 0.f, 1.f), 3.f); }
std::string Time(double value) { const int seconds = static_cast<int>(std::clamp(value, 0.0, 359999.0)); return std::format("{}:{:02}", seconds / 60, seconds % 60); }
}

struct MusicHud::Impl final : private overlay::ClientOverlay {
public:
    explicit Impl(MusicHudSettings& settings) : source_(settings.source),
        size_(&settings.size), lyricsSize_(&settings.lyricsSize), lyrics_(&settings.lyrics),
        lyricsShadow_(&settings.lyricsShadow), controls_(&settings.controls), hidePaused_(&settings.hidePaused) {}
    void SetEnabled(bool enabled) {
        if (enabled_ == enabled) return;
        enabled_ = enabled;
        if (enabled_) { smtc::SelectSource(source_.get()); smtc::Start(); }
        else { smtc::RequestStop(); seeking_ = false; }
    }
private:
    const char* OverlayId() const override { return "overlays.musichud"; }
    const char* OverlayName() const override { return "MusicHUD"; }
    ImVec2 DefaultPosition() const override { return ImVec2(.04f, .72f); }
    bool IsVisible() const override { return progress_ > 0; }
    float ContentWidth() const override { return 260 * scale_; }
public:
    void Draw(bool editing) {
        if (enabled_) { smtc::SelectSource(source_.get()); smtc::SetLyricsEnabled(lyrics_->get()); smtc::Start(); }
        DrawSnapshot(enabled_,editing,smtc::ReadSnapshot());
    }
    void DrawSnapshot(bool enabled,bool editing,std::shared_ptr<const smtc::MediaSnapshot> next) {
        if (!ImGui::GetCurrentContext() || !ImGui::GetFont()) return;
        const double now = ImGui::GetTime();
        const float delta = lastFrame_ < 0 ? 0.f : static_cast<float>(std::clamp(now-lastFrame_, 0.0, .1)); lastFrame_ = now;
        scale_ = static_cast<float>(size_->get() / 100.0) * ImGui::GetStyle().FontScaleMain * ImGui::GetStyle().FontScaleDpi;
        const float fit = std::max(.1f, (ImGui::GetIO().DisplaySize.x - 12) / 260.f); scale_ = std::min(scale_, fit);
        editing_ = enabled && editing;
        sources_ = next->sources;
        const bool ready = next->state == smtc::State::Ready && !next->title.empty();
        const bool visible = enabled && (editing_ || (ready && (!hidePaused_->get() || next->playing)));
        preview_ = editing_ && !ready;
        if (ready) {
            if (!current_ || next->track != current_->track || next->session != current_->session) {
                previous_ = current_; switched_ = now; transition_ = 0; seeking_ = false;
            }
            current_ = next;
        }
        transition_ = std::min(1.f, transition_ + delta / .3f);
        if (transition_ >= 1) previous_.reset();
        progress_ = std::clamp(progress_ + (visible ? delta : -delta) / .3f, 0.f, 1.f);
        expanded_ += ((editing_ && controls_->get() ? 1.f : 0.f) - expanded_) * (1.f-std::exp(-14.f*delta));
        if (!editing_) seeking_ = false;
        if (progress_ == 0) { current_.reset(); previous_.reset(); }
        music::RetainCovers(current_ && current_->artwork ? current_->artwork->version : 0,
            previous_ && previous_->artwork ? previous_->artwork->version : 0);
        lyricsOverlay_.Draw(next, lyrics_->get() && visible, editing_, static_cast<float>(lyricsSize_->get() / 100), delta, lyricsShadow_->get());
        Render(editing_);
    }
private:
    void Label(ImDrawList* draws, const std::string& value, ImVec2 at, float width, ImU32 color, bool scroll) {
        draws->PushClipRect(at, ImVec2(at.x+width, at.y+18*scale_), true);
        const float overflow = std::max(0.f, text::TextWidth(value.c_str())-width);
        float shift = 0;
        if (scroll && overflow > 0) {
            const double elapsed = std::max(0.0, ImGui::GetTime()-switched_-1.5);
            const double travel = overflow / (22*scale_);
            const double phase = std::fmod(elapsed, travel*2+2);
            shift = static_cast<float>(phase < travel ? phase*22*scale_ : phase < travel+1 ? overflow :
                phase < travel*2+1 ? overflow-(phase-travel-1)*22*scale_ : 0);
        }
        text::DrawText(draws, value.c_str(), at.x-shift, at.y, color, false);
        draws->PopClipRect();
    }
    void Artwork(ImDrawList* draws, const std::shared_ptr<const smtc::MediaSnapshot>& data, ImVec2 at, float alpha) {
        const ImVec2 max(at.x+44*scale_, at.y+44*scale_);
        if (data && music::DrawCover(draws, data->artwork, at, max, 8*scale_, alpha)) return;
        draws->AddRectFilled(at, max, IM_COL32(0,0,0,int(80*alpha)), 8*scale_);
        const ImU32 ink = IM_COL32(255,234,229,int(255*alpha));
        const ImVec2 a(at.x+19*scale_,at.y+13*scale_), b(at.x+30*scale_,at.y+10*scale_);
        draws->AddLine(a,b,ink,3*scale_); draws->AddLine(a,ImVec2(a.x,at.y+30*scale_),ink,2*scale_);
        draws->AddLine(b,ImVec2(b.x,at.y+27*scale_),ink,2*scale_);
        draws->AddCircleFilled(ImVec2(a.x-3*scale_,at.y+30*scale_),4*scale_,ink);
        draws->AddCircleFilled(ImVec2(b.x-3*scale_,at.y+27*scale_),4*scale_,ink);
    }
    void OnDraw(const Placement& placement, ImDrawList* draws) override {
        const auto display = ImGui::GetIO().DisplaySize;
        const float width = 260*scale_, height = (64+30*expanded_)*scale_;
        const float left = std::clamp(placement.pos.x-(placement.offset.x < 0 ? width : 0),0.f,std::max(0.f,display.x-width));
        const float top = std::clamp(placement.pos.y-(placement.offset.y < 0 ? height : 0),0.f,std::max(0.f,display.y-height));
        const float reveal = width*Ease(progress_);
        if (reveal < .5f) return;
        const ImVec2 min(left+(placement.offset.x < 0 ? width-reveal : 0),top), max(min.x+reveal,top+height);
        ui::hud::Background(draws,min,max,0xCC331914,std::min(12*scale_,reveal*.5f));
        draws->PushClipRect(min,max,true);
        const float fontScale = scale_ / (ImGui::GetStyle().FontScaleMain * ImGui::GetStyle().FontScaleDpi);
        ImGui::PushFont(Medium12, 13*fontScale);
        const ImU32 ink = 0xFFFFEAE5, muted = 0xBFFFEAE5;
        const float t = Ease(transition_);
        const auto data = preview_ ? nullptr : current_;
        if (previous_ && !preview_ && transition_ < 1) Artwork(draws,previous_,ImVec2(left+10*scale_,top+(10-6*t)*scale_),1-t);
        Artwork(draws,data,ImVec2(left+10*scale_,top+(10+6*(1-t))*scale_),t);
        const float textX = left+64*scale_, textWidth = width-76*scale_;
        draws->PushClipRect(ImVec2(textX,top+10*scale_),ImVec2(left+width-10*scale_,top+49*scale_),true);
        if (previous_ && !preview_ && transition_ < 1) {
            Label(draws,previous_->title,ImVec2(textX,top+(11-19*t)*scale_),textWidth,color::ReAlpha(ink,int(255*(1-t))),false);
            Label(draws,previous_->artist,ImVec2(textX,top+(30-19*t)*scale_),textWidth,color::ReAlpha(muted,int(191*(1-t))),false);
        }
        Label(draws,data ? data->title : "Nova Music",ImVec2(textX,top+(11+19*(1-t))*scale_),textWidth,color::ReAlpha(ink,int(255*t)),true);
        Label(draws,data ? (data->artist.empty() ? data->source : data->artist) : "Preview - no media session",
            ImVec2(textX,top+(30+19*(1-t))*scale_),textWidth,color::ReAlpha(muted,int(191*t)),false);
        draws->PopClipRect();
        const float barLeft = textX, barRight = left+width-12*scale_, barY = top+54*scale_;
        const bool duration = data && data->HasDuration();
        if (duration || preview_) {
            const float fraction = preview_ ? .35f : static_cast<float>(std::clamp(
                ((seeking_ ? seekTo_ : data->Position())-data->start)/(data->end-data->start),0.0,1.0));
            draws->AddRectFilled(ImVec2(barLeft,barY),ImVec2(barRight,barY+3*scale_),0x50000000,1.5f*scale_);
            if (fraction > 0) draws->AddRectFilled(ImVec2(barLeft,barY),ImVec2(barLeft+(barRight-barLeft)*fraction,barY+3*scale_),ink,1.5f*scale_);
        }
        if (expanded_ > .02f) {
            const auto time = duration ? Time((seeking_ ? seekTo_ : data->Position())-data->start)+" / "+Time(data->end-data->start) : "--:--";
            ImGui::PushFont(Medium12,10*fontScale);
            text::DrawText(draws,time.c_str(),left+10*scale_,top+73*scale_,muted,false);
            ImGui::PopFont();
        }
        if (editing_ && controls_->get() && expanded_ > .85f && progress_ > .95f)
            Input(draws,data,left,top,width,height,barLeft,barRight,barY);
        ImGui::PopFont(); draws->PopClipRect();
    }
    void Input(ImDrawList* draws, const std::shared_ptr<const smtc::MediaSnapshot>& data,
        float left,float top,float width,float height,float barLeft,float barRight,float barY) {
        ImGui::SetNextWindowPos(ImVec2(left,top)); ImGui::SetNextWindowSize(ImVec2(width,height));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(0,0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,0);
        ImGui::Begin("##MusicHUDControls",nullptr,ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoDecoration|
            ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoFocusOnAppearing|ImGuiWindowFlags_NoNav|ImGuiWindowFlags_NoBringToFrontOnFocus);
        ImGui::BringWindowToDisplayBack(ImGui::GetCurrentWindow());
        auto hit = [&](const char* id,float x,float y,float w,float h) {
            ImGui::SetCursorScreenPos(ImVec2(x,y)); return ImGui::InvisibleButton(id,ImVec2(w,h));
        };
        auto command = [&](smtc::Action action) { if(data) smtc::Enqueue({action,data->session,data->track,0}); };
        const float center = left+width-64*scale_, y = top+77*scale_;
        for (int i = 0; i < 3; ++i) {
            const bool enabled = data && (i == 0 ? data->canPrevious : i == 2 ? data->canNext :
                (data->canToggle || (data->playing ? data->canPause : data->canPlay)));
            const float x = center+(i-1)*25*scale_;
            const bool clicked = hit(i == 0 ? "prev" : i == 1 ? "toggle" : "next",x-10*scale_,y-10*scale_,20*scale_,20*scale_);
            const float target = enabled && ImGui::IsItemHovered() ? (ImGui::IsItemActive() ? 1.f : .65f) : 0.f;
            buttonHover_[i] += (target - buttonHover_[i]) * (1.f - std::exp(-18.f * ImGui::GetIO().DeltaTime));
            draws->AddRectFilled(ImVec2(x-10*scale_,y-10*scale_),ImVec2(x+10*scale_,y+10*scale_),
                color::ReAlpha(0xFFFFEAE5,static_cast<int>(38*buttonHover_[i])),6*scale_);
            const ImU32 ink = enabled ? (ImGui::IsItemHovered() ? 0xFFFFFFFF : 0xFFFFEAE5) : 0x60FFEAE5;
            if (i == 1 && data && data->playing) {
                draws->AddRectFilled(ImVec2(x-4*scale_,y-5*scale_),ImVec2(x-1*scale_,y+5*scale_),ink,scale_);
                draws->AddRectFilled(ImVec2(x+2*scale_,y-5*scale_),ImVec2(x+5*scale_,y+5*scale_),ink,scale_);
            } else {
                const float dir = i == 0 ? -1.f : 1.f;
                draws->AddTriangleFilled(ImVec2(x+dir*5*scale_,y),ImVec2(x-dir*4*scale_,y-5*scale_),ImVec2(x-dir*4*scale_,y+5*scale_),ink);
                if (i != 1) draws->AddLine(ImVec2(x+dir*7*scale_,y-5*scale_),ImVec2(x+dir*7*scale_,y+5*scale_),ink,2*scale_);
            }
            if (clicked && enabled) command(i == 0 ? smtc::Action::Previous : i == 1 ? smtc::Action::Toggle : smtc::Action::Next);
        }
        if (data && data->canSeek && data->HasDuration()) {
            hit("seek",barLeft,barY-4*scale_,barRight-barLeft,11*scale_);
            if (ImGui::IsItemActivated()) seeking_ = true;
            if (ImGui::IsItemActive()) seekTo_ = data->start+std::clamp(double((ImGui::GetIO().MousePos.x-barLeft)/(barRight-barLeft)),0.0,1.0)*(data->end-data->start);
            if (ImGui::IsItemDeactivated() && seeking_) {
                smtc::Enqueue({smtc::Action::Seek,data->session,data->track,seekTo_}); seeking_ = false;
            }
        }
        const float sourceX = left+width-14*scale_;
        if (hit("source",sourceX-10*scale_,y-10*scale_,20*scale_,20*scale_)) ImGui::OpenPopup("music-source");
        const float sourceTarget = ImGui::IsItemHovered() || ImGui::IsPopupOpen("music-source") ? 1.f : 0.f;
        buttonHover_[3] += (sourceTarget-buttonHover_[3])*(1.f-std::exp(-18.f*ImGui::GetIO().DeltaTime));
        draws->AddRectFilled(ImVec2(sourceX-10*scale_,y-10*scale_),ImVec2(sourceX+10*scale_,y+10*scale_),
            color::ReAlpha(0xFFFFEAE5,static_cast<int>(38*buttonHover_[3])),6*scale_);
        for (int i = -1; i <= 1; ++i) draws->AddCircleFilled(ImVec2(sourceX+i*4*scale_,y),1.2f*scale_,0xFFFFEAE5);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Media source");
        auto selected = source_.get();
        ui::music::DrawSourcePicker(ImVec2(left+width,top+height+6*scale_),scale_,selected,sources_);
        if (selected != source_.get()) source_.set(std::move(selected));
        ImGui::End(); ImGui::PopStyleVar(2);
    }
    SourceProperty& source_;
    Properties::NumberProp* size_{};
    Properties::NumberProp* lyricsSize_{};
    Properties::BoolProperty* lyrics_{};
    Properties::BoolProperty* lyricsShadow_{};
    music::LyricsOverlay lyricsOverlay_;
    Properties::BoolProperty *controls_{}, *hidePaused_{};
    std::shared_ptr<const smtc::MediaSnapshot> current_, previous_;
    std::vector<smtc::Source> sources_;
    float scale_ = 1, progress_{}, expanded_{}, transition_ = 1;
    float buttonHover_[4]{};
    double lastFrame_ = -1, switched_{}, seekTo_{};
    bool enabled_{}, editing_{}, preview_{}, seeking_{};
};

MusicHud::MusicHud() : impl_(std::make_unique<Impl>(settings_)) {}
MusicHud::~MusicHud() { impl_->SetEnabled(false); }
void MusicHud::SetEnabled(bool enabled) { impl_->SetEnabled(enabled); }
void MusicHud::Draw(bool editing) { impl_->Draw(editing); }
void MusicHud::DrawPreview() { impl_->DrawSnapshot(true,true,std::make_shared<smtc::MediaSnapshot>()); }
}
