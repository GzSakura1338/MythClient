#include "minecraft/modules/misc/antibot/AntiBot.h"
#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/java/api/game_player_list.h"
#include "minecraft/java/api/game_freecam.h"
#include "minecraft/java/api/game_fakeplayer.h"

#include <atomic>

namespace nova::module {
namespace {
std::atomic<bool> active{false};

class AntiBotModule final : public Module {
public:
    AntiBotModule() : Module("AntiBot", Category::Misc, "General", "General") {}
private:
    void onEnable() override { active.store(true); }
    void onDisable() override { active.store(false); }
};
}

namespace antibot {
bool IsBot(const java::EntityHandle& entity) {
    if(java::IsFakePlayer(entity.raw()))return false;
    if(java::FreeCamCamera(entity.raw()))return true;
    if (!active.load() || !entity) return false;
    const auto listed = java::OnlinePlayerListContains(entity);
    return listed.has_value() && !*listed;
}
}

std::unique_ptr<Module> MakeAntiBotModule() { return std::make_unique<AntiBotModule>(); }
}
