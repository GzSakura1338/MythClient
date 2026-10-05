#include <cassert>
#include <cmath>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <tuple>
#include <vector>
#include <algorithm>
#include <array>
#include <limits>
#include "minecraft/utils/rotation/Vec3.h"
#include "minecraft/utils/rotation/Rotation.h"
#include "minecraft/utils/rotation/MathConst.h"
#include "minecraft/utils/rotation/RotationUtils.h"
#include "minecraft/utils/rotation/MovementFix.h"
#include "minecraft/utils/rotation/Mth.h"
namespace rotation=nova::rotation;
// @HYPIXEL_HELPERS@
namespace scaffold=nova::module::scaffold;
namespace automlg {bool IsWorking(){return false;}}
namespace event {enum class EventType{Pre,Post};struct MotionEvent{EventType type;float yaw,pitch;};}
namespace nova::rotation {
Rotation effective{};
Rotation GetRotation(){return effective;}
double Random(){return .25;}
enum class RotationPriority { Medium, Highest };
struct RotationState { bool active=false; RotationPriority priority=RotationPriority::Medium;
 bool hasLastRotations=false;Rotation lastRotations{};} state;
RotationState& State(){return state;}
const void* owner=nullptr;
bool OwnsExclusiveRotations(const void* value){return owner==value;}
bool SetExclusiveRotations(const void* value,Rotation angle){if(owner&&owner!=value)return false;owner=value;effective=angle;return true;}
void ReleaseExclusiveRotations(const void* value){if(owner==value)owner=nullptr;}
void SetSmoothed(bool){}
void SetRotations(Rotation target,double speed){effective=Smooth(effective,target,speed,effective,.5,120,true);}
enum class BlockFace { Down,Up,North,South,West,East };
}
namespace java {
struct BlockPos {int x{},y{},z{};};
struct Box {double minX,minY,minZ,maxX,maxY,maxZ;};
enum class HitKind {Miss,Block,Entity};
struct BlockHitResultInfo {HitKind kind=HitKind::Miss; bool hasBlockPos{}; BlockPos blockPos; int direction=-1; rotation::Vec3 location;};
std::set<std::tuple<int,int,int>> blocks,excluded;
rotation::Vec3 position{1.4,66.0,1.4};
bool jump=true;
bool grounded=false;
float cameraYaw=0;
bool geometry=false;
int placements=0;
bool bridgeSimulation=false;
rotation::Vec3 motion{};
bool PlayerOnGround(){return grounded;}
bool PlayerDeltaMovement(rotation::Vec3& out){out=motion;return true;}
BlockHitResultInfo ray;
int reads{};
bool BlockStateReplaceable(BlockPos p){++reads;return !blocks.contains({p.x,p.y,p.z});}
bool BlockStateScaffoldExcluded(BlockPos p){return excluded.contains({p.x,p.y,p.z});}
bool KeyJumpDown(){return jump;}
bool PlayerRotation(float& yaw,float& pitch){yaw=cameraYaw;pitch=0;return true;}
bool PlayerSensitivity(double& value){value=.5;return true;}
float PlayerPitch(){return 0;}
int FacingStepX(int f){return f==4?-1:f==5?1:0;}
int FacingStepY(int f){return f==0?-1:f==1?1:0;}
int FacingStepZ(int f){return f==2?-1:f==3?1:0;}
int FacingOpposite(int f){return f^1;}
struct EntityHandle {
 bool Valid() const{return true;}
 bool Position(rotation::Vec3& out) const{out=position;return true;}
 bool GetY(double& out) const{out=position.y;return true;}
 bool BoundingBox(Box& out) const{out={position.x-.3,position.y,position.z-.3,position.x+.3,position.y+1.8,position.z+.3};return true;}
 bool EyePosition(float,rotation::Vec3& out) const{out=position.Add(0,1.62,0);return true;}
};
EntityHandle PlayerEntity(){return {};}
EntityHandle LevelEntity(){return {};}
bool Vec3DirectionFromRotation(float pitch,float yaw,rotation::Vec3& out){
 if(!geometry){out={0,-1,0};return true;}
 const double p=pitch*rotation::kToRadians,y=yaw*rotation::kToRadians;
 out={-std::sin(y)*std::cos(p),-std::sin(p),std::cos(y)*std::cos(p)};return true;
}
bool WorldClip(rotation::Vec3 from,rotation::Vec3 to,EntityHandle,BlockHitResultInfo& out){
 if(!geometry){out=ray;return true;}
 out={}; double nearest=2;
 for(auto [x,y,z]:blocks){
  double entry=0,exit=1;int face=-1;bool miss=false;
  const double origin[]={from.x,from.y,from.z},delta[]={to.x-from.x,to.y-from.y,to.z-from.z};
  const double minimum[]={double(x),double(y),double(z)};
  const int lowFaces[]={4,0,2},highFaces[]={5,1,3};
  for(int axis=0;axis<3;++axis){
   if(std::abs(delta[axis])<1e-10){if(origin[axis]<minimum[axis]||origin[axis]>minimum[axis]+1)miss=true;continue;}
   double a=(minimum[axis]-origin[axis])/delta[axis],b=(minimum[axis]+1-origin[axis])/delta[axis];
   int f=lowFaces[axis];if(a>b){std::swap(a,b);f=highFaces[axis];}
   if(a>entry){entry=a;face=f;}exit=std::min(exit,b);if(entry>exit)miss=true;
  }
  if(!miss&&face>=0&&entry<nearest&&entry<=1&&exit>=0){nearest=entry;out={HitKind::Block,true,{x,y,z},face,from.Add((to.x-from.x)*entry,(to.y-from.y)*entry,(to.z-from.z)*entry)};}
 }
 return true;
}
bool EffectiveSelectedSlot(int& slot){slot=0;return true;}
bool HotbarSlotPlaceable(int){return true;}
int HotbarSlotCount(int){return 64;}
void SendMoveRotation(float,float,bool){assert(false);}
bool UseItemOnBlock(BlockPos pos,int face,rotation::Vec3){
 ++placements;
 if(bridgeSimulation){
  blocks.emplace(pos.x+FacingStepX(face),pos.y+FacingStepY(face),pos.z+FacingStepZ(face));
  scaffold::snapWireHistory.Observe(false,false,0,0);
 }
 return true;
}
void SwingMainHand(){}
void SendSwingPacket(){}
}
namespace nova::rotation {
constexpr double kBlockReach=4.5;
// @OVER_BLOCK@
}
constexpr int kFaceUp=1,kFaceDown=0,kHypixelRotation=2;
constexpr double kReach=4.5;
int FloorToInt(double x){return static_cast<int>(std::floor(x));}
float RandomRange(float a,float b){return a+(b-a)*.25f;}
// @HIT_VECTOR@
// @CALCULATE_FACE@
struct Fixture {
 struct Toggle { bool value=false; bool get() const { return value; } } degrees, clutch;
 Toggle* degrees45_ = &degrees;
 Toggle* clutch_ = &clutch;
 bool hypixelTelly=false, tellyLayerValid_=false;
 bool fallingCatch_=false;
 bool HypixelTelly() const{return hypixelTelly;}
 bool hasRotation_ = false, hypixelAimValid_ = false;
 rotation::Rotation rotation_{};
 bool hasBlockPos_{};
 int enumFacing_=-1,lastEnumFacing_=-1,yLevel_=65;
 java::BlockPos blockPos_{};
 bool hold=false;
 struct Number {double value=0;double get() const{return value;}} mode,rayMode,speed{180},speed2{36},back{180},tick{0};
 Number *rotMod_=&mode,*rayCast_=&rayMode,*rotateSpeed_=&speed,*rotateSpeed2_=&speed2,*rotateBackSpeed_=&back,*tellyTick_=&tick;
 Toggle swing; Toggle* swing_=&swing;
 bool normalRotationActive_=false;
 Toggle snap;Toggle* snap_=&snap;
 bool SnapActive()const{return snap.value;}
 bool snapWaiting_=false,snapMotionComplete_=false,snapPlacementPending_=false;
 float snapBaseYaw_=0,snapBasePitch_=0;
 int snapHeldTicks_=0;
 rotation::Rotation snapRotation_{};
 bool snapHasPlacedDelta_=false;
 float snapPlacedDelta_=0;
 bool snapHasMotion_=false,snapRotatedSincePlace_=false;
 float snapLatestDelta_=0;
 rotation::Rotation snapLastMotion_{};
 int airTick_=10;
 float inputYaw_=0,rawInputYaw_=0,strafeInput_=0,forwardInput_=1;
 bool UseBoundedRotation()const{return false;}
 void HandleWatchdogTelly(){assert(false && "fall must use shared Rise path");}
 bool TellyActive() const{return hold;}
 bool InputMoving() const{return forwardInput_!=0 || strafeInput_!=0;}
 // @FALL_METHODS@
 // @PLACEMENT_SEARCH@
 // @NORMAL_ROTATION@
 // @SNAP_ROTATION@
 // @SNAP_LIFECYCLE@
 java::BlockPos Destination() const{return {blockPos_.x+java::FacingStepX(enumFacing_),blockPos_.y+java::FacingStepY(enumFacing_),blockPos_.z+java::FacingStepZ(enumFacing_)};}
};
// Independent native reference of Rise PlayerUtil.a(offsets, sameY) and
// PlayerUtil.a(target, allowCeiling), read from the supplied Java source.
// World reads use the fixture's immutable map, bypassing the production cache.
struct RiseChoice {bool valid=false;java::BlockPos support{};int face=-1;};
RiseChoice RiseReference(rotation::Vec3 offset,std::optional<int> height,float yaw){
 const auto player=java::position;
 const int radius=int(5+std::abs(offset.x)+std::abs(offset.z));
 auto empty=[](double x,double y,double z){return !java::blocks.contains({FloorToInt(x),FloorToInt(y),FloorToInt(z)});};
 std::vector<rotation::Vec3> candidates;
 for(int x=-radius;x<=radius;++x)for(int y=-radius;y<=radius;++y)for(int z=-radius;z<=radius;++z){
  auto at=player.Add(x,y,z);const auto key=std::tuple{FloorToInt(at.x),FloorToInt(at.y),FloorToInt(at.z)};
  if(java::excluded.contains(key)||empty(at.x,at.y,at.z))continue;
  for(int d:{-1,1})candidates.push_back(at.Add(d,0,0));
  for(int d:{-1,1})candidates.push_back(at.Add(0,d,0));
  for(int d:{-1,1})candidates.push_back(at.Add(0,0,d));
 }
 std::erase_if(candidates,[&](const auto& v){
  const auto d=v.Subtract(player);
  return float(std::sqrt(d.x*d.x+d.y*d.y+d.z*d.z))>5 || !empty(v.x,v.y,v.z) || (height && std::floor(v.y+1)!=*height);
 });
 auto metric=[&](const auto& v){const auto d=player.Add(offset.x,offset.y-1,offset.z).Subtract(v);return float(std::sqrt(d.x*d.x+d.y*d.y+d.z*d.z));};
 std::stable_sort(candidates.begin(),candidates.end(),[&](const auto& a,const auto& b){return metric(a)<metric(b);});
 if(candidates.empty())return {};
 const auto target=candidates.front();
 struct Face{int face,x,y,z;};std::vector<Face> faces;
 for(int d:{-1,1})if(!empty(target.x,target.y,target.z+d))faces.push_back({d<0?3:2,0,0,d});
 for(int d:{-1,1})if(!empty(target.x+d,target.y,target.z))faces.push_back({d>0?4:5,d,0,0});
 auto score=[&](Face face){
  const double a=std::fmod(std::atan2(double(face.z),double(face.x))*rotation::kToDegrees,360.0);
  const double b=std::fmod(yaw,360.f)+90.f;
  return std::abs(std::min(std::abs(a-b),std::min(std::abs(a-360)-std::abs(b),std::abs(b-360)-std::abs(a))));
 };
 std::stable_sort(faces.begin(),faces.end(),[&](auto a,auto b){return score(a)<score(b);});
 if(faces.empty()){
  if(!empty(target.x,target.y-1,target.z))faces.push_back({1,0,-1,0});
  else if(!empty(target.x,target.y+1,target.z))faces.push_back({0,0,1,0});
 }
 if(faces.empty())return {};
 const auto f=faces.front();
 return {true,{FloorToInt(target.x)+f.x,FloorToInt(target.y)+f.y,FloorToInt(target.z)+f.z},f.face};
}
int main(){
 int referenceChecks=0;
 for(int seed=0;seed<64;++seed){
  java::position={double(seed%5)-2.2,64.1+(seed%7)*.3,double(seed%3)-1.65};
  java::blocks.clear();java::excluded.clear();
  const int bx=FloorToInt(java::position.x),by=FloorToInt(java::position.y),bz=FloorToInt(java::position.z);
  for(int x=-3;x<=3;++x)for(int y=-3;y<=1;++y)for(int z=-3;z<=3;++z){
   const unsigned hash=unsigned((x+3)*31+(y+3)*67+(z+3)*97+seed*139);
   if(hash%9==0){java::blocks.emplace(bx+x,by+y,bz+z);if(hash%5==0)java::excluded.emplace(bx+x,by+y,bz+z);}
  }
  for(bool telly:{false,true})for(bool moving:{false,true})
  for(bool clutch:{false,true})for(bool jump:{false,true})
  for(auto offset:{rotation::Vec3{},rotation::Vec3{.65,0,.1},rotation::Vec3{-.49,0,.49},
                  rotation::Vec3{0,-1,0},rotation::Vec3{1.2,.5,-.4}}){
   Fixture hypixel;hypixel.mode.value=2;hypixel.hypixelTelly=telly;hypixel.tellyLayerValid_=telly;
   hypixel.hold=telly;hypixel.yLevel_=by-1;hypixel.clutch.value=clutch;
   hypixel.forwardInput_=moving?1:0;
   java::jump=jump;java::grounded=seed%2==0;java::motion={0,(seed%3-1)*.25,0};
   const float yaw=float((seed%9)*90-360);rotation::effective={yaw,82};
   const auto desired=java::position.Add(offset.x,offset.y,offset.z);
   const auto expected=RiseReference(offset,std::nullopt,yaw);
   hypixel.GetBlockInfo(&desired);
   assert(expected.valid==hypixel.hasBlockPos_);
   if(expected.valid)assert(expected.face==hypixel.enumFacing_ && expected.support.x==hypixel.blockPos_.x &&
     expected.support.y==hypixel.blockPos_.y && expected.support.z==hypixel.blockPos_.z);
   ++referenceChecks;
  }
 }
 java::excluded.clear();java::blocks.clear();java::motion={};java::jump=true;java::grounded=false;
 rotation::effective={};
 std::cout<<"PASS: Hypixel search vs independent Rise reference: "<<referenceChecks<<" normal/Telly/movement/jump/Clutch/vertical-offset/negative-coordinate cases\n";
 // Rise defaults Same Y to Off, even in Telly. A nearer elevated target must
 // survive Nova's takeoff-level state; other modes retain their height policy.
 {
  java::position={.5,66,.5};java::blocks={{0,63,0},{0,66,0}};
  java::jump=false;
  Fixture hypixel;hypixel.mode.value=2;hypixel.hypixelTelly=true;
  hypixel.hold=true;hypixel.yLevel_=64;
  const auto expected=RiseReference({},std::nullopt,0);
  hypixel.GetBlockInfo();
  assert(expected.valid && hypixel.hasBlockPos_);
  assert(expected.face==hypixel.enumFacing_ && expected.support.x==hypixel.blockPos_.x &&
         expected.support.y==hypixel.blockPos_.y && expected.support.z==hypixel.blockPos_.z);
  assert(hypixel.Destination().y==65);
  Fixture rise;rise.hold=true;rise.yLevel_=64;
  rise.GetBlockInfo();assert(rise.hasBlockPos_ && rise.Destination().y==64);
  java::jump=true;
 }
 // Compile the actual Normal search: a miss must still establish a turning
 // target, and a currently matching ray must not replace that stored target.
 {
  Fixture aim;
  aim.hasBlockPos_=true; aim.enumFacing_=5; aim.blockPos_={0,64,0};
  java::position={1.5,65,.5}; java::ray={};
  aim.ComputeHypixelRotation();
  assert(aim.hasRotation_ && !aim.hypixelAimValid_);
  const auto expected=CalculateFace(aim.blockPos_,5,java::position.Add(0,1.62,0));
  assert(aim.rotation_.yaw==expected.yaw && aim.rotation_.pitch==expected.pitch);
  const auto retained=aim.rotation_;
  rotation::effective={40,70};
  java::ray={java::HitKind::Block,true,{0,64,0},5,{1,64.5,.5}};
  aim.ComputeHypixelRotation();
  assert(aim.hypixelAimValid_ && aim.rotation_.yaw==retained.yaw && aim.rotation_.pitch==retained.pitch);
  java::ray={}; rotation::effective={};
 }
 Fixture f;
 java::position={.5,65,.5}; java::blocks={{0,63,0},{-1,64,0}};
 f.GetBlockInfo(); auto p=f.Destination();
 assert(f.hasBlockPos_ && p.x==0 && p.y==64 && p.z==0);
 assert(f.enumFacing_==5 && f.blockPos_.x==-1); // Horizontal before the floor.
 java::blocks={{0,64,-1},{0,64,1},{-1,64,0},{1,64,0}};
 for(auto [yaw,face] : {std::pair{0.f,2},std::pair{90.f,5},std::pair{-90.f,4}}){
  rotation::effective.yaw=yaw; f.GetBlockInfo(); assert(f.enumFacing_==face);
 }
 rotation::effective={};
 java::blocks={{0,63,0}}; f.GetBlockInfo(); assert(f.enumFacing_==1);
 java::blocks={{0,65,0}}; f.GetBlockInfo(); assert(f.enumFacing_==0); // Ceiling retry.
 java::position={1.4,66,1.4}; java::blocks={{0,65,0},{1,63,1}};
 f.GetBlockInfo(); p=f.Destination();
 assert(p.y==65 && (p.x==0 || p.z==0)); // Rise picks an equal-distance side extension before a lower footprint.
 java::position={.5,65,.5}; java::blocks={{0,64,0}};
 f.GetBlockInfo(); p=f.Destination(); assert(!java::blocks.contains({p.x,p.y,p.z}));
 java::excluded=java::blocks; f.GetBlockInfo(); assert(!f.hasBlockPos_);
 java::excluded.clear(); java::blocks={{0,58,0}};
 f.GetBlockInfo(); assert(!f.hasBlockPos_); // Beyond the five-block candidate radius.
 java::position={-.2,65,-.2}; java::blocks={{-1,63,-1}};
 f.GetBlockInfo(); p=f.Destination(); assert(p.x==-1 && p.y==64 && p.z==-1);
 java::position={.5,66,.5}; java::blocks={{0,63,0},{0,66,0}};
 java::jump=false; f.hold=true; f.yLevel_=64;
 f.GetBlockInfo(); assert(f.hasBlockPos_ && f.Destination().y==64);
 java::jump=true; f.hold=false;
 // Equal-distance candidates preserve generation order (Java stable sort).
 java::position={.5,65,.5}; java::blocks={{-1,64,-1}};
 f.GetBlockInfo(); p=f.Destination(); assert(p.x==0 && p.z==-1 && p.y==64);
 // Blacklist applies to candidate origins, not the later support-face lookup.
 java::blocks={{0,63,0},{-1,64,0}}; java::excluded={{-1,64,0}};
 f.GetBlockInfo(); assert(f.blockPos_.x==-1 && f.enumFacing_==5); java::excluded.clear();
 // Cache ensures duplicate candidates do not multiply native world reads.
 java::reads=0; f.GetBlockInfo(); assert(java::reads<=1400);
 // The existing explicit same-height option remains. Hypixel must not add a
 // second lower-layer veto when that constraint is absent.
 {
  Fixture telly;
  telly.hypixelTelly=true; telly.tellyLayerValid_=true; telly.hold=true;
  telly.yLevel_=64;
  Fixture rise; rise.hold=true; rise.yLevel_=64;
  auto sameAsRise=[&] {
   rise.GetBlockInfo(); telly.GetBlockInfo();
   assert(rise.hasBlockPos_==telly.hasBlockPos_);
   assert(rise.enumFacing_==telly.enumFacing_);
   if (rise.hasBlockPos_) assert(rise.blockPos_.x==telly.blockPos_.x &&
       rise.blockPos_.y==telly.blockPos_.y && rise.blockPos_.z==telly.blockPos_.z);
  };
  for (bool jump : {false,true}) {
   java::jump=jump;
   java::blocks={{0,63,0}}; java::position={.5,63.4,.5};
   sameAsRise(); assert(telly.hasBlockPos_);
   java::blocks={{0,60,0}}; java::position={.5,61.4,.5};
   sameAsRise(); assert(telly.hasBlockPos_);
  }
  for (bool jump : {false,true}) {
   java::jump=jump; java::motion={0,-1.2,0};
   java::position={.5,63.4,.5}; java::blocks={{0,61,0}};
   sameAsRise(); assert(telly.hasBlockPos_);
   java::position={.5,61.4,.5}; java::blocks={{1,60,0}};
   telly.GetBlockInfo();
   assert(telly.fallingCatch_ && telly.hasBlockPos_);
   assert(telly.Destination().x==0 && telly.Destination().y==60 && telly.Destination().z==0);
   // Do not pre-emptively remove a side candidate that Rise can attempt.
   java::blocks={{2,60,0}};
   sameAsRise(); assert(telly.hasBlockPos_);
  }
  // All four diagonal quadrants, stationary falls and falls with horizontal drift.
  for (int x : {-1,1}) for (int z : {-1,1}) for (double drift : {0.0,.35}) {
   java::position={x*.5,61.4,z*.5}; java::motion={x*drift,-1.2,z*drift};
   const int bx=FloorToInt(java::position.x), bz=FloorToInt(java::position.z);
   java::blocks={{bx+x,60,bz}};
   sameAsRise(); assert(telly.hasBlockPos_);
   assert(telly.Destination().x==bx && telly.Destination().y==60 && telly.Destination().z==bz);
  }
  java::position={.5,61.4,.5}; java::blocks={{1,60,0}};
  java::grounded=true;
  sameAsRise(); assert(!telly.fallingCatch_ && telly.hasBlockPos_);
  java::grounded=false; java::motion.y=.1;
  sameAsRise(); assert(!telly.fallingCatch_ && telly.hasBlockPos_);
  java::motion={}; java::blocks={{0,60,0}};
  // Explicit Clutch and other modes retain their lower-layer targets.
  telly.clutch.value=true;
  telly.GetBlockInfo(); assert(telly.hasBlockPos_ && telly.Destination().y<64);
  telly.clutch.value=false; telly.hypixelTelly=false;
  telly.GetBlockInfo(); assert(telly.hasBlockPos_ && telly.Destination().y<64);
  // Holding jump still permits upward diagonal bridging in all quadrants.
  telly.hypixelTelly=true;
  for (int x : {-1,1}) for (int z : {-1,1}) {
   java::position={x*1.4,66,z*1.4};
   java::blocks={{x<0?-1:0,65,z<0?-1:0}};
   telly.GetBlockInfo(); assert(telly.hasBlockPos_ && telly.Destination().y==65);
  }
 }
 for(int face=0;face<6;++face){
  java::ray={}; const auto hit=HitVector({10,20,30},face);
  assert(hit.x>=10 && hit.x<=11 && hit.y>=20 && hit.y<=21 && hit.z>=30 && hit.z<=31);
  if(face<2)assert(hit.y==20+(face==1));
  else if(face<4)assert(hit.z==30+(face==3));
  else assert(hit.x==10+(face==5));
 }
 java::ray={java::HitKind::Block,true,{10,20,30},5,{11,20.8,30.9}};
 auto hit=HitVector({10,20,30},5); assert(hit.y==20.8 && hit.z==30.9);
 java::ray.direction=4; hit=HitVector({10,20,30},5); assert(hit.y==20.25 && hit.z==30.25);
 // End-to-end comparison using production selection, angle search, shared
 // smoothing, real OverBlock gates and Place against ray/cube intersections.
 // Include a landing footprint miss, side ledges, negative coordinates and
 // reachable faces outside Watchdog's 77.5-86.5 degree pitch band.
 java::geometry=true; java::grounded=false; java::motion={.2,-1.2,.1};
 int comparisons=0,successes=0;
 for(bool strict:{false,true}) for(bool jump:{false,true})
 for(float yaw:{-135.f,-30.f,45.f,135.f}) for(double height:{61.1,62.4,63.4})
 for(auto support:std::vector<java::BlockPos>{{1,60,0},{2,60,0},{-1,60,0},{0,59,0},{-1,61,-1}}){
  java::blocks={{support.x,support.y,support.z}};
  java::position={.5,height,.5};java::cameraYaw=yaw;java::jump=jump;
  Fixture rise,hypixel;
  for(auto* f:{&rise,&hypixel}){f->hold=true;f->tellyLayerValid_=true;f->yLevel_=64;
      f->hasRotation_=true;f->rotation_={yaw,10};f->inputYaw_=f->rawInputYaw_=yaw;f->rayMode.value=strict;}
  hypixel.hypixelTelly=true;hypixel.mode.value=2;
  rotation::effective={yaw,10};java::placements=0;
  rise.GetBlockInfo();rise.HandleTelly();const int placed=java::placements;
  const auto angle=rotation::effective;
  rotation::effective={yaw,10};java::placements=0;
  hypixel.GetBlockInfo();hypixel.HandleTelly();
  assert(hypixel.fallingCatch_&&rise.hasBlockPos_==hypixel.hasBlockPos_);
  assert(rise.enumFacing_==hypixel.enumFacing_&&rise.blockPos_.x==hypixel.blockPos_.x&&rise.blockPos_.y==hypixel.blockPos_.y&&rise.blockPos_.z==hypixel.blockPos_.z);
  assert(angle.yaw==rotation::effective.yaw&&angle.pitch==rotation::effective.pitch);
  assert(java::placements==placed);++comparisons;successes+=placed;
 }
 assert(successes>0);
 {
  Fixture fresh;
  fresh.hold=fresh.hypixelTelly=true;fresh.mode.value=2;fresh.rayMode.value=1;
  fresh.airTick_=1;fresh.tick.value=20;fresh.yLevel_=60;
  java::position={.5,61.1,.5};java::blocks={{1,60,0}};java::cameraYaw=0;java::jump=true;
  rotation::effective={0,10};java::placements=0;
  fresh.GetBlockInfo();fresh.HandleTelly();
  assert(fresh.fallingCatch_&&fresh.hasRotation_&&java::placements==1);
 }
 // Compare the former first-valid grid candidate against the production Snap
 // selector on actual cube intersections, not a permissive fake ray result.
 int snapCases=0, improved=0, duplicateCases=0, alternativeCases=0;
 double oldProjection=0,newProjection=0;
 java::blocks={{0,64,0}};java::geometry=true;
 for(int face:{0,1,2,3,4,5}) for(double side:{-.25,.15,.5,.85,1.25})
 for(float camera:{-179.f,-135.f,-37.f,0.f,22.f,45.f,91.f,179.f})
 for(float strafe:{-1.f,0.f,1.f}) for(double sensitivity:{0.,.5,1.}) {
  Fixture snap;snap.hasBlockPos_=true;snap.blockPos_={0,64,0};snap.enumFacing_=face;
  snap.forwardInput_=1;snap.strafeInput_=strafe;
  rotation::Vec3 eye;
  if(face==0)eye={side,63.5,.5};
  else if(face==1)eye={side,66.62,.5};
  else if(face<=3)eye={side,66.62,face==2?-.15:1.15};
  else eye={face==4?-.15:1.15,66.62,side};
  java::cameraYaw=camera;
  const rotation::Rotation previous{camera,10};
  rotation::Rotation oldAngle{},selected{};
  bool oldFound=false;
  const auto center=snap.HitVectorForSnap();
  for(double a:{0.,-.3,.3}) for(double b:{0.,-.3,.3}) {
   if(oldFound)continue;
   auto point=center;
   if(face<=1){point.x+=a;point.z+=b;}else if(face<=3){point.x+=a;point.y+=b;}else{point.y+=a;point.z+=b;}
   auto target=rotation::RelateToPlayerRotation(rotation::Calculate(eye,point),previous);
   auto angle=rotation::ApplySensitivityPatch(target,previous,sensitivity);
   java::BlockHitResultInfo hit;
   if(snap.TracePlacement(angle,{},eye,hit)){oldFound=true;oldAngle=angle;}
  }
  const bool found=snap.FindSnapRotation({},eye,previous,sensitivity,selected);
  if(oldFound)assert(found); // Never remove an existing valid placement.
  if(!found)continue;
  java::BlockHitResultInfo hit;assert(snap.TracePlacement(selected,{},eye,hit));
  if(!oldFound)continue;
  auto error=[&](rotation::Rotation angle){
   rotation::MoveInput input{1,strafe};rotation::FixMovement(input,angle.yaw,camera);
   return std::fabs(scaffold::AngleDelta(rotation::Direction(input.forward,input.strafe,angle.yaw),
                                        rotation::Direction(1,strafe,camera)));
  };
  const double before=error(oldAngle),after=error(selected);
  // Forward-input tie preference permits at most a tenth-degree tradeoff.
  assert(after<=before+.11);
  oldProjection+=std::cos(before*rotation::kToRadians);
  newProjection+=std::cos(after*rotation::kToRadians);
  ++snapCases;if(after+1<before)++improved;
  const float placedDelta=std::fabs(selected.yaw-previous.yaw);
  if(placedDelta>2) {
   ++duplicateCases;
   // A fixed travel/camera yaw reproduces the old identical placement delta.
   // History is applied after sensitivity quantization, then alternate real
   // face points must still pass the exact production ray check.
   snap.snapHasPlacedDelta_=true;snap.snapPlacedDelta_=placedDelta;
   rotation::Rotation alternate;
   if(snap.FindSnapRotation({},eye,previous,sensitivity,alternate)) {
    assert(snap.TracePlacement(alternate,{},eye,hit));
    const float nextDelta=std::fabs(alternate.yaw-previous.yaw);
    assert(nextDelta<=2 || std::fabs(nextDelta-placedDelta)>=.0001f);
    ++alternativeCases;
   }
  }
 }
 assert(snapCases>1000&&improved>100&&newProjection>oldProjection);
 assert(duplicateCases>1000 && alternativeCases>1000);
 // Consecutive translated bridge blocks with the same relative eye/face
 // position used to repeat the same snap every cycle, despite changing pitch.
 for(float camera:{0.f,45.f,179.f,720.f}) {
  Fixture snap;snap.enumFacing_=3;java::cameraYaw=camera;
  for(int block=0;block<32;++block) {
   snap.blockPos_={0,64,block};java::blocks={{0,64,block}};
   const rotation::Rotation previous{camera,10};
   rotation::Rotation selected;
   const rotation::Vec3 eye{.5,66.62,block+1.15};
   assert(snap.FindSnapRotation({},eye,previous,.5,selected));
   const float delta=std::fabs(selected.yaw-previous.yaw);
   if(snap.snapHasPlacedDelta_&&delta>2)
    assert(std::fabs(delta-snap.snapPlacedDelta_)>=.0001f);
   java::BlockHitResultInfo hit;assert(snap.TracePlacement(selected,{},eye,hit));
   snap.snapHasPlacedDelta_=true;snap.snapPlacedDelta_=delta;
  }
 }
 java::blocks={{0,64,0}};
 // A normal straight bridge's rear face cannot provide a forward sprint ray.
 // This guards against claiming the selector makes backwards sprint valid.
 {
  Fixture snap;snap.blockPos_={0,64,0};snap.enumFacing_=3;
  java::cameraYaw=0;rotation::Rotation selected;
  assert(snap.FindSnapRotation({}, {.5,66.62,1.15}, {0,10}, .5, selected));
  rotation::MoveInput input{1,0};rotation::FixMovement(input,selected.yaw,0);
  assert(input.forward<=0);
  java::blocks.clear();
  assert(!snap.FindSnapRotation({}, {.5,66.62,1.15}, {0,10}, .5, selected));
 }
 // Moving bridge simulation: production search, strict cube rays, lifecycle,
 // dispatched packet history and local placement. A missed support under the
 // full 0.6-wide footprint plus vanilla gravity/collision. Walking off an edge
 // has one zero-Y-motion tick; losing support alone is not already a fall.
 int bridgeFalls=0,bridgeRuns=0,bridgeFlags=0;
 java::bridgeSimulation=true;java::geometry=true;java::jump=false;
 for(float camera:{0.f,45.f,-45.f,90.f,180.f,359.f})
 for(double velocity:{.18,.24,.28,.32}) {
  ++bridgeRuns;scaffold::ResetSnapWireHistory();
  java::blocks.clear();for(int x=-1;x<=1;++x)for(int z=-1;z<=1;++z)java::blocks.emplace(x,64,z);
  java::position={.5,65,.5};java::cameraYaw=camera;java::grounded=true;
  java::motion={-std::sin(camera*rotation::kToRadians)*velocity,0,std::cos(camera*rotation::kToRadians)*velocity};
  Fixture snap;snap.snap.value=true;snap.mode.value=2;
  rotation::state={};rotation::effective={camera,0};
  scaffold::snapWireHistory.Observe(true,true,camera,0);
  // Enter this span after a previous snap and its symmetric camera return:
  // current camera yaw is normal, but the stored consumed/return deltas match.
  scaffold::snapWireHistory.delta=180;
  scaffold::snapWireHistory.placedDelta=180;
  scaffold::snapWireHistory.rotated=scaffold::snapWireHistory.hasPlaced=true;
  for(int tick=0;tick<160;++tick){
   auto& wire=scaffold::snapWireHistory;
   rotation::state.hasLastRotations=true;rotation::state.lastRotations={wire.yaw,wire.pitch};
   if(snap.BeginSnapTick()){snap.GetBlockInfo();snap.HandleSnap();}
   const double oldY=java::position.y;
   java::position=java::position.Add(java::motion.x,java::motion.y,java::motion.z);
   bool supported=false;
   for(auto [x,y,z]:java::blocks)if(y==64&&java::position.x+.3>x&&java::position.x-.3<x+1&&java::position.z+.3>z&&java::position.z-.3<z+1)supported=true;
   java::grounded=supported && oldY>=65 && java::position.y<=65;
   if(java::grounded){java::position.y=65;java::motion.y=0;}
   else java::motion.y=(java::motion.y-.08)*.98;
   if(java::position.y<64.99){++bridgeFalls;break;}
   const auto angle=rotation::effective;
   if(wire.pendingPlace&&wire.rotated&&wire.delta>2&&std::fabs(wire.delta-wire.placedDelta)<.0001f)++bridgeFlags;
   wire.Observe(true,angle.yaw!=wire.yaw||angle.pitch!=wire.pitch,angle.yaw,angle.pitch);
   event::MotionEvent post{event::EventType::Post,angle.yaw,angle.pitch};snap.OnSnapMotion(post);
  }
  snap.ResetSnap();
 }
 java::bridgeSimulation=false;scaffold::ResetSnapWireHistory();
 assert(bridgeFalls==0 && bridgeFlags==0);
 std::cout<<"Snap bridge support simulation: falls="<<bridgeFalls<<" / "<<bridgeRuns<<", flags="<<bridgeFlags<<"\n";
 std::cout<<"PASS: Snap geometry cases="<<snapCases<<", improved="<<improved
          <<", mean acceleration projection="<<oldProjection/snapCases<<" -> "<<newProjection/snapCases<<" (not running speed)\n";
 std::cout<<"PASS: repeated-delta cases="<<duplicateCases<<", validated alternatives="<<alternativeCases
          <<", 128 translated bridge candidate checks\n";
 std::cout<<"PASS: "<<comparisons<<" Rise/Hypixel fall comparisons, "<<successes<<" matched ray-validated placements\n";
 std::cout<<"PASS: Rise nearest candidates, stable ties, horizontal/floor/ceiling faces, exact blacklist stage, height lock, negative coordinates and ray hit vectors\n";
}
