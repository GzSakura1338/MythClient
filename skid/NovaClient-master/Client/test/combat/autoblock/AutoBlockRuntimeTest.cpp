int main() {
    using namespace nova::module;
    using mock::Check;
    AutoBlockSettings aura;
    aura.mode = AutoBlockMode::Vanilla;
    mock::Reset();
    killaura::blockTarget = true;
    Check(autoblock::VelocityShouldBlock(), "Prediction uses block targets even without selected aura targets");
    killaura::blockTarget = false;
    Check(!autoblock::VelocityShouldBlock(), "Prediction releases when block-range targets disappear");
    Check(!autoblock::Active() && !autoblock::QueueAttack(false), "disabled runtime rejects attacks");
    Check(!autoblock::OwnsItemUse(), "disabled runtime does not reserve item use");
    AutoBlockSettings lagNew; lagNew.mode = AutoBlockMode::LagNew;
    autoblock::ConfigureAura(true, lagNew);
    Check(autoblock::AttackCycleActive() && !autoblock::HypixelActive(), "Lag New owns cadence without changing Hypixel identity");
    runtime.Tick();
    Check(mock::lagNewTicks == 1, "Lag New drives its own input phase");
    autoblock::ConfigureAura(false, lagNew);
    Check(!autoblock::AttackCycleActive(), "Lag New ownership clears on disable");
    antibot::bot = true;
    autoblock::ConfigureAura(true, aura);
    autoblock::SubmitAttacks(1);
    runtime.Tick(); mock::GameTick();
    Check(mock::attackCount == 0, "AntiBot rejects automated attack fallback");
    antibot::bot = false;
    autoblock::ConfigureAura(true, aura);
    Check(autoblock::Active(), "aura can enable engine with standalone off");
    mock::target = false;
    Check(!autoblock::OwnsItemUse(), "idle enabled runtime permits throwing");
    autoblock::SubmitAttacks(3);
    Check(autoblock::OwnsItemUse(), "queued attacks reserve item use before engine runs");
    mock::target = true;
    runtime.Tick(); mock::GameTick();
    Check(mock::attackCount == 1, "managed attacks coalesce into one attack");

    runtime.standaloneSettings.mode = AutoBlockMode::Grim;
    runtime.standaloneEnabled = true;
    runtime.Synchronize();
    Check(runtime.owner == Owner::Standalone && runtime.settings.mode == AutoBlockMode::Grim,
          "standalone takes precedence over aura");
    for (int tick = 0; tick < 60; ++tick) {
        autoblock::ConfigureAura(true, aura);
        Check(runtime.settings.mode == AutoBlockMode::Grim, "aura update cannot overwrite standalone mode");
        autoblock::SubmitAttacks(1);
        runtime.Tick(); mock::GameTick();
    }
    Check(mock::attackCount > 5, "shared engine continues attacking with both modules enabled");
    autoblock::QueueAttack(true);
    runtime.standaloneEnabled = false;
    runtime.Synchronize();
    Check(runtime.owner == Owner::Aura && runtime.settings.mode == AutoBlockMode::Vanilla,
          "disabling standalone resumes configured aura mode");
    Check(!mock::lag && runtime.attacks.load() == 0 && !runtime.releaseEarly.load(),
          "handoff releases buffering and stale attack requests");
    autoblock::ConfigureAura(false, aura);
    Check(!autoblock::Active() && !mock::key && !mock::lag, "both disabled releases runtime");

    runtime.standaloneEnabled = true;
    runtime.standaloneSettings.mode = AutoBlockMode::Vanilla;
    runtime.Synchronize();
    autoblock::ConfigureAura(false, aura);
    Check(autoblock::Active(), "disabled aura must not disable standalone");
    ++mock::attackClicks;
    const int before = mock::attackCount;
    runtime.Tick(); mock::GameTick();
    Check(mock::attackCount == before + 1, "standalone routes actual native attack requests");
    autoblock::QueueAttack(true);
    nova::clickgui::open = true;
    runtime.Tick();
    Check(!mock::key && runtime.attacks.load() == 0, "ClickGUI clears block and queued requests");
    nova::clickgui::open = false;

    runtime.standaloneSettings.mode = AutoBlockMode::Lag;
    runtime.Synchronize();
    Check(!autoblock::QueueAttack(false), "Lag keeps unmodified velocity attack path");
    mock::right = false;
    autoblock::SubmitAttacks(2);
    runtime.Tick();
    Check(mock::attackClicks == 2, "Lag submits aura clicks exactly once");
    mock::GameTick();
    runtime.standaloneEnabled = false;
    runtime.Synchronize();
    Check(!autoblock::Active(), "final owner disable stops runtime");
    aura.mode = AutoBlockMode::Hypixel;
    autoblock::ConfigureAura(true, aura);
    Check(autoblock::HypixelActive(), "Hypixel owner activates dedicated timing");
    autoblock::SubmitAttacks(4);
    runtime.Tick();
    Check(mock::hypixelPrepares == 1 && runtime.attacks.load() == 4,
          "Hypixel keeps external requests for player PRE and prepares input at interact");
    autoblock::ConfigureAura(false, aura);
    Check(!autoblock::HypixelActive(), "Hypixel timing disabled with owner");
    mock::Reset();aura.mode=AutoBlockMode::Slinky;
    autoblock::ConfigureAura(true,aura);
    autoblock::SubmitAttacks(2);runtime.Tick();
    Check(mock::attackClicks==2 && !runtime.engine.Blocking(),"Slinky forwards aura clicks at interact, blocks only on motion");
    runtime.Motion();Check(mock::key && runtime.engine.Blocking(),"Slinky motion drives shared engine");
    Check(!autoblock::QueueAttack(false),"Slinky does not hijack Velocity attack path");
    runtime.standaloneEnabled=true;runtime.standaloneSettings.mode=AutoBlockMode::Slinky;
    runtime.standaloneSettings.slinky.requireLeft=true;runtime.Synchronize();runtime.Motion();
    Check(!mock::key,"standalone Slinky options take precedence");
    runtime.standaloneEnabled=false;runtime.Synchronize();runtime.Motion();
    Check(mock::key,"aura Slinky options restored after handoff");
    nova::clickgui::open=true;runtime.Motion();Check(!mock::key,"Slinky motion respects ClickGUI guard");
    nova::clickgui::open=false;autoblock::ConfigureAura(false,aura);
    Check(!autoblock::Active()&&!mock::key,"Slinky owner cleanup");
    std::puts("AutoBlock runtime checks passed: owner precedence, handoff, input, cleanup, attack routing");
}
