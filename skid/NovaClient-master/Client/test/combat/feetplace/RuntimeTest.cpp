#include "minecraft/java/api/game_blockplace.h"
#include "minecraft/modules/combat/feetplace/PlacementPlan.h"
#include "minecraft/modules/combat/shared/BlockAim.h"
#include "minecraft/modules/combat/shared/HotbarSession.h"
#include <cstdlib>
namespace mock {bool jumping=false,accept=true,riding=false;int placed=0;}
namespace nova::java {
bool KeyJumpDown(){return mock::jumping;}
bool EntityTypes(const EntityHandle&,unsigned& out){out=kTypeLiving;return true;}
bool ObsidianContextReady(){return mock::supported&&!mock::riding;}
bool ObsidianBuildable(const BlockPos& p){
    const auto b=mock::FindBase(p);if(!b||b->built)return false;
    Box self;PlayerEntity().BoundingBox(self);
    return !nova::module::crystal::Overlaps(CrystalPlacementVolume(p,true),self);
}
bool ReadObsidian(const BlockPos& p,bool& out){auto b=mock::FindBase(p);out=b&&b->built;return true;}
bool PlaceObsidian(const BlockPos& p,const BlockHitResultInfo& hit,int slot,const void* owner,bool& entered){
    assert(ObsidianBuildable(p)&&!mock::used&&!mock::motion&&mock::owner==owner&&mock::slotOwner==owner);
    assert(hit.direction==1&&slot==5&&slot==(mock::spoof<0?mock::selected:mock::spoof));
    assert(mock::attack.Acquire(owner));entered=mock::used=mock::locked=true;++mock::placed;mock::lastPlaced=p;
    if(mock::accept)for(auto& b:mock::bases)if(mock::Same(b.pos,p))b.built=true;
    return mock::accept;
}
}
// @PRODUCTION_MODULE@
void Tick(){mock::used=mock::locked=mock::motion=false;mock::attack.BeginTick();
    nova::event::Post<nova::event::GameTickEvent>();nova::event::Post<nova::event::PrePlayerInteractEvent>();
    nova::event::Post(nova::event::MotionEvent{nova::event::EventType::Post,mock::angle.yaw,mock::angle.pitch});}
auto Start(){
    using namespace mock;using namespace nova::module;
    owner=slotOwner=nullptr;selected=0;oldIdentity=100;spoof=-1;physicalLease=yieldable=false;attack={};
    world=2;selfPosition={.5,64,.5};selfGrounded=true;angle={};
    target=mock::crystal=wall=rescue=screen=dropOnAim=jumping=riding=false;
    supported=obsidian=syncOK=accept=true;quantized=true;placed=0;
    explosive::feetWorking=explosive::anchorConfirming=explosive::crystalConfirming=false;
    bases={{{-1,64,0},false,1},{{1,64,0},false,1},{{0,64,-1},false,1},{{0,64,1},false,1}};
    auto m=MakeFeetPlaceModule();m->Number("Place Delay",0);m->Number("Rotation.Speed",180);m->Enable();return m;
}
void Complete(){static int scenario=0;++scenario;for(int i=0;i<240&&mock::placed<4;++i)Tick();
    if(mock::placed!=4)std::fprintf(stderr,"Complete scenario %d placed=%d\n",scenario,mock::placed);
    assert(mock::placed==4);Tick();}
