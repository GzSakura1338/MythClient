#pragma once

#include <string>
#include <unordered_map>

#include "minecraft/java/api/base.h"

namespace nova::java {

// --- health bypass -----------------------------------------------------------
//
// The scoreboard objective some servers put a player's real health in.
//
// The client is told another player's health through entity metadata, and a server
// that does not want that known simply never puts the true value there -- it sends
// a full bar and the number is a lie. What it still has to send is the scoreboard:
// an objective named "health" or "belowHealth" is how a plugin shows a health bar
// next to a name in the tab list, and every change to one arrives as a score packet
// naming the player it belongs to.
//
// Reads one of those. False for every other score in the game, which is most of
// them: a sidebar, a death counter, a coin balance.
//
// A removal carries no score, so it is refused rather than read as a zero. Taking
// it would pin the player at a sliver of a heart instead of letting the metadata
// value stand.

bool ReadHealthScore(jobject packet, std::string& owner, int& score);

// Read already-applied scores on the game thread (currently Lunar 1.21.11).
// True with an empty map means the scoreboard exists but has no matching scores.
bool ReadStoredHealthScores(std::unordered_map<std::string,int>& scores,
                           std::string* objectives = nullptr);

}
