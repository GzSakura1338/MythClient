#include <algorithm>
#include "minecraft/events/GameEvents.h"
#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/modules/combat/killaura/KillAura.h"
#include "minecraft/modules/misc/healthbypass/HealthBypass.h"
#include "minecraft/java/api/game_click.h"
#include "minecraft/java/api/game_entity.h"
#include "minecraft/java/api/game_skin.h"
#include "minecraft/java/api/game_presentation_name.h"
#include "ui/clickgui/ClickGui.h"
#include "ui/overlay/OverlaySpin.h"
#include "ui/hud/targethud/TargetHud.h"
#include "minecraft/host/presentation/PlayerPortrait.h"

namespace nova::module {
namespace {
using ui::ScopedSpin;
using ui::SpinLock;
using ui::targethud::Snapshot;

class TargetHudModule final : public Module {
public:
    TargetHudModule():Module("TargetHUD",Category::Render,"General","Overlay") {
        enableOverlay();
        target_=&addMode("Target",0,{"Auto","Hover","KillAura"});
        players_=&addBool("Players",true);
        auto rows=view_.Options().Rows();
        addSettings({rows.front()}); // Fade retains its original position in old configs.
        multiTarget_=&addBool("Multi Target",true);
        bypass_=&addBool("Health Bypass",false);
        rows.erase(rows.begin());addSettings(std::move(rows));
        gameTick_=event::EventBus::Get().Subscribe<event::GameTickEvent>([this](auto& e){OnGameTick(e);});
    }
private:
    void onEnable() override {
        view_.RestartTiming();
        ScopedSpin lock(snapshotMutex_);snapshots_.clear();
    }
    void onDisable() override {
        {ScopedSpin lock(snapshotMutex_);snapshots_.clear();}
        healthbypass::SetHudEnabled(false);
    }
    void OnGameTick(event::GameTickEvent& e) {
        if(!isEnabled() || e.type!=event::EventType::Pre)return;
        healthbypass::SetHudEnabled(bypass_->get());
        if(bypass_->get())healthbypass::Apply();
        // JNI stays outside the lock and on its original game tick.
        std::vector<Snapshot> fresh;GatherTargets(fresh);
        ScopedSpin lock(snapshotMutex_);snapshots_=std::move(fresh);
    }
    void GatherTargets(std::vector<Snapshot>& out) {
        if(clickgui::IsOpen()) {
            const auto self=java::PlayerEntity();Snapshot snap;
            if(self.Valid() && Fill(self,snap))out.push_back(std::move(snap));
            return;
        }
        if(multiTarget_->get()) {
            std::vector<int> ids;
            if(killaura::SwitchTargets(ids))for(int id:ids) {
                auto entity=java::EntityById(id);Snapshot snap;
                if(entity.Valid() && Usable(entity) && Fill(entity,snap))out.push_back(std::move(snap));
            }
        }
        if(!out.empty())return;
        auto target=PickTarget();Snapshot snap;
        if(target.Valid() && Fill(target,snap))out.push_back(std::move(snap));
    }
    java::EntityHandle PickTarget() {
        const int source=target_->get();
        if(source==0 || source==1) {
            java::EntityHandle hovered;
            if(java::AttackTargetEntity(hovered) && Usable(hovered))return hovered;
        }
        if(source==0 || source==2) {
            const int id=killaura::TargetId();
            if(id!=0) {
                auto aura=java::EntityById(id);
                if(aura.Valid() && Usable(aura))return aura;
            }
        }
        return {};
    }
    bool Usable(const java::EntityHandle& entity) {
        if(!entity.IsAlive())return false;
        unsigned types=0;
        if(!java::EntityTypes(entity,types) || (types&java::kTypeLiving)==0)return false;
        return !players_->get() || (types&java::kTypePlayer)!=0;
    }
    static bool Fill(const java::EntityHandle& entity,Snapshot& out) {
        int id=0;if(java::EntityId(entity,id))out.id=id;
        float health=0,absorption=0;
        if(!entity.Health(health,absorption))return false;
        out.health=health;out.absorption=absorption;
        float maximum=0;
        if(!entity.MaxHealth(maximum) || maximum<=0)maximum=20;
        out.maxHealth=maximum;
        if(!entity.DisplayName(out.name) || out.name.empty())out.name="?";
        out.name=java::PresentationName(out.name);
        int hurtTime=0;if(entity.HurtTime(hurtTime))out.hurtTime=hurtTime;
        auto self=java::PlayerEntity();double distance=0;
        if(self.Valid() && !entity.IsSame(self) && entity.DistanceTo(self,distance))out.distance=static_cast<float>(distance);
        out.portrait=minecraft::PlayerPortrait(java::PlayerSkinTexture(entity));
        out.valid=true;return true;
    }
    void onRender() override {
        std::vector<Snapshot> fresh;
        {ScopedSpin lock(snapshotMutex_);fresh=snapshots_;}
        view_.Draw(clickgui::IsOpen(),fresh);
    }
    ui::targethud::TargetHud view_;
    Properties::ModeProperty* target_=nullptr;
    Properties::BoolProperty *players_=nullptr,*multiTarget_=nullptr,*bypass_=nullptr;
    event::Subscription gameTick_;
    SpinLock snapshotMutex_;
    std::vector<Snapshot> snapshots_;
};
}
std::unique_ptr<Module> MakeTargetHudModule() {return std::make_unique<TargetHudModule>();}
}
