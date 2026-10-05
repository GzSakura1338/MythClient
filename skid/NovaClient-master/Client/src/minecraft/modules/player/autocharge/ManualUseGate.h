#pragma once
#include <atomic>
namespace nova::module::autocharge {
// Stop/cleanup intentionally do not release this gate mid-tick.
class ManualUseGate {
public:
    void Begin(){blocked_.store(true);}
    bool Blocks()const{return blocked_.load();}
    bool BeginTick(bool working){return blocked_.exchange(working)&&!working;}
    bool Reset(){return blocked_.exchange(false);}
private:
    std::atomic<bool> blocked_{false};
};
}
