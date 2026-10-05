#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/java/api/game_freecam.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/java/api/game_entity.h"
#include "minecraft/host/render/WorldProjection.h"
#include "ui/hud/world/ProjectedBox.h"
#include <format>
namespace nova::module {
namespace {
class FreeCam final:public Module {
public:
    FreeCam():Module("FreeCam",Category::Render,"General","General") {
        enableOverlay();
        speed_=&addNumber("Speed",.5,10,.5,2.5,{},[](double n){return std::format("{:.1f}",n);});
        damage_=&addBool("Disable on damage",true);
        digging_=&addBool("Allow digging",false);
        interacting_=&addBool("Allow interacting",false);
        placing_=&addBool("Allow placing",false);
        arm_=&addBool("Show arm",false);
        auto& bus=event::EventBus::Get();
        tick_=bus.Subscribe<event::GameTickEvent>([this](auto& e){
            if(e.type==event::EventType::Post&&isEnabled()){
                Publish();if(!java::UpdateFreeCam())setEnabled(false);
            }
        },event::Priority::Highest);
        frame_=bus.Subscribe<event::RenderFrameEvent>([](auto&){java::RenderFreeCam();},event::Priority::Highest);
    }
    ~FreeCam() override{frame_.Reset();tick_.Reset();java::StopFreeCam();}
private:
    void Publish(){java::ConfigureFreeCam({isEnabled(),speed_->get(),damage_->get(),arm_->get(),digging_->get(),interacting_->get(),placing_->get()});}
    void onEnable() override{Publish();}
    void onUpdate() override{Publish();}
    void onDisable() override{java::StopFreeCam();}
    void onRender() override{
        if(!java::FreeCamActive())return;
        auto player=java::PlayerEntity();java::Box box;java::CameraView view;
        const float partial=java::FrameTime();
        if(!player||!player.BoundingBox(box)||!java::ReadCameraView(partial,view))return;
        rotation::Vec3 now,before;
        if(!player.PositionPair(now,before))return;
        const auto shift=before.Subtract(now).Scale(1-partial);
        auto basis=minecraft::projection::MakeBasis(view,partial);
        ui::ProjectedBox corners;
        for(int i=0;i<8;++i){
            minecraft::projection::ScreenPoint point;
            rotation::Vec3 p{(i&1?box.maxX+.1:box.minX-.1)+shift.x,(i&2?box.maxY+.1:box.minY-.1)+shift.y,(i&4?box.maxZ+.1:box.minZ-.1)+shift.z};
            if(!minecraft::projection::Project(basis,p,ImGui::GetIO().DisplaySize,point))return;
            corners[i]={point.x,point.y};
        }
        auto* draw=ImGui::GetBackgroundDrawList();
        ui::DrawFilledBox(draw,corners,IM_COL32(0,255,0,64));
        for(int i=0;i<8;++i)for(int bit:{1,2,4})if(!(i&bit))draw->AddLine(corners[i],corners[i|bit],IM_COL32(0,255,0,255),2);
    }
    Properties::NumberProp* speed_{};
    Properties::BoolProperty *damage_{},*digging_{},*interacting_{},*placing_{},*arm_{};
    event::Subscription frame_,tick_;
};
}
std::unique_ptr<Module> MakeFreeCamModule(){return std::make_unique<FreeCam>();}
}
