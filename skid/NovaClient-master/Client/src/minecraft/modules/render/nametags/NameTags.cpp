#include "imgui.h"
#include "ui/hud/nametags/NameTagView.h"
#include "ui/hud/nametags/Appearance.h"
#include <algorithm>
#include <cmath>
#include <format>
#include <memory>
#include <string>
#include <vector>

#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/modules/render/nametags/ClassicNameTags.h"
#include "minecraft/modules/misc/antibot/AntiBot.h"
#include "minecraft/compat/minecraft263/Minecraft263Access.h"
#include "minecraft/compat/lunar1710/Access.h"
#include "minecraft/java/api/game_teams.h"
#include "minecraft/java/api/game_chest.h"
#include "minecraft/java/api/game_entity.h"
#include "minecraft/java/api/game_presentation_name.h"
#include "minecraft/java/api/game_rotation.h"
#include "third_party/fatalwidgets/imgui/imgui.h"
#include "minecraft/host/render/WorldProjection.h"
#include "ui/text/ImTextRenderer.h"
#include "utils/Color.h"

// Default retains Nova's rounded HUD pills; Classic follows Rise's flat pixel-font labels.

namespace nova::module {
namespace {

using minecraft::projection::CameraBasis;
using minecraft::projection::MakeBasis;
using minecraft::projection::Project;
using minecraft::projection::ScreenPoint;
using rotation::Vec3;

namespace access = compat::mc263::access;
struct ClassicMapping {
    access::Member options{}, perspective{}, scaled{}, window{}, scale{}, width{}, height{}, sneaking{};
    access::Member displayName{}, flat{}, text{}, style{}, color{}, rgb{}, bold{}, italic{}, strike{}, underline{};
};
#define NOVA_NAMETAGS_MAPPING
#define NOVA_NAMETAGS(id, owner, name, desc) .id = {owner, name, desc},
constexpr ClassicMapping lunarClassic{
#include "minecraft/compat/mappings/Lunar189.inc"
};
constexpr ClassicMapping forgeClassic{
#include "minecraft/compat/mappings/forge1201.inc"
};
constexpr ClassicMapping modernClassic{
#include "minecraft/compat/mappings/minecraft263.inc"
};
#undef NOVA_NAMETAGS
#undef NOVA_NAMETAGS_MAPPING
const ClassicMapping& ClassicNames() { return compat::IsLegacyLunar() ? lunarClassic : compat::UsesModernRuntime() ? modernClassic : forgeClassic; }

// Modern getString() drops styles. Flatten inherited Component styles locally,
// leaving the shared EntityHandle and Default mode's name rendering unchanged.
bool ReadClassicName(const java::EntityHandle& entity, std::string& out) {
    if (compat::IsLegacyLunar()) return entity.DisplayName(out);
    access::Frame f;
    if (!f) return false;
    const auto& m = ClassicNames();
    auto component = f.call(static_cast<jobject>(entity.raw()), m.displayName);
    auto list = f.call(component, m.flat);
    if (!list || !f) return false;
    const int count = f.call<int>(list, {"java.util.List", "size", "()I"});
    if (count < 0 || count > 1024) return false;
    std::string result;
    for (int i = 0; i < count && f; ++i) {
        access::Frame part(f.env);
        auto node = part.call(list, {"java.util.List", "get", "(I)Ljava/lang/Object;"}, i);
        auto style = part.call(node, m.style);
        auto color = part.call(style, m.color);
        result += "\xC2\xA7" "r";
        if (color) result += std::format("\xC2\xA7#{:06x}", part.call<int>(color, m.rgb) & 0xffffff);
        if (part.call<bool>(style, m.bold)) result += "\xC2\xA7" "l";
        if (part.call<bool>(style, m.italic)) result += "\xC2\xA7" "o";
        if (part.call<bool>(style, m.strike)) result += "\xC2\xA7" "m";
        if (part.call<bool>(style, m.underline)) result += "\xC2\xA7" "n";
        result += part.text(part.call(node, m.text));
        if (!part) return false;
    }
    if (!f || result.empty()) return false;
    out = std::move(result);
    return true;
}

// How far above the entity's own top the row sits. The original's half block, and
// it is what keeps the tag clear of the head instead of across it.
constexpr double kHeadroom = 0.5;

[[nodiscard]] std::string Hundredths(double value) { return std::format("{:.2f}", value); }

[[nodiscard]] std::string WholeNumber(double value) { return std::format("{:.0f}", value); }

struct Tag {
    std::string name;
    std::string health;

