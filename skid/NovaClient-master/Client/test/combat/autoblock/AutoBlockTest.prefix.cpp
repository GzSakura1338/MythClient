#include <algorithm>
#include <atomic>
#include <cmath>
#include <limits>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <utility>

namespace mock {
bool right, left, sword = true, target = true, screen, key, animation, usingItem;
bool inputManaged = false;
bool manualSuppressed = false;
bool world = true, dead, blockHit, hovered = true, teammate, lag;
std::uint64_t now=10000, resetSequence=0, slinkyDeadline=0;
bool slinkyLag=false, slinkyAttack=false;
double targetX=2;
std::string targetName="Enemy", ownDisplay="Self", targetDisplay="Enemy", blockName;
int hurt, useClicks, attackClicks, attackCount, lagStarts, delay, slowTicks;
int hypixelPrepares = 0;
int lagNewTicks = 0;
std::uint64_t movement;
std::string packets;
std::string queued;
bool reblockPending;
bool serverUsing, released, attacked, interacted, interactedAt, swung, used;
void Check(bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "%s; packets=%s\n", message, packets.c_str()); std::exit(1); }
}
void Packet(char packet) {
    if (lag) { queued += packet; return; }
    packets += packet;
    switch (packet) {
    case 'R':
        Check(!attacked && !used && !interacted, "release after action in same movement interval");
        serverUsing = false; released = true; swung = false; break;
    case 'S': swung = true; break;
    case 'A':
        Check(swung && !serverUsing && !released && !used && !interacted, "invalid attack order or using state");
        swung = false; attacked = true; ++attackCount; break;
    case 'T':
        Check(!released && !interactedAt, "invalid interact-at order");
        interactedAt = true; swung = false; break;
    case 'I':
        Check(interactedAt && !released, "invalid interact order");
        interactedAt = false; interacted = true; swung = false; break;
    case 'U':
        Check(!released && (!attacked || interacted), "use after release or missing interaction");
        serverUsing = used = true; swung = false; break;
    case 'M':
        Check(!interactedAt, "incomplete entity interaction");
        released = attacked = interacted = interactedAt = swung = used = false;
        ++movement; break;
    }
}
void Flush() {
    lag = reblockPending = false;
    const auto pending = std::exchange(queued, {});
    for (const char packet : pending) Packet(packet);
}
void RightClick() {
    delay = 4;
    Check(!blockHit, "automated use on a block");
    if (target && hovered) { Packet('T'); Packet('I'); }
    if (manualSuppressed && sword) return; // Native item-use fallback guard.
    Packet('U'); usingItem = true;
    if (reblockPending) Flush();
}
void GameTick(bool sendMovement = true) {
    if (delay > 0) --delay;
    if (!screen && world) {
        if (usingItem) {
            if (!key) { Packet('R'); usingItem = false; }
            attackClicks = useClicks = 0;
        } else {
            while (attackClicks > 0) {
                --attackClicks;
                Packet('S');
                if (hovered && target) Packet('A');
            }
            while (useClicks > 0) { --useClicks; RightClick(); }
        }
        if (key && !usingItem && delay == 0) RightClick();
        const float forward = usingItem ? 0.2f : 1.0f;
        if (usingItem) ++slowTicks;
        Check(lag || !serverUsing || forward == 0.2f, "use state and movement slowdown disagree");
        if (sendMovement) Packet('M');
    }
}
void Reset() {
    inputManaged = manualSuppressed = false;
    right = left = screen = key = animation = usingItem = dead = blockHit = teammate = lag = false;
    sword = target = world = hovered = true;
    hurt = useClicks = attackClicks = attackCount = lagStarts = delay = slowTicks = 0;
    movement = 0;
    packets.clear();
    queued.clear();
    reblockPending = false;
    serverUsing = released = attacked = interacted = interactedAt = swung = used = false;
    slinkyLag=slinkyAttack=false;now=10000;resetSequence=0;targetX=2;
    targetName="Enemy";ownDisplay="Self";targetDisplay="Enemy";blockName.clear();
}
}

std::uint64_t GetTickCount64(){return mock::now;}
void LogTo(const char*,...){}

