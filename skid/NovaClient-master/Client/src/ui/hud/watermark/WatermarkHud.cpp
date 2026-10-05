#include "ui/overlay/HudBackground.h"
#include "ui/hud/watermark/WatermarkHud.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include <ctime>
#include <format>
#include <memory>
#include <string>

#include "FatalWidgets.h"
#include "ui/clickgui/Fonts.h"
#include "ui/clickgui/panel/Theme.h"
#include "ui/clickgui/panel/PanelClickGui.h"
#include "ui/clickgui/panel/ClientSettings.h"
#include "ui/text/ImTextRenderer.h"
#include "ui/text/RollingText.h"
#include "utils/Color.h"

namespace nova::ui::watermark {
namespace {

float EaseOut(float value) {
    const float inverse = 1.0f - value;
    return 1.0f - inverse * inverse * inverse;
}

std::string CurrentTimeText() {
    const std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_s(&local, &now);
    return std::format("{:02d}:{:02d}:{:02d}", local.tm_hour, local.tm_min, local.tm_sec);
}

using text::RollingText;

}

struct WatermarkHud::Impl {
public:
    explicit Impl(Settings& s) {
        name_=&s.name;fps_=&s.fps;clock_=&s.clock;server_=&s.server;player_=&s.player;
        ping_=&s.ping;tps_=&s.tps;coordinates_=&s.coordinates;speed_=&s.speed;version_=&s.version;
        corner_=&s.corner;margin_=&s.margin;animation_=&s.animation;background_=&s.background;textColor_=&s.text;
    }

    static std::string ShortText(std::string value, std::size_t limit) {
        for (char& c : value) if (static_cast<unsigned char>(c) < 32) c = ' ';
        if (value.size() <= limit) return value;
        while (limit > 0 && (static_cast<unsigned char>(value[limit]) & 0xC0) == 0x80) --limit;
        return value.substr(0, limit) + "...";
    }

    static float DigitWidth() {
        float width = text::TextWidth(":");
        for (char digit = '0'; digit <= '9'; ++digit) {
            const char value[]{digit, 0};
            width = std::max(width, text::TextWidth(value));
        }
        return width;
    }

    // Match ClickGUI's ExtraBold wordmark, tracking and two-second color split.
    static float WordmarkWidth(const char* value) {
        const float tracking = ImGui::GetFontSize() * (1.2f / 28.0f);
        float width = 0;
        for (const char* glyph = value; *glyph;) {
            const char* end=glyph+1;
            while((static_cast<unsigned char>(*end)&0xc0)==0x80)++end;
            width += ImGui::CalcTextSize(glyph,end).x + tracking;
            glyph=end;
        }
        return std::max(0.0f, width - tracking);
    }

    static ImVec2 WordmarkSize(const char* value) {
        return ImVec2(WordmarkWidth(value), ImGui::GetTextLineHeight());
    }

    static void DrawWordmark(ImDrawList* draws, const char* value, ImVec2 pos, ImU32 color) {
        const float tracking = ImGui::GetFontSize() * (1.2f / 28.0f);
        for (const char* glyph = value; *glyph;) {
            const char* end=glyph+1;
            while((static_cast<unsigned char>(*end)&0xc0)==0x80)++end;
            draws->AddText(nullptr,0,pos,color,glyph,end);
            pos.x += ImGui::CalcTextSize(glyph,end).x + tracking;
            glyph=end;
        }
    }

    static void DrawLogo(ImDrawList* draws, float x, float centerY) {
        ImGui::PushFont(ExtraBold20, 22.0f);
        static FatalWidgets::TextRenderer renderer{
            &DrawWordmark, &WordmarkSize, &WordmarkWidth, &ImGui::GetTextLineHeight};
        const auto& theme = menu::panel::ThemeColors(menu::panel::Theme::Dark);
        const auto name=menu::panel::ClientSettings::Get().clientName();
        FatalWidgets::Rendering::DrawFatalTitle(name.c_str(),
            ImVec2(x, centerY - ImGui::GetTextLineHeight() * 0.5f),
            std::fmod(static_cast<float>(ImGui::GetTime()) / 2.0f, 1.0f),
            ImGui::GetFontSize() * (1.5f / 28.0f),
            theme.clientNameColor, theme.iconHighlightColor, theme.clientIconBgColor, draws, &renderer);
        ImGui::PopFont();
    }

