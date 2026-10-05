#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <format>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <tuple>
#include <vector>
#include "minecraft/events/EventBus.h"

namespace mock {
enum Kind { Motion, Ordinary, Position, Explosion, Respawn, Disconnect, Move, Teleport, Ping, LookAt, Attack, Interact, Animation, Flying, TransactionReply, Join };
struct Packet { int serial; Kind kind; int entity = 7, x = 100, y = 100, z = 0; };
bool player = true, ground = true, failRef = false, legacy = false;
int hurt = 0, refs = 0, wanted = 0;
int playerIdentity = 1, worldIdentity = 2, ticks = 100;
std::function<void(bool)> endSlinky;
std::function<void(bool)> syncSlinky;
std::function<bool()> slinkyLagging;
std::function<bool()> hypixelLagging;
std::function<void(bool)> endHypixel;
bool target = false, targetPlayer = true, alive = true, aura = false, scaffold = false;
bool screen = false, menu = false, teammate = false, blockManaged = false;
int queuedAttacks = 0;
std::function<void()> onAttack;
float fall = 0;
int rotations = 0;
std::string actions;
std::vector<int> handled;
std::vector<Kind> wire;
std::function<void(Packet*)> replayObserver;
void Check(bool ok, const char* message) {
    if (!ok) { std::fprintf(stderr, "%s\n", message); std::exit(1); }
}
}
namespace nova::compat { bool IsLegacyLunar() { return mock::legacy; } }
using jobject = mock::Packet*;
void LogTo(const char*, ...) {}
struct MockEnv {
    bool ExceptionCheck(){return false;}
    void ExceptionClear(){}
    jobject NewGlobalRef(jobject packet) {
        if (mock::failRef) return nullptr;
        ++mock::refs;
        return packet;
    }
    void DeleteGlobalRef(jobject) { --mock::refs; }
};
struct JniAttach {
    MockEnv storage;
    MockEnv* env = &storage;
    explicit operator bool() const { return true; }
};
namespace nova::compat::lunar {
enum class PacketKind { EntityTeleport, Join };
bool IsPacket(jobject p,PacketKind k){return p->kind==(k==PacketKind::Join?mock::Join:mock::Teleport);}
bool IsAttackPacket(jobject p){return p->kind==mock::Attack;}
void EndSlinkyBlockLag(bool send = true) { if(mock::endSlinky)mock::endSlinky(send); }
bool SlinkyBlockLagging() { return mock::slinkyLagging && mock::slinkyLagging(); }
bool HypixelBlockLagging() { return mock::hypixelLagging && mock::hypixelLagging(); }
void EndHypixelBlockLag(bool send = true) { if(mock::endHypixel)mock::endHypixel(send); }
}

