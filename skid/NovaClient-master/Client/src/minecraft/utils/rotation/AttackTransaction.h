#pragma once

namespace nova::rotation {
// Game-thread ownership. Ordinary attacks keep their existing cadence; a leased
// attack allows exactly one native controller entry in its synchronous scope.
class AttackTransaction {
public:
    void BeginTick() { attacked_=false; submitted_=nullptr; permit_=consumed_=false; }
    bool Acquire(const void* owner) {
        if(!owner || (owner_&&owner_!=owner) || attacked_ || submitted_)return false;
        owner_=owner;return true;
    }
    void Release(const void* owner) { if(owner&&owner_==owner)owner_=nullptr; }
    bool Reserved() const {return owner_||submitted_;}
    bool Owns(const void* owner) const {return owner&&owner_==owner;}
    bool Ready(const void* owner) const {return Owns(owner)&&!attacked_&&!submitted_;}
    bool Begin(const void* owner) {
        if(!Ready(owner))return false;
        submitted_=owner;permit_=true;consumed_=false;return true;
    }
    bool CanCall() const {return !Reserved()||(permit_&&!consumed_);}
    bool EnterNative() {
        if(!CanCall())return false;
        attacked_=true;if(permit_)consumed_=true;return true;
    }
    bool End() {permit_=false;return consumed_;}
private:
    const void* owner_=nullptr;
    const void* submitted_=nullptr;
    bool attacked_=false,permit_=false,consumed_=false;
};
}
