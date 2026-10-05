#include "minecraft/java/api/game_anchor.h"
#include "minecraft/java/api/game_blockplace.h"
#include "minecraft/modules/combat/shared/BlockAim.h"
#include "minecraft/modules/combat/shared/HotbarSession.h"
namespace mock {
int charges=-1,placed=0,charged=0,detonated=0;
bool explodes=true,glowstone=true,offhandGlowstone=false,accept=true;
bool slowReads=false;
float targetYaw=0;bool yawReadable=true;
std::vector<nova::java::BlockPos> shields;
int shieldUses=0,glowCount=64;bool shieldAccept=true,shieldEffective=true,shieldSupport=true;
}
namespace nova::java {
int AnchorGlowstoneCount(){return mock::glowstone?mock::glowCount:0;}
bool ReadGlowstone(const BlockPos& p,bool& out){out=std::any_of(mock::shields.begin(),mock::shields.end(),[&](auto b){return mock::Same(b,p);});return true;}
bool GlowstoneSupport(const BlockPos& p){return mock::shieldSupport&&CrystalBaseSupport(p);}
bool PlaceGlowstone(const BlockPos& p,const BlockHitResultInfo& hit,int slot,const void* owner,bool& entered){
    assert(mock::owner==owner&&mock::slotOwner==owner&&!mock::used&&slot==5&&slot==(mock::spoof<0?mock::selected:mock::spoof));
    assert(GlowstoneSupport(hit.blockPos)&&mock::glowCount>0);
    entered=mock::used=true;++mock::shieldUses;
    if(mock::shieldAccept){mock::shields.push_back(p);--mock::glowCount;
        for(auto& b:mock::bases)if(mock::Same(p,b.pos))b.built=true;}
    return mock::shieldAccept;
}
bool AnchorTargetYaw(const EntityHandle&,float& out){out=mock::targetYaw;return mock::yawReadable;}
bool KeyShiftDown(){return false;}
bool ReadAnchor(const BlockPos& p,AnchorState& out){
    if(mock::slowReads)std::this_thread::sleep_for(std::chrono::milliseconds(1));
    out={};
    bool shield=false;ReadGlowstone(p,shield);if(shield)return true;
    if(mock::Same(p,mock::base)){out.anchor=mock::charges>=0;out.charges=std::max(0,mock::charges);out.replaceable=!out.anchor;return true;}
    if(auto b=mock::FindBase(p)){out.anchor=b->built;out.charges=out.anchor?1:0;out.replaceable=!out.anchor;return true;}
    return true;
}
bool AnchorExplodes(const BlockPos&,bool& out){out=mock::explodes;return true;}
bool AnchorBuildable(const BlockPos& p){AnchorState s;ReadAnchor(p,s);return s.replaceable&&PlacementEntitiesClear(p,true);}
bool AnchorSlot(int slot,AnchorItem kind){return kind==AnchorItem::Anchor?mock::stock&&slot==4:kind==AnchorItem::Glowstone?mock::glowstone&&slot==5:slot==0&&!mock::offhandGlowstone;}
bool AnchorExposure(const EntityHandle& e,const BlockPos& p,const Box&,double& out){
    if(mock::Id(e)==1&&!mock::shields.empty()&&mock::shieldEffective){out=0;return true;}
    return CrystalExposure(e,{p.x+.5,double(p.y+1),p.z+.5},out);
}
bool AnchorExposureWithShield(const EntityHandle& e,const BlockPos& p,const Box& box,const BlockPos* shield,double& out){
    if(shield&&mock::Id(e)==1&&mock::shieldEffective){out=0;return true;}
    return AnchorExposure(e,p,box,out);
}
bool UseAnchor(const BlockPos& p,const BlockHitResultInfo&,int slot,AnchorAction action,const void* owner,bool& entered){
    assert(!mock::used&&mock::owner==owner&&mock::slotOwner==owner&&slot==(mock::spoof<0?mock::selected:mock::spoof));
    assert(mock::explodes);entered=mock::used=true;mock::lastPlaced=p;
    if(action==AnchorAction::Place){assert(slot==4&&mock::charges==-1);++mock::placed;if(mock::accept){mock::charges=0;mock::baseExists=true;}}
    if(action==AnchorAction::Charge){assert(slot==5&&mock::charges==0);++mock::charged;if(mock::accept)mock::charges=1;}
    if(action==AnchorAction::Detonate){assert(slot==0&&mock::charges>0&&!mock::offhandGlowstone);++mock::detonated;if(mock::accept){mock::charges=-1;mock::baseExists=false;}}
    if(mock::accept)for(auto& b:mock::bases)if(mock::Same(b.pos,p))b.built=mock::charges>=0;
    return mock::accept;
}
}
// @PRODUCTION_MODULE@
void Tick(){mock::used=mock::motion=mock::locked=false;mock::attack.BeginTick();nova::event::Post<nova::event::GameTickEvent>();nova::event::Post<nova::event::PrePlayerInteractEvent>();
    nova::event::Post(nova::event::MotionEvent{nova::event::EventType::Post,mock::angle.yaw,mock::angle.pitch});}
