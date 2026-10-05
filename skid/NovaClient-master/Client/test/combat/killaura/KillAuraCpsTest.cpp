// run_killaura_cps.ps1 inserts the production mode predicate and attack callback.
#include <atomic>
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>
#include <stdexcept>
#include "minecraft/modules/combat/killaura/CriticalSprint.h"
#include "minecraft/modules/combat/shared/ExplosionCoordinator.h"
namespace explosive = nova::module::explosive;

namespace killaura = nova::module::killaura;
namespace event {
enum class EventType { Pre, Post };
struct GameTickEvent { EventType type=EventType::Pre; };
struct MoveInputEvent { float forward=1, strafe=1; };
}

struct Property { int value = 0; int get() const { return value; } };
namespace compat { bool lunar = false; bool IsLegacyLunar() { return lunar; } }
namespace autoblock { bool active = false; bool Active() { return active; } }
namespace antikb {
int queue = 0; bool enabled = false;
int AttackQueue() { return queue; } bool IsEnabled() { return enabled; }
}
namespace velocity { int queue = 0; int AttackQueue() { return queue; } }
namespace prediction { bool StoppedBlock() { return false; } bool CancelsAura() { return false; } }
namespace clickgui { bool open = false; bool IsOpen() { return open; } }
namespace scaffold { bool enabled = false; bool IsEnabled() { return enabled; } }
namespace automace { bool IsWorking() { return false; } }
namespace automlg { bool IsWorking() { return false; } }
namespace autocharge { bool IsWorking() { return false; } }
namespace crystal { bool working=false; bool IsWorking() { return working; } }
namespace breaker { bool BlocksAura() { return false; } }
namespace java {
// @CRITICAL_STATE@
CriticalAttackState criticalState;
bool criticalReadable = true;
int criticalReads = 0;
bool ReadCriticalAttackState(CriticalAttackState& out) { ++criticalReads; out=criticalState; return criticalReadable; }
using DWORD = unsigned;
DWORD GetCurrentThreadId(){return 1;}
std::atomic<DWORD> interruptedThread{0};
bool CriticalSprintInterrupted();
std::vector<std::string> order;
bool failAttack=false, throwAttack=false, keepSprint=false;
bool screen = false, usingItem = false, target = true, player = true;
bool hurtReadable = true;
int hurt = 0, selfHurt = 0, sent = 0, cooldownReads = 0;
float strength = 0.0f;
struct EntityHandle {
    bool IsPlayer() const { return player; }
    bool HurtTime(int& out) const { out = hurt; return hurtReadable; }
};
bool ScreenOpen() { return screen; }
bool PlayerUsingItem() { return usingItem; }
bool AttackTargetEntity(EntityHandle&) { return target; }
int PlayerHurtTime() { return selfHurt; }
bool PlayerAttackCooldownReady() { ++cooldownReads; return strength >= 1.0f; }
bool AttackAndSwing(const EntityHandle&) {
    order.push_back("attack");
    if(keepSprint && !CriticalSprintInterrupted()) criticalState.sprinting=true;
    if(throwAttack) throw std::runtime_error("attack failed");
    if(failAttack) return false;
    ++sent; strength=0.0f; return true;
}
// @CRITICAL_METHODS@
}
std::atomic<bool> g_hasTargets{true};
namespace antibot {
bool bot = false;
bool IsBot(const java::EntityHandle&) { return bot; }
}
namespace teams {
bool teammate = false;
bool IsTeammate(const java::EntityHandle&) { return teammate; }
}
long long NowMillis() { return 1000; }

struct Fixture {
    Property mode{1}, block{}, hitSelect{}, waitCritical{}, interruptSprint{};
    Property* interruptSprint_ = &interruptSprint;
    Property* waitCritical_ = &waitCritical;
    Property* cpsMode_ = &mode;
    Property* autoBlock_ = &block;
    Property* hitSel_ = &hitSelect;
    struct Block { void Reset() {} } block_;
    bool enabled = true;
    bool isEnabled() const { return enabled; }
    int attacks_ = 0;
    long long lastAttackTime_ = 0;
    killaura::CriticalSprint criticalSprint_;
    // @KILLAURA_METHODS@
};

