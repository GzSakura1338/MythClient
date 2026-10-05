#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/java/api/game_entity.h"
#include "minecraft/java/api/game_skin.h"
#include "minecraft/java/api/game_presentation_name.h"
#include "minecraft/java/api/game_network.h"
#include "minecraft/java/api/game_chat.h"
#include "minecraft/java/api/game_victory.h"
#include "minecraft/modules/render/cswin/VictoryText.h"
#include "minecraft/host/presentation/PlayerPortrait.h"
#include "ui/hud/cswin/CsWin.h"
#include "ui/hud/cswin/CsWinAudio.h"
#include <chrono>
#include <mutex>
#include <random>

namespace nova::module {
namespace {
class CSWinModule final : public Module {
public:
    CSWinModule():Module("CSWin",Category::Render,"General","HUD"),
        onlyOnVictory_(addBool("Only On Victory",false)),
        music_(addMode("Music",0,{"Inhuman","EZ4ENCE","Auto","None"})),
        style_(addMode("Style",1,{"CT","T","Auto"})) {
        enableOverlay();
        tick_=event::EventBus::Get().Subscribe<event::GameTickEvent>([this](auto& e){
            if(e.type!=event::EventType::Pre)return;
            unsigned long long generation;
            {std::lock_guard lock(mutex_);if(!active_)return;generation=sessionGeneration_;}
            auto self=java::PlayerEntity();if(!self.Valid()){ResetSession();return;}
            ui::cswin::Snapshot snapshot;
            std::string identity;
            if(!self.ScoreboardName(identity))identity.clear();
            snapshot.name=identity.empty()?"Player":identity;
            snapshot.portrait=minecraft::PlayerPortrait(java::PlayerSkinTexture(self));
            std::lock_guard lock(mutex_);
            if(active_ && generation==sessionGeneration_) {
                if((!playing_ && !pending_) || !waitOnly_ || !havePortrait_) {
                    player_=std::move(snapshot);havePortrait_=true;
                }
                identity_=std::move(identity);
            }
        });
        packet_=event::EventBus::Get().Subscribe<event::PacketEvent>([this](auto& e){OnPacket(e);},event::Priority::Lowest);
        respawn_=event::EventBus::Get().Subscribe<event::RespawnEvent>([this](auto&){ResetSession();});
    }
    ~CSWinModule() override {packet_.Reset();tick_.Reset();respawn_.Reset();onDisable();}
private:
    using Clock=std::chrono::steady_clock;
    void onEnable() override {
        std::lock_guard lock(mutex_);
        player_={};identity_.clear();playing_=pending_=havePortrait_=false;++generation_;++sessionGeneration_;
        active_=true;waitOnly_=onlyOnVictory_.get();
        if(waitOnly_)java::SetIncomingPacketsWanted(true);
        else StartPlayback();
    }
    void onDisable() override {
        std::lock_guard lock(mutex_);playing_=pending_=havePortrait_=false;audio_.Stop();player_={};identity_.clear();++generation_;++sessionGeneration_;
        if(active_ && waitOnly_)java::SetIncomingPacketsWanted(false);
        active_=waitOnly_=false;
    }
    void ResetSession() {
        std::lock_guard lock(mutex_);if(!active_ || !waitOnly_)return;
        identity_.clear();++sessionGeneration_;
        // A completed round may immediately transfer to a lobby/world. Keep
        // the winning portrait, original playback deadline and audio intact.
        // An accepted win belongs to the completed round even if its first
        // overlay update occurs after the transfer. Keep queued wins as well.
        if(!playing_ && !pending_) {player_={};havePortrait_=false;}
    }
    void OnPacket(event::PacketEvent& e) {
        if(e.type!=event::EventType::Incoming || !e.packet)return;
        unsigned long long generation;std::string name;
        {std::lock_guard lock(mutex_);if(!active_ || !waitOnly_)return;generation=generation_;name=identity_;}
        if(e.IsCancelled())return;
        if(java::IsDisconnectPacket(e.packet) || java::IsRespawnPacket(e.packet) || java::IsChatSessionStart(e.packet)){ResetSession();return;}
        java::Announcement message;
        if(!java::ReadAnnouncement(e.packet,message) || !cswin::IsVictory(message.text,name))return;
        std::lock_guard lock(mutex_);
        // A title, subtitle and chat announcement for the same win start only
        // one presentation. JNI packets never survive this receive callback.
        // Explicit victory text does not depend on the next player tick or skin
        // lookup succeeding. Named-winner messages were already matched above.
        const bool finished=playing_ && std::chrono::duration<double>(Clock::now()-start_).count()>=duration_;
        if(active_ && generation==generation_ && (!playing_ || finished) && !pending_) {
            pending_=true;
        }
    }
    void onUpdate() override {
        std::lock_guard lock(mutex_);
        if(waitOnly_!=onlyOnVictory_.get()) {
            waitOnly_=onlyOnVictory_.get();java::SetIncomingPacketsWanted(waitOnly_);
            ++generation_;++sessionGeneration_;pending_=playing_=false;audio_.Stop();
            if(!waitOnly_)StartPlayback();
        }
        if(pending_) {
            pending_=false;
            // A stalled/minimized render loop must not consume a confirmed win.
            StartPlayback();
        }
        if(playing_ && std::chrono::duration<double>(Clock::now()-start_).count()>=duration_) {
            playing_=false;audio_.Stop();
            if(!waitOnly_)setEnabled(false);
        }
    }
    // Called only while holding mutex_; audio is started on the update thread.
    void StartPlayback() {
        const int style=style_.get()==2?std::uniform_int_distribution<int>(0,1)(random_):style_.get();
        playingStyle_=style==0?ui::cswin::Style::CT:ui::cswin::Style::T;
        const int choice=music_.get()==2?std::uniform_int_distribution<int>(0,1)(random_):music_.get();
        const auto track=choice==1?ui::cswin::Track::EZ4ENCE:ui::cswin::Track::Inhuman;
        if(choice==3)audio_.Stop();
        const bool started=choice!=3 && audio_.Play(track);
        duration_=started && track==ui::cswin::Track::EZ4ENCE?audio_.DurationSeconds():ui::cswin::Duration;
        start_=Clock::now();playing_=true;
    }
    void onRender() override {
        ui::cswin::Snapshot snapshot;double elapsed,duration;
        {
            std::lock_guard lock(mutex_);if(!playing_)return;
            elapsed=std::chrono::duration<double>(Clock::now()-start_).count();snapshot=player_;duration=duration_;
            snapshot.style=playingStyle_;
        }
        snapshot.name=java::PresentationName(snapshot.name);
        ui::cswin::Draw(elapsed,snapshot,duration);
    }
    std::mutex mutex_;
    Properties::BoolProperty& onlyOnVictory_;
    Properties::ModeProperty& music_;
    Properties::ModeProperty& style_;
    ui::cswin::Style playingStyle_=ui::cswin::Style::T;
    std::mt19937 random_{std::random_device{}()};
    double duration_=ui::cswin::Duration;
    bool active_=false,waitOnly_=false,playing_=false,pending_=false,havePortrait_=false;
    unsigned long long generation_=0;
    unsigned long long sessionGeneration_=0;
    Clock::time_point start_;
    ui::cswin::Snapshot player_;
    std::string identity_;
    ui::cswin::Audio audio_;
    event::Subscription tick_,packet_,respawn_;
};
}
std::unique_ptr<Module> MakeCSWinModule(){return std::make_unique<CSWinModule>();}
}
