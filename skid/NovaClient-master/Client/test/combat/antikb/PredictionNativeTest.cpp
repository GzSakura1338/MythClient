#include <cassert>
#include <cmath>
#include <cstdio>
#include "minecraft/modules/combat/antikb/prediction/PredictionVelocity.h"

// Only JNI and the event transport are fakes. The packet classifier, reported
// rotation reader, input bridge and Prediction input callback are production.
#define JNICALL
using jclass = void*;
constexpr bool JNI_TRUE = true, JNI_FALSE = false;
struct Object {
    int kind = 0, id = 0, opcode = 0;
    float yaw = 0, pitch = 0, forward = 0, strafe = 0;
    bool jump = false, sneak = false;
};
using jobject = Object*;
struct Handles {
    int DisconnectPacket = 1, JoinPacket = 2, RespawnPacket = 3,
        KeepAlivePacket = 4, EntityStatusPacket = 5, PositionPacket = 6, PingPacket = 7;
    int StatusEntityId = 10, StatusOpcode = 11, EntityId = 12,
        ReportedYaw = 13, ReportedPitch = 14, UpdateInput = 15,
        MoveSneak = 16, MoveJump = 17, MoveForward = 18, MoveStrafe = 19,
        UseKey = 20, KeyCode = 21, Mouse = 22, MouseButton = 23, Keyboard = 24, KeyboardKey = 25;
} h;
struct JNIEnv {
    bool error = false;
    int physicalClass = 0, physicalCode = 0;
    bool CallStaticBooleanMethod(int owner, int, int code) { physicalClass=owner; physicalCode=code; return true; }
    bool IsInstanceOf(jobject o, int kind) { return o && o->kind == kind; }
    int GetIntField(jobject o, int) { return o->id; }
    int CallByteMethod(jobject o, int) { return o->opcode; }
    int CallIntMethod(jobject o, int) { return o->id; }
    float GetFloatField(jobject o, int field) {
        if (field == h.ReportedYaw) return o->yaw;
        if (field == h.ReportedPitch) return o->pitch;
        return field == h.MoveForward ? o->forward : o->strafe;
    }
    bool GetBooleanField(jobject o, int field) { return field == h.MoveSneak ? o->sneak : o->jump; }
    void SetFloatField(jobject o, int field, float v) {
        (field == h.MoveForward ? o->forward : o->strafe) = v;
    }
    void SetBooleanField(jobject o, int field, bool v) {
        (field == h.MoveSneak ? o->sneak : o->jump) = v;
    }
    void CallVoidMethod(jobject, int) {} // input supplied is already vanilla's result
    bool ExceptionCheck() const { return error; }
} env;
Object self;
Object key;
namespace java { bool screen=false; bool ScreenOpen() { return screen; } }
bool playerPresent = true, resolved = true, inputObserved = true;
struct Frame {
    JNIEnv* env = &::env;
    explicit operator bool() const { return true; }
    jobject player() const { return playerPresent ? &self : nullptr; }
    jobject key(int) const { return &::key; }
};
bool Good(JNIEnv* e) { return !e->error; }
bool EntityById(int) { return resolved; }
bool IsCircuitOpen() { return false; }
void LogTo(const char*, ...) {}
namespace event {
struct Event {};
// @MOVE_EVENT@
struct EventBus {
    static EventBus& Get() { static EventBus bus; return bus; }
    template<class T> bool HasListeners() const { return true; }
};
}
struct Runtime {
    bool active = true;
    nova::module::PredictionSettings settings;
    nova::module::PredictionVelocity state;
    // @MOVE_CALLBACK@
} runtime;
void Dispatch(event::MoveInputEvent& e) { runtime.MoveInput(e); }
// @NATIVE_PRODUCTION@

int main() {
    key.id=-99;
    assert(PhysicalUseDown() && env.physicalClass==h.Mouse && env.physicalCode==1);
    key.id=18;
    assert(PhysicalUseDown() && env.physicalClass==h.Keyboard && env.physicalCode==18);
    java::screen=true; assert(!PhysicalUseDown()); java::screen=false;
    self.id = 27; self.yaw = 137.5f; self.pitch = -16;
    float yaw = 0, pitch = 0;
    assert(PlayerReportedRotation(yaw, pitch) && yaw == 137.5f && pitch == -16);
    env.error = true;
    assert(!PlayerReportedRotation(yaw, pitch));
    env.error = false;
    Object packet{h.EntityStatusPacket, 27, 2};
    assert(PredictionIncomingKind(&packet) == 3); // own hurt stays buffered
    packet.opcode = 3;
    assert(PredictionIncomingKind(&packet) == 4);
    packet.id = 28; packet.opcode = 2;
    assert(PredictionIncomingKind(&packet) == 4);
    resolved = false;
    assert(PredictionIncomingKind(&packet) == 0); // unresolved S19 must be queued
    packet.id = 27;
    assert(PredictionIncomingKind(&packet) == 0);
    resolved = true; playerPresent = false;
    assert(PredictionIncomingKind(&packet) == 0);
    playerPresent = true;
    packet.kind = h.KeepAlivePacket; assert(PredictionIncomingKind(&packet) == 1);
    packet.kind = h.JoinPacket; assert(PredictionIncomingKind(&packet) == 2);
    packet.kind = h.RespawnPacket; assert(PredictionIncomingKind(&packet) == 2);
    packet.kind = h.DisconnectPacket; assert(PredictionIncomingKind(&packet) == 5);

    runtime.settings.autoMove = true; runtime.state.rotateCounter = 2;
    Object input;
    input.sneak = true; input.strafe = -.3f; input.forward = .3f;
    InputHook(&env, nullptr, &input);
    assert(input.forward == 1 && std::abs(input.strafe + .3f) < 1e-6 && input.sneak);
    input = {}; input.strafe = 1;
    InputHook(&env, nullptr, &input);
    assert(input.forward == 1 && input.strafe == 1);
    runtime.state.rotateCounter = 0;
    input.sneak = true; input.forward = -.3f; input.strafe = .3f;
    InputHook(&env, nullptr, &input);
    assert(std::abs(input.forward + .3f) < 1e-6 && std::abs(input.strafe - .3f) < 1e-6);
    runtime.state.rotateCounter = 2; runtime.active = false;
    InputHook(&env, nullptr, &input);
    assert(std::abs(input.forward + .3f) < 1e-6);
    puts("PASS: production last-reported fields, unresolved S19 classification and final sneak input");
}