namespace nova {
namespace module::autoblock {
void SetCoverageActive(bool) {}
void SynchronizePacketDelays(bool = false) {}
}
namespace module::antikb { bool BlocksAutoBlockBlink() { return false; } }
namespace rotation {
struct Vec3 { double x = 0, y = 0, z = 0; Vec3 Add(double a,double b,double c)const{return {x+a,y+b,z+c};} };
double Random() { return 0.5; }
}
namespace java {
struct BlockPos{int x=0,y=0,z=0;};
enum class HitKind{Miss,Block};
struct BlockHitResultInfo{HitKind kind=HitKind::Miss;bool hasBlockPos=false;BlockPos blockPos;};
struct Box { double minX = 1, minY = 0, minZ = 0, maxX = 2, maxY = 1, maxZ = 1; };
struct EntityHandle {
    bool valid = false;
    int id = 0;
    explicit operator bool() const { return valid; }
    void* raw() const { return nullptr; }
    bool BoundingBox(Box&) const { return true; }
    bool PickRadius(double& radius) const { radius = 0.1; return true; }
    bool Valid() const { return valid; }
    bool IsPlayer() const { return true; }
    bool IsSame(const EntityHandle& other) const { return id == other.id; }
    bool EyePosition(float, rotation::Vec3&) const { return true; }
    bool Position(rotation::Vec3& out) const { out={id==0?0:mock::targetX,0,0};return true; }
    bool DisplayName(std::string& out)const{out=id==0?mock::ownDisplay:mock::targetDisplay;return true;}
    bool ScoreboardName(std::string& out)const{out=mock::targetName;return true;}
    bool DeathTime(int& ticks) const { ticks = 0; return true; }
};
EntityHandle PlayerEntity() { return {mock::world, 0}; }
bool ScreenOpen() { return mock::screen; }
int PlayerHurtTime() { return mock::hurt; }
bool PlayerRotation(float& yaw,float& pitch){yaw=pitch=0;return true;}
bool Vec3DirectionFromRotation(float,float,rotation::Vec3& out){out={1,0,0};return true;}
bool WorldClip(rotation::Vec3,rotation::Vec3,const EntityHandle&,BlockHitResultInfo& out){out.kind=mock::blockHit?HitKind::Block:HitKind::Miss;out.hasBlockPos=mock::blockHit;return true;}
bool AttackTargetIsBlock() { return mock::blockHit; }
bool AttackTargetEntity(EntityHandle& entity) {
    entity = {mock::target && mock::hovered, 1}; return entity.Valid();
}
bool WorldPlayers(std::vector<EntityHandle>& players) {
    if (mock::target) players.push_back({true, 1}); return true;
}
}
namespace module::antibot {
bool bot = false;
bool IsBot(const java::EntityHandle&) { return bot; }
}
namespace module::teams {
bool IsTeammate(const java::EntityHandle&, const java::EntityHandle&) { return mock::teammate; }
bool IsTeammate(const java::EntityHandle&) { return mock::teammate; }
}
namespace compat {
bool IsLegacyLunar() { return true; }
namespace lunar {
bool PhysicalMouseDown(int button) { return button == 1 ? mock::right : mock::left; }
bool PhysicalUseDown() { return mock::right; }
bool EntityDead(void*) { return mock::dead; }
bool HoldingSword() { return mock::sword; }
bool PlayerUsingItem() { return mock::usingItem; }
bool StartSwordBlock() { mock::key = true; ++mock::useClicks; return true; }
bool StopSwordBlock() { mock::key = false; return true; }
bool SetUseKeyDown(bool down) { mock::key = down; return true; }
bool ClickAttack() { ++mock::attackClicks; return true; }
bool PendingAttackClick() { return mock::attackClicks > 0; }
bool ClearCombatClicks() { mock::attackClicks = mock::useClicks = 0; return true; }
std::uint64_t MovementPacketSequence() { return mock::movement; }
std::uint64_t GrimResetSequence() { return mock::resetSequence; }
bool SelectedSlot(int& slot) { slot = 0; return true; }
void SetAutoBlockEnabled(bool) {}
void SetAutoBlockManualSuppressed(bool suppressed) { mock::manualSuppressed = suppressed; }
void SetAutoBlockState(bool managed, bool animation) { mock::inputManaged=managed; mock::animation = animation; }
void BeginBlockLag() { mock::lag = true; ++mock::lagStarts; }
bool BeginGrimBlockLag(int) { if (mock::lag) return false; BeginBlockLag(); return true; }
bool GrimBlockLagExpired() { return !mock::lag; }
bool PrepareGrimBlockLagRelease(std::uint64_t& boundary) {
    bool release = false, moved = false;
    std::uint64_t movements = 0;
    for (char packet : mock::queued) {
        if (packet == 'R') { release = true; moved = false; }
        if (packet == 'M') { ++movements; if (release) moved = true; }
    }
    if (!moved) return false;
    boundary = mock::movement + movements;
    mock::reblockPending = true;
    return true;
}
bool BlockLagging() { return mock::lag; }
void ReleaseBlockLag() { mock::lag = false; }
void EndBlockLag(bool = true) { mock::Flush(); }
bool BeginSlinkyBlockLag(int ms,int,bool){if(mock::lag)return false;mock::lag=mock::slinkyLag=true;mock::slinkyDeadline=mock::now+ms;return true;}
void EndSlinkyBlockLag(){if(mock::slinkyLag){mock::slinkyLag=false;mock::Flush();}mock::slinkyAttack=false;}
void DrainSlinkyBlockLag(){}
bool SlinkyBlockLagging(){return mock::slinkyLag;}
bool ConsumeSlinkyAttackRelease(){return std::exchange(mock::slinkyAttack,false);}
bool BlockRegistryName(const java::BlockPos&,std::string& out){out=mock::blockName;return true;}
}
}
}
