#include "cs2/modules/render/esp2d/Esp2D.h"
#include "cs2/modules/render/esp2d/Projection.h"
#include "cs2/modules/ModuleManager.h"
#include "cs2/modules/misc/team/Team.h"
#include "ui/notifications/Notifications.h"
#include "imgui.h"

namespace nova::cs2::modules {
namespace {
class Esp2D final : public Module {
public:
    Esp2D() : Module("2DESP", "render", "General", true) {
        box_ = &addMode("Box", 0, {"Normal", "CS", "None"});
        outline_ = &addBool("Outline", true);
        health_ = &addMode("Health Bar Mode", 0, {"Health", "Standard", "Gradient", "None"});
        armor_ = &addBool("Armor Bar", false);
        addSettings({{"Name", &names_, {{"Text Shadow", &shadow_}}},
            {"Targets", nullptr, {{"Player", &players_}}}});
    }
private:
    void onEnable() override {
        if (!sdk::players::Available()) {
            notifications::Post(sdk::players::Status(), notifications::Type::Error);
            setEnabled(false);
        }
    }
    void onRender() override {
        if (!players_.get() || !ImGui::GetCurrentContext()) return;
        const auto display = ImGui::GetIO().DisplaySize;
        sdk::players::Snapshot snapshot;
        if (!sdk::players::Capture(snapshot)) return;
        // Farthest first: a nearby player's frame and text remain on top when
        // players overlap. Camera depth is sufficient for the visible overlay.
        const auto depth = [&](const sdk::players::Player& p) {
            return double(snapshot.matrix[3][0]) * p.origin.x + double(snapshot.matrix[3][1]) * p.origin.y + double(snapshot.matrix[3][2]) * p.origin.z;
        };
        std::sort(snapshot.players.begin(), snapshot.players.end(), [&](const auto& a, const auto& b) {
            const auto da = depth(a), db = depth(b);
            return da != db ? da > db : a.handle < b.handle;
        });
        // Minecraft defines the shared half-pixel strokes in GUI units. CS2
        // has no Minecraft GUI scale; use two GUI pixels at 1080p, scaling with
        // the viewport rather than with the independent ClickGUI window size.
        const float scale = std::clamp(display.y / 540.f, 1.f, 4.f);
        const auto* teams = dynamic_cast<const Team*>(ModuleManager::Get().Find("Team"));
        for (const auto& player : snapshot.players) {
            if (teams && teams->ShouldIgnore(snapshot.localTeam, player.team)) continue;
            auto bounds = esp2d::Project(player, snapshot.matrix, display.x, display.y);
            if (!bounds) continue;
            bounds->left /= scale; bounds->right /= scale; bounds->top /= scale; bounds->bottom /= scale;
            ui::esp2d::Draw(*bounds, scale, ui::esp2d::BoxMode(box_->get()), ui::esp2d::HealthMode(health_->get()),
                float(player.health), float(player.maximum), float(player.armor), armor_->get(),
                ui::esp2d::RasterMode::PixelCoverage, 100.f, true, ui::esp2d::Layout::Compact, outline_->get());
            if (names_.get()) ui::esp2d::DrawName(*bounds, scale, player.name, shadow_.get());
        }
    }
    Properties::ModeProperty *box_, *health_;
    Properties::BoolProperty *armor_, *outline_;
    Properties::BoolProperty players_{true}, names_{true}, shadow_{true};
};
}
std::unique_ptr<Module> MakeEsp2D() { return std::make_unique<Esp2D>(); }
}
