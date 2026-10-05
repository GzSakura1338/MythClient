#pragma once
#include <cstdint>

namespace nova::module::invmove {
struct WatchdogState {
    bool clicking=false,confirmed=false,awaiting=false,delayPassed=false;
    int sentTick=-1,latency=-1,chestTick=-1;
    std::uint64_t blockStart=0;
    void Tick(int tick,bool inventory,bool chest,bool clicked,bool attempt,int earlyTicks){
        if(!inventory)clicking=false;else if(clicked)clicking=true;
        if(attempt){sentTick=tick;awaiting=true;}
        if(awaiting){
            if(chest){latency=tick-sentTick;awaiting=false;}
            else if(tick-sentTick>40)awaiting=false;
        }
        if(!chest){chestTick=-1;confirmed=false;}
        else {
            if(chestTick<0)chestTick=tick;
            if(latency>=0&&tick-chestTick>=latency-earlyTicks)confirmed=true;
        }
    }
    bool BlockInput(int tick,std::uint64_t now,bool chest,bool prediction,bool speed,bool ground){
        bool blocked=chest&&confirmed&&tick%5!=0;
        if((chest||clicking)&&prediction&&(speed||!ground))blocked=true;
        else if(clicking&&!delayPassed){blocked=true;if(!blockStart)blockStart=now;}
        else blockStart=0;
        if(blockStart&&now-blockStart>=60){delayPassed=true;blockStart=0;}
        if(!chest&&!clicking)delayPassed=false;
        return blocked;
    }
};
}
