#include "minecraft/modules/combat/autoblock/AutoBlock.h"
#include "minecraft/modules/combat/autoblock/AutoBlockPacketSync.h"
#include "minecraft/modules/combat/antikb/AntiKB.h"
#include "minecraft/modules/misc/teams/Teams.h"
#include "minecraft/modules/misc/antibot/AntiBot.h"
#include "minecraft/compat/lunar189/LunarRuntime.h"
#include "minecraft/java/api/game_click.h"
#include "minecraft/java/api/game_rotation.h"
#include "minecraft/utils/rotation/Random.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

namespace nova::module {

namespace {
namespace lunar = compat::lunar;

bool SlinkyNameBot(const std::string& name) {
    return name.size() < 3 || name.size() > 16 || name.find(' ') != std::string::npos;
}
bool SlinkyNameTeam(const std::string& a, const std::string& b) {
    // Java compares the first two UTF-16 characters; section-sign is two UTF-8 bytes.
    return a.size() >= 3 && b.size() >= 3 && a.starts_with("\xc2\xa7") &&
        b.starts_with("\xc2\xa7") && a.substr(0,3) == b.substr(0,3);
}
}

void AutoBlock::DebugSlinky(const std::string& message) {
    if (!settings_.slinky.debug) return;
    const auto now = GetTickCount64();
    if (message == slinky_.debugMessage && now - slinky_.lastDebug < 1000) return;
    slinky_.debugMessage = message;
    slinky_.lastDebug = now;
    LogTo("AutoBlock Slinky: %s", message.c_str());
}

void AutoBlock::ResetSlinky() {
    lunar::EndSlinkyBlockLag();
    const auto self = java::PlayerEntity();
    lunar::SetUseKeyDown(self && !lunar::EntityDead(self.raw()) && !java::ScreenOpen() && lunar::PhysicalUseDown());
    blocking_ = false;
    slinky_ = {};
    slinky_.reset = lunar::GrimResetSequence();
}

void AutoBlock::StartSlinky() {
    if (!slinky_.canBlock || !lunar::HoldingSword()) return;
    // The supplied script only changes the held key. Do not turn this into a
    // queued right-click or direct C08: vanilla's rightClickDelay remains intact.
    if (!lunar::SetUseKeyDown(true)) return;
    blocking_ = true;
    slinky_.awaitingUse = settings_.disableManualBlock && !lunar::PlayerUsingItem();
    slinky_.blockEnd = GetTickCount64() + std::clamp(settings_.slinky.holdMs,50,500);
    DebugSlinky("BLOCK");
}

void AutoBlock::StopSlinky(bool lag) {
    blocking_ = slinky_.canBlock = false;
    slinky_.awaitingUse = false;
    const auto& s = settings_.slinky;
    if (lag && !antikb::BlocksAutoBlockBlink() && s.lagChance > 0 && rotation::Random() * 100.0 <= s.lagChance &&
        lunar::BeginSlinkyBlockLag(s.lagMs,s.queueSize,s.preventDelayAttacks)) {
        slinky_.lagEnd = GetTickCount64() + std::clamp(s.lagMs,0,500);
        autoblock::SynchronizePacketDelays();
    }
    // Start the lag window before vanilla sees the released key, so its C07 is
    // buffered too. This is the reference's defining stopBlockingAndLag order.
    lunar::SetUseKeyDown(false);
    DebugSlinky("release");
}

bool AutoBlock::SlinkyEnemyNearby(const java::EntityHandle& self) {
    const auto& s = settings_.slinky;
    rotation::Vec3 position;
    if (!self.Position(position)) return false;
    std::string ownName;
    if (s.ignoreTeammates) self.DisplayName(ownName);
    std::vector<java::EntityHandle> players;
    if (!java::WorldPlayers(players)) return false;
    double best = std::clamp(s.range,1.0,6.0);
    bool found = false;
    for (const auto& player : players) {
        if (!player || player.IsSame(self) || lunar::EntityDead(player.raw()) ||
            (s.ignoreMobs && !player.IsPlayer())) continue;
        // Preserve Nova's centralized target exclusions in addition to the
        // script's optional simple color/name checks. Nova has no friend list.
        if (teams::IsTeammate(self,player) || antibot::IsBot(player)) continue;
        std::string name;
        if (s.ignoreBots && (!player.ScoreboardName(name) || SlinkyNameBot(name))) continue;
        if (s.ignoreTeammates && player.DisplayName(name) && SlinkyNameTeam(ownName,name)) continue;
        rotation::Vec3 at;
        if (!player.Position(at)) continue;
        const double x=at.x-position.x,y=at.y-position.y,z=at.z-position.z;
        const double distance=std::sqrt(x*x+y*y+z*z);
        if (distance < best) { best=distance; found=true; }
    }
    return found;
}

bool AutoBlock::SlinkyInteractable() {
    const auto self = java::PlayerEntity();
    rotation::Vec3 eye, direction;
    float yaw=0,pitch=0;
    java::BlockHitResultInfo hit;
    if (!self || !self.EyePosition(1,eye) || !java::PlayerRotation(yaw,pitch) ||
        !java::Vec3DirectionFromRotation(pitch,yaw,direction) ||
        !java::WorldClip(eye,eye.Add(direction.x*6,direction.y*6,direction.z*6),self,hit) ||
        hit.kind != java::HitKind::Block || !hit.hasBlockPos) return false;
    std::string name;
    if (!lunar::BlockRegistryName(hit.blockPos,name)) return false;
    for (const char* part : {"chest","furnace","crafting","anvil","enchant","brewing","hopper",
                            "dropper","dispenser","beacon","door","gate","lever","button","trap"})
        if (name.find(part) != std::string::npos) return true;
    return false;
}

void AutoBlock::TickSlinky() {
    autoblock::SynchronizePacketDelays();
    const auto self = java::PlayerEntity();
    if (slinky_.reset != lunar::GrimResetSequence() || !self ||
        lunar::EntityDead(self.raw()) || java::ScreenOpen()) { ResetSlinky(); return; }
    const auto now = GetTickCount64();
    const auto& s = settings_.slinky;
    const int hurt = java::PlayerHurtTime();
    if (hurt > 0 && slinky_.lastHurt == 0) slinky_.lastDamage = now;
    slinky_.lastHurt = hurt;
    const bool attackReleased = lunar::ConsumeSlinkyAttackRelease();
    bool lagging = lunar::SlinkyBlockLagging();
    const bool expired = lagging && now >= slinky_.lagEnd;
    if (expired) { lunar::EndSlinkyBlockLag(); lagging=false; }
    if (!lunar::HoldingSword()) {
        lunar::EndSlinkyBlockLag();
        if (blocking_) StopSlinky(false);
        slinky_.canBlock = false;
        // Once our own hold is released, leave native bow/rod/food input alone.
        DebugSlinky("stop: holding non-sword");
        return;
    }
    // Prediction may take ownership of this same queue. Never drain its packets.
    if (lunar::BlockLagging() && !lagging) {
        blocking_ = slinky_.canBlock = false;
        lunar::SetUseKeyDown(false);
        DebugSlinky("stop: another packet owner");
        return;
    }
    if (blocking_ || lagging || slinky_.interacting)
        slinky_.interacting = SlinkyInteractable();
    slinky_.enemy = SlinkyEnemyNearby(self);
    const bool conditions = (!s.requireLeft || lunar::PhysicalMouseDown(0)) &&
        (!s.requireRight || lunar::PhysicalMouseDown(1)) &&
        (!s.damagedOnly || (slinky_.lastDamage != 0 && now - slinky_.lastDamage <= static_cast<std::uint64_t>(s.damageWindow)));
    if (!conditions || slinky_.interacting || !slinky_.enemy) {
        // The script returns here before checking lag expiry, leaving lagActive
        // stuck. End our own queue on loss of eligibility instead of porting that bug.
        lunar::EndSlinkyBlockLag();
        if (blocking_) StopSlinky(false);
        lunar::SetUseKeyDown(lunar::PhysicalUseDown());
        slinky_.canBlock = false;
        DebugSlinky(slinky_.interacting ? "stop: interacting with block" :
            !slinky_.enemy ? "stop: no enemy within range" : "stop: conditions fail");
        return;
    }
    slinky_.canBlock = true;
    // Packet interception only publishes this notice. Apply the key change on
    // the client thread, rechecking the current target/item/permission first.
    if (attackReleased && s.blockAgain) { StartSlinky(); return; }
    if (expired || lagging) return;
    if (!blocking_) {
        if (s.immediately || (hurt > 0 && hurt <= s.hurtMs / 50)) StartSlinky();
        else {
            lunar::SetUseKeyDown(lunar::PhysicalUseDown());
            DebugSlinky("stop: waiting hurt window");
        }
    } else if (slinky_.awaitingUse) {
        // Key-down is only a request: vanilla may still be waiting for its
        // right-click cooldown. Do not consume the hold window before use starts.
        // Eligibility and interaction checks above still cancel this pending hold.
        lunar::SetUseKeyDown(true);
        if (lunar::PlayerUsingItem()) {
            slinky_.awaitingUse = false;
            slinky_.blockEnd = now + std::clamp(s.holdMs,50,500);
        } else DebugSlinky("wait: vanilla use pending");
    } else if (now >= slinky_.blockEnd) StopSlinky(true);
}
}
