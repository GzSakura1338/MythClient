#include "minecraft/modules/combat/displace/DisplaceLogic.h"
#include "minecraft/modules/combat/displace/DisplacePackets.h"
#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/modules/combat/killaura/KillAura.h"
#include "minecraft/modules/misc/antibot/AntiBot.h"
#include "minecraft/compat/lunar189/LunarRuntime.h"
#include "minecraft/compat/minecraft263/Minecraft263Access.h"
#include "minecraft/compat/lunar1710/Access.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/java/api/game_network.h"
#include "minecraft/java/api/game_rotation.h"
#include "minecraft/utils/rotation/RotationManager.h"
#include <format>
#include <map>
#include <sstream>
#include <tuple>

namespace nova::module {
namespace {
namespace access=compat::mc263::access;
namespace mapping {
#define NOVA_DISPLACE_MAPPING
#define NOVA_DISPLACE(id, owner, name, desc) inline constexpr access::Member id{owner,name,desc};
#define NOVA_DISPLACE_CLASS(id, name) inline constexpr const char* id=name;
#include "minecraft/compat/mappings/Lunar189.inc"
#undef NOVA_DISPLACE_CLASS
#undef NOVA_DISPLACE
#undef NOVA_DISPLACE_MAPPING
}
using displace::Target;
using displace::Vec3;
using displace::Box;

struct GameIO {
    access::Frame f;
    java::EntityHandle self=java::PlayerEntity(),world=java::LevelEntity(),target;
    jobject player=static_cast<jobject>(self.raw()),level=static_cast<jobject>(world.raw());
    jobject options=nullptr;
    std::string items;
    float serverYaw=0;
    std::map<std::tuple<int,int,int>,bool> airCache;
    explicit GameIO(std::string list,float yaw):items(std::move(list)),serverYaw(yaw) {
        if(f&&self&&world)options=f.get(f.client(),mapping::options);
    }
    bool Ready(){return f&&self&&world&&options;}
    bool Key(access::Member key){return f.call<bool>(f.get(options,key),mapping::keyDown);}
    bool Movement(){return Key(mapping::forwardKey)||Key(mapping::backKey)||Key(mapping::leftKey)||Key(mapping::rightKey);}
    bool Attacking(){return Key(mapping::attackKey)||(killaura::IsEnabled()&&killaura::TargetId()!=0);}
    bool Knockback(){
        if(!Ready())return false;
        auto member=mapping::knockback;
        if(compat::IsLunar1710()) member.descriptor="(Lnet/minecraft/entity/EntityLivingBase;Lnet/minecraft/entity/EntityLivingBase;)I";
        auto id=access::Method(f.env,member,true);
        if(!id){f.ok=false;return false;}
        const int value=compat::IsLunar1710() ? f.env->CallStaticIntMethod(access::Class(f.env,member.owner),id,player,player) : f.env->CallStaticIntMethod(access::Class(f.env,member.owner),id,player);
        return f.check()&&value>0;
    }
    void AttackKeyTick(){
        const int code=f.call<int>(f.get(options,mapping::attackKey),mapping::keyCode);
        if(!f||!code)return;
        auto id=access::Method(f.env,mapping::keyTick,true);
        if(!id){f.ok=false;return;}
        f.env->CallStaticVoidMethod(access::Class(f.env,mapping::keyTick.owner),id,code);f.check();
    }
    bool Whitelist(){
        auto stack=f.call(player,mapping::heldItem);
        auto item=stack?f.call(stack,mapping::item):nullptr;
        std::string storage;
        if(item){
            auto name=f.call(f.stat(mapping::registry),mapping::registryName,item);
            storage=f.text(f.call(name,{"java.lang.Object","toString","()Ljava/lang/String;"}));
            if(f.call<bool>(item,mapping::subtypes)&&!f.call<bool>(item,mapping::damageable)){
                int meta=f.call<int>(stack,mapping::metadata);if(meta)storage+=":"+std::to_string(meta);
            }
        }
        std::string registry=storage;
        if(const auto colon=registry.find(':',registry.find(':')+1);colon!=std::string::npos)registry.resize(colon);
        std::string list=items;std::replace(list.begin(),list.end(),',',' ');std::replace(list.begin(),list.end(),';',' ');
        std::istringstream input(list);std::string id;
        while(input>>id){
            if(id=="@fist"&&!stack)return true;
            if(!item)continue;
            if(id==storage||id==registry+":*")return true;
            const std::pair<const char*,const char*> categories[]{
                {"@category:sword",mapping::sword},{"@category:bow",mapping::bow},
                {"@category:axe",mapping::axe},{"@category:pickaxe",mapping::pickaxe},
                {"@category:shovel",mapping::shovel},{"@category:hoe",mapping::hoe},
                {"@category:shears",mapping::shears}};
            for(const auto& [token,type]:categories)if(id==token&&f.instance(item,type))return true;
            if(id=="@category:tool"&&(f.instance(item,mapping::tool)||f.instance(item,mapping::hoe)||f.instance(item,mapping::shears)))return true;
        }
        return false;
    }
    bool Trackable(int id){
        auto e=java::EntityById(id);int death=0;
        return e&&e.IsPlayer()&&!compat::lunar::EntityDead(e.raw())&&e.DeathTime(death)&&death==0;
    }
    bool Teammate(const java::EntityHandle& e){
        auto other=static_cast<jobject>(e.raw());
        if(f.call<bool>(player,mapping::sameTeam,other))return true;
        auto name=f.call(f.call(other,mapping::displayName),mapping::unformatted);
        // Java substring uses UTF-16 indices; preserve the source's short-name catch.
        if(!name||f.env->GetStringLength(static_cast<jstring>(name))<2)return false;
        auto prefix=f.call(name,{"java.lang.String","substring","(II)Ljava/lang/String;"},0,2);
        auto own=f.call(f.call(player,mapping::displayName),mapping::unformatted);
        const access::Member starts{"java.lang.String","startsWith","(Ljava/lang/String;)Z"};
        if(f.call<bool>(own,starts,prefix))return true;
        if(compat::IsLunar1710()) return false; // 1.7 tab entries contain names/ping, no team profile.
        auto connection=f.call(f.client(),mapping::connection);
        auto info=f.call(connection,mapping::playerInfo,f.call(player,mapping::uuid));
        if(!info)return false;
        auto profile=f.call(info,mapping::profile);
        auto profileName=f.call(profile,{"com.mojang.authlib.GameProfile","getName","()Ljava/lang/String;"});
        auto formatted=f.staticCall(mapping::formatName,f.call(info,mapping::team),profileName);
        return f.call<bool>(formatted,starts,prefix);
    }
    std::optional<Target> Closest(bool ignoreTeams){
        std::vector<java::EntityHandle> players;if(!java::WorldPlayers(players))return {};
        const auto eye=Eye();double closest=std::numeric_limits<double>::max();std::optional<Target> result;
        for(auto& e:players){
            int death=0,id=0;Box box;
            if(e.IsSame(self)||compat::lunar::EntityDead(e.raw())||!e.DeathTime(death)||death||antibot::IsBot(e))continue;
            // Nova has no friend list; the empty Raven friend list rejects nobody.
            access::Frame locals(f.env);
            if(ignoreTeams&&Teammate(e))continue;
            if(!e.BoundingBox(box)||!java::EntityId(e,id))continue;
            double border=0;if(!e.PickRadius(border))continue;
            const double distance=eye.DistanceToSqr(std::clamp(eye.x,box.minX-border,box.maxX+border),std::clamp(eye.y,box.minY-border,box.maxY+border),std::clamp(eye.z,box.minZ-border,box.maxZ+border));
            if(distance>9.||distance>=closest)continue;
            Target t;t.id=id;t.box=box;
            if(!e.Position(t.position)||!e.EyeHeight(t.eyeHeight))continue;
            t.width=f.get<float>(static_cast<jobject>(e.raw()),mapping::width);
            closest=distance;result=t;target=java::AdoptEntity(e.raw());
        }
        return result;
    }
    Vec3 SelfPosition(){Vec3 v;self.Position(v);return v;}
    Vec3 Eye(){Vec3 v;self.EyePosition(1,v);return v;}
    float ServerYaw(){return serverYaw;}
    bool Air(int x,int y,int z){
        const auto key=std::tuple{x,y,z};if(auto it=airCache.find(key);it!=airCache.end())return it->second;
        access::Frame locals(f.env);
        const bool air=compat::IsLunar1710()?f.call<bool>(level,compat::lunar1710::names::Air,x,y,z):f.call<bool>(level,mapping::air,f.create(mapping::pos,x,y,z));
        return airCache[key]=f&&air;
    }
    bool Collision(int, const Box& b){
        access::Frame scope(f.env);
        auto box=f.create(mapping::box,b.minX,b.minY,b.minZ,b.maxX,b.maxY,b.maxZ);
        auto collisions=f.create({"java.util.ArrayList","<init>","()V"});
        for(int x=int(std::floor(b.minX));x<int(std::floor(b.maxX+1));++x)
            for(int z=int(std::floor(b.minZ));z<int(std::floor(b.maxZ+1));++z){
                access::Frame column(f.env);
                if(compat::IsLunar1710() ? !f.call<bool>(level,compat::lunar1710::names::Loaded,x,64,z) : !f.call<bool>(level,mapping::loaded,f.create(mapping::pos,x,64,z)))return true;
                for(int y=int(std::floor(b.minY));y<int(std::floor(b.maxY+1));++y){
                    if(y<0||y>=256)return true;
                    access::Frame cell(f.env);
                    if(compat::IsLunar1710()) {
                        auto block=compat::lunar1710::Block(f,level,x,y,z);
                        f.invoke(block,compat::lunar1710::names::AddCollisions,level,x,y,z,box,collisions,static_cast<jobject>(target.raw()));
                    } else {
                        auto pos=f.create(mapping::pos,x,y,z),state=f.call(level,mapping::state,pos),block=f.call(state,mapping::block);
                        f.invoke(block,mapping::collisions,level,pos,state,box,collisions,static_cast<jobject>(target.raw()));
                    }
                    if(!f||!f.call<bool>(collisions,{"java.util.ArrayList","isEmpty","()Z"}))return true;
                }
            }
        return !f;
    }
};

class DisplaceModule final:public Module {
public:
    DisplaceModule():Module("KBDisplace",Category::Combat,"General","General"){
        mode_=&addMode("Dynamic angle",0,{"Static","Dynamic"});
        auto fixed=[this]{return mode_->get()==0;};
        offset_=&addNumber("Yaw offset",0,180,1,90,{},[](double v){return std::format("{:.0f}",v);},fixed);
        delay_=&addNumber("Delay",0,500,50,0,{},[](double v){return std::format("{:.0f} ms",v);});
        direction_=&addMode("Direction",0,{"Left","Right"},{},fixed);
        findVoid_=&addBool("Find void",false,{},fixed);
        blink_=&addBool("Blink",false);
        ignoreTeams_=&addBool("Ignore teammates",true);
        hasKB_=&addBool("Has knockback",false);
        whitelist_=&addBool("Item whitelist",false);
        addSettings({Properties::PropDesc("Whitelisted items",&items_,{},[this]{return whitelist_->get();})});
        auto& bus=event::EventBus::Get();
        subscriptions_.push_back(bus.Subscribe<event::GameTickEvent>([this](auto& e){
            if(e.type!=event::EventType::Pre||!isEnabled())return;
            if(state_.releaseBlinkNextTick){displace::EndBlink();state_.releaseBlinkNextTick=false;}
            calculated_=rotating_=false;picking_=0;
            auto player=java::PlayerEntity(),world=java::LevelEntity();
            if(!player||!world||(world_&&!world_.IsSame(world))||(player_&&!player_.IsSame(player))){Reset(false);}
            player_=std::move(player);world_=std::move(world);
        },event::Priority::Highest));
        subscriptions_.push_back(bus.Subscribe<event::ClientRotationEvent>([this](auto&){Update();},event::Priority::Lowest));
        subscriptions_.push_back(bus.Subscribe<event::CursorPickEvent>([this](auto& e){
            if(e.type==event::EventType::Pre)++picking_;else picking_=std::max(0,picking_-1);
        }));
        subscriptions_.push_back(bus.Subscribe<event::PlayerTickEvent>([this](auto&){Update();},event::Priority::Highest));
        subscriptions_.push_back(bus.Subscribe<event::MoveInputEvent>([this](auto& e){
            if(!rotating_)return;
            float yaw=0,pitch=0;if(!java::PlayerRotation(yaw,pitch))return;
            const float scale=e.sneak?.3f:1.f;
            float forward=e.forward*scale,strafe=e.strafe*scale;
            displace::FixInput(forward,strafe,yaw_,yaw,e.sneak);
            e.forward=forward/scale;e.strafe=strafe/scale;e.skipRotationFix=true;
        },event::Priority::High-1));
        subscriptions_.push_back(bus.Subscribe<event::MoveInputEvent>([this](auto& e){
            if(!isEnabled()||!compat::IsLegacyLunar())return;
            const float scale=e.sneak?static_cast<float>(e.sneakSlowDownMultiplier):1.f;
            float forward=e.hasFinalForward?e.finalForward:e.forward*scale;
            float strafe=e.hasFinalStrafe?e.finalStrafe:e.strafe*scale;
            GameIO io({},0);state_.Input(forward,strafe,io.Ready()&&io.Movement());
            if(state_.active){e.hasFinalForward=e.hasFinalStrafe=true;e.finalForward=forward;e.finalStrafe=strafe;}
        },event::Priority::Lowest+1));
        subscriptions_.push_back(bus.Subscribe<event::MotionEvent>([this](auto& e){
            if(e.type!=event::EventType::Pre)return;
            if(rotating_){e.yaw=yaw_;e.pitch=pitch_;}
            lastYaw_=e.yaw;lastPitch_=e.pitch;haveLast_=true;
        },event::Priority::Lowest+1));
        subscriptions_.push_back(bus.Subscribe<event::RayTraceEvent>([this](auto& e){
            if(!rotating_||!picking_)return;auto p=java::PlayerEntity();JniAttach a;
            if(p&&a&&a.env->IsSameObject(static_cast<jobject>(p.raw()),e.entity)){e.yaw=yaw_;e.pitch=pitch_;}
        },event::Priority::Lowest+1));
        subscriptions_.push_back(bus.Subscribe<event::StrafeEvent>([this](auto& e){if(rotating_)e.yaw=yaw_;},event::Priority::Lowest+1));
        subscriptions_.push_back(bus.Subscribe<event::JumpEvent>([this](auto& e){if(rotating_)e.yaw=yaw_;},event::Priority::Lowest+1));
        subscriptions_.push_back(bus.Subscribe<event::PacketEvent>([this](auto& e){
            if(!isEnabled()||!compat::IsLegacyLunar())return;
            if(e.type==event::EventType::Incoming){
                if(java::IsDisconnectPacket(e.packet)||java::IsRespawnPacket(e.packet)||
                   compat::lunar::IsPacket(e.packet,compat::lunar::PacketKind::Join)||java::IsPlayerPositionPacket(e.packet))Reset(false);
            }else if(!e.IsCancelled()&&blink_->get()&&state_.active&&state_.displacing&&!state_.releaseBlinkNextTick&&
                compat::lunar::IsPacket(e.packet,compat::lunar::PacketKind::MovePlayer)){
                displace::BeginBlink();state_.releaseBlinkNextTick=true;
            }
        },event::Priority::High));
        subscriptions_.push_back(bus.Subscribe<event::RespawnEvent>([this](auto&){Reset(false);}));
    }
    ~DisplaceModule()override{if(isEnabled())java::SetIncomingPacketsWanted(false);displace::EndBlink(false);}
    std::string suffix()const override{return std::format("{:.0f}ms",delay_->get());}
private:
    void Reset(bool send){displace::EndBlink(send);state_={};rotating_=calculated_=haveLast_=false;picking_=0;player_.Reset();world_.Reset();}
    void onEnable()override{Reset(true);java::SetIncomingPacketsWanted(true);}
    void onDisable()override{java::SetIncomingPacketsWanted(false);Reset(true);}
    void Update(){
        if(!isEnabled()||!compat::IsLegacyLunar()||calculated_)return;
        calculated_=true;rotating_=false;
        float playerYaw=0,playerPitch=0;if(!java::PlayerRotation(playerYaw,playerPitch)){state_.ClearActive();return;}
        if(!haveLast_){lastYaw_=playerYaw;lastPitch_=playerPitch;compat::lunar::PlayerReportedRotation(lastYaw_,lastPitch_);haveLast_=true;}
        GameIO io(items_.get(),lastYaw_);
        displace::Settings s;
        s.dynamic=mode_->get()==1;s.yawOffset=static_cast<float>(offset_->get());s.delay=delay_->get();s.left=direction_->get()==0;
        s.findVoid=findVoid_->get();s.blink=blink_->get();s.ignoreTeammates=ignoreTeams_->get();s.requireKnockback=hasKB_->get();s.whitelist=whitelist_->get();
        state_.Rotation(io,s);
        if(!io.f){state_.ClearActive();return;}
        if(!state_.displacing||!state_.yaw)return;
        double sensitivity=.5;java::PlayerSensitivity(sensitivity);
        yaw_=displace::FixYaw(*state_.yaw,lastYaw_,static_cast<float>(sensitivity));
        pitch_=rotation::IsActive()?rotation::GetRotation().pitch:playerPitch;
        const float base=static_cast<float>(sensitivity)*.6f+.2f;
        const double gcd=static_cast<double>(base*base*base)*1.2;
        pitch_=std::clamp(lastPitch_+static_cast<float>(std::floor((pitch_-lastPitch_)/gcd+.5)*gcd),-90.f,90.f);
        rotating_=yaw_!=playerYaw||pitch_!=playerPitch;
    }
    displace::State state_;
    java::EntityHandle player_,world_;
    bool calculated_=false,rotating_=false,haveLast_=false;
    float yaw_=0,pitch_=0,lastYaw_=0,lastPitch_=0;
    int picking_=0;
    Properties::ModeProperty *mode_=nullptr,*direction_=nullptr;
    Properties::NumberProp *offset_=nullptr,*delay_=nullptr;
    Properties::BoolProperty *findVoid_=nullptr,*blink_=nullptr,*ignoreTeams_=nullptr,*hasKB_=nullptr,*whitelist_=nullptr;
    Properties::StringProperty items_;
    std::vector<event::Subscription> subscriptions_;
};
}
std::unique_ptr<Module> MakeDisplaceModule(){return std::make_unique<DisplaceModule>();}
}