    // Both ends of the tick, because the frame is drawn between them.
    rotation::Vec3 position;
    rotation::Vec3 previous;

    double height = 0.0;
    double distance = 0.0;
    bool team = false;
    bool valid = false;
    double width = 0.0;
    bool sneaking = false;
    std::string classicLabel;
};

class NameTagsModule final : public Module {
public:
    NameTagsModule() : Module("NameTags", Category::Render, "General", "Overlay") {
        enableOverlay();
        mode_ = &addMode("Mode", 0, {"Default", "Classic"});
        auto classic = [this] { return IsClassic(); };
        auto original = [this] { return !IsClassic(); };

        // The original scales its pills from the same setting, over the range its
        // own text is drawn at. Here the unit is the hud's own text size, so one is
        // a nametag the same size as everything else the client draws.
        scale_ = &addNumber("Scale", 0.5, 2.0, 0.05, 0.9, {}, Hundredths);
        limitRange_ = &addBool("Limit range", false);
        range_ = &addNumber("Range", 8, 512, 1, 48, {}, WholeNumber,
            [this] { return limitRange_->get(); });
        players_ = &addBool("Players", true);
        mobs_ = &addBool("Mobs", false);
        status_ = &addBool("Status", true, {}, original);
        health_ = &addBool("Health", true, {}, original);
        invisibles_ = &addBool("Invisibles", false, {}, classic);
        animals_ = &addBool("Animals", false, {}, classic);
        teammates_ = &addBool("Player Teammates", true, {}, classic);
        teamTag_ = &addBool("Show Team Tag", false, {}, classic);
        targetTag_ = &addBool("Show Target Tag", false, {Properties::PropDesc("Names", &targetNames_)}, classic);
        friendTag_ = &addBool("Show Friend Tag", false, {Properties::PropDesc("Names", &friendNames_)}, classic);
        shortened_ = &addBool("Shortened Tags", false, {}, classic);

        configRows_ = settings();
        configRows_.emplace_back("Default Appearance",nullptr,defaultAppearance_.ConfigRows());
        configRows_.emplace_back("Classic Appearance",nullptr,classicAppearance_.ConfigRows());
        addSettings(defaultAppearance_.Rows(original));
        addSettings(classicAppearance_.Rows(classic));
    }

