#include <cmath>
#include <memory>
#include <vector>

#include "minecraft/events/EventBus.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "ui/hud/compass/CompassHud.h"
#include "minecraft/java/api/game_compass.h"
#include "minecraft/java/api/game_entity.h"
#include "minecraft/java/api/game_inventory.h"
#include "third_party/fatalwidgets/imgui/imgui.h"

// Compass -- a port of Naven's module of the same name, drawn with ImGui.
//
// An arrow at the middle of the screen pointing at the world spawn: it reads the
// player's position and yaw lerped to the current frame, works out the bearing to
// the spawn, and rotates a small triangle by it. The triangle sits above the
// screen centre, so when the player faces the spawn it points straight up.
//
// Like the original it only points anywhere in a natural dimension, and it has two
// guards: "Compass Only" requires a compass in the inventory, and "No Player Only"
// hides it while another player is around.
//
// The drawing is ImGui rather than the original's PoseStack. That changes only the
// transform, not the shape: Naven pushes a rotation about the screen centre and
// draws a four-vertex tracer at (cx, cy - 45) with the fifth vertex dropped, which
// is a triangle -- apex 45 above centre, base 10 wide and 10 below the apex. The
// same three points are rotated here and handed to AddTriangleFilled.

namespace nova::module {
namespace {

constexpr double kPi = 3.14159265358979323846;

class CompassModule final : public Module {
public:
    CompassModule() : Module("Compass", Category::Render, "General", "Overlay") {
        enableOverlay();

        compassOnly_ = &addBool("Compass Only", true);
        noPlayerOnly_ = &addBool("No Player Only", true);

        // The original refreshes the inventory and spawn on the motion event's PRE
        // half -- once a tick, rather than once a frame, because neither answer can
        // change within a tick.
        motion_ = event::EventBus::Get().Subscribe<event::MotionEvent>(
            [this](event::MotionEvent& e) { OnMotion(e); });
    }

private:
    void onDisable() override {
        hasCompass_ = false;
        hasSpawn_ = false;
    }

    void OnMotion(event::MotionEvent& e) {
        if (!isEnabled() || e.type != event::EventType::Pre)
            return;
        hasCompass_ = java::InventoryHasItem(java::KnownItem::Compass);
        hasSpawn_ = java::WorldSpawn(spawnX_, spawnZ_);
    }

    void onRender() override {
        if (ImGui::GetCurrentContext() == nullptr)
            return;
        if (compassOnly_->get() && !hasCompass_)
            return;
        if (noPlayerOnly_->get() && OtherPlayerPresent())
            return;
        if (!hasSpawn_)
            return;

        java::CompassView view;
        if (!java::ReadCompassView(java::FrameTime(), view))
            return;

        const ImVec2 display = ImGui::GetIO().DisplaySize;
        if (display.x <= 1.0f || display.y <= 1.0f)
            return;

        // The original draws in GUI-scaled pixels and the draw list works in the
        // window's own, so the offsets are converted rather than assumed equal --
        // otherwise the arrow would come out guiScale times too small.
        float unit = 1.0f;
        int windowWidth = 0;
        int windowHeight = 0;
        double guiScale = 0.0;
        if (java::WindowMetrics(windowWidth, windowHeight, guiScale) && windowWidth > 0 &&
            guiScale > 0.0) {
            const double guiScaledWidth = static_cast<double>(windowWidth) / guiScale;
            if (guiScaledWidth > 0.0)
                unit = static_cast<float>(static_cast<double>(display.x) / guiScaledWidth);
        }

        // Bearing to the spawn, less the quarter turn the triangle is drawn at and
        // the player's own yaw.
        const double bearing =
            std::atan2(static_cast<double>(spawnZ_) - view.z,
                       static_cast<double>(spawnX_) - view.x);
        const double degrees = bearing * (180.0 / kPi) - 90.0 - static_cast<double>(view.yaw);
        const float radians = static_cast<float>(degrees * (kPi / 180.0));
        ui::DrawCompass(radians,unit);
    }

    // Naven also skips its own BlinkingPlayer class here, which is a fake entity it
    // spawns itself. Nova has no such thing, so the test is just "someone else".
    [[nodiscard]] static bool OtherPlayerPresent() {
        std::vector<java::EntityHandle> players;
        if (!java::WorldPlayers(players))
            return false;
        const java::EntityHandle self = java::PlayerEntity();
        for (const java::EntityHandle& player : players) {
            if (!self.Valid() || !player.IsSame(self))
                return true;
        }
        return false;
    }

    Properties::BoolProperty* compassOnly_ = nullptr;
    Properties::BoolProperty* noPlayerOnly_ = nullptr;
    event::Subscription motion_;

    bool hasCompass_ = false;
    bool hasSpawn_ = false;
    int spawnX_ = 0;
    int spawnZ_ = 0;
};

}

std::unique_ptr<Module> MakeCompassModule() {
    return std::make_unique<CompassModule>();
}

}
