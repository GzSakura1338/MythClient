#pragma once
// Canonical account names from bake 7cad03eb. AccountMapping translates the
// selected newer build for both JNI and hook rules; retain old-build support.
namespace nova::minecraft::accounts::mapping {
inline constexpr char Client[] = "com.moonsworth.lunar.client.RCRHRICCICICHOOCCCIRRCIRCICIOC";
inline constexpr char Manager[] = "com.moonsworth.lunar.client.OIOIICIHIOORICIIHRRIHIHOOCOOOO.HCHORCCCCHCOHHHROOCIRICCCIRHHH.RCRHRICCICICHOOCCCIRRCIRCICIOC";
inline constexpr char Account[] = "com.moonsworth.lunar.client.OIOIICIHIOORICIIHRRIHIHOOCOOOO.RCRHRICCICICHOOCCCIRRCIRCICIOC.RCRHRICCICICHOOCCCIRRCIRCICIOC";
inline constexpr char Handler[] = "com.moonsworth.lunar.client.OOIRIHRRCHRIHRHHCOOCICRIRCHIIO.COCIICOHOOCCHCHRIRHHCHORROOCOR.OHOIIOOOCOOIRIHRIHIIRROIICRICH.RCRHRICCICICHOOCCCIRRCIRCICIOC.COCIICOHOOCCHCHRIRHHCHORROOCOR.RCRHRICCICICHOOCCCIRRCIRCICIOC";
inline constexpr char SessionBridge[] = "com.moonsworth.lunar.CROOIHHIORIOCIHCRRCHOIOCOHRCHO.OIRIIHIORRRICHCIORHHCRHCHOICRH.OHORROCRIHOOIROOOHOOHICCHRIRHO";
inline constexpr char Model[] = "com.moonsworth.lunar.client.OOIRIHRRCHRIHRHHCOOCICRIRCHIIO.COCIICOHOOCCHCHRIRHHCHORROOCOR.OHOIIOOOCOOIRIHRIHIIRROIICRICH.CROOIHHIORIOCIHCRRCHOIOCOHRCHO";
inline constexpr char Util[] = "com.moonsworth.lunar.client.util.IHCRHRORCCHOIIOICCCRRRCHHHOIHR";
inline constexpr char Ipc[] = "com.moonsworth.lunar.client.ROIOCCORICHHCIHICHCHHICRHCCICH.RCRHRICCICICHOOCCCIRRCIRCICIOC";
inline constexpr char IpcState[] = "com.moonsworth.lunar.client.COCIICOHOOCCHCHRIRHHCHORROOCOR.HCHORCCCCHCOHHHROOCIRICCCIRHHH";
inline constexpr char Singleton[]="RROOCRCIRCROCCRRHIHRRHHRHOHRIC";
inline constexpr char GetManager[]="IROOORRHHIOHCOCRCOCIROHHICICOC";
inline constexpr char GetAccounts[]="OOOCHCIOHORCCCORORIRHIOCRROHIH";
inline constexpr char GetCurrent[]="RCRHHRICCIICIHCCIHIOHHCIIICRCO";
inline constexpr char SetSelected[]="COCOOHCOIROORRHROCOCHOHCRORHHO";
inline constexpr char GetProfile[]="HROROIOCCOORIIOOCRHOCROOHRHRRI";
inline constexpr char Profile[]="com.moonsworth.lunar.client.OIOIICIHIOORICIIHRRIHIHOOCOOOO.RCRHRICCICICHOOCCCIRRCIRCICIOC.OHRIIRCCRIROIORIIHHOCRRIICIROI";
inline constexpr char AddAccount[]="CHIOCIIROCHCHHHCCRORRIOHICOIIC";
inline constexpr char SelectAccount[]="RROIIHHOCIOCCRORIOHOCOHCCOICRR";
inline constexpr char GetModel[]="CIOCOORRORROCRCHHOIRROCOOCOIHH";
inline constexpr char ModelMap[]="HHOCRHRRRIHIRCRIIOCOCCCHHHIOOI";
inline constexpr char SessionChanged[]="RCRHRICCICICHOOCCCIRRCIRCICIOC";
inline constexpr char Launcher[]="IRHHRORRHICHOCCRIHIOROIOIHORII";
inline constexpr char LauncherState[]="HIOHCROIICHROHOCIIORCCOOOIRICH";
}