    const std::vector<Properties::PropDesc>& configSettings() const override { return configRows_; }

private:
    bool IsClassic() const { return mode_->get() == 1; }
    void Gather(std::vector<Tag>& out, bool thirdPerson = false) {
        java::EntityHandle self = java::PlayerEntity();
        if (!self.Valid()) return;

        // Player visibility must not depend on a short-range collision query.
        // Enumerate the client's loaded players; apply a distance cap only when requested.
        std::vector<java::EntityHandle> entities;
        if (players_->get()) java::WorldPlayers(entities);
        const bool classic = IsClassic();
        if (mobs_->get() || (classic && animals_->get())) {
            std::vector<java::EntityHandle> nearby;
            if (java::WorldEntities(self, SearchArea(self), nearby)) {
                for (auto& entity : nearby) {
                    // Players already come from WorldPlayers, including those outside the mob query.
                    if (entity.IsPlayer()) continue;
                    entities.push_back(std::move(entity));
                }
            }
        }
        if (classic && thirdPerson && players_->get()) {
            entities.erase(std::remove_if(entities.begin(), entities.end(), [&](const auto& e) { return e.IsSame(self); }), entities.end());
            entities.push_back(java::PlayerEntity());
        }

        const double limit = range_->get();
        const bool wantTeam = classic ? (teamTag_->get() || !teammates_->get()) : status_->get();
        const auto selfTeam = classic && wantTeam ? java::ReadTeamInfo(self, true) : java::TeamInfo{};
        std::string selfName;
        if (classic && wantTeam && compat::IsLegacyLunar()) self.DisplayName(selfName);

        for (java::EntityHandle& entity : entities) {
            const bool isSelf = entity.IsSame(self);
            if (!entity.Valid() || (isSelf && !(classic && thirdPerson && players_->get())))
                continue;
            if (!entity.IsAlive())
                continue;

            unsigned types = 0;
            if (!java::EntityTypes(entity, types))
                continue;
            const bool player = (types & java::kTypePlayer) != 0;
            if (player) {
                if (!players_->get() || entity.IsSpectator())
                    continue;
            } else if (!mobs_->get() || (types & java::kTypeLiving) == 0) {
                if (!(classic && animals_->get() && (types & java::kTypeAnimal))) continue;
            }
            if (classic) {
                bool invisible = false;
                if (!entity.IsInvisible(invisible) || (invisible && !invisibles_->get())) continue;
                if (!isSelf && antibot::IsBot(entity)) continue;
                if ((types & java::kTypeAnimal) && !animals_->get()) continue;
            }

            double distance = 0.0;
            if (!entity.DistanceTo(self, distance) || !std::isfinite(distance) ||
                (limitRange_->get() && distance > limit))
                continue;

            Tag tag;
            tag.distance = distance;

            java::Box box;
            if (!entity.BoundingBox(box))
                continue;
            tag.height = box.maxY - box.minY;
            if (classic) {
                access::Frame f;
                const auto& m = ClassicNames();
                auto object = static_cast<jobject>(entity.raw());
                tag.width = compat::IsLegacyLunar() ? f.get<float>(object, m.width) : f.call<float>(object, m.width);
                tag.height = compat::IsLegacyLunar() ? f.get<float>(object, m.height) : f.call<float>(object, m.height);
                tag.sneaking = f.call<bool>(object, m.sneaking);
                if (!f) continue;
            }

            if (!entity.RenderPositionPair(tag.position, tag.previous))
                continue;

            if (!(classic ? ReadClassicName(entity, tag.name) : entity.DisplayName(tag.name)) || tag.name.empty())
                tag.name = "?";

            float hp = 0.0f;
            float absorption = 0.0f;
            entity.Health(hp, absorption);
            tag.health = FormatHealth(hp, absorption);

            // The original asks its own teams module; the game's allied test is what
            // that module was standing on -- same scoreboard team, either direction.
            if (wantTeam && player && self.Valid()) {
                if (classic) {
                    const auto other = java::ReadTeamInfo(entity, true);
                    tag.team = !selfTeam.name.empty() && !other.name.empty() && selfTeam.color && other.color && selfTeam.color == other.color;
                    if (compat::IsLegacyLunar()) {
                        // Rise compares the leading display-name formatting code,
                        // which may differ from the scoreboard team's own color.
                        tag.team = !selfTeam.name.empty() && !other.name.empty() &&
                            selfName.starts_with("\xC2\xA7") && tag.name.starts_with("\xC2\xA7") &&
                            selfName.size() > 2 && tag.name.size() > 2 && selfName[2] == tag.name[2];
                    }
                } else {
                    bool allied = false;
                    if (entity.IsAlliedTo(self, allied)) tag.team = allied;
                }
            }
            if (classic) {
                if (!isSelf && player && tag.team && !teammates_->get()) continue;
                tag.name=java::PresentationName(tag.name);
                std::string name;
                entity.ScoreboardName(name);
                float maxHealth = 20.f;
                entity.MaxHealth(maxHealth);
                tag.classicLabel = nametags::ClassicLabel(tag.name, hp, teamTag_->get() && tag.team,
                    targetTag_->get() && nametags::Listed(targetNames_.get(), name),
                    friendTag_->get() && nametags::Listed(friendNames_.get(), name), shortened_->get(), maxHealth);
            }

            if(!classic)tag.name=java::PresentationName(tag.name);
            tag.valid = true;
            out.push_back(std::move(tag));
        }
    }

