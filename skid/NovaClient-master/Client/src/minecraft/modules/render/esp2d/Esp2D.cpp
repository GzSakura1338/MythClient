#include "minecraft/modules/render/esp2d/Esp2DLogic.h"
#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/modules/misc/antibot/AntiBot.h"
#include "minecraft/modules/misc/teams/Teams.h"
#include "minecraft/compat/minecraft263/Minecraft263Access.h"
#include "minecraft/compat/lunar1710/Access.h"
#include "minecraft/java/api/game_entity.h"
#include "minecraft/java/api/game_rotation.h"
#include "minecraft/host/render/WorldProjection.h"

namespace nova::module::esp2d {
namespace {
namespace access=compat::mc263::access;
struct Mapping {
    const char* armorStand;
    access::Member armor{},width{},height{},sneaking{},view{},options{},perspective{},scaled{},window{},scale{};
};
#define NOVA_ESP2D_MAPPING
#define NOVA_ESP2D_CLASS(name) .armorStand=name,
#define NOVA_ESP2D(key,owner,name,desc) .key={owner,name,desc},
constexpr Mapping lunar{
#include "minecraft/compat/mappings/Lunar189.inc"
};
constexpr Mapping forge{
#include "minecraft/compat/mappings/forge1201.inc"
};
constexpr Mapping modern{
#include "minecraft/compat/mappings/minecraft263.inc"
};
#undef NOVA_ESP2D
#undef NOVA_ESP2D_CLASS
#undef NOVA_ESP2D_MAPPING
constexpr access::Member ordinal{"java.lang.Enum","ordinal","()I"};

class Esp2DModule final : public Module {
public:
    Esp2DModule():Module("2DESP",Category::Render,"General","Overlay") {
        enableOverlay();
        box_=&addMode("Box",0,{"Normal","CS","None"});
        health_=&addMode("Health Bar Mode",0,{"Health","Standard","Gradient","None"});
        armor_=&addBool("Armor Bar",false);
        addSettings({Properties::PropDesc("Targets",nullptr,{
            {"Player",&players_},{"Invisibles",&invisibles_},{"Animals",&animals_},
            {"Mobs",&mobs_},{"Player Teammates",&teammates_}})});
    }
private:
    void onRender() override {
        if(!ImGui::GetCurrentContext()) return;
        const ImVec2 display=ImGui::GetIO().DisplaySize;
        if(display.x<=1 || display.y<=1) return;
        auto self=java::PlayerEntity();
        if(!self) return;
        const float partial=java::FrameTime();
        java::CameraView view;
        if(!std::isfinite(partial)||!java::ReadCameraView(partial,view)) return;
        const auto basis=minecraft::projection::MakeBasis(view,partial);
        const Mapping& n=compat::IsLegacyLunar()?lunar:compat::UsesModernRuntime()?modern:forge;
        access::Frame frame;
        auto client=frame.client();
        auto camera=compat::IsLunar1710()?compat::lunar1710::View(frame):frame.call(client,n.view);
        auto options=frame.get(client,n.options);
        int perspective=compat::IsLegacyLunar()?frame.get<int>(options,n.perspective):
            frame.call<int>(frame.call(options,n.perspective),ordinal);
        double guiScale=1;
        if(compat::IsLegacyLunar()) guiScale=frame.call<int>(compat::IsLunar1710()?compat::lunar1710::Scaled(frame,n.scaled,client):frame.create(n.scaled,client),n.scale);
        else {
            auto window=frame.call(client,n.window);
            guiScale=compat::UsesModernRuntime()?frame.call<int>(window,n.scale):frame.call<double>(window,n.scale);
        }
        if(!frame || !camera || !std::isfinite(guiScale)||guiScale<=0) return;
        // Rise's half-pixel lines are defined in the game's GUI coordinate system.
        const float scale=float(guiScale)*(view.windowWidth>0?display.x/view.windowWidth:1.f);
        if(!std::isfinite(scale)||scale<=0) return;
        const Targets targets{players_.get(),invisibles_.get(),animals_.get(),mobs_.get(),teammates_.get()};
        std::vector<java::EntityHandle> entities;
        // Collect living entities here; the target filter below requires an
        // explicitly enabled type rather than admitting unclassified creatures.
        rotation::Vec3 position;
        if(!self.Position(position)) return;
        int chunks=0;
        const double range=java::RenderDistance(chunks)&&chunks>0?(double(chunks)+1)*16:512;
        java::WorldEntities(self,{position.x-range,position.y-range,position.z-range,
            position.x+range,position.y+range,position.z+range},entities);
        // Loaded player enumeration avoids spatial-query limits on distant players.
        entities.erase(std::remove_if(entities.begin(),entities.end(),[](const auto& e){return e.IsPlayer();}),entities.end());
        std::vector<java::EntityHandle> players;
        java::WorldPlayers(players);
        for(auto& entity:players) if(!entity.IsSame(self)) entities.push_back(std::move(entity));
        if(perspective!=0) entities.push_back(java::PlayerEntity());
        for(const auto& entity:entities) {
            if(!entity || !entity.IsAlive()) continue;
            const bool isSelf=entity.IsSame(self);
            if(!isSelf && frame.env->IsSameObject(static_cast<jobject>(entity.raw()),camera)) continue;
            if(isSelf && perspective==0) continue;
            unsigned types=0;
            if(!java::EntityTypes(entity,types)||(types&java::kTypeLiving)==0) continue;
            access::Frame f;
            auto object=static_cast<jobject>(entity.raw());
            if(!compat::IsLunar1710() && f.instance(object,n.armorStand)) continue;
            bool invisible=false;
            if(!entity.IsInvisible(invisible)) continue;
            if(!isSelf) {
                const bool isPlayer=(types&java::kTypePlayer)!=0;
                bool teammate=false;
                if(isPlayer && !targets.teammates) {
                    entity.IsAlliedTo(self,teammate);
                    teammate=teammate||teams::IsTeammate(self,entity);
                }
                if(!Accept(targets,isPlayer,(types&java::kTypeAnimal)!=0,
                        (types&(java::kTypeMonster|java::kTypeVillager))!=0,invisible,teammate) || antibot::IsBot(entity)) continue;
            } else if(invisible && !targets.invisibles) continue;
            Vec3 now,before;
            if(!entity.RenderPositionPair(now,before)) continue;
            float width=compat::IsLegacyLunar()?f.get<float>(object,n.width):f.call<float>(object,n.width);
            float height=compat::IsLegacyLunar()?f.get<float>(object,n.height):f.call<float>(object,n.height);
            bool sneaking=f.call<bool>(object,n.sneaking);
            if(!f) continue;
            auto bounds=ProjectBounds(now,before,width,height,sneaking,partial,display.x,display.y,
                [&](Vec3 p,double& x,double& y) {
                    minecraft::projection::ScreenPoint screen;
                    if(!minecraft::projection::Project(basis,p,display,screen)) return false;
                    x=screen.x;y=screen.y;return true;
                });
            if(!bounds) continue;
            float health=0,absorption=0,maximum=0;
            if(health_->get()!=int(HealthMode::None)) {entity.Health(health,absorption);entity.MaxHealth(maximum);}
            int armor=armor_->get()?f.call<int>(object,n.armor):0;
            if(!f) continue;
            auto b=*bounds;b.left/=scale;b.top/=scale;b.right/=scale;b.bottom/=scale;
            ui::esp2d::Draw(b,scale,BoxMode(box_->get()),HealthMode(health_->get()),health,maximum,armor,armor_->get());
        }
    }
    Properties::ModeProperty *box_,*health_;
    Properties::BoolProperty* armor_;
    Properties::BoolProperty players_{true},invisibles_{false},animals_{false},mobs_{false},teammates_{true};
};
}
std::unique_ptr<Module> MakeModule() {return std::make_unique<Esp2DModule>();}
}
namespace nova::module {
std::unique_ptr<Module> MakeEsp2DModule() {return esp2d::MakeModule();}
}