    enum class InfoIcon { None, Player, Server, Frames, Ping, Ticks, Position, Speed, Version, Clock };

    static void DrawInfoIcon(ImDrawList* draws, InfoIcon icon, ImVec2 center, float scale, ImU32 color) {
        const float stroke = 1.5f * scale;
        const auto point = [&](float x, float y) { return ImVec2(center.x + x * scale, center.y + y * scale); };
        const auto line = [&](float x1, float y1, float x2, float y2) {
            draws->AddLine(point(x1, y1), point(x2, y2), color, stroke);
        };
        switch (icon) {
        case InfoIcon::Player:
            draws->AddCircle(point(0, -3.5f), 2.5f * scale, color, 16, stroke);
            draws->PathArcTo(point(0, 6), 5 * scale, 3.14159265f, 6.2831853f, 16);
            draws->PathStroke(color, ImDrawFlags_None, stroke);
            break;
        case InfoIcon::Server:
            for (float y : {-5.5f, 1.0f}) {
                draws->AddRect(point(-6, y), point(6, y + 4.5f), color, scale, 0, stroke);
                draws->AddCircleFilled(point(-3.5f, y + 2.25f), 0.8f * scale, color);
                line(0, y + 2.25f, 3.5f, y + 2.25f);
            }
            break;
        case InfoIcon::Frames:
            draws->AddRect(point(-6, -5), point(6, 3), color, 1.2f * scale, 0, stroke);
            line(0, 3, 0, 6); line(-3, 6, 3, 6);
            break;
        case InfoIcon::Ping:
            for (int i = 0; i < 3; ++i) {
                const float x = -5.0f + i * 4;
                draws->AddRectFilled(point(x, 2.0f - i * 4), point(x + 2, 6), color, 0.7f * scale);
            }
            break;
        case InfoIcon::Ticks:
            line(-7, 0, -4, 0); line(-4, 0, -2, -5);
            line(-2, -5, 1, 5); line(1, 5, 3, 0); line(3, 0, 7, 0);
            break;
        case InfoIcon::Position:
            draws->AddCircle(center, 4 * scale, color, 20, stroke);
            draws->AddCircleFilled(center, scale, color);
            line(-7, 0, -4, 0); line(4, 0, 7, 0);
            line(0, -7, 0, -4); line(0, 4, 0, 7);
            break;
        case InfoIcon::Speed: {
            const ImVec2 bolt[]{point(1, -7), point(-5, 1), point(0, 1),
                point(-1, 7), point(5, -1), point(0, -1)};
            draws->AddPolyline(bolt, 6, color, ImDrawFlags_Closed, stroke);
            break;
        }
        case InfoIcon::Version: {
            const ImVec2 cube[]{point(0, -6), point(6, -3), point(6, 3),
                point(0, 6), point(-6, 3), point(-6, -3)};
            draws->AddPolyline(cube, 6, color, ImDrawFlags_Closed, stroke);
            line(-6, -3, 0, 0); line(6, -3, 0, 0); line(0, 0, 0, 6);
            break;
        }
        case InfoIcon::Clock:
            draws->AddCircle(center, 6 * scale, color, 24, stroke);
            line(0, -3.5f, 0, 0); line(0, 0, 3, 1.5f);
            break;
        case InfoIcon::None: break;
        }
    }

