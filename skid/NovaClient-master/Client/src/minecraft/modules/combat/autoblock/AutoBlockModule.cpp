#include "minecraft/modules/misc/antibot/AntiBot.h"
#include <atomic>
#include <memory>

#include "minecraft/compat/lunar189/LunarRuntime.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/events/EventBus.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/modules/combat/autoblock/AutoBlockModuleBase.h"
#include "minecraft/modules/combat/autoblock/AutoBlockRuntime.h"
#include "minecraft/modules/combat/antikb/AntiKB.h"
#include "minecraft/modules/combat/killaura/KillAura.h"
#include "minecraft/modules/player/scaffold/Scaffold.h"
#include "minecraft/modules/player/automlg/AutoMLG.h"
#include "minecraft/modules/misc/teams/Teams.h"
#include "minecraft/java/api/game_click.h"
#include "ui/clickgui/ClickGui.h"

namespace nova::module {
namespace {

enum class Owner { None, Aura, Standalone };

struct AutoBlockRuntime {
    AutoBlock engine;
    bool auraEnabled = false;
    bool standaloneEnabled = false;
    AutoBlockSettings auraSettings;
    AutoBlockSettings standaloneSettings;
    AutoBlockSettings settings;
    Owner owner = Owner::None;
    std::atomic<bool> active{false};
    std::atomic<bool> managedAttacks{false};
    std::atomic<bool> hypixelActive{false};
    std::atomic<bool> lagNewActive{false};
    std::atomic<double> hypixelRange{6.0};
    std::atomic<int> attacks{0};
    std::atomic<bool> releaseEarly{false};

    void ClearRequests() {
        attacks.store(0);
        releaseEarly.store(false);
    }

    void Synchronize() {
        const Owner next = (!compat::IsLegacyLunar() || automlg::IsWorking()) ? Owner::None :
            standaloneEnabled ? Owner::Standalone : auraEnabled ? Owner::Aura : Owner::None;
        const AutoBlockSettings selected = next == Owner::Standalone ? standaloneSettings : auraSettings;
        if (next != owner || (next != Owner::None && selected.mode != settings.mode)) {
            active.store(false);
            managedAttacks.store(false);
            hypixelActive.store(false);
            lagNewActive.store(false);
            engine.SetEnabled(false);
            ClearRequests();
        }
        owner = next;
        if (owner == Owner::None) return;
        settings = selected;
        hypixelRange.store(settings.mode == AutoBlockMode::LagNew ? settings.range : settings.hypixel.range);
        engine.Configure(settings);
        engine.SetEnabled(true);
        managedAttacks.store(settings.mode != AutoBlockMode::Lag && settings.mode != AutoBlockMode::Slinky);
        hypixelActive.store(settings.mode == AutoBlockMode::Hypixel);
        lagNewActive.store(settings.mode == AutoBlockMode::LagNew);
        active.store(true);
    }

    void Motion() {
        if (!active.load() || settings.mode != AutoBlockMode::Slinky) return;
        if (java::ScreenOpen() || clickgui::IsOpen() || scaffold::IsEnabled()) {
            ClearRequests();
            engine.Reset();
            return;
        }
        engine.Tick(settings);
    }