int main(){
    _set_error_mode(_OUT_TO_STDERR);_set_abort_behavior(0,_CALL_REPORTFAULT);
    using namespace nova::module;using namespace feetplace;
    assert(Footprint({.2,64,.2,.8,65.8,.8}).size()==1);
    assert(Footprint({.7,64,.2,1.3,65.8,.8}).size()==2);
    auto four=Footprint({.7,64,.7,1.3,65.8,1.3});assert(four.size()==4&&Ring(four).size()==12);
    assert(Footprint({0,64,0,1,65.8,1}).size()==1); // exact upper edge is excluded
    assert(Footprint({.2,64.5,.2,.8,66.3,.8})[0].y==65);
    {std::vector<Pos> visited;Pos first;
     auto build=[](Pos p){return p.z==0&&p.y==64&&p.x>=0&&p.x<=2;};
     auto aim=[](Pos p){return p.x==0;};
     assert(FirstStep({2,64,0},2,visited,build,aim,first)&&first.x==0);
     visited.clear();assert(!FirstStep({2,64,0},1,visited,build,aim,first));
     visited.clear();assert(!FirstStep({0,64,0},10,visited,[](Pos){return true;},[](Pos){return false;},first)&&visited.size()==32);}
    for(bool silent:{false,true}){auto m=Start();m->Bool("Silent",silent);Complete();
        assert(mock::selected==0&&mock::slotOwner==nullptr&&!explosive::feetWorking);
        for(int i=0;i<20;++i)Tick();assert(mock::placed==4);
        mock::bases[0].built=false;std::this_thread::sleep_for(std::chrono::milliseconds(110));
        for(int i=0;i<100&&mock::placed<5;++i)Tick();assert(mock::placed==5);
        m->Disable();Tick();assert(mock::selected==0&&mock::slotOwner==nullptr);}
    {auto m=Start();Tick();mock::jumping=true;for(int i=0;i<100;++i)Tick();assert(mock::placed<=1&&!explosive::feetWorking);}
    {auto m=Start();Tick();mock::selfPosition.x+=2;for(int i=0;i<50;++i)Tick();assert(mock::placed<=1&&!explosive::feetWorking);}
    {auto m=Start();m->Bool("Auto Disable",false);mock::selfGrounded=false;for(int i=0;i<20;++i)Tick();assert(mock::placed==0);
     mock::selfGrounded=true;Complete();}
    for(int reason=0;reason<5;++reason){auto m=Start();
        if(reason==0)mock::obsidian=false;if(reason==1)mock::rescue=true;if(reason==2)mock::riding=true;
        if(reason==3)explosive::anchorConfirming=true;if(reason==4)explosive::crystalConfirming=true;
        for(int i=0;i<30;++i)Tick();assert(mock::placed==0&&mock::selected==0&&!explosive::feetWorking);}
    {auto m=Start();mock::accept=false;for(int i=0;i<100&&mock::placed==0;++i)Tick();assert(mock::placed==1);
     for(int i=0;i<20;++i)Tick();assert(mock::placed==1);
     std::this_thread::sleep_for(std::chrono::milliseconds(310));Tick();assert(mock::placed<=2);
     m->Disable();Tick();assert(mock::selected==0&&mock::slotOwner==nullptr);}
    {auto m=Start();mock::accept=false;for(int i=0;i<100&&mock::placed==0;++i)Tick();
     mock::selected=7;Tick();assert(mock::selected==7);
     mock::world=8;Tick();assert(mock::slotOwner==nullptr&&!explosive::feetWorking);}
    {auto m=Start();m->Number("Place Delay",250);for(int i=0;i<80&&mock::placed==0;++i)Tick();assert(mock::placed==1);
     for(int i=0;i<30;++i)Tick();assert(mock::placed==1);}
    // Emergency starts after planning in this tick; FeetPlace must not steal rotation.
    {auto m=Start();auto emergency=nova::event::EventBus::Get().Subscribe<nova::event::GameTickEvent>([](auto&){mock::rescue=true;},0);
     Tick();assert(!explosive::feetWorking&&mock::owner==nullptr&&mock::placed==0);emergency.Reset();mock::rescue=false;Complete();}
    {auto m=Start();const int swings=mock::swings,packets=mock::swingPackets;Complete();
     assert(mock::swings==swings&&mock::swingPackets==packets+4);}
    {auto m=Start();m->Bool("Swing",true);const int swings=mock::swings;Complete();assert(mock::swings==swings+4);}
    {auto m=Start();mock::syncOK=false;for(int i=0;i<80;++i)Tick();assert(mock::placed==0&&mock::spoof==-1);
     mock::syncOK=true;std::this_thread::sleep_for(std::chrono::milliseconds(110));Complete();}
    explosive::ResetAnchor();explosive::feetWorking=true;explosive::OfferAnchor(100,false);
    assert(!explosive::ChooseAnchor()&&!explosive::ChooseCrystal(100,false));
    assert(explosive::ChooseCrystal(100,true));explosive::feetWorking=false;
    std::puts("PASS: FeetPlace geometry, support dependencies, native module placement/repair, switching, pause, timeout and arbitration");
}
