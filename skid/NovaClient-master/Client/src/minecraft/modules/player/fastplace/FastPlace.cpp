#include <format>
#include <memory>
#include <string>

#include "minecraft/events/EventBus.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/java/api/game_place.h"

// FastPlace -- aligned with Naven's module of the same name.
//
// Naven does not expose a delay at all. It keeps a counter and credits the player
// with cps/20 of a placement every tick, spending a whole one by clearing the
// right click delay once a full placement has accumulated. At 10 cps that is half
// a placement a tick, so a block goes down every second tick -- which is what
// "10 cps" means at 20 ticks a second. The counter is reset the moment the use key
// comes up or the held item stops being a block, so a fresh press always starts
// from empty.
//
// The one setting is CPS, 5 to 20 in steps of one, defaulting to 10.
//
// This replaces an earlier FastPlace that carried four settings (tick delay,
// activation time, blocks only, pitch check) and also cancelled outgoing use-item
// packets. None of that is Naven's, and the packet cancellation in particular was
// a second, independent change to the same behaviour -- so it is gone.

namespace nova::module {
namespace {

[[nodiscard]] std::string CpsLabel(double value) { return std::format("{:.0f}", value); }

class FastPlaceModule final : public Module {
public:
    FastPlaceModule() : Module("FastPlace", Category::Misc, "General", "General") {
        cps_ = &addNumber("CPS", 5, 20, 1, 10, {}, CpsLabel);

        // Naven hangs this off the motion event's PRE half, which is the head of
        // the player's own tick -- the same place the rotation bridge runs.
        motion_ = event::EventBus::Get().Subscribe<event::MotionEvent>(
            [this](event::MotionEvent& e) { OnMotion(e); });
    }

private:
    void onDisable() override { counter_ = 0.0f; }

    void OnMotion(event::MotionEvent& e) {
        if (!isEnabled() || e.type != event::EventType::Pre)
            return;

        // A null client, player, or main hand reads as false here rather than
        // throwing, so the reset below also covers "not in a world".
        if (!java::UseKeyDown() || !java::HoldingBlockItem()) {
            counter_ = 0.0f;
            return;
        }

        const float cps = static_cast<float>(cps_->get());
        counter_ += cps / 20.0f;
        if (counter_ >= 1.0f / cps) {
            java::SetRightClickDelay(0);
            counter_ -= 1.0f;
        }
    }

    Properties::NumberProp* cps_ = nullptr;
    event::Subscription motion_;

    float counter_ = 0.0f;
};

}

std::unique_ptr<Module> MakeFastPlaceModule() {
    return std::make_unique<FastPlaceModule>();
}

}
