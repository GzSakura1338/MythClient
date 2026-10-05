#pragma once
#include "contracts/AccountService.h"
#include <string_view>

namespace nova::minecraft::accounts {
// This is a menu-navigation exception, not a claim of online authentication.
inline bool CanOpenGameMenu(std::string_view screen,const nova::accounts::Snapshot& snapshot,
                            std::string_view uuid,std::string_view name) {
    if(screen!="SINGLEPLAYER"&&screen!="MULTIPLAYER")return false;
    if(!snapshot.supported||snapshot.busy||uuid.empty()||name.empty())return false;
    for(const auto& row:snapshot.rows) {
        if(row.current&&row.uuid==uuid&&row.name==name&&!row.invalid&&
           (row.source==nova::accounts::Source::Offline||row.source==nova::accounts::Source::Microsoft||
            row.source==nova::accounts::Source::Token))return true;
    }
    return false;
}
}
