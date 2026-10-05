#include "minecraft/compat/minecraft263/Minecraft263Game.h"
#include "minecraft/compat/minecraft263/Minecraft263Access.h"
#include "minecraft/compat/easecation/EaseCation.h"
#include <algorithm>
#include <cmath>

namespace nova::compat::mc263 {
using namespace names;
using access::Frame;
namespace {
constexpr names::Member listSize{"java.util.List", "size", "()I"};
constexpr names::Member listGet{"java.util.List", "get", "(I)Ljava/lang/Object;"};
constexpr names::Member ordinal{"java.lang.Enum", "ordinal", "()I"};
jobject Vector(Frame& f, const rotation::Vec3& v) { return f.create(Vec3_Ctor, v.x, v.y, v.z); }
bool ReadVector(Frame& f, jobject v, rotation::Vec3& out) {
    if (!v) return false;
    out = {f.get<double>(v, Vec3_Field_x), f.get<double>(v, Vec3_Field_y), f.get<double>(v, Vec3_Field_z)};
    return f.check();
}
bool ReadList(Frame& f, jobject list, std::vector<java::EntityHandle>& out) {
    if (!list) return false;
    const int n = f.call<int>(list, listSize);
    for (int i = 0; i < n && f; ++i) {
        auto object = f.call(list, listGet, i);
        if (object) out.push_back(java::AdoptEntity(object));
        if (object) f.env->DeleteLocalRef(object);
    }
    return f.check();
}
}
bool ReadEntityBool(void* object, EntityBool prop, bool& out) {
    Frame f; auto obj = static_cast<jobject>(object); if (!f || !obj) return false;
    if (prop == EntityBool::Player) out = f.instance(obj, names::Player);
    else {
        const auto m = prop == EntityBool::Spectator ? Entity_isSpectator : prop == EntityBool::Pickable ? Entity_isPickable :
            prop == EntityBool::Alive ? Entity_isAlive : prop == EntityBool::Invisible ? Entity_isInvisible : Entity_onGround;
        out = f.call<bool>(obj, m);
    }
    return f.check();
}
bool ReadEntityInt(void* object, EntityInt prop, int& out) {
    Frame f; auto obj = static_cast<jobject>(object); if (!f || !obj) return false;
    if ((prop == EntityInt::Hurt || prop == EntityInt::Death) && !f.instance(obj, names::LivingEntity)) return false;
    out = prop == EntityInt::Id ? f.call<int>(obj, Entity_getId) : f.get<int>(obj,
        prop == EntityInt::Ticks ? Entity_Field_tickCount : prop == EntityInt::Hurt ? LivingEntity_Field_hurtTime : LivingEntity_Field_deathTime);
    return f.check();
}
bool ReadEntityFloat(void* object, EntityFloat prop, float& out) {
    Frame f; auto obj = static_cast<jobject>(object); if (!f || !obj) return false;
    if (prop == EntityFloat::FallDistance) out = static_cast<float>(f.get<double>(obj, Entity_Field_fallDistance));
    else {
        if ((prop == EntityFloat::Health || prop == EntityFloat::Absorption || prop == EntityFloat::MaxHealth) && !f.instance(obj, names::LivingEntity)) return false;
        out = f.call<float>(obj, prop == EntityFloat::EyeHeight ? Entity_getEyeHeight_2 : prop == EntityFloat::PickRadius ? Entity_getPickRadius :
            prop == EntityFloat::Health ? LivingEntity_getHealth : prop == EntityFloat::Absorption ? LivingEntity_getAbsorptionAmount : LivingEntity_getMaxHealth);
    }
    return f.check();
}
bool EntityPosition(void* object, rotation::Vec3& out, bool previous) {
    Frame f; auto obj = static_cast<jobject>(object); if (!f || !obj) return false;
    if (previous) out = {f.get<double>(obj, Entity_Field_xo), f.get<double>(obj, Entity_Field_yo), f.get<double>(obj, Entity_Field_zo)};
    else return ReadVector(f, f.call(obj, Entity_position), out);
    return f.check();
}
bool EntityRenderPositionPair(void* object, rotation::Vec3& now, rotation::Vec3& before) {
    Frame f;auto obj=static_cast<jobject>(object);if(!obj||!EntityPosition(object,now))return false;
    // EntityRenderer interpolates xOld; Entity.getPosition uses the separate xo history.
    before={f.get<double>(obj,Entity_Field_xOld),f.get<double>(obj,Entity_Field_yOld),f.get<double>(obj,Entity_Field_zOld)};
    return f.check();
}
bool EntityEyePosition(void* object, float partialTicks, rotation::Vec3& out) {
    Frame f; return ReadVector(f, f.call(static_cast<jobject>(object), Entity_getEyePosition_2, partialTicks), out);
}
bool EntityBox(void* object, java::Box& out) {
    Frame f; auto box = f.call(static_cast<jobject>(object), Entity_getBoundingBox); if (!box) return false;
    out = {f.get<double>(box, AABB_Field_minX), f.get<double>(box, AABB_Field_minY), f.get<double>(box, AABB_Field_minZ),
           f.get<double>(box, AABB_Field_maxX), f.get<double>(box, AABB_Field_maxY), f.get<double>(box, AABB_Field_maxZ)};
    return f.check();
}
bool EntityName(void* object, std::string& out, bool formatted) {
    Frame f; auto obj = static_cast<jobject>(object); if (!obj) return false;
    auto name = formatted ? f.call(f.call(obj, Entity_getDisplayName), Component_getString) : f.call(obj, Entity_getScoreboardName);
    out = f.text(name); return name && f.check();
}
bool EntitySameTeam(void* object, void* other, bool& out) {
    Frame f; if (!object || !other) return false;
    out = f.call<bool>(static_cast<jobject>(object), Entity_isAlliedTo, static_cast<jobject>(other)); return f.check();
}
bool EntitySameVehicle(void* object, void* other) {
    Frame f; if (!object || !other) return false;
    auto a = f.call(static_cast<jobject>(object), Entity_getRootVehicle);
    auto b = f.call(static_cast<jobject>(other), Entity_getRootVehicle);
    return a && b && f.env->IsSameObject(a,b) && f.check();
}
bool EntitySetHealth(void* object, float value) {
    Frame f; auto obj = static_cast<jobject>(object);
    return f.instance(obj, names::LivingEntity) && f.invoke(obj, LivingEntity_setHealth, value);
}
java::EntityHandle PlayerEntity() { Frame f; return f ? java::AdoptEntity(f.player()) : java::EntityHandle{}; }
java::EntityHandle LevelEntity() { Frame f; return f ? java::AdoptEntity(f.level()) : java::EntityHandle{}; }
java::EntityHandle EntityById(int id) { Frame f; return java::AdoptEntity(f.call(f.level(), ClientLevel_getEntity, id)); }
bool WorldPlayers(std::vector<java::EntityHandle>& out) {
    out.clear(); Frame f; return ReadList(f, f.call(f.level(), ClientLevel_players), out);
}
bool WorldEntities(const java::EntityHandle& source, const java::Box& area, std::vector<java::EntityHandle>& out) {
    out.clear(); Frame f;
    // Iterate the client entity collection; avoids passing a null Predicate to Level.
    auto iterable = f.call(f.level(), ClientLevel_entitiesForRendering);
    auto it = f.call(iterable, {"java.lang.Iterable", "iterator", "()Ljava/util/Iterator;"});
    while (f.call<bool>(it, {"java.util.Iterator", "hasNext", "()Z"})) {
        auto obj = f.call(it, {"java.util.Iterator", "next", "()Ljava/lang/Object;"});
        if (!obj) break;
        auto handle = java::AdoptEntity(obj);
        java::Box box;
        if ((!source || !handle.IsSame(source)) && EntityBox(handle.raw(), box) &&
            box.maxX > area.minX && box.minX < area.maxX && box.maxY > area.minY && box.minY < area.maxY &&
            box.maxZ > area.minZ && box.minZ < area.maxZ) out.push_back(std::move(handle));
        f.env->DeleteLocalRef(obj);
    }
    return it && f.check();
}
float FrameTime() {
    Frame f; auto dt = f.call(f.client(), Minecraft_getDeltaTracker);
    auto value = f.call<float>(dt, DeltaTracker_getGameTimeDeltaPartialTick, JNI_FALSE);
    return f ? std::clamp(value, 0.0f, 1.0f) : 0.0f;
}
bool EntityTypes(const java::EntityHandle& entity, unsigned& out) {
    Frame f; auto obj = static_cast<jobject>(entity.raw()); out = 0; if (!obj) return false;
    if (f.instance(obj, names::LivingEntity)) out |= java::kTypeLiving;
    if (f.instance(obj, names::Player)) out |= java::kTypePlayer;
    if (f.instance(obj, names::AbstractVillager)) out |= java::kTypeVillager;
    if (f.instance(obj, names::Monster) || f.instance(obj, names::Slime) || f.instance(obj, names::EnderDragon) || f.instance(obj, names::EnderDragonPart)) out |= java::kTypeMonster;
    // Modern squid/dolphins inherit AgeableWaterCreature, not Animal/WaterAnimal.
    if (f.instance(obj, names::Animal) || f.instance(obj, names::AmbientCreature) ||
        f.instance(obj, names::WaterAnimal) || f.instance(obj, names::AgeableWaterCreature)) out |= java::kTypeAnimal;
    return f.check();
}
bool WorldClip(const rotation::Vec3& from, const rotation::Vec3& to, const java::EntityHandle& source, java::BlockHitResultInfo& out, java::ClipMode mode) {
    Frame f; auto level = f.level(); if (!level || !source) return false;
    auto ctx = f.create(ClipContext_Ctor, Vector(f, from), Vector(f, to),
        f.stat(mode==java::ClipMode::Collision?ClipContext_Block_Field_COLLIDER:ClipContext_Block_Field_OUTLINE),
        f.stat(mode==java::ClipMode::WaterSource?ClipContext_Fluid_Field_SOURCE_ONLY:ClipContext_Fluid_Field_NONE), static_cast<jobject>(source.raw()));
    auto hit = f.call(level, BlockGetter_clip, ctx); if (!hit) return false;
    out = {}; out.kind = static_cast<java::HitKind>(f.call<int>(f.call(hit, HitResult_getType), ordinal));
    if (!ReadVector(f, f.call(hit, HitResult_getLocation), out.location)) return false;
    if (out.kind == java::HitKind::Block) {
        auto pos = f.call(hit, BlockHitResult_getBlockPos);
        out.blockPos = {f.call<int>(pos, Vec3i_getX), f.call<int>(pos, Vec3i_getY), f.call<int>(pos, Vec3i_getZ)};
        out.direction = f.call<int>(f.call(hit, BlockHitResult_getDirection), ordinal);
        out.hasBlockPos = true;
    }
    return f.check();
}
java::EntityHandle MakeEntityHit(const java::EntityHandle& entity, const rotation::Vec3& location) {
    Frame f; return entity ? java::AdoptEntity(f.create(EntityHitResult_Ctor_2, static_cast<jobject>(entity.raw()), Vector(f, location))) : java::EntityHandle{};
}
bool BoxClip(const java::Box& box, const rotation::Vec3& from, const rotation::Vec3& to, bool& hit, rotation::Vec3& out) {
    Frame f; hit = false;
    auto bounds = f.create(AABB_Ctor, box.minX, box.minY, box.minZ, box.maxX, box.maxY, box.maxZ);
    auto optional = f.call(bounds, AABB_clip, Vector(f, from), Vector(f, to));
    if (f.call<bool>(optional, {"java.util.Optional", "isPresent", "()Z"}))
        hit = ReadVector(f, f.call(optional, {"java.util.Optional", "get", "()Ljava/lang/Object;"}), out);
    return optional && f.check();
}
bool DirectionFromRotation(float pitch, float yaw, rotation::Vec3& out) {
    Frame f; return ReadVector(f, f.staticCall(Vec3_directionFromRotation_2, pitch, yaw), out);
}
bool PlayerRotation(float& yaw, float& pitch, bool previous) {
    Frame f; auto p = f.player(); if (!p) return false;
    yaw = f.get<float>(p, previous ? Entity_Field_yRotO : Entity_Field_yRot);
    pitch = f.get<float>(p, previous ? Entity_Field_xRotO : Entity_Field_xRot);
    return f.check();
}
bool SetPlayerRotation(float yaw, float pitch) {
    Frame f; auto p = f.player();
    return f.invoke(p, Entity_setYRot, yaw) && f.invoke(p, Entity_setXRot, pitch);
}
bool PlayerEyePosition(double& x, double& y, double& z) {
    auto player = mc263::PlayerEntity(); rotation::Vec3 v;
    if (!EntityEyePosition(player.raw(), 1, v)) return false;
    x=v.x; y=v.y; z=v.z; return true;
}
namespace {
jobject Option(Frame& f, names::Member m) { return f.get(f.call(f.get(f.client(), Minecraft_Field_options), m), OptionInstance_Field_value); }
}
bool PlayerSensitivity(double& out) { Frame f; auto o=Option(f, Options_sensitivity); out=f.call<double>(o, {"java.lang.Double", "doubleValue", "()D"}); return o && f.check(); }
bool RenderDistance(int& out) { Frame f; auto o=Option(f, Options_renderDistance); out=f.call<int>(o, {"java.lang.Integer", "intValue", "()I"}); return o && f.check(); }
bool Fov(int& out) { Frame f; auto o=Option(f, Options_fov); out=f.call<int>(o, {"java.lang.Integer", "intValue", "()I"}); return o && f.check(); }
int ClientFps() { Frame f; return f.call<int>(f.client(), Minecraft_getFps); }
void PickCursor(float partialTicks) { Frame f; if(UsesGlfwModernRuntime())f.invoke(f.get(f.client(),Minecraft_Field_gameRenderer),easecation::names::Pick,partialTicks);else f.invoke(f.client(), Minecraft_pick, partialTicks); }
bool PlayerOnGround() { Frame f; return f.call<bool>(f.player(), Entity_onGround); }
bool PlayerUsingItem() { Frame f; return f.call<bool>(f.player(), LivingEntity_isUsingItem); }
bool PlayerAttackCooldownReady() {
    Frame f;
    if (!f) return false;
    auto player = f.player();
    if (!player) return false;
    const float strength = f.call<float>(player, Player_getAttackStrengthScale, 0.0f);
    return f.check() && std::isfinite(strength) && strength >= 1.0f;
}
bool PlayerInCreative() { Frame f; return f.get<bool>(f.call(f.player(), Player_getAbilities), Abilities_Field_instabuild); }
bool PlayerCanEdit() { Frame f; return f.get<bool>(f.call(f.player(), Player_getAbilities), Abilities_Field_mayBuild); }
bool PlayerDeltaMovement(rotation::Vec3& out) { Frame f; return ReadVector(f, f.call(f.player(), Entity_getDeltaMovement), out); }
bool SetPlayerDeltaMovement(double x, double y, double z) { Frame f; return f.invoke(f.player(), Entity_setDeltaMovement_2, x,y,z); }
bool SetPlayerSprinting(bool value) { Frame f; return f.invoke(f.player(), Entity_setSprinting, value ? JNI_TRUE : JNI_FALSE); }
bool PlayerSprinting(bool& out) { Frame f; auto p=f.player(); out=f.call<bool>(p, Entity_isSprinting); return p && f.check(); }
bool PlayerSpeed(float& out) { Frame f; auto p=f.player(); out=f.call<float>(p, LivingEntity_getSpeed); return p && f.check(); }
bool ReadCompassView(float partial, java::CompassView& out) {
    auto p=mc263::PlayerEntity(); rotation::Vec3 now, old; float yaw,pitch,previousYaw,previousPitch;
    if (!EntityRenderPositionPair(p.raw(),now,old) || !PlayerRotation(yaw,pitch) || !PlayerRotation(previousYaw,previousPitch,true)) return false;
    out.x=old.x+(now.x-old.x)*partial; out.z=old.z+(now.z-old.z)*partial;
    out.yaw=previousYaw+std::remainder(yaw-previousYaw,360.0f)*partial; return true;
}
bool WindowMetrics(int& width, int& height, double& scale) {
    Frame f; auto w=f.call(f.client(),Minecraft_getWindow); if (!w) return false;
    width=f.call<int>(w,Window_getWidth); height=f.call<int>(w,Window_getHeight); scale=f.call<int>(w,Window_getGuiScale);
    return f.check() && width>0 && height>0 && scale>0;
}
void JNICALL WorldCameraHook(JNIEnv* env, jclass, jobject camera, jobject modelView, jobject projection) {
    java::InvalidateWorldCamera();
    if (IsCircuitOpen() || env->ExceptionCheck()) return;
    Frame f(env);
    rotation::Vec3 position;
    if (!ReadVector(f, f.call(camera, easecation::names::CameraPos), position)) return;
    java::CameraView view;
    view.x=position.x; view.y=position.y; view.z=position.z;
    view.yaw=f.call<float>(camera,easecation::names::CameraYaw);
    view.pitch=f.call<float>(camera,easecation::names::CameraPitch);
    auto array=f.env->NewFloatArray(16);
    if (!array || !f.check()) return;
    auto copy=[&](jobject matrix,float* values) {
        if (!f.call(matrix,{"org.joml.Matrix4f","get","([F)[F"},array)) return false;
        f.env->GetFloatArrayRegion(array,0,16,values);
        return f.check();
    };
    double scale=1;
    view.hasModelView=copy(modelView,view.modelView);
    if (view.hasModelView && copy(projection,view.projection) &&
        WindowMetrics(view.windowWidth,view.windowHeight,scale) && f.check())
        java::PublishWorldCamera(view);
}

bool ReadCameraView(float partialTick, java::CameraView& out) {
    Frame f; auto renderer=f.get(f.client(),Minecraft_Field_gameRenderer);
    if(UsesGlfwModernRuntime()) return java::ReadWorldCamera(out);
    auto state=f.get(f.call(renderer,GameRenderer_gameRenderState),GameRenderState_Field_levelRenderState);
    auto cam=f.get(state,LevelRenderState_Field_cameraRenderState);
    if (!cam || !f.get<bool>(cam,CameraRenderState_Field_initialized)) return false;
    rotation::Vec3 pos; if (!ReadVector(f,f.get(cam,CameraRenderState_Field_pos),pos)) return false;
    out={}; out.x=pos.x;out.y=pos.y;out.z=pos.z;
    out.yaw=f.get<float>(cam,CameraRenderState_Field_yRot);out.pitch=f.get<float>(cam,CameraRenderState_Field_xRot);
    auto matrix=[&](names::Member field,float* values) {
        auto m=f.get(cam,field); auto a=f.env->NewFloatArray(16);
        f.call(m,{"org.joml.Matrix4f","get","([F)[F"},a);
        if (!f.check()) return false;
        f.env->GetFloatArrayRegion(a,0,16,values);return f.check();
    };
    double scale;
    out.hasModelView=matrix(CameraRenderState_Field_viewRotationMatrix,out.modelView);
    return out.hasModelView && matrix(CameraRenderState_Field_projectionMatrix,out.projection) && mc263::WindowMetrics(out.windowWidth,out.windowHeight,scale);
}
}
