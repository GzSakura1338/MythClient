#include <algorithm>
#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

template<class... Args> void LogTo(const char*,Args...) {}

// Execute the production module with an in-memory game and event boundary.
namespace nova::event {
enum class EventType { Pre, Post, Incoming, Outgoing };
namespace Priority { constexpr int High=5; }
struct PacketEvent { EventType type; void* packet; };
struct GameTickEvent { EventType type; };
struct RespawnEvent {};
struct Subscription {};
class EventBus {
    template<class E> static auto& List() { static std::vector<std::function<void(E&)>> list;return list; }
public:
    static EventBus& Get() { static EventBus bus;return bus; }
    template<class E,class F> Subscription Subscribe(F f,int=10) { List<E>().push_back(f);return {}; }
    template<class E> void Post(E& e) { for(auto& f:List<E>())f(e); }
};
}
namespace nova::module {
enum class Category { Misc };
class Module {
    bool enabled_=false;
public:
    Module(const char*,Category,const char*,const char*) {}
    virtual ~Module()=default;
    void setEnabled(bool value) { if(value==enabled_)return;enabled_=value;if(value)onEnable();else onDisable(); }
private:
    virtual void onEnable() {}
    virtual void onDisable() {}
};
}
namespace nova::java {
struct Entity { std::string name;float health=20; };
struct EntityHandle {
    Entity* ptr=nullptr;
    EntityHandle(Entity* p=nullptr):ptr(p) {}
    EntityHandle(EntityHandle&&)=default;
    EntityHandle& operator=(EntityHandle&&)=default;
    EntityHandle(const EntityHandle&)=delete;
    explicit operator bool()const { return ptr!=nullptr; }
    bool IsSame(const EntityHandle& other)const { return ptr==other.ptr; }
    bool ScoreboardName(std::string& out)const { out=ptr->name;return true; }
    bool Health(float& value,float& absorption)const { value=ptr->health;absorption=0;return true; }
    bool SetHealth(float value) { ptr->health=value;return true; }
};
Entity worldA{"A"},worldB{"B"},self{"self"},alice{"alice"},bob{"bob"};
Entity* world=&worldA;
int interests=0;
bool boardReady=false;
std::unordered_map<std::string,int> board;
bool ReadStoredHealthScores(std::unordered_map<std::string,int>& out,std::string* objectives) {
    out=board;if(objectives)*objectives="health";return boardReady;
}
EntityHandle LevelEntity() { return world; }
EntityHandle PlayerEntity() { return &self; }
bool WorldPlayers(std::vector<EntityHandle>& out) { out.emplace_back(&self);out.emplace_back(&alice);out.emplace_back(&bob);return true; }
void SetIncomingPacketsWanted(bool value) { interests+=value?1:-1;assert(interests>=0); }
struct ScorePacket { std::string owner;int value;bool readable=true; };
bool ReadHealthScore(void* packet,std::string& owner,int& value) {
    const auto& p=*static_cast<ScorePacket*>(packet);owner=p.owner;value=p.value;return p.readable;
}
}

// @PRODUCTION_MODULE@

// Exercise the actual modern decoder independently of JNI.
namespace modern {
enum Binding { ClientboundSetScorePacket,ClientboundSetScorePacket_objectiveName,
    ClientboundSetScorePacket_owner,ClientboundSetScorePacket_score };
struct Packet { std::string objective,owner;int score;bool correctType=true,valid=true; };
using jobject=Packet*;
struct Frame {
    bool valid=true;
    bool instance(jobject p,Binding) { return p && p->correctType; }
    template<class T=std::string> T call(jobject p,Binding field) {
        valid=valid && p->valid;
        if constexpr(std::is_same_v<T,int>)return p->score;
        else return field==ClientboundSetScorePacket_objectiveName?p->objective:p->owner;
    }
    std::string text(std::string value) { return value; }
    explicit operator bool()const { return valid; }
    bool check()const { return valid; }
};
// @PRODUCTION_DECODER@
}

int main() {
    using namespace nova;
    auto module=module::MakeHealthBypassModule();
    auto tick=[] { event::GameTickEvent e{event::EventType::Pre};event::EventBus::Get().Post(e); };
    auto packet=[](const char* name,int score,event::EventType type=event::EventType::Incoming,bool readable=true) {
        java::ScorePacket p{name,score,readable};event::PacketEvent e{type,&p};event::EventBus::Get().Post(e);
    };
    packet("alice",4);tick();assert(java::alice.health==20);
    module->setEnabled(true);assert(java::interests==1);
    packet("self",1);packet("alice",7);packet("bob",0);tick();
    assert(java::self.health==20 && java::alice.health==7 && java::bob.health==.01f);
    packet("alice",12);packet("bob",-10);tick();
    assert(java::alice.health==12 && java::bob.health==.01f);
    packet("alice",3,event::EventType::Outgoing);packet("alice",2,event::EventType::Incoming,false);tick();
    assert(java::alice.health==12);
    module::healthbypass::SetHudEnabled(true);module::healthbypass::SetHudEnabled(true);
    assert(java::interests==1);
    module->setEnabled(false);assert(java::interests==1);
    packet("alice",9);tick();assert(java::alice.health==9);
    module::healthbypass::SetHudEnabled(false);assert(java::interests==0);
    java::alice.health=20;module->setEnabled(true);tick();assert(java::alice.health==20);
    packet("alice",6);java::world=&java::worldB;tick();assert(java::alice.health==20);
    packet("alice",11);tick();assert(java::alice.health==11);
    event::RespawnEvent respawn;event::EventBus::Get().Post(respawn);
    java::alice.health=20;tick();assert(java::alice.health==20);
    packet("alice",5);java::world=nullptr;tick();java::world=&java::worldB;tick();assert(java::alice.health==20);
    module::healthbypass::SetHudEnabled(true);module::healthbypass::SetHudEnabled(false);
    assert(java::interests==1);module->setEnabled(false);assert(java::interests==0);

    // A score received before injection/enabling must work without another packet.
    java::boardReady=true;java::board={{"alice",8},{"self",1}};
    module->setEnabled(true);tick();assert(java::alice.health==8 && java::self.health==20);
    // Existing client state is authoritative, including updates and removals.
    packet("alice",3);java::board["alice"]=14;tick();assert(java::alice.health==14);
    java::board.clear();java::alice.health=20;tick();assert(java::alice.health==20);
    java::board["bob"]=-2;tick();assert(java::bob.health==.01f);
    module->setEnabled(false);java::board["alice"]=4;tick();assert(java::alice.health==20);
    java::boardReady=false;java::board.clear();assert(java::interests==0);

    std::string owner;int value;
    for(const auto* objective:{"health","belowHealth"}) {
        modern::Packet p{objective,"alice",8};
        assert(modern::ReadHealthScore(&p,owner,value) && owner=="alice" && value==8);
    }
    for(const auto* objective:{"coins","sidebar","Health",""}) {
        modern::Packet p{objective,"alice",100};assert(!modern::ReadHealthScore(&p,owner,value));
    }
    modern::Packet malformed{"health","",5};assert(!modern::ReadHealthScore(&malformed,owner,value));
    malformed.owner="alice";malformed.valid=false;assert(!modern::ReadHealthScore(&malformed,owner,value));
    malformed.valid=true;malformed.correctType=false;assert(!modern::ReadHealthScore(&malformed,owner,value));
    assert(!modern::ReadHealthScore(nullptr,owner,value));
    std::cout<<"HealthBypass: pre-existing scoreboard, removals, score updates, source ownership, world lifecycle and objective filtering passed\n";
}
