#pragma once

#include "minecraft/events/EventBus.h"
#include "minecraft/java/api/base.h"

#include <string>

namespace nova::event {

class GameTickEvent : public Event {
public:
    explicit GameTickEvent(EventType type) : type(type) {}

    EventType type;
};

class PlayerTickEvent : public CancellableEvent {};
class PostPlayerTickEvent : public Event {};
class LivingUpdateEvent : public Event {};
class PrePlayerInteractEvent : public Event {};
// Once per client tick before vanilla mouse-over; modules may finalize rotations.
class ClientRotationEvent : public Event {};
class CursorPickEvent : public Event {
public:
    explicit CursorPickEvent(EventType type, float partialTicks = -1.f)
        : type(type), partialTicks(partialTicks) {}
    EventType type;
    float partialTicks;
};
class BlockDamageEvent : public Event {
public:
    BlockDamageEvent(int x, int y, int z) : x(x), y(y), z(z) {}
    int x, y, z;
};
class RenderFrameEvent : public Event {};

class MotionEvent : public CancellableEvent {
public:
    MotionEvent(EventType type, float yaw, float pitch)
        : type(type), yaw(yaw), pitch(pitch) {}

    MotionEvent(EventType type, double x, double y, double z,
                float yaw, float pitch, bool onGround)
        : type(type), x(x), y(y), z(z), yaw(yaw), pitch(pitch), onGround(onGround) {}

    EventType type;
    double x = 0.0, y = 0.0, z = 0.0;
    float yaw = 0.0f, pitch = 0.0f;
    bool onGround = false;
};

class RespawnEvent : public Event {};

class AppliedVelocityEvent : public Event {
public:
    AppliedVelocityEvent(double x, double y, double z) : x(x), y(y), z(z) {}
    double x, y, z;
};

class SlowdownEvent : public CancellableEvent {};

class JumpEvent : public Event {
public:
    explicit JumpEvent(float yaw) : yaw(yaw) {}

    float yaw;
};

class FallFlyingEvent : public Event {
public:
    explicit FallFlyingEvent(float pitch) : pitch(pitch) {}

    float pitch;
};

class RotationAnimationEvent : public Event {
public:
    RotationAnimationEvent(jobject entity, float yaw, float lastYaw,
                           float pitch, float lastPitch)
        : entity(entity), yaw(yaw), lastYaw(lastYaw), pitch(pitch), lastPitch(lastPitch) {}

    jobject entity;
    float yaw, lastYaw;
    float pitch, lastPitch;
};

class AttackYawEvent : public Event {
public:
    explicit AttackYawEvent(float yaw) : yaw(yaw) {}

    float yaw;
};

class AttackEntityEvent : public Event {
public:
    explicit AttackEntityEvent(jobject target) : target(target) {}
    jobject target;
};

class AttackSlowdownEvent : public CancellableEvent {
public:
    AttackSlowdownEvent(bool ground, int hurt) : onGround(ground), hurtTime(hurt) {}
    double slowdown = 0.6;
    bool sprint = false;
    bool onGround;
    int hurtTime;
};

class StrafeEvent : public Event {
public:
    explicit StrafeEvent(float yaw) : yaw(yaw) {}

    float yaw;
};

class StuckInBlockEvent : public CancellableEvent {
public:
    StuckInBlockEvent(jobject state, jobject stuckSpeedMultiplier)
        : state(state), stuckSpeedMultiplier(stuckSpeedMultiplier) {}

    jobject state;
    jobject stuckSpeedMultiplier;
};

class RayTraceEvent : public Event {
public:
    RayTraceEvent(jobject entity, float yaw, float pitch)
        : entity(entity), yaw(yaw), pitch(pitch) {}

    jobject entity;
    float yaw;
    float pitch;
};

class UseItemRayTraceEvent : public Event {
public:
    UseItemRayTraceEvent(float yaw, float pitch) : yaw(yaw), pitch(pitch) {}

    float yaw;
    float pitch;
};

class PacketEvent : public CancellableEvent {
public:
    PacketEvent(EventType type, jobject packet) : type(type), packet(packet) {}

    EventType type;
    jobject packet;
};

class PositionItemEvent : public CancellableEvent {
public:
    explicit PositionItemEvent(jobject packet) : packet(packet) {}

    jobject packet;
};

class ChatEvent : public CancellableEvent {
public:
    ChatEvent(std::string message, bool isCommand)
        : message(std::move(message)), isCommand(isCommand) {}

    std::string message;
    bool isCommand;
};

class ClickEvent : public CancellableEvent {};

class RenderWorldEvent : public Event {
public:
    RenderWorldEvent(float partialTick, jobject poseStack)
        : partialTick(partialTick), poseStack(poseStack) {}

    float partialTick;
    jobject poseStack;
};

class RenderSkiaEvent : public Event {
public:
    RenderSkiaEvent(jobject guiGraphics, float partialTick)
        : guiGraphics(guiGraphics), partialTick(partialTick) {}

    jobject guiGraphics;
    float partialTick;
};

class MoveInputEvent : public Event {
public:
    MoveInputEvent(float forward, float strafe, bool jump, bool sneak,
                   double sneakSlowDownMultiplier)
        : forward(forward), strafe(strafe), jump(jump), sneak(sneak),
          sneakSlowDownMultiplier(sneakSlowDownMultiplier) {}

    float forward;
    float strafe;
    bool jump;
    bool sneak;
    double sneakSlowDownMultiplier;
    // Optional final value after the input bridge reapplies sneak scaling.
    // Prediction mirrors Leader's write to raw MovementInput.moveForward.
    bool hasFinalForward = false;
    float finalForward = 0;
    bool hasFinalStrafe = false;
    float finalStrafe = 0;
    bool skipRotationFix = false;
    bool suppressSprint = false;
    // Request through vanilla input; suppression from another module wins.
    bool requestSprint = false;
};

}
