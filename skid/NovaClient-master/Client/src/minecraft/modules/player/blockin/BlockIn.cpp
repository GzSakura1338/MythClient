#include "minecraft/modules/player/blockin/BlockIn.h"
#include "minecraft/modules/player/blockin/BlockInLogic.h"
#include "minecraft/modules/player/breaker/Breaker.h"
#include "minecraft/modules/Module.h"
#include "minecraft/modules/player/automlg/AutoMLG.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/compat/lunar1710/Access.h"
#include "minecraft/compat/lunar189/LunarRuntime.h"
#include "minecraft/compat/minecraft263/Minecraft263Access.h"
#include "bind/PhysicalKey.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/java/api/game_click.h"
#include "minecraft/java/api/game_inventory.h"
#include "minecraft/java/api/game_place.h"
#include "minecraft/java/api/game_player_list.h"
#include "minecraft/java/api/game_rotation.h"
#include "ui/clickgui/ClickGui.h"
#include "ui/hud/blockin/BlockInHud.h"
#include "utils/Keyboard.h"
#include "minecraft/utils/rotation/RotationManager.h"
#include "imgui.h"
#include <atomic>
#include <format>
#include <random>
#include <sstream>

namespace nova::module::blockin {
namespace {
std::atomic<bool> placing{false};
namespace access=compat::mc263::access;
namespace lunar=compat::lunar;
namespace names {
#define NOVA_BLOCKIN_MAPPING
#define NOVA_BLOCKIN(id, owner, name, desc) inline constexpr access::Member id{owner,name,desc};
#include "minecraft/compat/mappings/Lunar189.inc"
#undef NOVA_BLOCKIN
#undef NOVA_BLOCKIN_MAPPING
}
jobject Position(access::Frame& f,Pos p) {
    if(!f) return nullptr;
    auto cls=access::Class(f.env,names::posCtor.owner);
    auto ctor=access::Method(f.env,names::posCtor);
    if(!cls || !ctor) { f.ok=false; return nullptr; }
    auto pos=f.env->NewObject(cls,ctor,p.x,p.y,p.z); f.check(); return pos;
}
bool Ignored(const std::string& list,const std::string& id,int meta) {
    std::string clean=list;
    std::replace(clean.begin(),clean.end(),',',' ');
    std::replace(clean.begin(),clean.end(),';',' ');
    std::istringstream stream(clean); std::string token;
    while(stream>>token) {
        if(!token.starts_with("minecraft:")) token="minecraft:"+token;
        if(token==id+":*" || token==id+":"+std::to_string(meta) || (meta==0 && token==id)) return true;
    }
    return false;
}
struct World {
    const java::EntityHandle& self;
    Vec3 eye;
    std::mt19937& random;
    const std::string ignored;
    bool ok=true;
    std::array<Stack,9> stacks{};
    std::array<bool,9> scanned{};
    double Random() { return std::generate_canonical<double,53>(random); }
    Cell Read(Pos p) {
        access::Frame f;
        const bool v17=compat::IsLunar1710();
        auto world=f.get(f.client(),names::level),pos=v17?nullptr:Position(f,p);
        auto block=v17?compat::lunar1710::Block(f,world,p.x,p.y,p.z):f.call(f.call(world,names::state,pos),names::block);
        Cell c;
        c.replaceable=v17?f.call<bool>(f.call(block,compat::lunar1710::names::Material),compat::lunar1710::names::Replaceable):f.call<bool>(block,names::replaceable,world,pos);
        c.air=f.instance(block,"net.minecraft.block.BlockAir");
        if(c.replaceable) {ok &= bool(f) && block!=nullptr;return c;}
        // Raven's isInteractable plus the two roof-specific exclusions.
        for(const char* type:{"BlockTrapDoor","BlockDoor","BlockContainer","BlockJukebox","BlockFenceGate",
            "BlockChest","BlockEnderChest","BlockEnchantmentTable","BlockBrewingStand","BlockBed","BlockDropper",
            "BlockDispenser","BlockHopper","BlockAnvil","BlockNote","BlockWorkbench","BlockFence","BlockWall"}) {
            const auto name=std::string("net.minecraft.block.")+type;
            if(f.instance(block,name.c_str())) { c.roofExcluded=true; break; }
        }
        ok &= bool(f) && block!=nullptr;
        return c;
    }
    Stack Item(int slot) {
        if(scanned[slot]) return stacks[slot];
        scanned[slot]=true;
        access::Frame f; jobject stack=nullptr;
        if(!f || !java::InventorySlot(slot,stack)) { ok=false; return {}; }
        if(!stack) return {};
        auto item=f.call(stack,names::item);
        if(f.get<int>(stack,names::count)<=0 || !f.instance(item,names::itemBlock.owner)) return {};
        if(!ignored.empty()) {
            auto registry=f.stat(names::registry);
            auto id=f.text(f.call(f.call(registry,names::registryName,item),names::toString));
            if(Ignored(ignored,id,f.call<int>(stack,names::meta))) return {};
        }
        const bool v17=compat::IsLunar1710();
        auto block=v17?f.get(item,compat::lunar1710::names::ItemBlock):f.call(item,names::itemBlock),world=f.get(f.client(),names::level);
        float hardness=v17?f.call<float>(block,compat::lunar1710::names::Hardness,world,0,0,0):f.call<float>(block,names::hardness,world,jobject(nullptr));
        bool noTool=f.call<bool>(f.call(block,names::material),names::noTool);
        ok &= bool(f);
        if(!f) return {};
        return stacks[slot]={true,hardness<0?std::numeric_limits<float>::max():hardness*(noTool?30.f:100.f)};
    }
    bool CanPlace(int slot,Pos p,int side) {
        if(side<0||side>5) return false;
        access::Frame f; jobject stack=nullptr;
        if(!f || !java::InventorySlot(slot,stack) || !stack) return false;
        auto item=f.call(stack,names::item);
        if(!f.instance(item,names::itemBlock.owner)) return false;
        if(compat::IsLunar1710()) {
            const bool result=f.call<bool>(item,compat::lunar1710::names::CanPlace,f.get(f.client(),names::level),p.x,p.y,p.z,side,self.raw(),stack);
            ok &= bool(f); return result;
        }
        auto cls=access::Class(f.env,names::faces.owner);
        auto method=access::Method(f.env,names::faces,true);
        if(!f || !cls || !method) {ok=false;return false;}
        auto faces=static_cast<jobjectArray>(f.env->CallStaticObjectMethod(cls,method));
        if(!f.check()||!faces) {ok=false;return false;}
        auto face=f.env->GetObjectArrayElement(faces,side);
        bool result=f.call<bool>(item,names::canPlace,f.get(f.client(),names::level),Position(f,p),face,self.raw(),stack);
        ok &= bool(f); return result;
    }
    std::optional<Hit> Trace(Rotation r) {
        constexpr float pi=3.1415927f,rad=pi/180.f;
        float horizontal=-rotation::mth::Cos(-r.pitch*rad);
        Vec3 look{rotation::mth::Sin(-r.yaw*rad-pi)*horizontal,rotation::mth::Sin(-r.pitch*rad),
                  rotation::mth::Cos(-r.yaw*rad-pi)*horizontal};
        java::BlockHitResultInfo hit;
        if(!java::WorldClip(eye,eye.Add(look.x*4.5,look.y*4.5,look.z*4.5),self,hit)) {ok=false;return {};}
        if(hit.kind!=java::HitKind::Block || !hit.hasBlockPos || hit.direction<0 || hit.direction>5) return {};
        return Hit{{hit.blockPos.x,hit.blockPos.y,hit.blockPos.z},hit.direction,hit.location};
    }
    std::optional<Vec3> ClosestPlayer() {
        std::vector<java::EntityHandle> players; Vec3 position;
        if(!java::WorldPlayers(players)||!self.Position(position)) return {};
        double best=100; std::optional<Vec3> closest; // Raven argument is squared distance.
        for(auto& p:players) {
            Vec3 at;
            if(p.IsSame(self)||!p.Position(at)||!java::OnlinePlayerListContains(p).value_or(false)) continue;
            double d=position.DistanceToSqr(at);
            if(d<best) {best=d;closest=at;}
        }
        return closest;
    }
};

class BlockInModule final:public Module {
public:
    BlockInModule():Module("BlockIn",Category::Player,"General","General"),key_(0,true) {
        speed_=&addNumber("Speed",1,30,1,10,{},[](double v){return std::format("{:.0f}",v);});
        randomization_=&addNumber("Randomization",0,100,1,10,{},[](double v){return std::format("{:.0f}%",v);});
        tolerance_=&addNumber("Rotation Tolerance",20,100,1,25,{},[](double v){return std::format("{:.0f} deg",v);});
        key_.setGate([]{return false;});
        addSettings({Properties::PropDesc("Select Keybind",&key_)});
        ignore_=&addBool("Ignore blocks",false,{Properties::PropDesc("Items",&ignored_)});
        enableOverlay();
        auto& bus=event::EventBus::Get();
        tick_=bus.Subscribe<event::GameTickEvent>([this](auto& e){
            if(e.type==event::EventType::Pre && !Allowed()) Reset();
        });
        rotation_=bus.Subscribe<event::ClientRotationEvent>([this](auto&){Select();},event::Priority::Low);
        interact_=bus.Subscribe<event::PrePlayerInteractEvent>([this](auto&){Place();},event::Priority::High);
    }
private:
    bool Allowed() {
        return isEnabled() && !automlg::IsWorking() && compat::IsLegacyLunar() && !java::ScreenOpen() && !clickgui::IsOpen() &&
            input::IsProcessForeground() && bind::IsPhysicalBindingDown(key_.keybind());
    }
    void ReleaseRotation() {
        const auto& s=rotation::State();
        if(ownsRotation_ && s.direct && s.rotations.yaw==applied_.yaw && s.rotations.pitch==applied_.pitch)
            rotation::ReleaseDirectRotations();
        ownsRotation_=false;
    }
    void Reset() {
        bool was=placing.exchange(false);
        queued_.reset(); filled_.store(0);
        if(!was && savedSlot_<0 && !ownsRotation_) return;
        auto self=java::PlayerEntity(),world=java::LevelEntity();
        bool same=self && world && player_ && world_ && self.IsSame(player_) && world.IsSame(world_);
        int slot=-1;
        if(same && savedSlot_>=0 && java::SelectedSlot(slot) && slot==selectedSlot_) java::SetSelectedSlot(savedSlot_);
        if(was && same && !java::ScreenOpen()) {
            lunar::SetAttackDown(lunar::PhysicalMouseDown(0));
            lunar::SetUseKeyDown(lunar::PhysicalUseDown());
        }
        savedSlot_=selectedSlot_=-1;
        ReleaseRotation(); player_.Reset(); world_.Reset();
    }
    void onDisable() override { reset_.store(true); }
    void Select() {
        if(reset_.exchange(false)) Reset();
        if(!Allowed() || breaker::OwnsMining()) { Reset();return; }
        auto self=java::PlayerEntity(),world=java::LevelEntity();
        if(!self || !world || !self.IsAlive() || self.IsSpectator()) {Reset();return;}
        if(player_ && (!self.IsSame(player_) || !world.IsSame(world_))) Reset();
        Vec3 position,eye; float yaw=0,pitch=0;
        if(!self.Position(position)||!self.EyePosition(1,eye)||!java::PlayerRotation(yaw,pitch)) {Reset();return;}
        const auto validPosition=[](Vec3 p) {
            return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z) &&
                std::abs(p.x)<30000001 && std::abs(p.z)<30000001 && std::abs(p.y)<30000001;
        };
        if(!validPosition(position)||!validPosition(eye)||!std::isfinite(yaw)||!std::isfinite(pitch)) {Reset();return;}
        const auto& state=rotation::State();
        Rotation previous{yaw,pitch};
        if(state.hasLastRotations) previous=state.hasAnimation?state.animationRotation:state.lastRotations;
        else lunar::PlayerReportedRotation(previous.yaw,previous.pitch);
        Rotation base=state.active&&!ownsRotation_?rotation::GetRotation():previous;
        if(!std::isfinite(previous.yaw)||!std::isfinite(previous.pitch)||!std::isfinite(base.yaw)||!std::isfinite(base.pitch)) {Reset();return;}
        World io{self,eye,random_,ignore_->get()?ignored_.get():std::string{}};
        Planner planner(io,position,eye,previous);
        auto choice=planner.Select();
        int fill=planner.Filled();
        if(!io.ok || !choice) {Reset();filled_.store(io.ok?fill:0);return;}
        if(!placing.load()) {
            if(!java::SelectedSlot(savedSlot_)) {Reset();return;}
            player_=java::PlayerEntity(); world_=java::LevelEntity();
            placing.store(true);
        }
        access::Frame f;
        auto options=f.get(f.client(),names::options);
        bool userAction=f.call<bool>(f.get(options,names::attack),names::down) ||
            f.call<bool>(f.get(options,names::use),names::down);
        lunar::SetAttackDown(false); lunar::SetUseKeyDown(false); lunar::ClearCombatClicks();
        if(!f || !java::SetSelectedSlot(choice->slot)) {Reset();return;}
        selectedSlot_=choice->slot; queued_.reset(); filled_.store(fill);
        if(userAction) {ReleaseRotation();return;}
        auto smooth=Smooth(base,choice->aim.rotation,int(speed_->get()),float(randomization_->get()),[&]{return io.Random();});
        auto hit=io.Trace(smooth);
        bool queue=QueuePlacement(choice->aim,smooth,previous,tolerance_->get(),hit);
        double sensitivity=0;
        if(!io.ok || !java::PlayerSensitivity(sensitivity)) {Reset();return;}
        applied_=Fix(smooth,previous,sensitivity);
        if(!rotation::SetDirectRotations(applied_)) {Reset();return;}
        applied_=rotation::State().rotations;
        ownsRotation_=true; rotation::SetMoveInputFix(true);
        if(queue) queued_=hit;
    }
    void Place() {
        if(!Allowed() || breaker::OwnsMining()) {Reset();return;}
        // Mouse polling follows ClientRotation and can enqueue fresh vanilla clicks.
        // Consume them before runTick reaches its attack/use loops, even while turning.
        if(placing.load()) lunar::ClearCombatClicks();
        if(!queued_) return;
        auto hit=std::exchange(queued_,{});
        auto self=java::PlayerEntity(),world=java::LevelEntity(); int slot=-1;
        if(!placing.load()||!self ||!world ||!self.IsSame(player_)||!world.IsSame(world_)||
           !java::SelectedSlot(slot)||slot!=selectedSlot_) {Reset();return;}
        const auto& rotations=rotation::State();
        if(!rotations.direct || rotations.rotations.yaw!=applied_.yaw || rotations.rotations.pitch!=applied_.pitch) return;
        if(java::UseItemOnBlock({hit->block.x,hit->block.y,hit->block.z},hit->side,hit->point)) java::SwingMainHand();
    }
    void onRender() override {
        if(!ImGui::GetCurrentContext())return;
        const int fill=filled_.load();
        view_.Draw(fill,fill>0 && Allowed(),static_cast<double>(GetTickCount64()));
    }
    bind::BindProperty key_;
    Properties::StringProperty ignored_;
    Properties::NumberProp *speed_,*randomization_,*tolerance_;
    Properties::BoolProperty* ignore_;
    event::Subscription tick_,rotation_,interact_;
    java::EntityHandle player_,world_;
    int savedSlot_=-1,selectedSlot_=-1;
    bool ownsRotation_=false;
    Rotation applied_;
    std::optional<Hit> queued_;
    std::atomic<bool> reset_{false};
    std::atomic<int> filled_{0};
    ui::BlockInHud view_;
    std::mt19937 random_{std::random_device{}()};
};
}
bool IsPlacing() {return placing.load(std::memory_order_acquire);}
std::unique_ptr<Module> MakeModule() {return std::make_unique<BlockInModule>();}
}
namespace nova::module {
std::unique_ptr<Module> MakeBlockInModule() {return blockin::MakeModule();}
}
