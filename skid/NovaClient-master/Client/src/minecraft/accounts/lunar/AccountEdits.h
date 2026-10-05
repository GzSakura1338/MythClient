#pragma once
#include "minecraft/hooks/Edits.h"
#include "minecraft/accounts/lunar/AccountMapping.h"
#include <deque>

namespace nova::minecraft::accounts::mapping {
inline constexpr char AuthUtilInternal[]="com/moonsworth/lunar/client/util/HIOORCHCOIHRCHRRIIOIICCOHCHOCO";
inline constexpr char HandlerInternal[]="com/moonsworth/lunar/client/OOIRIHRRCHRIHRHHCOOCICRIRCHIIO/COCIICOHOOCCHCHRIRHHCHORROOCOR/OHOIIOOOCOOIRIHRIHIIRROIICRICH/RCRHRICCICICHOOCCCIRRCIRCICIOC/COCIICOHOOCCHCHRIRHHCHORROOCOR/RCRHRICCICICHOOCCCIRRCIRCICIOC";
inline constexpr char XboxAccountInternal[]="com/moonsworth/lunar/client/OIOIICIHIOORICIIHRRIHIHOOCOOOO/RCRHRICCICICHOOCCCIRRCIRCICIOC/HIOORCHCOIHRCHRRIIOIICCOHCHOCO";
inline constexpr char NavigationInternal[]="com/moonsworth/lunar/client/OOIRIHRRCHRIHRHHCOOCICRIRCHIIO/COCIICOHOOCCHCHRIRHHCHORROOCOR/RCRHRICCICICHOOCCCIRRCIRCICIOC/OHRIIRCCRIROIORIIHHOCRRIICIROI";
inline constexpr char ScreenEventsInternal[]="com/moonsworth/lunar/client/HOHOHOHCHOCRRCCOHCOHCRIRIIIOHC/COCIICOHOOCCHCHRIRHHCHORROOCOR/CROOIHHIORIOCIHCRRCHOIOCOHRCHO";
inline constexpr char ManagerInternal[]="com/moonsworth/lunar/client/OIOIICIHIOORICIIHRRIHIHOOCOOOO/HCHORCCCCHCOHHHROOCIRICCCIRHHH/RCRHRICCICICHOOCCCIRRCIRCICIOC";
inline constexpr char ScreenInternal[]="com/moonsworth/lunar/client/OOIRIHRRCHRIHRHHCOOCICRIRCHIIO/COCIICOHOOCCHCHRIRHHCHORROOCOR/RCRHRICCICICHOOCCCIRRCIRCICIOC/HOCIIIHOIOHRCHOHOHRROCCHCOOHOC";
inline constexpr hook::Redirect AccountPageRedirects[]={
    {"CHIOCIIROCHCHHHCCRORRIOHICOIIC","()V",AuthUtilInternal,"RHCOHCRRIRIIRRCRRCCIIROHIHORHO","(Ljava/util/function/Consumer;)V","accountBrowserAdd","(Ljava/lang/Object;)V",1},
    // Queue the selection before Lunar checks for a missing access token.
    {"RROIIHHOCIOCCRORIOHOCOHCCOICRR","(Ljava/lang/String;)Ljava/lang/String;",HandlerInternal,"ORCRROHIIOCIOOCCROIHOOOHRORRCR","(Ljava/lang/String;)Ljava/util/Optional;","accountBrowserSelect","(Ljava/lang/String;)Ljava/util/Optional;",1}
};
inline constexpr hook::Redirect AccountRefreshRedirects[]={
    {"RHRHICRCRHOHIOCOORCOCROCOCCHCH","(Ljava/util/function/Consumer;)V",AuthUtilInternal,"RCRHRICCICICHOOCCCIRRCIRCICIOC","(Lcom/moonsworth/lunar/client/OIOIICIHIOORICIIHRRIHIHOOCOOOO/RCRHRICCICICHOOCCCIRRCIRCICIOC/RCRHRICCICICHOOCCCIRRCIRCICIOC;Ljava/util/function/Consumer;)V","accountBrowserRefresh","(Ljava/lang/Object;Ljava/lang/Object;)V",1}
};
inline constexpr hook::Redirect AccountNavigationRedirects[]={
    {"RCRHRICCICICHOOCCCIRRCIRCICIOC","(Lcom/moonsworth/lunar/client/OOIRIHRRCHRIHRHHCOOCICRIRCHIIO/COCIICOHOOCCHCHRIRHHCHORROOCOR/RCRHRICCICICHOOCCCIRRCIRCICIOC/HOCIIIHOIOHRCHOHOHRROCCHCOOHOC;)V",ScreenInternal,"isRequiresAuthentication","()Z","accountScreenRequiresLogin","(Ljava/lang/Object;)Z",1}
};
inline constexpr hook::Redirect AccountScreenEventRedirects[]={
    // This call is already guarded by instanceof the multiplayer-screen bridge.
    // Keep the global Lunar authentication check and all other callsites intact.
    {"RCRHRICCICICHOOCCCIRRCIRCICIOC","(Lcom/moonsworth/lunar/client/OOHHRRCCOIORCHHIIORCHRCRRCHRCI/RCRHRICCICICHOOCCCIRRCIRCICIOC/CROOIHHIORIOCIHCRRCHOIOCOHRCHO/HOCIIIHOIOHRCHOHOHRROCCHCOOHOC;)V",ManagerInternal,"OHIICIRIICRCIRCCIORRRCHOIICRIC","()Z","accountCanOpenMultiplayer","(Ljava/lang/Object;)Z",1}
};
inline constexpr hook::ClassEdits AccountEdits[]={
    {HandlerInternal,AccountPageRedirects,2},
    {XboxAccountInternal,AccountRefreshRedirects,1},
    {NavigationInternal,AccountNavigationRedirects,1},
    {ScreenEventsInternal,AccountScreenEventRedirects,1}
};
// Own every translated string/array for the entire transformer lifetime.
struct ResolvedAccountEdits {
    std::deque<std::string> strings;
    std::vector<std::vector<hook::Redirect>> redirects;
    std::vector<hook::ClassEdits> classes;
    void Reset(Build build){
        classes.clear();redirects.clear();strings.clear();
        if(build==Build::Unknown)return;
        redirects.resize(std::size(AccountEdits));classes.reserve(std::size(AccountEdits));
        auto name=[&](const char* source){strings.push_back(Remap(source,build));return strings.back().c_str();};
        for(size_t i=0;i<std::size(AccountEdits);++i){
            auto row=AccountEdits[i];auto& calls=redirects[i];calls.reserve(row.count);
            for(int k=0;k<row.count;++k){auto r=row.redirects[k];
                // The October bake renames this event type independently from
                // the account-screen enum that used to share its identifier.
                if(build==Build::October2&&row.redirects==AccountScreenEventRedirects)r.methodDesc="(Lcom/moonsworth/lunar/client/OCCRIICCOCOIORRORHIRHCHCOHHHHC/IOHOHIHHHHORRCCRHCRCHCRIOIHRHH/IHCCRCCOHRCCIORHHCROORROCICRHI/OCRRORHCHRCRIIHRORIRIRROCORHCI;)V";
                r.method=name(r.method);r.methodDesc=name(r.methodDesc);
                r.calleeOwner=name(r.calleeOwner);r.calleeName=name(r.calleeName);r.calleeDesc=name(r.calleeDesc);
                calls.push_back(r);
            }
            row.internalName=name(row.internalName);row.redirects=calls.data();classes.push_back(row);
        }
    }
};
}
