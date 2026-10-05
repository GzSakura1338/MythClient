#include "cs2/modules/misc/team/Team.h"
#include "cs2/sdk/players/Players.h"

namespace nova::cs2::modules {
Team::Team() : Module("Team", "misc", "General") {}
bool Team::SameTeam(int localTeam, int playerTeam) {
    return sdk::players::SameTeam(localTeam, playerTeam);
}
bool Team::ShouldIgnore(int localTeam, int playerTeam) const {
    return isEnabled() && SameTeam(localTeam, playerTeam);
}
}
