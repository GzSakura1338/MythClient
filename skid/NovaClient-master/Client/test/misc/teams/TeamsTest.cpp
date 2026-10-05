#include <atomic>
#include <cassert>
#include <cstdio>
#include <utility>
#include "minecraft/modules/misc/teams/Teams.h"

namespace fixture {
struct Player { bool player = true; nova::java::TeamInfo team; } self, target;
bool inWorld = true;
int reads = 0;
int armorReads = 0;
}

namespace nova::java {
EntityHandle::~EntityHandle() = default;
EntityHandle::EntityHandle(EntityHandle&& other) noexcept : obj(std::exchange(other.obj, nullptr)) {}
EntityHandle& EntityHandle::operator=(EntityHandle&& other) noexcept { obj = std::exchange(other.obj, nullptr); return *this; }
bool EntityHandle::IsPlayer() const { return obj && static_cast<fixture::Player*>(obj)->player; }
EntityHandle AdoptEntity(void* localRef) { EntityHandle e; e.obj = localRef; return e; }
EntityHandle PlayerEntity() { return AdoptEntity(fixture::inWorld ? &fixture::self : nullptr); }
TeamInfo ReadTeamInfo(const EntityHandle& entity, bool) {
    ++fixture::reads;
    return static_cast<fixture::Player*>(entity.raw())->team;
}
TeamInfo ReadArmorTeamInfo(const EntityHandle& entity) {
    ++fixture::armorReads;
    return static_cast<fixture::Player*>(entity.raw())->team;
}
}

namespace nova::module {
namespace { std::atomic<int> activeMode{-1}; }
// @PRODUCTION_TEAMS_API@
}

int main() {
    using namespace nova::module;
    using namespace fixture;
    assert(nova::java::LeadingTeamColor("\xc2\xa7" "cPlayer")==12);
    assert(!nova::java::LeadingTeamColor("Player"));
    assert(!nova::java::LeadingTeamColor("\xc2\xa7"));
    assert(!nova::java::LeadingTeamColor("\xc2\xa7" "rPlayer"));
    auto other = nova::java::AdoptEntity(&target);
    self.team = {"red", 0xFF5555}; target.team = self.team;
    assert(!teams::IsTeammate(other) && reads == 0);
    activeMode = static_cast<int>(teams::Mode::Color);
    assert(teams::IsTeammate(other));
    target.team.name = "different-red-team";
    assert(teams::IsTeammate(other)); // Color deliberately compares colors, not names.
    activeMode = static_cast<int>(teams::Mode::Scoreboard);
    assert(teams::IsTeammate(other));
    target.team.name = "red"; target.team.color = 0x5555FF;
    assert(!teams::IsTeammate(other)); // Same name does not override different colors.
    activeMode = static_cast<int>(teams::Mode::Color);
    assert(!teams::IsTeammate(other));
    self.team = {}; target.team = {};
    assert(!teams::IsTeammate(other));
    activeMode = static_cast<int>(teams::Mode::Scoreboard);
    assert(!teams::IsTeammate(other)); // Missing player info and two null teams are not allies.
    self.team = {"white", 0xFFFFFF}; target.team = self.team;
    activeMode = static_cast<int>(teams::Mode::Color);
    assert(teams::IsTeammate(other)); // Explicit white remains a valid team color.
    target.team.color.reset();
    assert(!teams::IsTeammate(other)); // Unspecified color does not become default white.
    target.team = self.team; target.player = false;
    assert(!teams::IsTeammate(other));
    target.player = true; inWorld = false;
    assert(!teams::IsTeammate(other));
    inWorld = true; target.team = {"blue", 0x5555FF};
    assert(!teams::IsTeammate(other)); // Reconnect/team changes read current information.
    target.team = self.team; activeMode = -1;
    assert(!teams::IsTeammate(other));
    activeMode = static_cast<int>(teams::Mode::Armor);
    const int scoreboardReads = reads;
    self.team = {}; target.team = {};
    assert(!teams::IsTeammate(other));
    // No scoreboard membership is needed; head and chest may have different colors.
    self.team.headColor = 0;
    self.team.chestColor = 0xFFFFFF;
    target.team = self.team;
    assert(teams::IsTeammate(other));
    target.team.headColor = 0xFFFFFF;
    assert(!teams::IsTeammate(other));
    target.team = self.team; target.team.chestColor = 0;
    assert(!teams::IsTeammate(other));
    const auto complete = self.team;
    // Missing, undyed and non-leather pieces all read as absent colors.
    for (int piece = 0; piece < 4; ++piece) {
        self.team = complete; target.team = complete;
        auto& team = piece < 2 ? self.team : target.team;
        (piece % 2 == 0 ? team.headColor : team.chestColor).reset();
        assert(!teams::IsTeammate(other));
    }
    self.team = complete; target.team = complete;
    assert(teams::IsTeammate(other)); // Equipment changes are read immediately.
    assert(reads == scoreboardReads);
    const int equipmentReads = armorReads;
    target.player = false;
    assert(!teams::IsTeammate(other));
    target.player = true; inWorld = false;
    assert(!teams::IsTeammate(other));
    inWorld = true; activeMode = -1;
    assert(!teams::IsTeammate(other));
    assert(armorReads == equipmentReads);
    std::puts("Teams: all three modes, armor colors/missing pieces, shared gate and live changes passed");
}
