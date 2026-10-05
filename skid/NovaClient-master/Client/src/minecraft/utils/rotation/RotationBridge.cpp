#include "minecraft/utils/rotation/RotationBridge.h"

#include <vector>

#include "core/Instance.h"
#include "minecraft/events/EventBus.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/java/api/base.h"
#include "minecraft/java/api/game_entity.h"
#include "minecraft/java/api/game_rotation.h"
#include "minecraft/utils/rotation/RotationManager.h"

namespace nova::rotation {
namespace {

namespace event = nova::event;

std::vector<event::Subscription>& Subscriptions() {
    return Instance<std::vector<event::Subscription>>();
}

bool IsLocalPlayer(jobject entity) {
    if (!entity)
        return false;

    java::EntityHandle player = java::PlayerEntity();
    if (!player.Valid())
        return false;

    JniAttach attach;
    if (!attach)
        return false;
    return attach.env->IsSameObject(entity, static_cast<jobject>(player.raw())) != JNI_FALSE;
}

void HandleRespawn(event::RespawnEvent&) {
    OnRespawn();
}

void HandlePlayerTick(event::PlayerTickEvent&) {
    UpdateGlobalYaw();
}

void HandleAnimation(event::RotationAnimationEvent& e) {
    if (!IsLocalPlayer(e.entity))
        return;

    OnAnimation(e.yaw, e.lastYaw, e.pitch, e.lastPitch);
}

void HandleMotion(event::MotionEvent& e) {
    if (e.type != event::EventType::Pre)
        return;

    OnMotionPre(e.yaw, e.pitch);
}

void HandleMoveInput(event::MoveInputEvent& e) {
    if (e.skipRotationFix) return;
    MoveInput input{e.forward, e.strafe};
    OnMoveInput(input);
    e.forward = input.forward;
    e.strafe = input.strafe;
}

void HandleRayTrace(event::RayTraceEvent& e) {
    java::EntityHandle entity = java::AdoptEntity(e.entity);
    OnRayTrace(entity, e.yaw, e.pitch);
}

void HandleItemRayTrace(event::UseItemRayTraceEvent& e) {
    OnItemRayTrace(e.yaw, e.pitch);
}

void HandleStrafe(event::StrafeEvent& e) {
    OnStrafe(e.yaw);
}

void HandleJump(event::JumpEvent& e) {
    OnJump(e.yaw);
}

void HandlePositionItem(event::PositionItemEvent& e) {
    if (!IsActive() || !State().hasRotations)
        return;

    java::PosRot packet;
    if (!java::ReadPosRot(e.packet, packet))
        return;

    float yaw = packet.yaw;
    float pitch = packet.pitch;
    OnPositionItem(yaw, pitch);

    const java::PosRot replaced{packet.x, packet.y, packet.z, yaw, pitch, packet.onGround, packet.horizontalCollision};
    if (void* made = java::MakePosRot(replaced))
        e.packet = static_cast<jobject>(made);
}

void HandleFallFlying(event::FallFlyingEvent& e) {
    OnFallFlying(e.pitch);
}

void HandleAttack(event::AttackYawEvent& e) {
    OnAttack(e.yaw);
}

}

void InstallBridge() {
    std::vector<event::Subscription>& subscriptions = Subscriptions();
    if (!subscriptions.empty())
        return;

    event::EventBus& bus = event::EventBus::Get();

    subscriptions.push_back(
        bus.Subscribe<event::RespawnEvent>(HandleRespawn, event::Priority::Medium));
    subscriptions.push_back(
        bus.Subscribe<event::PlayerTickEvent>(HandlePlayerTick, event::Priority::Low));
    subscriptions.push_back(
        bus.Subscribe<event::RotationAnimationEvent>(HandleAnimation, event::Priority::Medium));
    subscriptions.push_back(
        bus.Subscribe<event::MotionEvent>(HandleMotion, event::Priority::Lowest));
    subscriptions.push_back(
        bus.Subscribe<event::MoveInputEvent>(HandleMoveInput, event::Priority::High));
    subscriptions.push_back(
        bus.Subscribe<event::RayTraceEvent>(HandleRayTrace, event::Priority::Medium));
    subscriptions.push_back(
        bus.Subscribe<event::UseItemRayTraceEvent>(HandleItemRayTrace, event::Priority::Medium));
    subscriptions.push_back(
        bus.Subscribe<event::StrafeEvent>(HandleStrafe, event::Priority::Medium));
    subscriptions.push_back(
        bus.Subscribe<event::JumpEvent>(HandleJump, event::Priority::Medium));
    subscriptions.push_back(
        bus.Subscribe<event::PositionItemEvent>(HandlePositionItem, event::Priority::Highest));
    subscriptions.push_back(
        bus.Subscribe<event::FallFlyingEvent>(HandleFallFlying, event::Priority::Medium));
    subscriptions.push_back(
        bus.Subscribe<event::AttackYawEvent>(HandleAttack, event::Priority::Medium));
}

}
