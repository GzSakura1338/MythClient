#include "minecraft/java/api/game_fakeplayer.h"
#include "minecraft/java/api/game_crystal.h"
#include "minecraft/java/api/game_rotation.h"
#include "minecraft/compat/minecraft263/Minecraft263Access.h"
#include "minecraft/modules/misc/fakeplayer/Damage.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>

namespace nova::java {
namespace {
using namespace compat::mc263::access;
using namespace compat::mc263::names;
struct Mapping {Member ctor,add,remove,setId,move,skinInfo,info,equip,duration,absorption,copy;};
#define NOVA_FAKEPLAYER_MAPPING
#define NOVA_FAKE(key,owner,name,desc) .key={owner,name,desc},
constexpr Mapping mapping{
#include "minecraft/compat/mappings/minecraft263.inc"
};
#undef NOVA_FAKE
#undef NOVA_FAKEPLAYER_MAPPING
std::recursive_mutex mutex;
std::condition_variable_any stopped;
FakePlayerSettings requested;
std::atomic<FakePlayerStatus> status{FakePlayerStatus::Off};
std::atomic<int> reservedId{0};
EntityHandle dummy,world,player;
bool inserted=false;
rotation::Vec3 position;
float yaw=0,pitch=0;
module::fakeplayer::HitState health;
struct Explosion {rotation::Vec3 position;float radius;};
std::deque<Explosion> explosions;

bool SameWorld(){auto w=LevelEntity(),p=PlayerEntity();return w&&p&&w.IsSame(world)&&p.IsSame(player);}
bool Remove() {
    // Entity.discard performs the world's own storage callback. Never remove by
    // a reused id, and retry a failed cleanup before allowing DLL unmapping.
    if(dummy&&inserted) {
        Frame f;
        if(!f.invoke(static_cast<jobject>(dummy.raw()),mapping.remove))return false;
    }
    inserted=false;dummy.Reset();world.Reset();player.Reset();explosions.clear();
    stopped.notify_all();return true;
}
void Finish(FakePlayerStatus result) {
    requested.enabled=false;
    if(Remove())status=result;
}
bool Start() {
    Frame f;auto local=PlayerEntity(),level=LevelEntity();
    if(!f||!local||!level||!local.IsAlive()||local.IsSpectator())return false;
    auto p=static_cast<jobject>(local.raw()),w=static_cast<jobject>(level.raw());
    if(!local.Position(position)||!PlayerRotation(yaw,pitch))return false;
    int id=-900001;
    for(int i=0;i<128&&EntityById(id);++i)--id;
    if(EntityById(id))return false;
    auto uuid=f.staticCall({"java.util.UUID","randomUUID","()Ljava/util/UUID;"});
    auto name=f.env->NewStringUTF("FakePlayer");
    if(!uuid||!name||!f.check())return false;
    auto profile=f.create({"com.mojang.authlib.GameProfile","<init>","(Ljava/util/UUID;Ljava/lang/String;)V"},uuid,name);
    if(!profile)return false;
    dummy=AdoptEntity(f.create(mapping.ctor,w,profile));if(!dummy)return false;
    world=std::move(level);player=std::move(local);auto d=static_cast<jobject>(dummy.raw());
    auto skin=f.call(p,mapping.skinInfo);
    // Reuse the already-loaded skin information without sharing identity or inventory.
    if(skin&&!f.set(d,mapping.info,skin))return false;
    if(!f.invoke(d,mapping.setId,id)||!f.invoke(d,mapping.move,position.x,position.y,position.z))return false;
    for(const auto field:{Entity_Field_yRot,Entity_Field_yRotO,LivingEntity_Field_yHeadRot,LivingEntity_Field_yHeadRotO,
                         LivingEntity_Field_yBodyRot,LivingEntity_Field_yBodyRotO})if(!f.set(d,field,yaw))return false;
    if(!f.set(d,Entity_Field_xRot,pitch)||!f.set(d,Entity_Field_xRotO,pitch))return false;
    if(!f.set(d,Entity_Field_xo,position.x)||!f.set(d,Entity_Field_yo,position.y)||!f.set(d,Entity_Field_zo,position.z)||
       !f.set(d,Entity_Field_xOld,position.x)||!f.set(d,Entity_Field_yOld,position.y)||!f.set(d,Entity_Field_zOld,position.z))return false;
    if(requested.copyEquipment)for(const auto slot:{EquipmentSlot_Field_HEAD,EquipmentSlot_Field_CHEST,EquipmentSlot_Field_LEGS,
                                                 EquipmentSlot_Field_FEET,EquipmentSlot_Field_MAINHAND,EquipmentSlot_Field_OFFHAND}) {
        auto s=f.stat(slot),stack=f.call(p,LivingEntity_getItemBySlot,s),copy=f.call(stack,mapping.copy);
        if(!copy||!f.invoke(d,mapping.equip,s,copy))return false;
    }
    health={};health.health=std::clamp(requested.health,1.f,20.f);
    if(!f.invoke(d,LivingEntity_setHealth,health.health)||!f.invoke(d,mapping.absorption,0.f))return false;
    reservedId=id;inserted=true;
    if(!f.invoke(w,mapping.add,d))return false;
    auto live=EntityById(id);if(!live||!live.IsSame(dummy))return false;
    status=FakePlayerStatus::Active;LogTo("FakePlayer: created local dummy id=%d health=%.0f",id,health.health);return true;
}
void Hurt(double damage) {
    if(!requested.damage||!health.Hit(damage,requested.autoReset,requested.health))return;
    Frame f;auto d=static_cast<jobject>(dummy.raw());
    f.set(d,LivingEntity_Field_hurtTime,10);f.set(d,mapping.duration,10);
    f.invoke(d,LivingEntity_setHealth,health.health);f.invoke(d,mapping.absorption,health.absorption);
    if(health.health<=0)Finish(FakePlayerStatus::Finished);
}
}
bool FakePlayerSupported(){return compat::IsLunar12111()||compat::IsMinecraft263();}
void ConfigureFakePlayer(const FakePlayerSettings& settings) {
    std::lock_guard lock(mutex);
    if(settings.enabled&&!requested.enabled)status=FakePlayerStatus::Waiting;
    requested=settings;
}
FakePlayerStatus GetFakePlayerStatus(){return status.load();}
bool IsFakePlayer(void* entity) {
    if(!entity||status!=FakePlayerStatus::Active)return false;
    std::lock_guard lock(mutex);Frame f;
    return f&&dummy&&f.env->IsSameObject(static_cast<jobject>(entity),static_cast<jobject>(dummy.raw()));
}
void TickFakePlayer() {
    std::lock_guard lock(mutex);
    if(!requested.enabled||IsCircuitOpen()){if(Remove()&&status!=FakePlayerStatus::Failed&&status!=FakePlayerStatus::Finished)status=FakePlayerStatus::Off;return;}
    if(!FakePlayerSupported()){Finish(FakePlayerStatus::Failed);return;}
    if(dummy&&(!SameWorld()||!player.IsAlive())){Finish(FakePlayerStatus::Finished);return;}
    if(!dummy){if(!Start()){LogTo("FakePlayer: spawn failed (world unavailable or JNI lookup failed)");Finish(FakePlayerStatus::Failed);}return;}
    auto live=EntityById(reservedId);if(!live||!live.IsSame(dummy)){Finish(FakePlayerStatus::Finished);return;}
    health.Tick();Frame f;auto d=static_cast<jobject>(dummy.raw());
    // RemotePlayer has no AI. Pin server-less push drift to the spawn point.
    if(!f.invoke(d,mapping.move,position.x,position.y,position.z)||!f.invoke(d,Entity_setDeltaMovement_2,0.,0.,0.)) {Finish(FakePlayerStatus::Failed);return;}
    while(!explosions.empty()&&dummy) {
        const auto explosion=explosions.front();explosions.pop_front();
        if(!requested.damage)continue;
        CrystalDefense defense;double exposure=0;
        if(!ReadCrystalDefense(dummy,defense)||!CrystalExposure(dummy,explosion.position,exposure))continue;
        const auto raw=module::fakeplayer::Explosion(position.DistanceTo(explosion.position),exposure,explosion.radius,defense.difficulty);
        Hurt(module::fakeplayer::Mitigate(raw,defense.armor,defense.toughness,defense.resistance,defense.protection));
    }
}
bool AttackFakePlayer(JNIEnv* env,jobject target) {
    if(!IsFakePlayer(target))return false;
    std::lock_guard lock(mutex);
    if(!dummy||!SameWorld()||!requested.damage)return true;
    Frame f(env);auto self=f.player();
    const double attack=f.call<double>(self,LivingEntity_getAttributeValue,f.stat(Attributes_Field_ATTACK_DAMAGE));
    const float strength=f.call<float>(self,Player_getAttackStrengthScale,.5f);
    f.invoke(self,Player_resetAttackStrengthTicker);
    CrystalDefense defense;
    if(!f.check()||!ReadCrystalDefense(dummy,defense))return true;
    // Ordinary melee approximation; no invented server hit or entity-id packet.
    Hurt(module::fakeplayer::Mitigate(module::fakeplayer::Melee(attack,strength),defense.armor,defense.toughness,defense.resistance,0));
    return true;
}
bool BlocksFakePlayerPacket(JNIEnv* env,jobject packet) {
    const int id=reservedId.load();if(id==0||!env||!packet)return false;
    Frame f(env);
    return f.instance(packet,ServerboundInteractPacket)&&f.get<int>(packet,ServerboundInteractPacket_Field_entityId)==id&&f.check();
}
void ObserveFakePlayerExplosion(JNIEnv* env,jobject packet) {
    if(status!=FakePlayerStatus::Active||!env||!packet)return;
    Frame f(env);if(!f.instance(packet,ClientboundExplodePacket))return;
    auto center=f.get(packet,ClientboundExplodePacket_Field_center);
    Explosion e{{f.get<double>(center,Vec3_Field_x),f.get<double>(center,Vec3_Field_y),f.get<double>(center,Vec3_Field_z)},
                f.get<float>(packet,ClientboundExplodePacket_Field_radius)};
    if(!f.check()||!center||!std::isfinite(e.position.x+e.position.y+e.position.z)||!std::isfinite(e.radius)||e.radius<=0||e.radius>64)return;
    std::lock_guard lock(mutex);
    // Only immutable values cross the network/game-thread boundary; bounded queue.
    if(status==FakePlayerStatus::Active&&requested.enabled&&explosions.size()<64)explosions.push_back(e);
}
bool WaitFakePlayerStopped(unsigned milliseconds) {
    std::unique_lock lock(mutex);requested.enabled=false;
    return stopped.wait_for(lock,std::chrono::milliseconds(milliseconds),[]{return !dummy&&!inserted;});
}
}
