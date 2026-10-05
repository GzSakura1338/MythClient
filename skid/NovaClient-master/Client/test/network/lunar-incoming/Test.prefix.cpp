#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include "minecraft/compat/lunar189/IncomingTasks.h"

#define JNICALL
using jboolean = bool;
using jclass = int;
constexpr bool JNI_TRUE = true, JNI_FALSE = false;
struct Object {
    int kind = 0, serial = 0;
    Object *packet = nullptr, *listener = nullptr;
};
using jobject = Object*;
void Check(bool ok, const char* text) {
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", text); std::exit(1); }
}
const auto clientThread = std::this_thread::get_id();
bool OnClient() { return std::this_thread::get_id() == clientThread; }
struct Handles {
    int NetHandler = 1, DisconnectPacket = 2, PositionPacket = 3, RespawnPacket = 4;
    int ProcessPacket = 5, TaskPacket = 6, TaskListener = 7, PacketTask = 8;
    int PacketTaskCtor = 9, IsClientThread = 10, ScheduleTask = 11, JoinPacket = 12;
} h;
Object minecraft, listener{1};
bool circuit = false;
bool IsCircuitOpen() { return circuit; }
bool lunar1710 = false;
bool IsLunar1710() { return lunar1710; }
std::recursive_mutex scheduledMutex, moduleMutex;
std::mutex objectsMutex;
std::vector<std::unique_ptr<Object>> objects;
std::deque<jobject> scheduled;
std::atomic<int> refs{0}, scheduleAttempts{0}, callbacks{0};
std::vector<int> delivered;
jobject held = nullptr;
bool holdFirst = false;
nova::compat::lunar::IncomingTasks<jobject> incomingTasks;

struct JNIEnv {
    bool exception = false, failRef = false, failSchedule = false;
    bool ExceptionCheck() const { return exception; }
    bool IsSameObject(jobject a, jobject b) const { return a == b; }
    bool IsInstanceOf(jobject p, int kind) const { return p && p->kind == kind; }
    jobject NewGlobalRef(jobject p) {
        if (failRef) { exception = true; return nullptr; }
        ++refs; return p;
    }
    void DeleteGlobalRef(jobject) { --refs; }
    void DeleteLocalRef(jobject) {}
    jobject NewObject(int, int, jobject packet, jobject target) {
        std::lock_guard lock(objectsMutex);
        auto task = std::make_unique<Object>();
        task->packet = packet; task->listener = target;
        const auto raw = task.get(); objects.push_back(std::move(task)); return raw;
    }
    jobject GetObjectField(jobject task, int field) {
        return field == h.TaskPacket ? task->packet : task->listener;
    }
    bool CallBooleanMethod(jobject, int) const { return OnClient(); }
    jobject CallObjectMethod(jobject, int method, jobject task) {
        Check(method == h.ScheduleTask, "unexpected Java call");
        ++scheduleAttempts;
        if (failSchedule) { exception = true; return nullptr; }
        std::lock_guard lock(scheduledMutex);
        scheduled.push_back(task);
        return nullptr;
    }
    void CallVoidMethod(jobject packet, int, jobject target) {
        if (!OnClient()) {
            // Vanilla PacketThreadUtil: an unmarked replay Runnable.
            auto task = NewObject(0, 0, packet, target);
            CallObjectMethod(&minecraft, h.ScheduleTask, task);
            return;
        }
        std::lock_guard lock(moduleMutex); // AppliedVelocity handler
        delivered.push_back(packet->serial);
    }
};
jobject Minecraft(JNIEnv*) { return &minecraft; }
namespace event {
enum class EventType { Incoming };
struct PacketEvent {
    EventType type; jobject packet;
    bool cancelled = false;
    PacketEvent(EventType t, jobject p) : type(t), packet(p) {}
};
}
namespace java { bool IncomingPacketsWanted() { return true; } }
namespace module::autoblock { void ResetCoverage() {} void SynchronizePacketDelays(bool = false) {} }
void MaintainWatchdogPhase(JNIEnv*,bool,bool = false) {}
std::atomic<bool> predictionBlockLag{false}, incomingObserved{false};
std::atomic<int> grimResetSequence{0};
bool hypixelBlockLag = false, grimBlockLag = false, slinkyBlockLag = false;
struct BlockLagLock { BlockLagLock() { Check(OnClient(), "Netty took the Blink lock"); } };
void DrainBlockPackets(JNIEnv*, bool, bool = false) { Check(OnClient(), "Netty drained Blink"); }
void LogTo(const char*) {}
bool Dispatch(event::PacketEvent& e) {
    Check(OnClient(), "Netty invoked a module callback");
    std::lock_guard lock(moduleMutex);
    ++callbacks;
    if (held) { JNIEnv env; env.CallVoidMethod(held, h.ProcessPacket, &listener); held = nullptr; }
    if (holdFirst && e.packet->serial == 1) { held = e.packet; return true; }
    return false;
}
