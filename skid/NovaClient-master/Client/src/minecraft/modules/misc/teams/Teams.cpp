#include "minecraft/modules/misc/teams/Teams.h"

#include <atomic>
#include <memory>

#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/java/api/game_fakeplayer.h"

namespace nova::module {
namespace {
std::atomic<int> activeMode{-1};

class TeamsModule final : public Module {
public:
    TeamsModule() : Module("Teams", Category::Misc, "General", "General") {
        mode_ = &addMode("Mode", static_cast<int>(teams::Mode::Color), {"Scoreboard", "Color", "Armor"});
    }

    std::string suffix() const override { return mode_->get() == 2 ? "Armor" : mode_->get() == 0 ? "Scoreboard" : "Color"; }

private:
    void onEnable() override { onUpdate(); }
    void onUpdate() override { activeMode.store(mode_->get()); }
    void onDisable() override { activeMode.store(-1); }
    Properties::ModeProperty* mode_ = nullptr;
};
}

namespace teams {
bool IsTeammate(const java::EntityHandle& self, const java::EntityHandle& other) {
    if(java::IsFakePlayer(other.raw()))return false;
    const int selected = activeMode.load();
    if (selected < 0 || !self || !other || !self.IsPlayer() || !other.IsPlayer()) return false;
    const auto mode = static_cast<Mode>(selected);
    if (mode == Mode::Armor)
        return Matches(java::ReadArmorTeamInfo(self), java::ReadArmorTeamInfo(other), mode);
    return Matches(java::ReadTeamInfo(self, true), java::ReadTeamInfo(other, true), mode);
}

bool IsTeammate(const java::EntityHandle& other) {
    if (activeMode.load() < 0) return false;
    return IsTeammate(java::PlayerEntity(), other);
}
}

std::unique_ptr<Module> MakeTeamsModule() { return std::make_unique<TeamsModule>(); }
}