int main() {
    Fixture aura;
    aura.attacks_ = 100;
    java::strength = 0.99f;
    aura.OnPlayerTick();
    assert(java::sent == 0 && aura.attacks_ == 0);
    java::strength = 1.0f;
    teams::teammate = true;
    aura.OnPlayerTick();
    teams::teammate = false;
    assert(java::sent == 0); // Team changes are checked immediately before attacking.
    antibot::bot = true;
    aura.OnPlayerTick();
    assert(java::sent == 0);
    antibot::bot = false;
    aura.attacks_ = 100;
    aura.OnPlayerTick();
    assert(java::sent == 1 && aura.attacks_ == 0);
    aura.OnPlayerTick();
    assert(java::sent == 1); // No duplicate after vanilla resets the cooldown.
    for (int i = 0; i < 30; ++i) aura.OnPlayerTick();
    assert(java::sent == 1); // No fixed-CPS or accumulated-click fallback.
    java::strength = 1.0f;
    aura.OnPlayerTick();
    assert(java::sent == 2); // A newly charged weapon permits the next attack.

    java::strength = 1.0f;
    java::usingItem = true; aura.OnPlayerTick(); java::usingItem = false;
    java::screen = true; aura.OnPlayerTick(); java::screen = false;
    clickgui::open = true; aura.OnPlayerTick(); clickgui::open = false;
    scaffold::enabled = true; aura.OnPlayerTick(); scaffold::enabled = false;
    antikb::queue = 1; aura.OnPlayerTick(); antikb::queue = 0;
    velocity::queue = 1; aura.OnPlayerTick(); velocity::queue = 0;
    g_hasTargets = false; aura.OnPlayerTick(); g_hasTargets = true;
    java::target = false; aura.OnPlayerTick(); java::target = true;
    aura.enabled = false; aura.OnPlayerTick(); aura.enabled = true;
    assert(java::sent == 2);

    aura.hitSelect.value = 1;
    java::hurt = 5;
    aura.OnPlayerTick();
    assert(java::sent == 2);
    java::selfHurt = 6;
    java::strength = 0.5f; aura.OnPlayerTick();
    assert(java::sent == 2); // Hit select cannot bypass the weapon cooldown.
    java::strength = 1.0f; aura.OnPlayerTick();
    assert(java::sent == 3);
    java::selfHurt = 0; java::hurt = 0;
    java::strength = -1.0f; aura.OnPlayerTick();
    assert(java::sent == 3); // Unavailable cooldown fails closed.

    aura.waitCritical.value = 1;
    for(bool sprint : {false,true}) {
        java::criticalState.sprinting=sprint;
        for(int i=0;i<3;++i) {
            const int before=java::sent;
            java::strength=1.0f; aura.OnPlayerTick();
            assert(java::sent==before+1 && !java::CriticalSprintInterrupted());
            aura.OnPlayerTick();assert(java::sent==before+1); // Ground still obeys cooldown.
        }
    }
    java::keepSprint=true; java::criticalState.sprinting=false;
    java::strength=1; aura.OnPlayerTick();
    assert(java::criticalState.sprinting); // Ground attacks retain KeepSprint's behavior.
    java::keepSprint=false;java::criticalState.sprinting=false;
    java::sent=3;java::strength=1.0f;
    java::criticalState.ground = false;
    aura.OnPlayerTick();
    assert(java::sent == 3); // Ascending also waits.
    java::criticalState.fallDistance = .1;
    java::criticalState.sprinting = true; aura.OnPlayerTick();
    java::criticalState.sprinting = false;
    java::criticalState.blind = true; aura.OnPlayerTick(); java::criticalState.blind = false;
    java::criticalState.climbing = true; aura.OnPlayerTick(); java::criticalState.climbing = false;
    java::criticalState.passenger = true; aura.OnPlayerTick(); java::criticalState.passenger = false;
    java::criticalReadable = false; aura.OnPlayerTick(); java::criticalReadable = true;
    assert(java::sent == 3);
    aura.OnPlayerTick();
    assert(java::sent == 4); // Charged, falling, and eligible.
    aura.OnPlayerTick(); assert(java::sent == 4); // No repeat during recharge.
    java::criticalState = {};
    for (int environment = 0; environment < 2; ++environment) {
        java::criticalState.water = environment == 0;
        java::criticalState.web = environment == 1;
        for (int i = 0; i < 3; ++i) {
            java::strength = 1.0f; aura.OnPlayerTick();
            const int attacks = java::sent;
            aura.OnPlayerTick(); assert(java::sent == attacks);
        }
    }
    assert(java::sent == 10); // Grounded water/web repeatedly attack at cooldown.
    java::criticalState = {};
    aura.waitCritical.value = 0;
    java::strength = 1.0f;
    aura.OnPlayerTick(); assert(java::sent == 11); // Switch off restores normal attacks.

    aura.waitCritical.value=1; aura.interruptSprint.value=1;
    java::criticalState={}; java::criticalState.sprinting=true;
    java::strength=1.0f; java::order.clear(); aura.OnPlayerTick();
    assert((java::order==std::vector<std::string>{"attack"}) && java::criticalState.sprinting);
    java::sent=11; java::order.clear();java::strength=1; // Ground attack does not stop sprint.
    java::criticalState.ground=false; aura.OnPlayerTick();
    assert(java::order.empty()); // Rising: keep sprint until the critical window.
    java::criticalState.fallDistance=.1;
    java::strength=.5f; aura.OnPlayerTick();
    assert(java::order.empty());
    java::strength=1; java::keepSprint=true; aura.OnPlayerTick();
    assert(java::order.empty() && java::criticalState.sprinting); // Never fake a stop inside the attack.
    event::GameTickEvent tick;
    event::MoveInputEvent input;
    aura.OnCriticalSprintTick(tick);
    aura.OnCriticalSprintInput(input);
    assert(input.forward==0 && input.strafe==1 && java::criticalState.sprinting);
    assert(java::order.empty()); // Input pulse does not call setSprinting or send a packet.
    input.forward=1; aura.OnCriticalSprintInput(input); assert(input.forward==1); // Consumed once.
    java::criticalState.sprinting=false; // Vanilla aiStep processes the zero-forward input.
    aura.OnPlayerTick();
    assert((java::order==std::vector<std::string>{"attack"}));
    assert(java::sent==12 && !java::criticalState.sprinting && !java::CriticalSprintInterrupted());
    for(int failure=0;failure<2;++failure){
        java::strength=1; java::order.clear();
        java::failAttack=failure==0; java::throwAttack=failure==1;
        try{aura.OnPlayerTick();}catch(const std::runtime_error&){}
        assert(java::sent==12 && !java::CriticalSprintInterrupted());
        assert((java::order==std::vector<std::string>{"attack"})); // No forced sprint restart even on failure.
    }
    java::failAttack=false; java::throwAttack=false; java::keepSprint=false;
    for(double fall : {0.,-.1,1.,2.}){
        java::criticalState.fallDistance=fall; input.forward=1;
        aura.OnCriticalSprintTick(tick);aura.OnCriticalSprintInput(input);assert(input.forward==1);
    }
    java::criticalState.fallDistance=.5;
    for(int blocked=0;blocked<6;++blocked){
        input.forward=1;
        velocity::queue=blocked==0;antikb::queue=blocked==1;
        g_hasTargets=blocked!=2;clickgui::open=blocked==3;
        aura.interruptSprint.value=blocked!=4;aura.enabled=blocked!=5;
        aura.OnCriticalSprintTick(tick);aura.OnCriticalSprintInput(input);assert(input.forward==1);
    }
    velocity::queue=0;antikb::queue=0;g_hasTargets=true;clickgui::open=false;
    aura.interruptSprint.value=1;aura.enabled=true;
    aura.OnCriticalSprintTick(tick);aura.interruptSprint.value=0;
    input.forward=1;aura.OnCriticalSprintInput(input);assert(input.forward==1); // Disable between tick/input clears pending.
    aura.interruptSprint.value=1;
    for(int environment=0;environment<2;++environment){
        java::criticalState={};java::criticalState.sprinting=true;
        java::criticalState.water=environment==0;java::criticalState.web=environment==1;
        java::criticalState.fallDistance=.5;input.forward=1;
        aura.OnCriticalSprintTick(tick);aura.OnCriticalSprintInput(input);assert(input.forward==1);
        java::strength=1;java::order.clear();aura.OnPlayerTick();
        assert((java::order==std::vector<std::string>{"attack"})); // No water/web sprint interruption.
    }
    java::criticalState={}; java::criticalState.ground=false; java::criticalState.fallDistance=.1;
    java::strength=1;java::order.clear();aura.OnPlayerTick();
    assert((java::order==std::vector<std::string>{"attack"}) && !java::criticalState.sprinting); // No forced sprint from walking.
    java::sent = 3; // Continue the existing legacy timing regression checks.

    aura.mode.value = 0;
    const int reads = java::cooldownReads;
    teams::teammate = true;
    aura.attacks_ = 3; aura.OnPlayerTick();
    assert(java::sent == 3 && aura.attacks_ == 0);
    teams::teammate = false;
    antibot::bot = true;
    aura.attacks_ = 3; aura.OnPlayerTick();
    assert(java::sent == 3 && aura.attacks_ == 0);
    antibot::bot = false;
    aura.attacks_ = 3; aura.OnPlayerTick();
    assert(java::sent == 6 && java::cooldownReads == reads);
    aura.mode.value = 1;
    compat::lunar = true;
    assert(!aura.UsesAttackCooldown());
    aura.attacks_ = 2; aura.OnPlayerTick();
    assert(java::sent == 8 && java::cooldownReads == reads);
    autoblock::active = true;
    aura.attacks_ = 1; aura.OnPlayerTick();
    assert(java::sent == 8 && aura.attacks_ == 1); // Lunar AB keeps its own path.
    autoblock::active=false;compat::lunar=false;
    for(int mode:{0,1}) {
        aura.mode.value=mode;aura.attacks_=3;java::strength=1;
        crystal::working=true;const int before=java::sent;
        aura.OnPlayerTick();assert(java::sent==before&&aura.attacks_==0);
        aura.waitCritical.value=aura.interruptSprint.value=1;assert(!aura.CriticalSprintEnabled());
        crystal::working=false;aura.waitCritical.value=0;aura.attacks_=1;
        aura.OnPlayerTick();assert(java::sent==before+1);
    }
    std::puts("KillAura CPS mode: all checks passed");
}
