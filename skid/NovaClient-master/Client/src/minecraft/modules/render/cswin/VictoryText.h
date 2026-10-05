#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <string>
#include <string_view>
#include <utility>

namespace nova::module::cswin {
inline std::wstring Normalize(std::string_view utf8) {
    if(utf8.empty() || utf8.size()>8192)return {};
    const int count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,utf8.data(),int(utf8.size()),nullptr,0);
    if(count<=0)return {};
    std::wstring raw(count,0);MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,utf8.data(),int(utf8.size()),raw.data(),count);
    std::wstring clean;
    for(size_t i=0;i<raw.size();++i) {
        wchar_t c=raw[i];
        if(c==L'§'){if(i+1<raw.size())++i;continue;}
        if(c>=0xff01 && c<=0xff5e)c-=0xfee0;
        if(c==0x200b || c==0x200c || c==0x200d || c==0xfeff || c==0x00ad || c==0x0307)continue;
        WORD type=0;GetStringTypeW(CT_CTYPE1,&c,1,&type);
        if(type&C1_SPACE){if(!clean.empty() && clean.back()!=L' ')clean+=L' ';}
        else clean+=c;
    }
    // Resource-pack icons decorate server titles without changing their text.
    // Remove edge icons only; never strip a player's name/prefix from chat.
    auto edgeIcon=[](wchar_t c){return (c>=0xe000 && c<=0xf8ff) || c==L' ';};
    while(!clean.empty() && edgeIcon(clean.front()))clean.erase(clean.begin());
    while(!clean.empty() && edgeIcon(clean.back()))clean.pop_back();
    if(clean.empty())return {};
    std::wstring lower(clean.size(),0);
    if(LCMapStringEx(LOCALE_NAME_INVARIANT,LCMAP_LOWERCASE,clean.data(),int(clean.size()),lower.data(),int(lower.size()),nullptr,nullptr,0))clean=std::move(lower);
    // Strip visual framing, but retain brackets/colons/angle brackets: those
    // identify player-chat senders and must not turn a quote into a victory.
    constexpr std::wstring_view trim=L" \t\r\n!！.。★☆✦✧✪-=─━|•·🎉";
    const auto first=clean.find_first_not_of(trim);
    if(first==clean.npos)return {};
    return clean.substr(first,clean.find_last_not_of(trim)-first+1);
}
inline bool WinPhrase(std::wstring_view text) {
    constexpr std::wstring_view phrases[]{
        L"victory",L"you win",L"you won",L"you won the game",L"you won the match",L"you won the round",L"round won",L"your team won",L"your team wins",L"your team has won",
        L"胜利",L"勝利",L"获胜",L"獲勝",L"你赢了",L"你贏了",L"你胜利了",L"你勝利了",L"你获胜了",L"你獲勝了",L"你获得了胜利",L"你獲得了勝利",L"你的队伍获胜了",L"你的隊伍獲勝了",L"游戏胜利",L"遊戲勝利",
        L"勝利しました",L"あなたの勝利",L"あなたの勝ち",L"あなたのチームの勝利",L"승리",L"승리했습니다",L"당신이 승리했습니다",
        L"victoire",L"vous avez gagné",L"vous avez gagne",L"votre équipe a gagné",L"sieg",L"du hast gewonnen",L"ihr habt gewonnen",L"dein team hat gewonnen",
        L"victoria",L"has ganado",L"ganaste",L"tu equipo ha ganado",L"vitória",L"vitoria",L"você venceu",L"voce venceu",L"você ganhou",L"a sua equipa venceu",
        L"победа",L"вы победили",L"ты победил",L"ваша команда победила",L"vittoria",L"hai vinto",L"avete vinto",
        L"zafer",L"kazandın",L"kazandin",L"kazandınız",L"kazandiniz",L"zwycięstwo",L"zwyciestwo",L"wygrałeś",L"wygrales",
        L"overwinning",L"je hebt gewonnen",L"jij hebt gewonnen",L"kemenangan",L"kamu menang",L"chiến thắng",L"bạn đã thắng"
    };
    for(auto phrase:phrases)if(text==phrase)return true;
    // Hypixel's clickable replay prompt is part of the same chat component.
    // Match the complete known sentence, not a loose "win" substring.
    if(text==L"you won! want to play again? click here")return true;
    if(text==L"你的对手因其他原因死亡, 你获得了胜利" ||
       text==L"你的對手因其他原因死亡, 你獲得了勝利")return true;
    return false;
}
inline bool DefeatPhrase(std::wstring_view text) {
    constexpr std::wstring_view phrases[]{L"defeat",L"you lost",L"you lose",L"game over",L"失败",L"失敗",L"战败",L"戰敗",L"你输了",L"你輸了",L"敗北",L"패배",L"défaite",L"niederlage",L"derrota",L"поражение",L"sconfitta",L"yenilgi",L"porażka"};
    for(auto phrase:phrases)if(text==phrase)return true;
    return false;
}
inline std::wstring WithoutServerPrefix(std::wstring text) {
    constexpr std::wstring_view prefixes[]{L"[server]",L"[system]",L"[game]",L"[hypixel]",L"[bedwars]",L"[skywars]",L"[系统]",L"[系統]",L"[服务器]",L"[伺服器]"};
    for(auto prefix:prefixes)if(text.starts_with(prefix)) {
        text.erase(0,prefix.size());const auto start=text.find_first_not_of(L" :»>");
        return start==text.npos?L"":text.substr(start);
    }
    return text;
}
inline bool OwnWinner(std::wstring_view text,std::wstring_view name) {
    if(name.empty())return false;
    constexpr std::wstring_view labels[]{L"winner:",L"winner -",L"获胜者:",L"獲勝者:",L"胜利者:",L"勝利者:",L"gagnant:",L"gewinner:",L"ganador:",L"vencedor:",L"победитель:",L"vincitore:",L"kazanan:",L"zwycięzca:",L"winnaar:",L"勝者:",L"승자:"};
    for(auto label:labels)if(text.starts_with(label)) {
        auto value=text.substr(label.size());
        while(!value.empty() && value.front()==L' ')value.remove_prefix(1);
        // Optional rank decoration is allowed only in an explicit winner field.
        if(value.starts_with(L"[")) {auto end=value.find(L']');if(end==value.npos)return false;value.remove_prefix(end+1);}
        while(!value.empty() && value.front()==L' ')value.remove_prefix(1);
        return value==name;
    }
    return false;
}
inline bool IsVictory(std::string_view text,std::string_view localName) {
    if(text.size()>8192)return false;
    const auto name=Normalize(localName);
    bool victory=false;
    bool first=true;
    while(!text.empty()) {
        const auto end=text.find('\n');const auto line=text.substr(0,end);
        auto normalized=WithoutServerPrefix(Normalize(line));
        if(first && !normalized.empty()) {
            if(normalized.starts_with(L"<") || normalized.starts_with(L"[") || normalized.find(L'»')!=normalized.npos ||
                (normalized.find(L':')!=normalized.npos && !OwnWinner(normalized,name)))return false;
            first=false;
        }
        if(DefeatPhrase(normalized))return false;
        // Anchored phrases intentionally reject "Alice: Victory", "not a
        // victory", kill messages, shop descriptions, and other winners.
        victory|=WinPhrase(normalized)||OwnWinner(normalized,name);
        if(end==text.npos)break;text.remove_prefix(end+1);
    }
    return victory;
}
}
