#pragma once

#include <cstdint>
#include <string>
#include "minecraft/java/api/game_entity.h"
#include "minecraft/modules/combat/autoblock/HypixelAutoBlock.h"
#include "minecraft/modules/combat/autoblock/LagNewAutoBlock.h"

namespace nova::module {

enum class AutoBlockMode {
    Lag = 0,
    Vanilla = 1,
    Grim = 2,
    Hypixel = 3,
    Slinky = 4,
    LagNew = 5,
};

struct SlinkyBlockSettings {
    double range = 3.5;
    int hurtMs = 200, holdMs = 150, lagChance = 0, lagMs = 100, queueSize = 3;
    int damageWindow = 2000;
    bool immediately = true, preventDelayAttacks = true, blockAgain = true;
    bool requireLeft = false, requireRight = false, damagedOnly = false;
    bool ignoreMobs = true, ignoreTeammates = false, ignoreBots = false, debug = false;
};

struct AutoBlockSettings {
    bool disableManualBlock = false;
    bool lagNewManualClick = false;
    int lagNewApsMode = 0; // Existing configs: 0 remains 7APS.
    HypixelBlockSettings hypixel;
    SlinkyBlockSettings slinky;
    AutoBlockMode mode = AutoBlockMode::Lag;
    int holdTicks = 1;
    int blinkTicks = 3;
    int moveTicks = 2;
    bool releaseOnAttack = false;
    double range = 4;
    int hurtMs = 200;
    int holdMs = 150;
    int lagChance = 100;
    int lagMs = 200;
    bool preventDelayAttacks = true;
    bool blockAgain = true;
    bool animation = true;
    bool requireLeft = true;
    bool requireRight = false;
    bool damaged = false;
};

class AutoBlock {
public:
    void SetEnabled(bool enabled);
    void Configure(const AutoBlockSettings& settings);
    void Tick(const AutoBlockSettings& settings, bool attackRequested = false,
              bool captureClicks = false);
    void OnAttackPacket();
    void Render();
    void Reset(bool releaseUseKey = true);
    bool Blocking() const { return blocking_; }
    // Game-thread query, including gaps between block/release and a cycle
    // about to start. Other item users must not switch slots in those gaps.
    bool OwnsItemUse();
    void TickHypixel(bool auraEnabled, bool attackRequested = false);
    void PostHypixel();
    void PrepareHypixelInput();
    void TickLagNew(bool auraEnabled, bool attackRequested);
    void PostLagNew();
    bool VelocityCanReduce(int phase) const;
    bool VelocityShouldBlock() const;
private:
    void ResetLagNew();
    LagNewAutoBlock lagNew_;
    java::EntityHandle lagNewTarget_;
    bool lagNewClick_ = false, lagNewQueue_ = false;
    std::uint64_t lagNewResetSequence_ = 0;
    void TickSlinky();
    void ResetSlinky();
    void StartSlinky();
    void StopSlinky(bool lag);
    void DebugSlinky(const std::string& message);
    bool SlinkyEnemyNearby(const java::EntityHandle& self);
    bool SlinkyInteractable();
    struct SlinkyState {
        bool canBlock = false, interacting = false, enemy = false;
        bool awaitingUse = false;
        int lastHurt = 0;
        std::uint64_t blockEnd = 0, lagEnd = 0, lastDamage = 0, reset = 0, lastDebug = 0;
        std::string debugMessage;
    } slinky_;
    void ResetHypixel();
    HypixelAutoBlock hypixel_;
    std::uint64_t hypixelResetSequence_ = 0;
    bool hypixelManualAttack_ = false;
    void TickGrim(bool attackRequested, bool captureClicks);
    enum class GrimPhase { Idle, Blocking, Blinking, Reblocking, Draining };
    GrimPhase grimPhase_ = GrimPhase::Idle;
    bool grimPendingAttack_ = false;
    int grimTicks_ = 0;
    int grimSlot_ = -1;
    int grimCooldown_ = 0;
    std::uint64_t grimMovement_ = 0;
    std::uint64_t grimResetSequence_ = 0;
    void TickVanilla(bool attackRequested, bool captureClicks);
    void TickLag();
    void OnLagAttackPacket();
    void Start();
    void Stop(bool forceRelease);
    void ReleaseLag();
    java::EntityHandle FindTarget();
    AutoBlockSettings settings_;
    bool blocking_ = false;
    bool enabled_ = false;
    bool manual_ = false;
    bool lagWindow_ = false;
    int tick_ = 0;
    int startTick_ = -1;
    int lagStartTick_ = -1;
    int lastHurt_ = 0;
    bool vanillaManaged_ = false;
    bool vanillaPendingAttack_ = false;
    bool vanillaReleasing_ = false;
    int vanillaHeldTicks_ = 0;
    std::uint64_t vanillaReleaseMovement_ = 0;
};

}
