// run_prediction.ps1 inserts the actual LunarRuntime buffering functions.
#include <algorithm>
#include <atomic>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <functional>
#include <mutex>
#include <vector>
enum Kind { Ordinary, KeepAlive, Chat, Transaction, Interact, Move, WindowClick, Close, LoginStart, Handshake };
struct Object { Kind kind = Ordinary; int serial = 0; double x = 1, z = 1; };
using jobject = Object*;
using jdouble = double;
using jboolean = bool;
using jclass = int;
using ULONGLONG = unsigned long long;
using SRWLOCK = std::mutex;
void AcquireSRWLockExclusive(SRWLOCK* m) { m->lock(); }
void ReleaseSRWLockExclusive(SRWLOCK* m) { m->unlock(); }
#define JNICALL
std::vector<int> sent;
std::function<void(jobject)> sendObserver;
int refs = 0;
bool failRef = false;
Object local;
Object phaseWorld;
jobject phasePlayer = &local, phaseLevel = &phaseWorld;
struct PhaseEntity {
    jobject object;
    explicit operator bool() const { return object != nullptr; }
    void* raw() const { return object; }
};
PhaseEntity PlayerEntity() { return {phasePlayer}; }
PhaseEntity LevelEntity() { return {phaseLevel}; }
void LogTo(const char*, ...) {}
struct JNIEnv {
    bool IsInstanceOf(jobject p, int kind) { return p && p->kind == kind; }
    bool IsSameObject(jobject a, jobject b) { return a == b; }
    jobject NewGlobalRef(jobject p) { if (failRef) return nullptr; ++refs; return p; }
    void DeleteGlobalRef(jobject p) { if (p) --refs; }
    void DeleteLocalRef(jobject) {}
    double GetDoubleField(jobject p, int field) { return field == 1 ? p->x : p->z; }
    void SetDoubleField(jobject p, int field, double value) { (field == 1 ? p->x : p->z) = value; }
    void CallNonvirtualVoidMethod(jobject, int, int, jobject p, jobject) {
        sent.push_back(p->serial);
        if (sendObserver) sendObserver(p);
    }
    void CallVoidMethod(jobject, int, bool) {}
    jobject CallObjectMethod(jobject p, int) { return p; }
    int CallIntMethod(jobject p, int) { return p->serial==3?1:0; }
} environment;
struct JniAttach { JNIEnv* env = &environment; explicit operator bool() const { return true; } };
struct Handles {
    bool ready = true;
    int KeepAliveReply = KeepAlive, ChatPacket = Chat, TransactionReply = Transaction;
    int InteractPacket = Interact, MovePacket = Move, MotionX = 1, MotionZ = 2;
    int NetworkManager = 0, DispatchPacket = 0, SetSprinting = 0;
    int WindowClickPacket=WindowClick, ClosePacket=Close, LoginStartPacket=LoginStart, HandshakePacket=Handshake;
    int InteractAction=0, Ordinal=0;
} h;
namespace compat { jobject LocalPlayer(JNIEnv*) { return &local; } }
bool Good(JNIEnv*) { return true; }
bool Local(JNIEnv*, jobject p) { return p == &local; }
void RecordBlockPacket(JNIEnv*, jobject) {}
bool BlockControlPacket(JNIEnv*, jobject p) { return p->kind == KeepAlive || p->kind == Transaction; }
ULONGLONG now=0;
ULONGLONG GetTickCount64() { return now; }
std::atomic<bool> blockLagActive{false}, predictionBlockLag{false};
std::atomic<std::uint64_t> movementPacketSequence{0}, grimResetSequence{0};
bool grimBlockLag = false, hypixelBlockLag = false, grimReblockPending = false;
bool slinkyBlockLag=false, slinkyPreventAttacks=true;
int slinkyMaxPackets=3;
ULONGLONG slinkyDeadline=0;
std::atomic<bool> slinkyAttackReleased{false};
ULONGLONG grimBlockDeadline = 0;
SRWLOCK blockLagLock;
bool IsCircuitOpen() { return false; }
namespace java { double BeginHitSlowdown(JNIEnv*, jobject) { return 0.6; } double HitSlowdown() { return 0.6; } }
// Combined fixture binds this to actual AntiKB ownership; standalone tests
// keep it false except when explicitly checking owner exclusion.
bool (*watchdogBlocksBlink)() = nullptr;
bool (*watchdogHoldingPackets)() = nullptr, (*watchdogReplayingPackets)() = nullptr;
namespace module::antikb {
bool BlocksAutoBlockBlink() { return watchdogBlocksBlink && watchdogBlocksBlink(); }
bool WatchdogHoldingPackets() { return watchdogHoldingPackets && watchdogHoldingPackets(); }
bool WatchdogReplayingPackets() { return watchdogReplayingPackets && watchdogReplayingPackets(); }
}
void EndSlinkyBlockLag(bool send = true);
void EndHypixelBlockLag(bool send = true);
namespace module::displace {
bool HoldOutput(JNIEnv*, jobject, jobject, jobject) { return false; }
}
// @BLINK_PRODUCTION@
int main() {
    Object manager, otherManager;
    Object ka{KeepAlive,1}, tx{Transaction,2}, attack{Interact,3}, interact{Interact,4}, at{Interact,5}, move{Move,6};
    BeginHypixelBlockLag();
    assert(!HoldBlockPacket(&environment,&manager,&ka,nullptr,false));
    assert(!HoldBlockPacket(&environment,&manager,&tx,nullptr,false));
    assert(HoldBlockPacket(&environment,&manager,&attack,nullptr,false));
    assert(BeginPredictionBlink());
    EndHypixelBlockLag(); ReleaseBlockLag(); EndBlockLag(true);
    assert(blockLagActive && predictionBlockLag && sent.empty());
    assert(HoldBlockPacket(&environment,&manager,&interact,nullptr,false));
    assert(HoldBlockPacket(&environment,&manager,&at,nullptr,false));
    assert(HoldBlockPacket(&environment,&manager,&move,nullptr,false));
    AttackMotionXHook(&environment,0,&local,.6);
    AttackMotionZHook(&environment,0,&local,.6);
    assert(local.x == 1 && local.z == 1); // suppressed while Velocity owns blink
    EndPredictionBlink(true);
    assert((sent == std::vector<int>{3,4,5,6}));
    assert(std::abs(local.x - .216) < 1e-12 && std::abs(local.z - .216) < 1e-12);
    assert(!predictionBlockLag && !blockLagActive && refs == 0 && movementPacketSequence == 1);
    AttackMotionXHook(&environment,0,&local,.1296);
    assert(local.x == .1296); // normal vanilla effect restored
    BeginPredictionBlink();
    HoldBlockPacket(&environment,&manager,&attack,nullptr,false);
    EndPredictionBlink(false);
    assert(sent.size() == 4 && refs == 0 && !predictionBlockLag); // disable drops as Leader
    BeginHypixelBlockLag(); HoldBlockPacket(&environment,&manager,&move,nullptr,false);
    EndHypixelBlockLag(); assert(sent.back() == 6 && refs == 0);
    BeginPredictionBlink(); HoldBlockPacket(&environment,&manager,&move,nullptr,false);
    assert(!HoldBlockPacket(&environment,&otherManager,&attack,nullptr,false));
    assert(refs == 0 && !predictionBlockLag); // manager switch never replays old packets
    BeginPredictionBlink(); failRef = true;
    assert(!HoldBlockPacket(&environment,&manager,&attack,nullptr,false));
    failRef = false; assert(refs == 0 && !predictionBlockLag);
    std::cout << "PASS: production Blink queue transfer, owner isolation, FIFO, C02 compensation, slowdown hooks and JNI cleanup\n";
    sent.clear(); movementPacketSequence=0; now=1000;
    Object window{WindowClick,7}, close{Close,8}, login{LoginStart,9}, handshake{Handshake,10};
    assert(BeginSlinkyBlockLag(100,3,true));
    assert(!BeginSlinkyBlockLag(100,3,true));
    assert(!HoldBlockPacket(&environment,&manager,&login,nullptr,false));
    assert(!HoldBlockPacket(&environment,&manager,&handshake,nullptr,false));
    assert(HoldBlockPacket(&environment,&manager,&ka,nullptr,false));
    assert(HoldBlockPacket(&environment,&manager,&tx,&window,false));
    assert(HoldBlockPacket(&environment,&manager,&move,nullptr,false));
    assert(refs==7 && sent.empty());
    assert(HoldBlockPacket(&environment,&manager,&ka,nullptr,false));
    assert((sent==std::vector<int>{1}) && blockPackets.size()==3);
    assert(!HoldBlockPacket(&environment,&manager,&attack,nullptr,false));
    assert((sent==std::vector<int>{1,2,6,1}) && refs==0 && movementPacketSequence==1);
    assert(ConsumeSlinkyAttackRelease() && !ConsumeSlinkyAttackRelease() && !SlinkyBlockLagging());
    for(auto* p:{&window,&close,&interact}) {
        assert(BeginSlinkyBlockLag(100,1,true));
        HoldBlockPacket(&environment,&manager,&move,nullptr,false);
        assert(!HoldBlockPacket(&environment,&manager,p,nullptr,false));
        assert(!SlinkyBlockLagging() && refs==0 && !ConsumeSlinkyAttackRelease());
    }
    assert(BeginSlinkyBlockLag(100,3,false));
    assert(HoldBlockPacket(&environment,&manager,&attack,nullptr,false));
    assert(HoldBlockPacket(&environment,&manager,&move,nullptr,false));
    now=1099; DrainSlinkyBlockLag(); assert(blockPackets.size()==2);
    now=1100; DrainSlinkyBlockLag(); assert(blockPackets.size()==1 && SlinkyBlockLagging());
    EndSlinkyBlockLag(); assert(!SlinkyBlockLagging() && refs==0);
    assert(BeginSlinkyBlockLag(0,3,true));
    assert(!HoldBlockPacket(&environment,&manager,&move,nullptr,false) && !SlinkyBlockLagging());
    assert(BeginSlinkyBlockLag(100,3,true));
    HoldBlockPacket(&environment,&manager,&move,nullptr,false);
    const auto before=sent.size();
    assert(!HoldBlockPacket(&environment,&otherManager,&move,nullptr,false));
    assert(sent.size()==before && refs==0 && !SlinkyBlockLagging());
    assert(BeginSlinkyBlockLag(100,3,true));
    failRef=true; assert(!HoldBlockPacket(&environment,&manager,&move,nullptr,false)); failRef=false;
    assert(refs==0 && !SlinkyBlockLagging());
    assert(BeginSlinkyBlockLag(100,3,true));
    HoldBlockPacket(&environment,&manager,&move,nullptr,false);
    assert(BeginPredictionBlink()); EndSlinkyBlockLag();
    assert(predictionBlockLag && blockPackets.size()==1 && !SlinkyBlockLagging());
    assert(!BeginSlinkyBlockLag(100,3,true));
    EndPredictionBlink(false); assert(refs==0);
    std::cout << "PASS: Slinky bounded FIFO, callbacks, attack/window/interact flush, deadlines, render drain, failed allocation and Prediction owner transfer\n";
    watchdogBlocksBlink=+[]{return true;};
    assert(!BeginSlinkyBlockLag(100,3,true));
    BeginHypixelBlockLag();assert(!hypixelBlockLag);EndHypixelBlockLag();
    assert(BeginPredictionBlink());EndSlinkyBlockLag(false);
    assert(predictionBlockLag);EndPredictionBlink(false);
    watchdogBlocksBlink=nullptr;
    assert(BeginSlinkyBlockLag(100,3,true));
    HoldBlockPacket(&environment,&manager,&move,nullptr,false);
    const auto sentBeforeDrop=sent.size();EndSlinkyBlockLag(false);
    assert(sent.size()==sentBeforeDrop && refs==0 && !SlinkyBlockLagging());
    std::cout << "PASS: Watchdog excludes Slinky/Hypixel delay and retains Prediction isolation; lifecycle drops release references\n";
}
