#pragma once

#include <optional>
#include <string>
#include "third_party/jni/jni.h"

#include "minecraft/java/api/game_entity.h"

namespace nova::java {
// Safe UTF-8 equivalent of Rise's leading legacy display-name color check.
inline std::optional<int> LeadingTeamColor(const std::string& text) {
    if(text.size()<3 || static_cast<unsigned char>(text[0])!=0xc2 || static_cast<unsigned char>(text[1])!=0xa7)return {};
    const char c=text[2];
    if(c>='0'&&c<='9')return c-'0';
    if(c>='a'&&c<='f')return c-'a'+10;
    if(c>='A'&&c<='F')return c-'A'+10;
    return {};
}

struct TeamInfo {
    std::string name;
    std::optional<int> color;
    std::optional<int> headColor;
    std::optional<int> chestColor;
};

// Reads entity scoreboard membership directly; absent teams/failed reads return empty data.
TeamInfo ReadTeamInfo(const EntityHandle& player, bool withColor);
// Only explicitly dyed leather head/chest equipment has a color. Independent of scoreboard.
TeamInfo ReadArmorTeamInfo(const EntityHandle& player);
void ReleaseTeamsAccess(JNIEnv* env);

}
