#pragma once
#include "cs2/modules/Module.h"

namespace nova::cs2::modules {
class Team final : public Module {
public:
    Team();
    // Only active T/CT teams count. Unassigned/spectator values must not hide
    // every player while joining a server or switching to spectator.
    static bool SameTeam(int localTeam, int playerTeam);
    bool ShouldIgnore(int localTeam, int playerTeam) const;
};
}