    void Draw(bool enabled,const Snapshot& data) {
        if (!ImGui::GetCurrentContext() || !ImGui::GetFont()) return;
        const float delta = std::clamp(ImGui::GetIO().DeltaTime, 0.0f, 0.1f);
        const bool fresh = progress_ <= 0;
        const float duration = std::max(0.1f, static_cast<float>(animation_->get()) / 1000.0f);
        const auto hasText = [&](const std::string& value) {
            return !data.hideUnavailable || (!value.empty() && value != "--");
        };
        const bool showPlayer = player_->get() && hasText(data.player);
        const bool showServer = server_->get() && hasText(data.server);
        const bool showPing = ping_->get() && (!data.hideUnavailable || data.ping >= 0);
        const bool showTps = tps_->get() && (!data.hideUnavailable || (std::isfinite(data.tps) && data.tps >= 0));
        const bool showCoordinates = coordinates_->get() && hasText(data.coordinates);
        const bool showSpeed = speed_->get() && (!data.hideUnavailable || (std::isfinite(data.speed) && data.speed >= 0));
        const bool showVersion = version_->get() && !data.version.empty();
        const bool visible = enabled && (name_->get() || fps_->get() || clock_->get() || showServer ||
            showPlayer || showPing || showTps || showCoordinates || showSpeed || showVersion);
        progress_ = std::clamp(progress_ + (visible ? delta : -delta) / duration, 0.0f, 1.0f);
        if (progress_ <= 0) return;

        const double now = ImGui::GetTime();
        if (fresh || now >= nextFpsSample_) {
            sampledFps_ = std::clamp(static_cast<int>(std::lround(ImGui::GetIO().Framerate)), 0, 99999);
            nextFpsSample_ = now + 0.5;
        }
        fpsText_.Update(std::format("{:>3d}", sampledFps_), delta, fresh);
        clockText_.Update(CurrentTimeText(), delta, fresh);

        ImGui::PushFont(Medium12, kOverlayTextSize);
        const float scale = ImGui::GetFontSize() / kOverlayTextSize;
        const float rowHeight = 34.0f * scale;
        const float cell = DigitWidth();
        const float line = text::TextHeight();
        ImGui::PushFont(ExtraBold20, 22.0f);
        const float nameWidth = WordmarkWidth(menu::panel::ClientSettings::Get().clientName().c_str());
        ImGui::PopFont();
        const double tps=data.tps;
        pingText_.Update(data.ping >= 0 ? std::format("{}", data.ping) : "--", delta, fresh);
        tpsText_.Update(tps >= 0 ? std::format("{:.1f}", tps) : "--", delta, fresh);
        speedText_.Update(data.speed >= 0 ? std::format("{:.1f}", data.speed) : "--", delta, fresh);
        struct Item {
            std::string value, label;
            RollingText* rolling = nullptr;
            bool logo = false;
            InfoIcon icon = InfoIcon::None;
            float width = 0, x = 0;
            int row = 0;
        };
        std::vector<Item> items;
        if (name_->get()) items.push_back({{}, {}, nullptr, true});
        if (showPlayer) items.push_back({ShortText(data.player, 24), {}, nullptr, false, InfoIcon::Player});
        if (showServer) items.push_back({ShortText(data.server, 40), {}, nullptr, false, InfoIcon::Server});
        if (fps_->get()) items.push_back({{}, "FPS", &fpsText_, false, InfoIcon::Frames});
        if (showPing) items.push_back({{}, "ms", &pingText_, false, InfoIcon::Ping});
        if (showTps) items.push_back({{}, "TPS", &tpsText_, false, InfoIcon::Ticks});
        if (showCoordinates) items.push_back({data.coordinates, {}, nullptr, false, InfoIcon::Position});
        if (showSpeed) items.push_back({{}, data.speedUnit, &speedText_, false, InfoIcon::Speed});
        if (showVersion) items.push_back({data.version, {}, nullptr, false, InfoIcon::Version});
        if (clock_->get()) items.push_back({{}, {}, &clockText_, false, InfoIcon::Clock});
        const float iconSize = 14 * scale;
        const float iconInset = iconSize + 6 * scale;
        const float gap = 20 * scale;
        const float margin = static_cast<float>(margin_->get()) * scale;
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        const float maxWidth = std::max(80.0f * scale, viewport->WorkSize.x - margin * 2);
        float targetWidth = 24 * scale, cursor = 12 * scale;
        int row = 0;
        for (auto& item : items) {
            item.width = item.logo ? nameWidth : item.rolling ? cell * item.rolling->current.size() : text::TextWidth(item.value.c_str());
            if (item.icon != InfoIcon::None) item.width += iconInset;
            if (!item.label.empty()) item.width += 5 * scale + text::TextWidth(item.label.c_str());
            if (cursor > 12 * scale && cursor + item.width + 12 * scale > maxWidth) {
                ++row; cursor = 12 * scale;
            }
            item.x = cursor; item.row = row;
            targetWidth = std::max(targetWidth, cursor + item.width + 12 * scale);
            cursor += item.width + gap;
        }
        const float targetHeight = rowHeight * (row + 1);
        if (fresh || lastScale_ != scale) height_ = targetHeight;
        else height_ += (targetHeight - height_) * (1 - std::exp(-12 * delta));
        const float height = height_;
        if (fresh || lastScale_ != scale) width_ = targetWidth;
        else width_ += (targetWidth - width_) * (1 - std::exp(-12 * delta));
        lastScale_ = scale;

        const bool right = corner_->get() == 1 || corner_->get() == 3;
        const bool bottom = corner_->get() >= 2;
        const float left = std::max(viewport->WorkPos.x, right
            ? viewport->WorkPos.x + viewport->WorkSize.x - margin - width_
            : viewport->WorkPos.x + margin);
        const float top = std::max(viewport->WorkPos.y, bottom
            ? viewport->WorkPos.y + viewport->WorkSize.y - margin - height
            : viewport->WorkPos.y + margin);
        const float revealed = width_ * EaseOut(progress_);
        if (revealed < 0.5f) { ImGui::PopFont(); return; }
        const float revealLeft = right ? left + width_ - revealed : left;
        const ImVec2 min(revealLeft, top), max(revealLeft + revealed, top + height);
        ImDrawList* draws = ImGui::GetBackgroundDrawList();
        ui::hud::Background(draws, min, max, background_->getColorAbgr(), std::min(rowHeight, revealed) * 0.5f);
        draws->PushClipRect(min, max, true);

        const ImU32 color = textColor_->getColorAbgr();
        const ImU32 muted = color::ReAlpha(color, 165);
        for (const auto& item : items) {
            const float x = left + item.x;
            const float rowTop = top + item.row * rowHeight;
            const float y = rowTop + (rowHeight - line) * 0.5f;
            if (item.x > 12 * scale) {
                draws->AddLine(ImVec2(x - gap * 0.5f, rowTop + 11 * scale),
                    ImVec2(x - gap * 0.5f, rowTop + rowHeight - 11 * scale),
                    color::ReAlpha(color, 45), scale);
            }
            if (item.logo) DrawLogo(draws, x, rowTop + rowHeight * 0.5f);
            else {
                float textX = x;
                if (item.icon != InfoIcon::None) {
                    DrawInfoIcon(draws, item.icon, ImVec2(x + iconSize * 0.5f, rowTop + rowHeight * 0.5f), scale, color);
                    textX += iconInset;
                }
                const float valueWidth = item.rolling ? cell * item.rolling->current.size() : text::TextWidth(item.value.c_str());
                if (item.rolling) item.rolling->Draw(draws, textX, y, cell, color);
                else text::DrawText(draws, item.value.c_str(), textX, y, color, false);
                if (!item.label.empty()) text::DrawText(draws, item.label.c_str(), textX + valueWidth + 5 * scale, y, muted, false);
            }
        }
        draws->PopClipRect();
        ImGui::PopFont();
    }

    Properties::BoolProperty *name_, *fps_, *clock_;
    Properties::ModeProperty* corner_;
    Properties::NumberProp *margin_, *animation_;
    Properties::ColorProp *background_, *textColor_;
    Properties::BoolProperty *server_, *player_, *ping_, *tps_, *coordinates_, *speed_, *version_;
    RollingText fpsText_, clockText_, pingText_, tpsText_, speedText_;
    float height_ = 0;
    float progress_ = 0;
    float width_ = 0;
    float lastScale_ = 0;
    int sampledFps_ = 0;
    double nextFpsSample_ = 0;
};

WatermarkHud::WatermarkHud():impl_(std::make_unique<Impl>(settings_)) {}
WatermarkHud::~WatermarkHud()=default;
void WatermarkHud::Draw(bool enabled,const Snapshot& snapshot) { impl_->Draw(enabled,snapshot); }

}