    void Tick() {
        if (!active.load()) return;
        if (java::ScreenOpen() || clickgui::IsOpen() || scaffold::IsEnabled()) {
            ClearRequests();
            engine.Reset();
            return;
        }
        if (settings.mode == AutoBlockMode::Hypixel) {
            engine.PrepareHypixelInput();
            return;
        }
        if (settings.mode == AutoBlockMode::LagNew) {
            releaseEarly.store(false);
            engine.TickLagNew(owner == Owner::Aura || killaura::HasTargets(), attacks.exchange(0) > 0);
            return;
        }
        auto tickSettings = settings;
        if (releaseEarly.exchange(false)) tickSettings.releaseOnAttack = true;
        const int requested = attacks.exchange(0);
        const bool reduceAttacking = antikb::AttackQueue() > 0;
        if (tickSettings.mode != AutoBlockMode::Slinky)
            engine.Tick(tickSettings, !reduceAttacking && requested > 0,
                        owner == Owner::Standalone && !reduceAttacking);
        if ((tickSettings.mode == AutoBlockMode::Lag || tickSettings.mode == AutoBlockMode::Slinky) && !reduceAttacking) {
            java::EntityHandle hovered;
            if (java::AttackTargetEntity(hovered) && (teams::IsTeammate(hovered) || antibot::IsBot(hovered))) return;
            for (int i = 0; i < requested; ++i) compat::lunar::ClickAttack();
        }
    }
};

AutoBlockRuntime runtime;

class AutoBlockModule final : public AutoBlockModuleBase {
public:
    AutoBlockModule() : AutoBlockModuleBase("AutoBlock") {
        addSettings(BuildBlockSettings());
        disableManualBlock_ = &addBool("Disable Manual Block", false);
        auto& bus = event::EventBus::Get();
        // KillAura submits its attacks first; only this listener drives the engine.
        interact_ = bus.Subscribe<event::PrePlayerInteractEvent>(
            [](event::PrePlayerInteractEvent&) { runtime.Tick(); }, event::Priority::Low);
        playerTick_ = bus.Subscribe<event::PlayerTickEvent>([](event::PlayerTickEvent& e) {
            if (!runtime.hypixelActive.load() || e.IsCancelled()) return;
            if (java::ScreenOpen() || clickgui::IsOpen() || scaffold::IsEnabled()) {
                runtime.engine.Reset();
                return;
            }
            runtime.releaseEarly.store(false);
            runtime.engine.TickHypixel(runtime.owner == Owner::Aura || killaura::HasTargets(),
                                       runtime.attacks.exchange(0) > 0);
        }, event::Priority::Lowest);
        postPlayerTick_ = bus.Subscribe<event::PostPlayerTickEvent>([](event::PostPlayerTickEvent&) {
            if (runtime.hypixelActive.load()) runtime.engine.PostHypixel();
            if (runtime.lagNewActive.load()) runtime.engine.PostLagNew();
        }, event::Priority::Lowest);
        packet_ = bus.Subscribe<event::PacketEvent>([](event::PacketEvent& e) {
            if (runtime.active.load() && e.type == event::EventType::Outgoing &&
                compat::lunar::IsAttackPacket(e.packet)) runtime.engine.OnAttackPacket();
        }, event::Priority::High);
        motion_ = bus.Subscribe<event::MotionEvent>([](event::MotionEvent& e) {
            if (e.type == event::EventType::Pre && !e.IsCancelled()) runtime.Motion();
        }, event::Priority::Low);
        render_ = bus.Subscribe<event::RenderFrameEvent>([](event::RenderFrameEvent&) {
            if (runtime.active.load()) runtime.engine.Render();
        });
    }

    std::string suffix() const override {
        switch (ReadBlockSettings().mode) {
        case AutoBlockMode::Lag: return "Lag";
        case AutoBlockMode::Vanilla: return "Vanilla";
        case AutoBlockMode::Grim: return "Grim";
        case AutoBlockMode::Hypixel: return "Hypixel";
        case AutoBlockMode::Slinky: return "Slinky";
        case AutoBlockMode::LagNew: return "Lag New";
        }
        return {};
    }

private:
    void onUpdate() override {
        runtime.standaloneSettings = ReadBlockSettings();
        runtime.standaloneSettings.disableManualBlock = disableManualBlock_->get();
        runtime.standaloneEnabled = true;
        runtime.Synchronize();
    }

    void onDisable() override {
        runtime.standaloneEnabled = false;
        runtime.Synchronize();
    }

    event::Subscription interact_;
    Properties::BoolProperty* disableManualBlock_ = nullptr;
    event::Subscription playerTick_, postPlayerTick_;
    event::Subscription packet_;
    event::Subscription motion_;
    event::Subscription render_;
};

}

namespace autoblock {

void ConfigureAura(bool enabled, const AutoBlockSettings& settings) {
    runtime.auraEnabled = enabled;
    runtime.auraSettings = settings;
    runtime.Synchronize();
}

bool Active() { return runtime.active.load(); }
void SuspendForInteraction() {runtime.Synchronize();}
bool OwnsItemUse() {
    return Active() && (runtime.attacks.load() > 0 || runtime.releaseEarly.load() || runtime.engine.OwnsItemUse());
}
bool HypixelActive() { return runtime.hypixelActive.load(); }
bool AttackCycleActive() { return runtime.hypixelActive.load() || runtime.lagNewActive.load(); }
double HypixelRange() { return runtime.hypixelRange.load(); }
bool VelocityCanReduce(int phase) { return runtime.engine.VelocityCanReduce(phase); }
bool VelocityShouldBlock() {
    return runtime.hypixelActive.load() ? runtime.engine.VelocityShouldBlock() : killaura::HasBlockTarget();
}

void SubmitAttacks(int count) {
    if (count > 0 && Active()) runtime.attacks.fetch_add(count);
}

bool QueueAttack(bool releaseEarly) {
    if (!runtime.managedAttacks.load()) return false;
    if (releaseEarly) runtime.releaseEarly.store(true);
    SubmitAttacks(1);
    return true;
}

}

std::unique_ptr<Module> MakeAutoBlockModule() {
    return std::make_unique<AutoBlockModule>();
}

}
