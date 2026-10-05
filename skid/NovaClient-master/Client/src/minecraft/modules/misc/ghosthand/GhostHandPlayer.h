#pragma once

namespace nova::module::ghosthand {
struct PlayerSettings {
    bool nonPlayer = true, bots = false, friendlies = false, enemies = true;
    bool everything = true, bed = false, adjacent = false;
    bool sword = false, tool = true, fists = true, bucket = true;
    bool flintSteel = true, cobweb = true, other = true;
    bool requireLeft = false, requireRight = false, notSword = false;
};
void PickPlayer(const PlayerSettings& settings, float partialTicks);
}
