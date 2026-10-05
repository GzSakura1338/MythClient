#pragma once
#include "minecraft/compat/minecraft263/Minecraft263.h"
namespace nova::compat::lunarnick {
using Member=mc263::names::Member;
inline constexpr const char* Utility="com.moonsworth.lunar.client.util.HCOHOICHOHHCCIHHOIHCOHIIIRIORH";
inline constexpr const char* Client="com.moonsworth.lunar.client.IOHOHIHHHHORRCCRHCRCHCRIOIHRHH";
inline constexpr const char* Manager="com.moonsworth.lunar.client.CRICRCRHRRICCICRHICIIIOHRIRHRH.HHICICROOCIIROOHOORRCIHOICHOIH.RCROOOHOIRCOCICCROHICRRIHCROOR";
inline constexpr const char* Nick="com.moonsworth.lunar.client.HRCIHRROICRHRHOHIRORRCCHOICRRI.CIIHHRHHHIOIIHHRROOHRHICIHOOOC.RCHCCCCHCHCCRICOIOCCRCCRRRIIRI.ROOCCIHRCCHORHCCOIIRIHHHIIHHHO";
inline constexpr Member Instance{Utility,"ROHHHIHCRIRHCHOCOOICCRRIHROHIO","()Lcom/moonsworth/lunar/client/IOHOHIHHHHORRCCRHCRCHCRIOIHRHH;"};
inline constexpr Member Modules{Client,"RICIICOOIOHIHOIHOIIRHIHROOHHRO","()Lcom/moonsworth/lunar/client/CRICRCRHRRICCICRHICIIIOHRIRHRH/HHICICROOCIIROOHOORRCIHOICHOIH/RCROOOHOIRCOCICCROHICRRIHCROOR;"};
inline constexpr Member NickHider{Manager,"OCCCRRROCORCRCHCHCROHRIROOROHO","()Lcom/moonsworth/lunar/client/HRCIHRROICRHRHOHIRORRCCHOICRRI/CIIHHRHHHIOIIHHRROOHRHICIHOOOC/RCHCCCCHCHCCRICOIOCCRCCRRRIIRI/ROOCCIHRCCHORHCCOIIRIHHHIIHHHO;"};
inline constexpr Member Format{Nick,"RCICCIIORHROICOIRRRRIIOIRCRHOI","(Ljava/lang/String;)Ljava/lang/String;"};
inline constexpr const char* OctoberNick="com.moonsworth.lunar.client.HRCIHRROICRHRHOHIRORRCCHOICRRI.CIIHHRHHHIOIIHHRROOHRHICIHOOOC.RIRROIIHHOCRIOHHIIOCCIOIHOORRI.ROOCCIHRCCHORHCCOIIRIHHHIIHHHO";
inline constexpr Member OctoberInstance{Utility,"IHOIRRICIROCIHRRICOICICIICHORI","()Lcom/moonsworth/lunar/client/IOHOHIHHHHORRCCRHCRCHCRIOIHRHH;"};
inline constexpr Member OctoberModules{Client,"HIHORRORRCHRROIIHHHHCHHOHCIICI","()Lcom/moonsworth/lunar/client/CRICRCRHRRICCICRHICIIIOHRIRHRH/HHICICROOCIIROOHOORRCIHOICHOIH/RCROOOHOIRCOCICCROHICRRIHCROOR;"};
inline constexpr Member OctoberNickHider{Manager,"OIOCRRIOROCRHOROHOCHORRRIOCIOR","()Lcom/moonsworth/lunar/client/HRCIHRROICRHRHOHIRORRCCHOICRRI/CIIHHRHHHIOIIHHRROOHRHICIHOOOC/RIRROIIHHOCRIOHHIIOCCIOIHOORRI/ROOCCIHRCCHORHCCOIIRIHHHIIHHHO;"};
inline constexpr Member OctoberFormat{OctoberNick,"CCCOOHCCOHHCIOHICHHOHROIRRHHHO","(Ljava/lang/String;)Ljava/lang/String;"};
struct Bindings {Member instance,modules,nickHider,format;};
inline constexpr Bindings ModernBuilds[]{
    {OctoberInstance,OctoberModules,OctoberNickHider,OctoberFormat},
    {Instance,Modules,NickHider,Format}
};
}
