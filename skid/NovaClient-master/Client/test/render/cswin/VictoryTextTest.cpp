#include "minecraft/modules/render/cswin/VictoryText.h"
#include <cstdio>
#include <cstdlib>
using nova::module::cswin::IsVictory;
void Check(bool condition,const char* text) {
    if(!condition){std::fprintf(stderr,"FAIL: %s\n",text);std::exit(1);}
}
int main() {
    for(auto text:{"VICTORY!","§6§lVICTORY!§r"," ＶＩＣＴＯＲＹ！ ","🎉 Victory! 🎉",
        "胜利！","勝利！","你赢了！","你的隊伍獲勝了","勝利しました","승리!",
        "VICTOIRE!","SIEG!","VICTORIA!","VITÓRIA!","ПОБЕДА!","VITTORIA!",
        "ZAFER!","ZWYCIĘSTWO!","OVERWINNING!","KEMENANGAN!","CHIẾN THẮNG!",
        "[Server] Victory!","[系统] 胜利！","\n---\nVictory!\nRound 3",
        "Winner: NovaUser","Winner: [MVP+] NovaUser","获胜者：NovaUser",
        "You won! Want to play again? Click here! ","YOU WON! Want to play again? CLICK HERE!",
        "§aYou won! §eWant to play again? §b§lClick here!§r",
        "\ue6f6 你的对手因其他原因死亡, 你获得了胜利！","你获得了胜利！",
        "\ue6f6 你的對手因其他原因死亡， 你獲得了勝利！",
        "\ue6f6 VICTORY!"," | 胜利者: NovaUser","Winner - NovaUser"})Check(IsVictory(text,"NovaUser"),text);
    for(auto text:{"","Defeat!","You lost!","失败！","DRAW!","Game over!","Alice: Victory!",
        "<Alice> Victory!","[VIP] Alice: Victory!","Alice » Victory!","Alice:\nVictory!",
        "[Server] Alice: Victory!","Winner: Alice","Winner: NovaUser2","not a victory",
        "Victory dance unlocked!","Victory?","Victory!\nDefeat!","/title @s title Victory!",
        "Alice: You won! Want to play again? Click here!","You won! Want to play again? Click here! Defeat!",
        "<Alice> 你的对手因其他原因死亡, 你获得了胜利！","\ue6f6 Alice: 胜利！",
        "You won! Want to play again? Click here!\nDEFEAT!"})Check(!IsVictory(text,"NovaUser"),text);
    Check(IsVictory("Winner: Player","Player"),"real username Player");
    Check(!IsVictory("Winner: Player",""),"missing identity cannot win by name");
    Check(!IsVictory(std::string(9000,'x'),"NovaUser"),"oversized text");
    Check(!IsVictory(std::string("\xff\xfe",2),"NovaUser"),"invalid UTF8");
    std::puts("PASS: multilingual victory, formatting, own winner, chat/defeat rejection");
}