auto Start(){
    using namespace mock;
    world=2;selected=0;oldIdentity=100;owner=slotOwner=nullptr;spoof=-1;yieldable=physicalLease=false;attack={};
    target=true;team=bot=teammate=wall=rescue=screen=selfDanger=crystal=dropOnAim=false;stock=supported=defenseOK=syncOK=true;
    targetX=3.4;targetY=64;targetZ=1;selfPosition={0,64,0};selfVelocity=observedVelocity={};
    selfGrounded=targetGrounded=true;charges=-1;placed=charged=detonated=0;explodes=glowstone=accept=true;offhandGlowstone=false;
    baseExists=false;supportExists=true;bases.clear();quantized=false;fallingWorld=false;slowReads=false;targetYaw=0;yawReadable=true;
    shields.clear();shieldUses=0;glowCount=64;shieldAccept=shieldEffective=shieldSupport=true;
    angle=nova::rotation::Calculate(selfPosition.Add(0,1.62,0),{2.5,63,.5});
    auto m=nova::module::MakeLegitAnchorAuraModule();m->Number("Target.FOV",360);m->Number("Rotation.Speed",180);m->Enable();return m;
}
int main(){
    using namespace nova::module;
    assert(crystal::Damage(0,1,2,0,0,-1,0,5)==71);
    assert(crystal::Damage(0,1,2,0,0,-1,0)==85);
    for(bool silent:{false,true}){
        auto m=Start();m->Bool("Place.Silent",silent);
        for(int i=0;i<100&&mock::detonated<2;++i)Tick();
        assert(mock::placed==2&&mock::charged==2&&mock::detonated==2);
        if(silent)assert(mock::selected==0);m->Disable();Tick();assert(mock::selected==0&&mock::slotOwner==nullptr);
    }
    {auto m=Start();mock::charges=1;mock::baseExists=true;mock::stock=mock::glowstone=false;
     for(int i=0;i<100&&mock::detonated==0;++i)Tick();assert(mock::detonated==1&&mock::placed==0&&mock::charged==0);}
    {auto m=Start();mock::charges=0;mock::baseExists=true;mock::stock=false;
     for(int i=0;i<100&&mock::detonated==0;++i)Tick();assert(mock::charged==1&&mock::detonated==1&&mock::placed==0);}
    {auto m=Start();m->Bool("Swing",false);const int swings=mock::swings,packets=mock::swingPackets;
     for(int i=0;i<100&&mock::detonated==0;++i)Tick();assert(mock::detonated==1&&mock::swings==swings&&mock::swingPackets==packets+3);}
    {auto m=Start();m->Number("Place.Delay",1000);
     for(int i=0;i<100&&mock::detonated==0;++i)Tick();assert(mock::detonated==1);
     for(int i=0;i<20;++i)Tick();assert(mock::placed==1);}
    {auto m=Start();m->Number("Rotation.Speed",1);mock::angle={};
     for(int i=0;i<12;++i)Tick();assert(mock::placed==0&&explosive::anchorWorking);}
    // Smooth rotation must converge from any initial look under mouse quantization.
    for(float yaw:{-179.f,-90.f,0.f,90.f,179.f}){
        auto m=Start();m->Number("Rotation.Speed",45);mock::angle={yaw,-70};mock::quantized=true;
        for(int i=0;i<120&&mock::detonated==0;++i)Tick();assert(mock::detonated==1);
    }
    // Slow JNI-like reads plus ordinary movement must not starve a full scan.
    {auto m=Start();mock::slowReads=true;
     for(int i=0;i<500&&mock::placed==0;++i){
         mock::selfPosition={.55*std::sin(i*.3),64,.55*std::cos(i*.3)};Tick();
     }assert(mock::placed==1);mock::slowReads=false;}
    {auto m=Start();mock::wall=true;for(int i=0;i<30;++i)Tick();assert(mock::placed==0);
     m->Bool("Target.Through Walls",true);for(int i=0;i<100&&mock::detonated==0;++i)Tick();assert(mock::detonated==1);}
    {auto m=Start();m->Bool("Safety.Ignore Self Damage",true);mock::teammate=true;
     for(int i=0;i<30;++i)Tick();assert(mock::placed==0);}
    {auto m=Start();mock::accept=false;for(int i=0;i<100&&mock::placed==0;++i)Tick();
     mock::world=8;Tick();assert(mock::slotOwner==nullptr&&!explosive::anchorWorking);}
    for(int reason=0;reason<5;++reason){auto m=Start();
        if(reason==0)mock::explodes=false;if(reason==1)mock::glowstone=false;if(reason==2)mock::offhandGlowstone=true;
        if(reason==3)mock::selfDanger=true;if(reason==4)mock::rescue=true;
        for(int i=0;i<40;++i)Tick();assert(mock::placed==0&&mock::charged==0&&mock::detonated==0&&mock::selected==0);
    }
    {auto m=Start();mock::accept=false;for(int i=0;i<100&&mock::placed==0;++i)Tick();assert(mock::placed==1);
     for(int i=0;i<8;++i)Tick();assert(mock::placed==1&&mock::charged==0);
     std::this_thread::sleep_for(std::chrono::milliseconds(310));Tick();assert(mock::placed==1&&mock::selected==0);}
    {auto m=Start();m->Bool("Detonate",false);mock::glowstone=false;
     for(int i=0;i<100&&mock::placed==0;++i)Tick();assert(mock::placed==1);
     for(int i=0;i<10;++i)Tick();assert(mock::charged==0&&mock::detonated==0&&mock::selected==0);}
    {auto m=Start();m->Bool("Place",false);for(int i=0;i<40;++i)Tick();assert(mock::placed==0);}
    {auto m=Start();m->Bool("Place.Auto Switch",false);for(int i=0;i<40;++i)Tick();assert(mock::placed==0);}
    {auto m=Start();m->Bool("Place.Auto Switch",false);mock::selected=4;
     for(int i=0;i<100&&mock::placed==0;++i)Tick();assert(mock::placed==1&&mock::selected==4);
     for(int i=0;i<3;++i)Tick();mock::selected=5;
     for(int i=0;i<100&&mock::charged==0;++i)Tick();assert(mock::charged==1&&mock::selected==5);
     for(int i=0;i<3;++i)Tick();mock::selected=0;
     for(int i=0;i<100&&mock::detonated==0;++i)Tick();assert(mock::detonated==1);}
    {auto m=Start();m->Bool("Safety.Ignore Self Damage",true);mock::selfDanger=true;
     for(int i=0;i<100&&mock::placed==0;++i)Tick();assert(mock::placed==1);}
    {auto m=Start();for(int i=0;i<100&&mock::placed==0;++i)Tick();mock::target=false;Tick();assert(mock::selected==0&&!explosive::anchorWorking);}
    {auto m=Start();for(int i=0;i<100&&mock::placed==0;++i)Tick();mock::selected=7;Tick();assert(mock::selected==7);}
    // Mio returns the first usable existing anchor, even if a new position
    // found earlier has much greater target damage.
    {auto m=Start();mock::charges=1;mock::baseExists=true;m->Bool("Target.Through Walls",true);
     mock::bases={{{-3,63,0},false,1},{{2,63,0},true,.5}};
     for(int i=0;i<100&&mock::detonated==0;++i)Tick();assert(mock::detonated==1&&mock::placed==0&&mock::lastPlaced.x==2);}
    // The new-position scan must finish and select damage, not its first hit.
    {auto m=Start();m->Bool("Target.Through Walls",true);m->Number("Rotation.Speed",1);mock::angle={};
     mock::bases={{{-3,63,0},false,.3},{{2,63,0},false,1}};
     for(int i=0;i<30;++i)Tick();m->Render();assert(crystal::rendered.world&&crystal::rendered.block.x==2);}
    // Behind means the target's yaw, including wraparound. Default remains damage-first.
    for(int scenario=0;scenario<7;++scenario){
        auto m=Start();m->Bool("Prefer Behind",scenario!=0);m->Number("Rotation.Speed",1);
        mock::angle={180,-80};mock::targetX=2.5;mock::targetZ=.5;
        mock::bases={{{2,63,-1},false,.3},{{2,63,1},false,1}};
        if(scenario==2)mock::bases[0].occluded=true; // rear ray unavailable
        if(scenario==3)mock::bases[0].selfExposure=1; // unsafe rear
        if(scenario==4)mock::bases[0].exposure=.001; // rear below minimum damage
        if(scenario==5)mock::yawReadable=false; // failed yaw read falls back
        if(scenario==6)mock::targetYaw=360;
        for(int i=0;i<40;++i)Tick();m->Render();
        assert(crystal::rendered.world&&mock::placed==0);
        assert(crystal::rendered.block.z==((scenario==1||scenario==6)?-1:1));
        if(scenario==1){
            mock::targetYaw=180;
            for(int i=0;i<30;++i)Tick();m->Render();assert(crystal::rendered.block.z==1);
        }
    }
    // Existing anchors still finish their charge/detonation cycle with preference on.
    {auto m=Start();m->Bool("Prefer Behind",true);mock::charges=1;mock::baseExists=true;
     for(int i=0;i<100&&mock::detonated==0;++i)Tick();assert(mock::detonated==1&&mock::placed==0);}
    // SafeAnchor requires a real glowstone shield before charging/detonating.
    for(bool silent:{false,true}){
        auto m=Start();m->Bool("SafeAnchor",true);m->Bool("Place.Silent",silent);mock::selfDanger=true;
        mock::bases={{{2,63,0},false,1,1},{{1,63,0},false,0}};
        for(int i=0;i<160&&mock::detonated==0;++i){
            Tick();if(mock::charged||mock::detonated)assert(mock::shieldUses==1&&!mock::shields.empty());
        }
        assert(mock::placed==1&&mock::shieldUses==1&&mock::charged==1&&mock::detonated==1);
        assert(mock::shields[0].x==1);m->Disable();Tick();assert(mock::selected==0&&mock::slotOwner==nullptr);
    }
    for(int reason=0;reason<3;++reason){
        auto m=Start();m->Bool("SafeAnchor",true);m->Bool("Safety.Ignore Self Damage",true);mock::selfDanger=true;
        mock::bases={{{2,63,0},false,1,1},{{1,63,0},false,0}};
        if(reason==0)mock::glowCount=1;if(reason==1)mock::shieldEffective=false;if(reason==2)mock::shieldSupport=false;
        for(int i=0;i<70;++i)Tick();assert(mock::placed==0&&mock::detonated==0);
    }
    {auto m=Start();m->Bool("SafeAnchor",true);mock::selfDanger=true;mock::shieldAccept=false;
     mock::bases={{{2,63,0},false,1,1},{{1,63,0},false,0}};
     for(int i=0;i<160&&mock::shieldUses==0;++i)Tick();assert(mock::placed==1&&mock::shieldUses==1);
     for(int i=0;i<20;++i)Tick();assert(mock::charged==0&&mock::detonated==0&&mock::shieldUses==1);}
    {auto m=Start();m->Bool("SafeAnchor",true);m->Bool("Detonate",false);mock::glowCount=1;
     mock::bases={{{2,63,0},false,1,1},{{1,63,0},false,0}};
     for(int i=0;i<160&&mock::shieldUses==0;++i)Tick();assert(mock::placed==1&&mock::shieldUses==1);
     for(int i=0;i<20;++i)Tick();assert(mock::charged==0&&mock::detonated==0);}
    {auto m=Start();m->Bool("SafeAnchor",true);mock::bases={{{2,63,0},false,1,1},{{1,63,0},false,0}};
     for(int i=0;i<160&&mock::charged==0;++i)Tick();assert(mock::charged==1&&mock::detonated==0);
     mock::shields.clear();mock::bases[1].built=false;mock::glowCount=0;
     for(int i=0;i<30;++i)Tick();assert(mock::detonated==0);}
    {auto m=Start();m->Bool("SafeAnchor",true);mock::bases={{{2,63,0},false,1,1},{{1,63,0},false,0}};
     for(int i=0;i<160&&mock::charged==0;++i)Tick();assert(mock::charged==1);
     mock::selfPosition.x=4;for(int i=0;i<30;++i)Tick();assert(mock::detonated==0);}
    {double clear=0,covered=0;const nova::java::Box body{0,64,0,.6,65.8,.6},shield{1,64,0,2,65,1};
     auto ray=[](auto,auto,bool& out){out=true;return true;};
     assert(crystal::SampleExposure(body,{2.5,64.5,.5},nullptr,ray,clear));
     assert(crystal::SampleExposure(body,{2.5,64.5,.5},&shield,ray,covered));
     assert(clear==1&&covered<clear);}
    // A competing crystal proposal is evaluated between anchor planning and
    // rotation application. It must prevent anchor from switching or using.
    {auto m=Start();auto competitor=nova::event::EventBus::Get().Subscribe<nova::event::GameTickEvent>([](auto&){explosive::ChooseCrystal(100,false);},1);
     for(int i=0;i<30;++i)Tick();assert(mock::placed==0&&mock::selected==0&&!explosive::anchorWorking);
     competitor.Reset();for(int i=0;i<100&&mock::placed==0;++i)Tick();assert(mock::placed==1);}
    explosive::ResetAnchor();explosive::OfferAnchor(15,false);
    // Hold placement confirmation open. Aura must get its original weapon,
    // rotations and attack lease back on the next full tick, without a resend.
    {auto m=Start();mock::accept=false;
     for(int i=0;i<100&&mock::placed==0;++i)Tick();assert(mock::placed==1);
     int aura=0,attacks=0;
     auto listener=nova::event::EventBus::Get().Subscribe<nova::event::GameTickEvent>([&](auto&){
         if(!explosive::anchorWorking&&nova::rotation::AcquireAttack(&aura)){
             assert(mock::selected==0&&mock::spoof==-1&&mock::owner==nullptr);++attacks;
             nova::rotation::ReleaseAttack(&aura);
         }
     },10);
     for(int i=0;i<5;++i){Tick();assert(!explosive::anchorWorking&&mock::slotOwner==nullptr);}
     assert(attacks==5&&mock::placed==1&&mock::charged==0);listener.Reset();m->Disable();Tick();}
    // Place/boom delays are checked before rotation or slot acquisition.
    {auto m=Start();m->Number("Detonate.Delay",1000);
     for(int i=0;i<100&&mock::detonated==0;++i)Tick();assert(mock::detonated==1);
     mock::charges=1;mock::baseExists=true;int aura=0,available=0;
     auto listener=nova::event::EventBus::Get().Subscribe<nova::event::GameTickEvent>([&](auto&){
         if(!explosive::anchorWorking&&nova::rotation::AcquireAttack(&aura)){
             assert(mock::owner==nullptr&&mock::selected==0);++available;nova::rotation::ReleaseAttack(&aura);
         }
     },10);
     for(int i=0;i<8;++i)Tick();assert(mock::detonated==1&&available==8);listener.Reset();m->Disable();Tick();}
    // A very slow, never-aligned turn cannot indefinitely starve aura. The
    // bounded lease preserves all ray checks; the release does not force use.
    {auto m=Start();m->Number("Rotation.Speed",1);mock::angle={180,-80};
     int longest=0,streak=0,aura=0,available=0;
     auto listener=nova::event::EventBus::Get().Subscribe<nova::event::GameTickEvent>([&](auto&){
         if(explosive::anchorWorking){longest=std::max(longest,++streak);}
         else {streak=0;if(nova::rotation::AcquireAttack(&aura)){
             assert(mock::selected==0&&mock::owner==nullptr);++available;nova::rotation::ReleaseAttack(&aura);
         }}
     },10);
     for(int i=0;i<35;++i)Tick();assert(longest>0&&longest<=6&&available>0&&mock::placed==0);
     listener.Reset();m->Disable();Tick();}
    // A foreign inventory owner blocks equip before we suppress aura/turn.
    {auto m=Start();int inventory=0;mock::slotOwner=&inventory;mock::physicalLease=true;mock::yieldable=false;
     for(int i=0;i<15;++i){Tick();assert(!explosive::anchorWorking&&mock::owner==nullptr&&mock::slotOwner==&inventory);}
     mock::slotOwner=nullptr;mock::physicalLease=false;m->Disable();Tick();}
    std::puts("PASS: confirmation/delay restore aura weapon and attack ownership; persistent misses yield within six ticks; blocked equip never steals rotation");
    explosive::ResetAnchor();explosive::OfferAnchor(15,false);
    assert(!explosive::ChooseCrystal(10,false)&&explosive::ChooseAnchor());
    assert(explosive::ChooseCrystal(20,false)&&!explosive::ChooseAnchor());
    explosive::ResetAnchor();explosive::OfferAnchor(15,true);assert(!explosive::ChooseCrystal(20,false));
    assert(explosive::ChooseCrystal(5,true));explosive::ResetAnchor();
    std::puts("PASS: anchor production module cycles, Mio placement ranking, existing anchors, slots, safety, confirmation and arbitration");
}