    // The original's health string: whole hearts, with absorption appended after a
    // plus when there is any.
    [[nodiscard]] static std::string FormatHealth(float health, float absorption) {
        const int hearts = static_cast<int>(std::lround(static_cast<double>(health)));
        std::string text = std::to_string(hearts);
        if (absorption > 0.0f)
            text += "+" + std::to_string(static_cast<int>(std::lround(static_cast<double>(absorption))));
        return text;
    }

    // Mobs use the world's spatial query. Without a manual cap, cover all rendered
    // chunks, including their diagonal corners.
    [[nodiscard]] java::Box SearchArea(const java::EntityHandle& self) {
        java::Box area;
        int chunks = 0;
        const double limit = limitRange_->get() ? range_->get() :
            (java::RenderDistance(chunks) && chunks > 0 ? (static_cast<double>(chunks) + 1.0) * 16.0 : 512.0);

        rotation::Vec3 where;
        if (!self.Valid() || !self.Position(where)) {
            area.minX = area.minY = area.minZ = 0.0;
            area.maxX = area.maxY = area.maxZ = 0.0;
            return area;
        }

        area.minX = where.x - limit;
        area.minY = where.y - limit;
        area.minZ = where.z - limit;
        area.maxX = where.x + limit;
        area.maxY = where.y + limit;
        area.maxZ = where.z + limit;
        return area;
    }

    // --- the draw, on the render thread --------------------------------------

    void onRender() override {
        if (ImGui::GetCurrentContext() == nullptr || ImGui::GetFont() == nullptr)
            return;

        const ImVec2 display = ImGui::GetIO().DisplaySize;
        if (display.x <= 1.0f || display.y <= 1.0f)
            return;

        float classicScale = 1;
        bool thirdPerson = false;
        if (IsClassic() && !ReadClassicView(display, classicScale, thirdPerson)) return;
        std::vector<Tag> tags;
        Gather(tags, thirdPerson);
        if (tags.empty())
            return;
        std::ranges::stable_sort(tags, [](const Tag& a, const Tag& b) {
            return a.distance > b.distance;
        });

        const float delta = java::FrameTime();
        java::CameraView view;
        if (!java::ReadCameraView(delta, view))
            return;

        const CameraBasis basis = MakeBasis(view, delta);
        if (IsClassic()) {
            classicScale *= static_cast<float>(scale_->get());
            for (const Tag& tag : tags) DrawClassic(basis, display, tag, delta, classicScale);
        } else {
            for (const Tag& tag : tags) DrawTag(basis, display, tag, delta);
        }
    }

    bool ReadClassicView(ImVec2 display, float& scale, bool& thirdPerson) const {
        access::Frame f;
        if (!f) return false;
        const auto& m = ClassicNames();
        auto client = f.client();
        auto options = f.get(client, m.options);
        thirdPerson = compat::IsLegacyLunar() ? f.get<int>(options, m.perspective) != 0 :
            f.call<int>(f.call(options, m.perspective), {"java.lang.Enum", "ordinal", "()I"}) != 0;
        double guiScale;
        if (compat::IsLegacyLunar()) guiScale = f.call<int>(compat::IsLunar1710() ? compat::lunar1710::Scaled(f,m.scaled,client) : f.create(m.scaled, client), m.scale);
        else {
            auto window = f.call(client, m.window);
            guiScale = compat::UsesModernRuntime() ? f.call<int>(window, m.scale) : f.call<double>(window, m.scale);
        }
        java::CameraView view;
        if (!f || !java::ReadCameraView(java::FrameTime(), view) || !std::isfinite(guiScale) || guiScale <= 0) return false;
        scale = float(guiScale) * (view.windowWidth > 0 ? display.x / view.windowWidth : 1.f);
        return std::isfinite(scale) && scale > 0;
    }

