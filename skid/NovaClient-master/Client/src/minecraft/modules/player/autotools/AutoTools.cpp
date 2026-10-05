#include <memory>

#include "minecraft/events/EventBus.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/modules/Module.h"
#include "minecraft/modules/player/automlg/AutoMLG.h"
#include "minecraft/modules/player/autocharge/AutoCharge.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/java/api/base.h"
#include "minecraft/java/api/game_entity.h"
#include "minecraft/java/api/game_place.h"
#include "minecraft/java/api/game_tool.h"

// AutoTools -- a port of Starain's module of the same name.
//
// While the game is breaking a block, the hotbar moves to whatever cuts that block
// fastest, and moves back when the break ends.
//
// The slot to return to is remembered at the moment of the first switch rather than
// at the moment the break started, so a manual switch part-way through a break
// becomes the slot that is returned to -- which is what the original does, and the
// reason it carries the slot across ticks instead of reading it back.

namespace nova::module {
namespace {

namespace event = nova::event;

class AutoToolsModule final : public Module {
public:
    AutoToolsModule() : Module("AutoTools", Category::Player, "General", "General") {
        switchBack_ = &addBool("Switch Back", true);
        onlySneak_ = &addBool("Only Sneak", false);

        motion_ = event::EventBus::Get().Subscribe<event::MotionEvent>(
            [this](event::MotionEvent& e) { OnMotion(e); });
        damage_ = event::EventBus::Get().Subscribe<event::BlockDamageEvent>(
            [this](event::BlockDamageEvent& e) {
                if (isEnabled()) SelectTool({e.x, e.y, e.z});
            });
    }

private:
    void onDisable() override { originSlot_ = -1; }

    void OnMotion(event::MotionEvent& e) {
        if(automlg::IsWorking()||autocharge::IsWorking()){originSlot_=-1;return;}
        if (!isEnabled())
            return;

        // The original hangs both halves off the same event: the switch out is on
        // PRE, the switch back on POST.
        if (e.type == event::EventType::Pre) {
            if (!java::GameModeDestroying())
                return;

            java::BlockPos pos;
            if (!java::HitBlockPos(pos))
                return;

            SelectTool(pos);
            return;
        }

        if (java::GameModeDestroying() || !switchBack_->get() || originSlot_ == -1)
            return;

        java::SetSelectedHotbarSlot(originSlot_);
        originSlot_ = -1;
    }

    void SelectTool(const java::BlockPos& pos) {
        if (onlySneak_->get() && !java::KeyShiftDown()) return;
        int bestTool = -1, selected = -1;
        if (!java::BestToolSlotFor(pos, bestTool) || bestTool == -1 ||
            !java::SelectedHotbarSlot(selected) || bestTool == selected) return;
        originSlot_ = selected;
        java::SetSelectedHotbarSlot(bestTool);
    }

    Properties::BoolProperty* switchBack_ = nullptr;
    Properties::BoolProperty* onlySneak_ = nullptr;
    event::Subscription motion_;
    event::Subscription damage_;

    // The slot to go back to, or -1 when nothing has been switched away from.
    int originSlot_ = -1;
};

}

std::unique_ptr<Module> MakeAutoToolsModule() {
    return std::make_unique<AutoToolsModule>();
}

}
