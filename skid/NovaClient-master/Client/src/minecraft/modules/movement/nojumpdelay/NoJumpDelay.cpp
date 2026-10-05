#include "minecraft/events/GameEvents.h"
#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/java/api/game_player.h"

namespace nova::module {
namespace {
class NoJumpDelayModule final : public Module {
public:
    NoJumpDelayModule() : Module("NoJumpDelay", Category::Movement, "General", "General") {
        tick_ = event::EventBus::Get().Subscribe<event::PlayerTickEvent>(
            [this](event::PlayerTickEvent& e) {
                if (!isEnabled() || e.IsCancelled()) return;
                // Rise PreUpdateEvent runs before the player's living update.
                // Its only exception requires Rise's Watchdog Telly Jump Delay
                // option, which Nova's current Scaffold does not implement.
                java::ResetPlayerJumpDelay();
            });
    }
private:
    event::Subscription tick_;
};
}
std::unique_ptr<Module> MakeNoJumpDelayModule() { return std::make_unique<NoJumpDelayModule>(); }
}