    ui::nametags::Style Style() const {
        const auto& appearance=IsClassic()?classicAppearance_:defaultAppearance_;
        return {appearance.background.getColorAbgr(),appearance.text.getColorAbgr(),appearance.shadow.get(),status_->get(),health_->get()};
    }
    void DrawClassic(const CameraBasis& basis, const ImVec2& display, const Tag& tag, float delta, float scale) const {
        auto anchor = nametags::ProjectClassic(tag.position, tag.previous, tag.width, tag.height, tag.sneaking, delta,
            [&](Vec3 point, float& x, float& y) { ScreenPoint out; if (!Project(basis, point, display, out)) return false; x = out.x; y = out.y; return true; });
        if (!anchor) return;
        std::vector<ui::nametags::Glyph> label;
        nametags::WalkText(tag.classicLabel,0,[](auto){return 0.f;},
            [&](std::string_view glyph,float,float,nametags::TextStyle style) {
                // Raven uses white as the unformatted/reset text color.
                ImU32 color = style.color < 0 ? IM_COL32_WHITE : nova::text::ColorByIndex(style.color);
                if (style.rgb >= 0) color = IM_COL32((style.rgb >> 16) & 255, (style.rgb >> 8) & 255, style.rgb & 255, 255);
                // Server-provided black team/prefix colors disappear on the dark
                // nameplate. Keep other formatting colors, but ensure legibility.
                if (((color >> IM_COL32_R_SHIFT) & 255) <= 32 &&
                    ((color >> IM_COL32_G_SHIFT) & 255) <= 32 &&
                    ((color >> IM_COL32_B_SHIFT) & 255) <= 32)
                    color = IM_COL32_WHITE;
                label.push_back({std::string(glyph),color,style.bold,style.italic,style.underline,style.strike});
            });
        ui::nametags::DrawClassic({anchor->x,anchor->y},scale,label,Style());
    }

    void DrawTag(const CameraBasis& basis, const ImVec2& display,
                 const Tag& tag, float delta) const {
        const rotation::Vec3 top{
            tag.previous.x + (tag.position.x - tag.previous.x) * static_cast<double>(delta),
            tag.previous.y + (tag.position.y - tag.previous.y) * static_cast<double>(delta) +
                tag.height + kHeadroom,
            tag.previous.z + (tag.position.z - tag.previous.z) * static_cast<double>(delta),
        };

        ScreenPoint screen;
        if (!Project(basis, top, display, screen))
            return;

        ui::nametags::DrawDefault({screen.x,screen.y},static_cast<float>(scale_->get()),tag.name,tag.health,tag.team,Style());
    }

    Properties::NumberProp* scale_ = nullptr;
    Properties::ModeProperty* mode_ = nullptr;
    Properties::BoolProperty *invisibles_ = nullptr, *animals_ = nullptr, *teammates_ = nullptr;
    Properties::BoolProperty *teamTag_ = nullptr, *targetTag_ = nullptr, *friendTag_ = nullptr, *shortened_ = nullptr;
    Properties::StringProperty targetNames_, friendNames_;
    Properties::BoolProperty* limitRange_ = nullptr;
    Properties::NumberProp* range_ = nullptr;
    Properties::BoolProperty* players_ = nullptr;
    Properties::BoolProperty* mobs_ = nullptr;
    Properties::BoolProperty* status_ = nullptr;
    Properties::BoolProperty* health_ = nullptr;
    ui::nametags::Appearance defaultAppearance_,classicAppearance_;
    std::vector<Properties::PropDesc> configRows_;

};

}

std::unique_ptr<Module> MakeNameTagsModule() {
    return std::make_unique<NameTagsModule>();
}

}
