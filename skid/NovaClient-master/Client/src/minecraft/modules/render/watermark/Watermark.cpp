#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <deque>
#include <format>
#include <mutex>
#include "minecraft/events/GameEvents.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/java/api/game_entity.h"
#include "minecraft/java/api/game_hud.h"
#include "minecraft/java/api/game_presentation_name.h"
#include "minecraft/java/api/game_network.h"
#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "ui/hud/watermark/WatermarkHud.h"

namespace nova::module {
namespace {
class WatermarkModule final : public Module {
public:
    WatermarkModule():Module("Watermark",Category::Render,"General","HUD") {
        enableOverlay();
        addSettings(view_.Options().Rows());
        tick_=event::EventBus::Get().Subscribe<event::GameTickEvent>([this](auto& e) {
            if(isEnabled() && e.type==event::EventType::Pre)SampleGame();
        });
        packet_=event::EventBus::Get().Subscribe<event::PacketEvent>([this](auto& e) {
            if(!listening_.load() || e.type!=event::EventType::Incoming)return;
            if(java::IsDisconnectPacket(e.packet) || java::IsRespawnPacket(e.packet)) {
                std::lock_guard lock(dataMutex_);
                samples_.clear();data_={};lastGameSample_=0;
                return;
            }
            long long ticks;
            if(!java::ReadHudServerTime(e.packet,ticks))return;
            const double now=Seconds();
            std::lock_guard lock(dataMutex_);
            if(!samples_.empty() && (ticks<=samples_.back().second || now-samples_.back().first>10))samples_.clear();
            samples_.emplace_back(now,ticks);
            while(samples_.size()>6)samples_.pop_front();
        },event::Priority::Highest-1);
    }
    ~WatermarkModule() override {
        tick_.Reset();packet_.Reset();
        if(listening_.exchange(false))java::SetIncomingPacketsWanted(false);
    }
private:
    bool renderWhenDisabled() const override {return true;}
    static double Seconds() {
        return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }
    void onEnable() override {
        {std::lock_guard lock(dataMutex_);data_={};samples_.clear();lastGameSample_=0;}
        if(!listening_.exchange(true))java::SetIncomingPacketsWanted(true);
    }
    void onDisable() override {
        if(listening_.exchange(false))java::SetIncomingPacketsWanted(false);
    }
    void SampleGame() {
        const double now=Seconds();
        std::lock_guard lock(dataMutex_);
        if(now-lastGameSample_<.25)return;
        const auto self=java::PlayerEntity();
        if(!self) {data_={};samples_.clear();lastGameSample_=0;return;}
        const int tick=java::PlayerTickCount();
        if(tick<lastPlayerTick_) {samples_.clear();lastGameSample_=0;}
        lastPlayerTick_=tick;
        self.ScoreboardName(data_.player);
        data_.player=java::PresentationName(data_.player);
        const auto info=java::ReadHudInfo();
        if(info.server!=data_.server) {samples_.clear();lastGameSample_=0;}
        data_.server=info.server;data_.ping=info.ping;
        rotation::Vec3 pos;
        if(self.Position(pos)) {
            data_.coordinates=std::format("{:.0f} / {:.0f} / {:.0f}",std::floor(pos.x),std::floor(pos.y),std::floor(pos.z));
            const double distance=std::hypot(pos.x-lastPosition_.x,pos.z-lastPosition_.z);
            data_.speed=lastGameSample_>0 && now>lastGameSample_?distance/(now-lastGameSample_):-1;
            if(data_.speed>100)data_.speed=-1;
            lastPosition_=pos;
        } else {data_.coordinates="--";data_.speed=-1;}
        lastGameSample_=now;
    }
    void onRender() override {
        ui::watermark::Snapshot data;
        {
            std::lock_guard lock(dataMutex_);
            data=data_;
            if(samples_.size()>=2 && Seconds()-samples_.back().first<=10) {
                const double elapsed=samples_.back().first-samples_.front().first;
                if(elapsed>.05)data.tps=std::clamp((samples_.back().second-samples_.front().second)/elapsed,0.,20.);
            }
        }
        data.version=compat::IsNeoForge12110()?"1.21.10":compat::IsLunar12111()?"1.21.11":compat::IsEaseCation()?"EaseCation":compat::IsLegacyLunar()?"1.8.9":compat::IsMinecraft263()?"26.3":"1.20.1";
        view_.Draw(isEnabled(),data);
    }
    ui::watermark::WatermarkHud view_;
    event::Subscription tick_,packet_;
    std::atomic<bool> listening_{false};
    std::mutex dataMutex_;
    ui::watermark::Snapshot data_;
    std::deque<std::pair<double,long long>> samples_;
    double lastGameSample_=0;
    int lastPlayerTick_=0;
    rotation::Vec3 lastPosition_;
};
}
std::unique_ptr<Module> MakeWatermarkModule() {return std::make_unique<WatermarkModule>();}
}
