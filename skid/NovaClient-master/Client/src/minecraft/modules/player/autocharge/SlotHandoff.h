#pragma once
#include <utility>

namespace nova::module::autocharge {
// Keep the effective slot until the native client has finished the use tick.
// Also covers a failed/ambiguous JNI return after a packet was already sent.
class SlotHandoff {
public:
    void BeforeUse(){pending_=true;}
    template<class Release> void BeginTick(Release&& release){
        if(!std::exchange(pending_,false))return;
        release();
    }
    template<class Release> void Cleanup(Release&& release){
        if(!pending_)release();
    }
private:
    bool pending_=false;
};
}
