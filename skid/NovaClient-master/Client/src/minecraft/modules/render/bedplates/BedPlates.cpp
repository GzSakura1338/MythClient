#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/events/EventBus.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/java/api/game_beds.h"
#include "minecraft/java/api/game_chest.h"
#include "ui/overlay/OverlaySpin.h"
#include "minecraft/host/render/WorldProjection.h"
#include "ui/hud/bedplates/BedPlateHud.h"
#include <atomic>
#include <format>

namespace nova::module {
namespace {
class BedPlates final : public Module {
public:
    BedPlates() : Module("BedPlates",Category::Render,"General","Overlay") {
        enableOverlay();
        range_=&addNumber("Range",20,200,10,200,{},[](double v){return std::format("{:.0f} blocks",v);});
        refresh_=&addNumber("Refresh Ticks",1,20,1,1,{},[](double v){return std::format("{:.0f}",v);});
        size_=&addNumber("Size",60,180,5,100,{},[](double v){return std::format("{:.0f}%",v);});
        own_=&addBool("Whitelist Own Bed",true);
        minimal_=&addBool("Minimal",false);
        distance_=&addBool("Show Distance",true);
        distanceScale_=&addBool("Distance Scale",true);
        shadow_=&addBool("Text Shadow",false);
        motion_=event::EventBus::Get().Subscribe<event::MotionEvent>([this](auto& e){
            if(e.type!=event::EventType::Pre) return;
            ui::ScopedSpin scannerLock(scannerLock_);
            if(reset_.exchange(false)) scanner_.Reset();
            if(!isEnabled()) return;
            auto self=java::PlayerEntity(); rotation::Vec3 player; std::vector<java::BedPlate> next;
            if(self && self.Position(player)) scanner_.Tick(player,static_cast<int>(range_->get()),static_cast<int>(refresh_->get()),next);
            else scanner_.Reset();
            ui::ScopedSpin lock(lock_); plates_=std::move(next);
        });
        respawn_=event::EventBus::Get().Subscribe<event::RespawnEvent>([this](auto&){reset_.store(true);Clear();});
    }
private:
    void Clear() { ui::ScopedSpin lock(lock_); plates_.clear(); }
    void onDisable() override { ui::ScopedSpin scannerLock(scannerLock_); scanner_.Reset(); Clear(); }
    void onEnable() override { reset_.store(true); }
    void onRender() override {
        if(!ImGui::GetCurrentContext() || !ImGui::GetFont()) return;
        java::CameraView view; const float partial=java::FrameTime();
        if(!java::ReadCameraView(partial,view)) { Clear(); return; }
        std::vector<java::BedPlate> plates;
        {ui::ScopedSpin lock(lock_);plates=plates_;}
        const auto basis=minecraft::projection::MakeBasis(view,partial); const auto display=ImGui::GetIO().DisplaySize;
        auto distance=[&](const java::BedPlate& p){return std::sqrt((p.center.x-view.x)*(p.center.x-view.x)+(p.center.y-view.y)*(p.center.y-view.y)+(p.center.z-view.z)*(p.center.z-view.z));};
        std::stable_sort(plates.begin(),plates.end(),[&](const auto& a,const auto& b){return distance(a)>distance(b);});
        for(const auto& plate:plates) {
            if(own_->get() && plate.own) continue;
            const double dist=distance(plate); if(dist>range_->get()+2) continue;
            minecraft::projection::ScreenPoint screen;
            if(!minecraft::projection::Project(basis,{plate.center.x,plate.center.y+.9,plate.center.z},display,screen)) continue;
            ui::DrawBedPlate({{screen.x,screen.y},plate.material,dist,plate.covered},
                {static_cast<float>(size_->get()/100),minimal_->get(),distance_->get(),distanceScale_->get(),shadow_->get()});
        }
    }
    Properties::NumberProp *range_{},*refresh_{},*size_{};
    Properties::BoolProperty *own_{},*minimal_{},*distance_{},*distanceScale_{},*shadow_{};
    java::BedScanner scanner_;
    ui::SpinLock scannerLock_;
    ui::SpinLock lock_;
    std::vector<java::BedPlate> plates_;
    std::atomic<bool> reset_{false};
    event::Subscription motion_,respawn_;
};
}
std::unique_ptr<Module> MakeBedPlatesModule() { return std::make_unique<BedPlates>(); }
}
