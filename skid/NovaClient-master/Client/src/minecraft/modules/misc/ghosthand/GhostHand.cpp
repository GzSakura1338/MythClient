#include <cmath>
#include <limits>
#include <memory>
#include <vector>

#include "minecraft/events/EventBus.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/java/api/game_chest.h"
#include "minecraft/java/api/game_entity.h"
#include "minecraft/java/api/game_place.h"
#include "minecraft/java/api/game_rotation.h"
#include "minecraft/modules/misc/ghosthand/GhostHandPlayer.h"

namespace nova::module {
namespace {

class GhostHandModule final : public Module {
public:
    GhostHandModule() : Module("GhostHand", Category::Misc, "General", "General") {
        mode_ = &addMode("Mode", 0, {"Chest", "Player"});
        auto player = [this] { return mode_->get() == 1; };
        addSettings({
            {"Interact through", nullptr, {{"Non-player entities", &nonPlayer_},
                {"Bots", &bots_}, {"Friendlies", &friendlies_}, {"Enemies", &enemies_}}, player},
            {"Preferred targets", nullptr, {{"Everything", &everything_},
                {"Bed", &bed_}, {"Next to bed", &adjacent_}}, player},
            {"Allow while using", nullptr, {{"Sword", &sword_}, {"Tool", &tool_},
                {"Fists", &fists_}, {"Bucket", &bucket_}, {"Flint and steel", &flintSteel_},
                {"Cobweb", &cobweb_}, {"Other", &other_}}, player},
            {"Conditions", nullptr, {{"Require Left mouse", &requireLeft_},
                {"Require Right mouse", &requireRight_}, {"Not holding a sword", &notSword_}}, player}
        });
        click_ = event::EventBus::Get().Subscribe<event::ClickEvent>(
            [this](event::ClickEvent& e) { OnClick(e); });
        pick_ = event::EventBus::Get().Subscribe<event::CursorPickEvent>(
            [this](event::CursorPickEvent& e) {
                if (isEnabled() && mode_->get() == 1 && e.type == event::EventType::Post)
                    ghosthand::PickPlayer({nonPlayer_.get(), bots_.get(), friendlies_.get(), enemies_.get(),
                        everything_.get(), bed_.get(), adjacent_.get(), sword_.get(), tool_.get(),
                        fists_.get(), bucket_.get(), flintSteel_.get(), cobweb_.get(), other_.get(),
                        requireLeft_.get(), requireRight_.get(), notSword_.get()},
                        e.partialTicks >= 0.f ? e.partialTicks : java::FrameTime());
            }, event::Priority::Lowest);
    }
    std::string suffix() const override { return mode_->get() == 1 ? "Player" : "Chest"; }

private:
    void OnClick(event::ClickEvent& e) {
        if (!isEnabled() || mode_->get() != 0 || e.IsCancelled() || !java::UseKeyDown()) return;
        if (GhostInteractWithChest()) e.Cancel();
    }

    bool GhostInteractWithChest() {
        auto self = java::PlayerEntity();
        rotation::Vec3 eye, look;
        float yaw = 0, pitch = 0;
        if (!self.Valid() || !self.EyePosition(1.0f, eye) ||
            !java::PlayerRotation(yaw, pitch) ||
            !java::Vec3DirectionFromRotation(pitch, yaw, look)) return false;
        const auto end = eye.Add(look.x * 4.5, look.y * 4.5, look.z * 4.5);

        // The shared adapter already selects ChestBlockEntity only, skips LEFT
        // and merges double-chest outlines. Starain's unused BLOCKS set does not
        // make ender chests or shulker boxes eligible.
        std::vector<java::ChestBox> chests;
        if (!java::LoadedChestBoxes(chests)) return false;
        const java::ChestBox* target = nullptr;
        rotation::Vec3 hit;
        double nearest = std::numeric_limits<double>::max();
        for (const auto& chest : chests) {
            const java::Box box{chest.minX, chest.minY, chest.minZ,
                                chest.maxX, chest.maxY, chest.maxZ};
            bool intersects = false;
            rotation::Vec3 intersection;
            if (!java::BoxClip(box, eye, end, intersects, intersection) || !intersects)
                continue;
            const double distance = intersection.DistanceTo(eye);
            if (!std::isfinite(distance) || distance >= nearest) continue;
            nearest = distance;
            target = &chest;
            hit = intersection;
        }
        if (!target) return false;

        // Match Starain: always MAIN_HAND / UP, swing and consume the click
        // after finding a target, independently of the interaction result.
        java::UseItemOnBlock({target->x, target->y, target->z}, 1, hit);
        java::SwingMainHand();
        return true;
    }

    Properties::ModeProperty* mode_ = nullptr;
    Properties::BoolProperty nonPlayer_{true}, bots_{false}, friendlies_{false}, enemies_{true};
    Properties::BoolProperty everything_{true}, bed_{false}, adjacent_{false};
    Properties::BoolProperty sword_{false}, tool_{true}, fists_{true}, bucket_{true};
    Properties::BoolProperty flintSteel_{true}, cobweb_{true}, other_{true};
    Properties::BoolProperty requireLeft_{false}, requireRight_{false}, notSword_{false};
    event::Subscription click_, pick_;
};
}

std::unique_ptr<Module> MakeGhostHandModule() { return std::make_unique<GhostHandModule>(); }
}
