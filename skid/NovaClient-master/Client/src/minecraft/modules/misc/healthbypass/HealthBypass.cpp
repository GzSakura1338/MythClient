#include "minecraft/modules/misc/healthbypass/HealthBypass.h"
#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/java/api/game_entity.h"
#include "minecraft/java/api/game_fakeplayer.h"
#include "minecraft/java/api/game_network.h"
#include "minecraft/java/api/game_score.h"
#include <algorithm>
#include <chrono>
#include <mutex>
#include <unordered_map>

namespace nova::module {
namespace {
struct Score {
    int value;
    std::shared_ptr<java::EntityHandle> world;
};
class HealthBypassModule;
HealthBypassModule* instance = nullptr;

class HealthBypassModule final : public Module {
public:
    HealthBypassModule() : Module("HealthBypass",Category::Misc,"General","General") {
        instance=this;
        auto& bus=event::EventBus::Get();
        packet_=bus.Subscribe<event::PacketEvent>([this](auto& e){Packet(e);});
        tick_=bus.Subscribe<event::GameTickEvent>([this](auto& e){
            if(e.type==event::EventType::Pre) Apply();
        },event::Priority::High);
        respawn_=bus.Subscribe<event::RespawnEvent>([this](auto&){Clear();});
    }
    void Hud(bool enabled) { SetSource(2,enabled); }
    void Apply() {
        std::unordered_map<std::string,Score> snapshot;
        bool report=false;unsigned incoming=0,accepted=0;
        {std::lock_guard lock(mutex_);
            if(!sources_)return;
            snapshot=scores_;
            const auto now=std::chrono::steady_clock::now();
            if(reportsLeft_ && now>=nextReport_){
                --reportsLeft_;nextReport_=now+std::chrono::seconds(5);report=true;
                incoming=incoming_;accepted=accepted_;
            }
        }
        auto world=java::LevelEntity(), self=java::PlayerEntity();
        if(!world || !self) {Clear();return;}
        std::unordered_map<std::string,int> stored;
        std::string objectives;
        const bool boardReady=java::ReadStoredHealthScores(stored,report?&objectives:nullptr);
        std::vector<java::EntityHandle> players;
        if(!java::WorldPlayers(players))return;
        unsigned matched=0,written=0;
        for(auto& player:players) {
            if(player.IsSame(self)||java::IsFakePlayer(player.raw()))continue;
            std::string name;
            if(!player.ScoreboardName(name))continue;
            int value;
            if(boardReady){
                const auto score=stored.find(name);
                if(score==stored.end())continue;
                value=score->second;
            }else{
                auto score=snapshot.find(name);
                if(score==snapshot.end() || !score->second.world->IsSame(world))continue;
                value=score->second.value;
            }
            ++matched;
            const float corrected=std::max(.01f,static_cast<float>(value));
            float health=0,absorption=0;
            // Lunar emits an extra health-change event on every setHealth call.
            if(player.Health(health,absorption) && health==corrected)++written;
            else if(player.SetHealth(corrected))++written;
        }
        if(report)LogTo("HealthBypass: board=%d objectives=[%s] stored=%zu incoming=%u accepted=%u cached=%zu players=%zu matched=%u applied=%u",
            boardReady,objectives.c_str(),stored.size(),incoming,accepted,snapshot.size(),players.size(),matched,written);
        // Release old-world references outside the lock. Late network callbacks
        // cannot apply stale health to a same-named player on a different server.
        std::unordered_map<std::string,Score> expired;
        std::vector<std::string> stale;
        for(const auto& [name,score]:snapshot)
            if(!score.world->IsSame(world))stale.push_back(name);
        {std::lock_guard lock(mutex_);
            for(const auto& name:stale) {
                auto current=scores_.find(name);
                if(current!=scores_.end() && current->second.world==snapshot.at(name).world)
                    expired.insert(scores_.extract(current));
            }
        }
    }
private:
    void onEnable() override {SetSource(1,true);}
    void onDisable() override {SetSource(1,false);}
    void SetSource(unsigned source,bool enabled) {
        std::unordered_map<std::string,Score> discarded;
        {std::lock_guard lock(mutex_);
            const bool was=sources_!=0;
            if(enabled)sources_|=source;else sources_&=~source;
            const bool want=sources_!=0;
            if(was!=want)java::SetIncomingPacketsWanted(want);
            if(want && !was){reportsLeft_=4;nextReport_={};incoming_=accepted_=0;}
            if(!want){scores_.swap(discarded);++generation_;}
        }
    }
    void Clear() {
        std::unordered_map<std::string,Score> discarded;
        {std::lock_guard lock(mutex_);scores_.swap(discarded);++generation_;}
    }
    void Packet(event::PacketEvent& e) {
        if(e.type!=event::EventType::Incoming || !e.packet)return;
        unsigned generation;
        {std::lock_guard lock(mutex_);if(!sources_)return;generation=generation_;++incoming_;}
        std::string owner;int value=0;
        if(!java::ReadHealthScore(e.packet,owner,value))return;
        auto world=java::LevelEntity(), self=java::PlayerEntity();
        std::string name;
        if(!world || !self || !self.ScoreboardName(name) || owner==name)return;
        Score replacement{value,std::make_shared<java::EntityHandle>(std::move(world))};
        {std::lock_guard lock(mutex_);
            if(!sources_ || generation!=generation_)return;
            ++accepted_;
            auto [it,inserted]=scores_.try_emplace(owner,replacement);
            if(!inserted)std::swap(it->second,replacement);
        }
    }
    std::mutex mutex_;
    unsigned sources_=0,generation_=0;
    unsigned reportsLeft_=0,incoming_=0,accepted_=0;
    std::chrono::steady_clock::time_point nextReport_{};
    std::unordered_map<std::string,Score> scores_;
    event::Subscription packet_,tick_,respawn_;
};
}
namespace healthbypass {
void SetHudEnabled(bool enabled){if(instance)instance->Hud(enabled);}
void Apply(){if(instance)instance->Apply();}
}
std::unique_ptr<Module> MakeHealthBypassModule(){return std::make_unique<HealthBypassModule>();}
}
