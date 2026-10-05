#include <atomic>
#include <cassert>
#include <cstdio>
#include <deque>
#include <format>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include "minecraft/events/EventBus.h"
using jobject = void*;
namespace mock {
int refs = 0, wanted = 0, world = 1, player = 1;
bool failRef = false;
std::vector<int> played;
std::function<void()> duringReplay;
struct Packet { int serial, kind = 0, id = 1, y = 0; }; // 1=motion, 2=correction, 3=disconnect, 4=respawn
}
struct MockEnv {
    jobject NewGlobalRef(jobject p) { if (mock::failRef) return nullptr; ++mock::refs; return p; }
    void DeleteGlobalRef(jobject) { assert(mock::refs > 0); --mock::refs; }
    bool ExceptionCheck() { return false; }
    void ExceptionClear() {}
};
struct JniAttach { MockEnv storage; MockEnv* env = &storage; explicit operator bool() const { return true; } };
namespace nova::java {
struct EntityHandle {
    int id = 0;
    explicit operator bool() const { return id != 0; }
    bool IsSame(const EntityHandle& other) const { return id != 0 && id == other.id; }
    void Reset() { id = 0; }
};
EntityHandle PlayerEntity() { return {mock::player}; }
EntityHandle LevelEntity() { return {mock::world}; }
bool EntityId(const EntityHandle& e, int& id) { id = e.id; return id != 0; }
bool IsMotionPacket(jobject p) { return static_cast<mock::Packet*>(p)->kind == 1; }
bool IsPlayerPositionPacket(jobject p) { return static_cast<mock::Packet*>(p)->kind == 2; }
bool IsDisconnectPacket(jobject p) { return static_cast<mock::Packet*>(p)->kind == 3; }
bool IsRespawnPacket(jobject p) { return static_cast<mock::Packet*>(p)->kind == 4; }
bool ReadMotionPacket(jobject p, int& id, int& x, int& y, int& z) {
    auto& packet = *static_cast<mock::Packet*>(p);
    id = packet.id; x = z = 0; y = packet.y; return true;
}
void SetIncomingPacketsWanted(bool enabled) { mock::wanted += enabled ? 1 : -1; assert(mock::wanted >= 0); }
bool HandleIncomingPacket(jobject p) {
    mock::played.push_back(static_cast<mock::Packet*>(p)->serial);
    auto callback = std::move(mock::duringReplay); mock::duringReplay = {};
    if (callback) callback();
    return true;
}
}
namespace nova::module {
enum class Category { Movement };
class Module {
    bool enabled_ = false, requested_ = false;
public:
    Module(const char*, Category, const char*, const char*) {}
    virtual ~Module() = default;
    virtual std::string suffix() const { return {}; }
    bool isEnabled() const { return enabled_; }
    void setEnabled(bool enabled) { requested_ = enabled; }
    void tick() {
        if (enabled_ != requested_) { enabled_ = requested_; if (enabled_) onEnable(); else onDisable(); }
    }
protected:
    virtual void onEnable() {}
    virtual void onDisable() {}
};
}