namespace Properties {
struct NumberProp { double value; double get() const { return value; } };
struct BoolProperty { bool value; bool get() const { return value; } };
struct ModeProperty { int value; int get() const { return value; } };
struct TestRow { void* prop; void* getProp() const { return prop; } operator int() const { return 0; } };
}
namespace nova::module {
enum class Category { Combat };
class Module {
public:
    Module(const char*, Category, const char*, const char*) {}
    virtual ~Module() = default;
    virtual std::string suffix() const { return {}; }
    bool isEnabled() const { return enabled; }
    void setEnabled(bool value) {
        if (enabled == value) return;
        enabled = value;
        if (value) onEnable(); else onDisable();
    }
    std::map<std::string, Properties::NumberProp> numbers;
    std::map<std::string, Properties::BoolProperty> bools;
    std::map<std::string, Properties::ModeProperty> modes;
protected:
    Properties::TestRow subNumber(const char* label, double, double, double, double initial,
        std::function<std::string(double)>) {
        return {&numbers.emplace(label, Properties::NumberProp{initial}).first->second};
    }
    virtual void onUpdate() {}
    virtual void onEnable() {}
    virtual void onDisable() {}
    Properties::NumberProp& addNumber(const char* label, double, double, double,
        double initial, std::vector<int>, std::function<std::string(double)>, std::function<bool()> = {}) {
        return numbers.emplace(label, Properties::NumberProp{initial}).first->second;
    }
    Properties::BoolProperty& addBool(const char* label, bool initial,
        std::vector<int> = {}, std::function<bool()> = {}) {
        return bools.emplace(label, Properties::BoolProperty{initial}).first->second;
    }
    Properties::ModeProperty& addMode(const char* label, int initial, std::vector<std::string>, std::vector<int> = {}) {
        return modes.emplace(label, Properties::ModeProperty{initial}).first->second;
    }
private:
    bool enabled = false;
};
// These tests exercise the legacy Delay/Reduce paths; Prediction has its own
// production-core, native queue, hook and property fixtures.
struct PredictionSettings {};
class PredictionModuleBase : public Module {
protected:
    PredictionModuleBase() : Module("AntiKB", Category::Combat, "General", "General") {}
    std::vector<int> BuildPredictionSettings(std::function<bool()>) { return {}; }
    PredictionSettings ReadPredictionSettings() const { return {}; }
};
namespace prediction {
void Initialize() {}
void Shutdown() {}
void Configure(bool, const PredictionSettings&) {}
bool OwnsIncoming() { return false; }
}
namespace mix {
struct Settings {};
bool enabled = false;
void Initialize() {}
void Shutdown() { enabled = false; }
void Configure(bool value, const Settings&) { enabled = value; }
bool OwnsIncoming() { return false; }
}
class MixModuleBase : public PredictionModuleBase {
protected:
    void AddMixSettings(std::function<bool()>) {}
    mix::Settings ReadMixSettings() const { return {}; }
};
}
namespace nova::java {
struct EntityHandle {
    bool valid = false;
    int identity = 0;
    explicit operator bool() const { return valid; }
    const void* raw() const { return valid ? this : nullptr; }
    bool IsPlayer() const { return mock::targetPlayer; }
    bool IsAlive() const { return mock::alive; }
    bool IsSame(const EntityHandle& other) const { return valid && other.valid && identity==other.identity; }
    bool TickCount(int& value) const { value=mock::ticks;return valid; }
    void Reset() { valid=false;identity=0; }
};
EntityHandle PlayerEntity() { return {mock::player,mock::playerIdentity}; }
EntityHandle LevelEntity() { return {mock::worldIdentity!=0,mock::worldIdentity}; }
bool EntityId(EntityHandle, int& id) { id = 7; return true; }
bool PlayerOnGround() { return mock::ground; }
int PlayerHurtTime() { return mock::hurt; }
void SetIncomingPacketsWanted(bool value) { mock::wanted += value ? 1 : -1; }
bool HandleIncomingPacket(jobject packet) {
    mock::handled.push_back(packet->serial);
    if (mock::replayObserver) mock::replayObserver(packet);
    return true;
}
bool IsMotionPacket(jobject packet) { return packet->kind == mock::Motion; }
bool IsDisconnectPacket(jobject packet) { return packet->kind == mock::Disconnect; }
bool IsRespawnPacket(jobject packet) { return packet->kind == mock::Respawn; }
bool IsPingPacket(jobject packet) { return packet->kind == mock::Ping; }
bool IsPlayerPositionPacket(jobject packet) { return packet->kind == mock::Position; }
bool IsExplodePacket(jobject packet) { return packet->kind == mock::Explosion; }
bool IsReduceDelayPacket(jobject packet) {
    return packet->kind == mock::Motion || packet->kind == mock::Position ||
        packet->kind == mock::Move || packet->kind == mock::Teleport ||
        packet->kind == mock::Ping || packet->kind == mock::LookAt;
}
bool PlayerFallDistance(float& out) { out = mock::fall; return true; }
bool AttackTargetEntity(EntityHandle& out) { out.valid = mock::target; return out.valid; }
bool AttackEntity(const EntityHandle&) { mock::actions += 'A'; return true; }
bool AttackAndSwing(const EntityHandle&) { mock::actions += "HA"; if (mock::onAttack) mock::onAttack(); return true; }
bool ScreenOpen() { return mock::screen; }
bool SwingHand() { mock::actions += 'H'; return true; }
bool SendSwingPacket() { mock::actions += 'S'; return true; }
bool ReadMotionPacket(jobject packet, int& id, int& x, int& y, int& z) {
    id = packet->entity; x = packet->x; y = packet->y; z = packet->z; return true;
}
}
namespace nova::module::antibot {
bool bot = false;
bool IsBot(const nova::java::EntityHandle&) { return bot; }
}
namespace nova::module::killaura {
bool HasTargets() { return mock::aura; }
bool QueueAutoBlockAttack() { if (!mock::blockManaged) return false; ++mock::queuedAttacks; return true; }
}
namespace nova::module::teams { bool IsTeammate(const nova::java::EntityHandle&) { return mock::teammate; } }
namespace nova::clickgui { bool IsOpen() { return mock::menu; } }
namespace nova::module::scaffold { bool IsEnabled() { return mock::scaffold; } }
namespace nova::rotation {
struct Rotation { float yaw = 0, pitch = 0; };
enum class RotationPriority { Medium, Highest };
Rotation applied;
Rotation GetRotation() { return applied; }
void SetRotations(Rotation r, double speed) {
    mock::Check(speed == 180, "Watchdog rotation must use Velocity's 180 speed");
    applied = r; ++mock::rotations;
}
void SetRotations(Rotation r, double speed, std::nullptr_t, RotationPriority priority) {
    mock::Check(speed == 180 && priority == RotationPriority::Highest, "Reduce rotation must use highest priority / 180 speed");
    applied = r; ++mock::rotations;
}
void SetRotations(Rotation r, double speed, std::nullptr_t, RotationPriority priority, bool randomize) {
    mock::Check(!randomize, "Delay rotation must not add direction noise");
    SetRotations(r,speed,nullptr,priority);
}
namespace mth {
float WrapDegrees(float angle) {
    angle = std::fmod(angle, 360.0f);
    if (angle >= 180) angle -= 360;
    if (angle < -180) angle += 360;
    return angle;
}
}
}
