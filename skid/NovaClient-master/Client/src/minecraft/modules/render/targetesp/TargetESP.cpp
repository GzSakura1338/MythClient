#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/modules/combat/killaura/KillAura.h"
#include "minecraft/modules/render/targetesp/RingProjection.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/java/api/game_click.h"
#include "minecraft/host/render/WorldProjection.h"
#include "ui/clickgui/panel/ClientSettings.h"
#include "ui/clickgui/panel/Theme.h"
#include <chrono>
#include <format>
#include <memory>
#include <mutex>

namespace nova::module::targetesp {
namespace {
using rotation::Vec3;
using Clock=std::chrono::steady_clock;
using ui::targetesp::Segments;
struct Target {
    int id=0;
    std::shared_ptr<java::EntityHandle> world;
};

class TargetEspModule final:public Module {
public:
    TargetEspModule():Module("TargetESP",Category::Render,"General","Overlay") {
        enableOverlay();
        sync_=&addBool("Sync Color",true);
        addSettings({Properties::PropDesc("Color",&color_,{},[this]{return !sync_->get();})});
        auto format=[](double value){return std::format("{:.2f}",value);};
        radius_=&addNumber("Radius",.1,2,.05,.75,{},format);
        alpha_=&addNumber("Alpha Factor",0,2,.05,1,{},format);
        through_=&addBool("Through Walls",true);
        tick_=event::EventBus::Get().Subscribe<event::GameTickEvent>([this](auto& e){
            if(isEnabled() && e.type==event::EventType::Pre)UpdateTarget();
        });
        respawn_=event::EventBus::Get().Subscribe<event::RespawnEvent>([this](auto&){Clear();});
    }
private:
    void Clear() { Target old; {std::lock_guard lock(mutex_);std::swap(old,target_);} }
    void onDisable() override {Clear();occlusionWorld_.reset();occlusionAt_={};}
    void onEnable() override {Clear();occlusionWorld_.reset();occlusionAt_={};}
    static bool Usable(const java::EntityHandle& entity,const java::EntityHandle& self) {
        unsigned types=0;
        return entity && !entity.IsSame(self) && java::EntityTypes(entity,types) &&
            (types&java::kTypeLiving)!=0;
    }
    void UpdateTarget() {
        auto self=java::PlayerEntity(),world=java::LevelEntity();
        if(!self || !world){Clear();return;}
        java::EntityHandle target;
        // Starain checks hover FIRST, including mobs, then the aura's live target.
        if(!java::AttackTargetEntity(target) || !Usable(target,self)) {
            target.Reset();
            if(killaura::IsEnabled()) {
                const int id=killaura::TargetId();
                if(id!=0)target=java::EntityById(id);
            }
        }
        Target fresh;
        if(Usable(target,self) && java::EntityId(target,fresh.id))
            fresh.world=std::make_shared<java::EntityHandle>(std::move(world));
        {std::lock_guard lock(mutex_);std::swap(fresh,target_);}
    }
    void onRender() override {
        if(!ImGui::GetCurrentContext())return;
        Target selected;
        {std::lock_guard lock(mutex_);selected=target_;}
        auto self=java::PlayerEntity(),world=java::LevelEntity();
        if(!self || !world || !selected.world || !selected.world->IsSame(world)) {
            occlusionWorld_.reset();occlusionAt_={};return;
        }
        auto entity=java::EntityById(selected.id);
        if(!entity || !entity.IsAlive() || entity.IsSame(self))return;
        Vec3 now,before;
        if(!entity.RenderPositionPair(now,before))return;
        const float partial=java::FrameTime();
        const auto display=ImGui::GetIO().DisplaySize;
        java::CameraView camera;
        if(!std::isfinite(partial) || display.x<=1 || display.y<=1 || !java::ReadCameraView(partial,camera))return;
        const auto basis=minecraft::projection::MakeBasis(camera,partial);
        const Vec3 position=before.Add((now.x-before.x)*partial,(now.y-before.y)*partial,(now.z-before.z)*partial);
        if(!std::isfinite(position.x)||!std::isfinite(position.y)||!std::isfinite(position.z))return;
        const auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        const auto ring=ui::targetesp::Animate(ms,radius_->get(),float(alpha_->get()));
        auto worldPoint=[&](Point p){return position.Add(p.x,p.y,p.z);};
        auto project=[&](Point point,ImVec2& screen){
            minecraft::projection::ScreenPoint result;
            if(!minecraft::projection::Project(basis,worldPoint(point),display,result) ||
                !std::isfinite(result.x)||!std::isfinite(result.y))return false;
            screen={result.x,result.y};return true;
        };
        const bool through=through_->get();
        if(through){occlusionAt_={};occlusionWorld_.reset();}
        else {
            const auto time=Clock::now();
            // No JNI work per triangle or pixel. At most 64 block traces per
            // 50ms, and none at all for the source's default Through Walls mode.
            if(!occlusionWorld_ || !occlusionWorld_->IsSame(world) || occlusionId_!=selected.id ||
                time-occlusionAt_>=std::chrono::milliseconds(50)) {
                for(int i=0;i<Segments;++i) {
                    const auto& a=ring.top[i];const auto& b=ring.bottom[i+1];
                    const auto point=worldPoint({(a.x+b.x)*.5,(a.y+b.y)*.5,(a.z+b.z)*.5});
                    java::BlockHitResultInfo hit;
                    visible_[i]=java::WorldClip(basis.eye,point,self,hit) &&
                        (hit.kind!=java::HitKind::Block ||
                         basis.eye.DistanceToSqr(hit.location)+.0001>=basis.eye.DistanceToSqr(point));
                }
                occlusionWorld_=selected.world;occlusionId_=selected.id;occlusionAt_=time;
            }
        }
        const auto tint=sync_->get()?menu::panel::ThemeColors(menu::panel::ClientSettings::Get().themeValue()).iconHighlightColor
                                    :color_.valueAbgr();
        const auto base=ImGui::ColorConvertU32ToFloat4(tint);
        const auto color=[&](float opacity){auto c=base;c.w=std::clamp(c.w*opacity,0.f,1.f);return c;};
        ui::targetesp::Mesh mesh;
        mesh.triangles.reserve(Segments*2);mesh.lines.reserve(Segments);
        for(int i=0;i<Segments;++i) {
            if(!through&&!visible_[i])continue;
            const WorldVertex bottom{ring.bottom[i],color(0)},nextBottom{ring.bottom[i+1],color(0)};
            const WorldVertex top{ring.top[i],color(.52f*ring.pulse)},nextTop{ring.top[i+1],color(.52f*ring.pulse)};
            Triangle({bottom,top,nextBottom},project,mesh);
            Triangle({top,nextBottom,nextTop},project,mesh);
            Line({ring.top[i],color(ring.pulse)},{ring.top[i+1],color(ring.pulse)},project,mesh);
        }
        ui::targetesp::Draw(mesh,ImGui::GetBackgroundDrawList(),display);
    }
    Properties::BoolProperty *sync_=nullptr,*through_=nullptr;
    Properties::NumberProp *radius_=nullptr,*alpha_=nullptr;
    Properties::ColorProperty color_{IM_COL32(255,255,255,233),true};
    std::mutex mutex_;
    Target target_;
    event::Subscription tick_,respawn_;
    Clock::time_point occlusionAt_{};
    int occlusionId_=0;
    std::shared_ptr<java::EntityHandle> occlusionWorld_;
    std::array<bool,Segments> visible_{};
};
}
}
namespace nova::module {
std::unique_ptr<Module> MakeTargetEspModule(){return std::make_unique<targetesp::TargetEspModule>();}
}
