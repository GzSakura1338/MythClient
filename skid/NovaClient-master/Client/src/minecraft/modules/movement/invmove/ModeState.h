#pragma once
#include <algorithm>
#include <cstdint>

namespace nova::module::invmove {
enum Mode { Normal, BufferAbuse, Cancel, Grim, Grim2, Watchdog };
struct BufferState {
    std::uint64_t clicks=0;
    bool waited=false,flushed=false;
    bool buffering(int interval)const{return clicks>0&&clicks%interval==0;}
    bool holding(int interval)const{return buffering(interval)&&!flushed;}
    bool click(int interval){if(holding(interval))return true;++clicks;return false;}
    bool motion(int interval){
        if(!buffering(interval)){waited=flushed=false;return false;}
        if(flushed)return false;
        if(!waited){waited=true;return false;}
        return true;
    }
};
struct GrimState {
    int clicks=0,ticks=0;
    bool update(int extra){
        if(clicks>0)ticks=std::max(ticks,clicks+extra);
        clicks=0;
        if(ticks<=0)return false;
        --ticks;return true;
    }
};
}
