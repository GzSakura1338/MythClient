int main() {
    using namespace nova::module;
    using mock::Check;
    for (auto mode : {AutoBlockMode::Lag, AutoBlockMode::Vanilla, AutoBlockMode::Grim,
                      AutoBlockMode::Hypixel, AutoBlockMode::Slinky, AutoBlockMode::LagNew}) {
        mock::Reset(); mock::target = false;
        AutoBlock block; AutoBlockSettings s; s.mode = mode;
        block.Configure(s); block.SetEnabled(true);
        Check(!block.OwnsItemUse(), "idle AutoBlock must allow projectile use");
        mock::target = true;
        Check(block.OwnsItemUse(), "reserve nearby target before a later block callback starts");
        Check(mock::packets.empty() && !mock::key && !mock::lag,
              "item ownership query must not mutate combat input or send packets");
        mock::sword = false;
        Check(!block.OwnsItemUse(), "an idle non-sword AutoBlock must not reserve projectiles");
        block.SetEnabled(false);
        Check(!block.OwnsItemUse(), "disabled engine releases item reservation");
    }
    for (auto mode : {AutoBlockMode::Lag, AutoBlockMode::Vanilla, AutoBlockMode::Grim, AutoBlockMode::Slinky}) {
        mock::Reset(); mock::target=false; mock::right=true;
        AutoBlock block; AutoBlockSettings s; s.mode=mode; s.disableManualBlock=true;
        block.Configure(s); block.SetEnabled(true); block.Tick(s);
        Check(mock::manualSuppressed&&mock::key==(mode!=AutoBlockMode::Slinky)&&!mock::inputManaged,
            "manual block switch suppresses sword fallback but preserves interaction input");
        s.disableManualBlock=false; block.Tick(s);
        Check(!mock::manualSuppressed&&mock::key,"turning switch off restores held manual input");
        s.disableManualBlock=true; mock::target=true; block.Tick(s);
        Check(!mock::manualSuppressed&&mock::inputManaged,"manual suppression must not block automatic cycle");
        block.SetEnabled(false);
        Check(!mock::manualSuppressed,"disabling clears manual suppression");
    }
    AutoBlockSettings settings;
    settings.mode = AutoBlockMode::Vanilla;
    settings.animation = false;
    auto enable = [&](AutoBlock& block) { block.Configure(settings); block.SetEnabled(true); };

    mock::Reset();
    AutoBlock normal;
    enable(normal);
    normal.Tick(settings, true); mock::GameTick();
    Check(mock::packets == "SATIUM", "initial attack and vanilla interaction chain");
    normal.Tick(settings, true); mock::GameTick();
    Check(mock::packets == "SATIUMRM", "release tick must not attack");
    mock::target = false; mock::sword = false;
    Check(normal.OwnsItemUse(), "release gap retains item ownership even if the target/held sword disappear");
    mock::target = true; mock::sword = true;
    normal.Tick(settings, false); mock::GameTick();
    Check(mock::packets == "SATIUMRMSATIUM", "retained attack must execute on following tick");
    Check(!mock::animation && mock::lagStarts == 0, "vanilla must not force animation or buffer packets");

    mock::Reset();
    AutoBlock boundary;
    enable(boundary);
    boundary.Tick(settings, false); mock::GameTick();
    boundary.Tick(settings, true); mock::GameTick(false);
    for (int i = 0; i < 5; ++i) {
        boundary.Tick(settings, true); mock::GameTick(false);
        Check(mock::attackCount == 0 && !mock::key, "missing movement boundary must postpone attack and reblock");
    }
    mock::Packet('M');
    boundary.Tick(settings, false); mock::GameTick();
    Check(mock::attackCount == 1, "requests accumulated during release must coalesce");

    mock::Reset();
    AutoBlock interrupted;
    settings.requireRight = true;
    mock::right = true;
    enable(interrupted);
    interrupted.Tick(settings, false); mock::GameTick();
    interrupted.Tick(settings, true); mock::GameTick(false);
    mock::right = false;
    interrupted.Tick(settings, true); mock::GameTick(false);
    Check(mock::attackCount == 0, "right mouse release must not bypass the movement boundary");
    mock::Packet('M');
    interrupted.Tick(settings, true); mock::GameTick();
    Check(mock::attackCount == 1 && !mock::usingItem, "fallback attacks resume after movement boundary");
    settings.requireRight = false;

    mock::Reset();
    AutoBlock held;
    settings.holdTicks = 3;
    enable(held);
    held.Tick(settings, false); mock::GameTick();
    for (int i = 0; i < 2; ++i) {
        mock::attackClicks = mock::useClicks = 4;
        held.Tick(settings, true); mock::GameTick();
        Check(mock::usingItem && mock::attackCount == 0, "hold duration and physical click suppression");
    }
    held.Tick(settings, true); mock::GameTick();
    Check(!mock::usingItem, "configured hold ends in release");
    held.Tick(settings, true); mock::GameTick();
    Check(mock::attackCount == 1, "one attack after hold");

    mock::Reset();
    AutoBlock absent;
    settings.holdTicks = 1;
    enable(absent);
    absent.Tick(settings, false); mock::GameTick();
    absent.Tick(settings, true); mock::GameTick();
    mock::target = false;
    absent.Tick(settings, false); mock::GameTick();
    mock::target = true;
    absent.Tick(settings, false); mock::GameTick();
    Check(mock::attackCount == 0, "target loss must discard the pending attack");

    mock::Reset();
    AutoBlock fallback;
    enable(fallback);
    mock::sword = false;
    fallback.Tick(settings, true); mock::GameTick();
    Check(mock::attackCount == 1 && !mock::usingItem, "non-sword aura attacks must continue");
    mock::sword = true; mock::blockHit = true; mock::hovered = false;
    fallback.Tick(settings, true); mock::GameTick();
    Check(!mock::usingItem, "do not open or use the aimed block to start blocking");

    mock::Reset();
    AutoBlock controls;
    settings.requireRight = true;
    enable(controls);
    controls.Tick(settings, true); mock::GameTick();
    Check(mock::attackCount == 1 && !mock::usingItem, "right mouse requirement only gates automatic blocking");
    mock::right = true;
    controls.Tick(settings, false); mock::GameTick();
    Check(mock::usingItem, "right mouse enables blocking");
    controls.SetEnabled(false);
    Check(mock::key, "disable restores held physical right mouse");
    mock::right = false; mock::key = false; mock::GameTick();
    settings.requireRight = false;

    mock::Reset();
    AutoBlock modes;
    enable(modes);
    modes.Tick(settings, false); mock::GameTick();
    modes.Tick(settings, true); mock::GameTick();
    AutoBlockSettings lagSettings;
    modes.Configure(lagSettings);
    modes.Configure(settings);
    modes.Tick(settings, false); mock::GameTick();
    Check(mock::attackCount == 0 && !mock::lag, "mode switch clears pending vanilla state and lag");
    mock::screen = true;
    modes.Tick(settings, true);
    Check(!mock::key && mock::attackClicks == 0, "screen suspends managed input");
    mock::screen = false; mock::usingItem = false; mock::serverUsing = false;
    mock::world = false;
    modes.Tick(settings, true);
    Check(mock::attackClicks == 0, "world loss must not queue clicks");

    for (int hold = 1; hold <= 10; ++hold) {
        mock::Reset();
        AutoBlock cycle;
        settings.holdTicks = hold;
        enable(cycle);
        for (int tick = 0; tick < 1000; ++tick) {
            const int before = mock::attackCount;
            mock::attackClicks += tick % 7 == 0 ? 3 : 0;
            mock::useClicks += tick % 11 == 0 ? 2 : 0;
            cycle.Tick(settings, tick % 3 != 0);
            mock::GameTick();
            cycle.Render();
            cycle.OnAttackPacket();
            Check(mock::attackCount - before <= 1, "never burst multiple attacks per tick");
            Check(!mock::animation && mock::lagStarts == 0, "vanilla event isolation");
        }
        Check(mock::attackCount > 20, "cycle should keep making progress");
    }
    std::puts("Vanilla AutoBlock checks passed: 10000 ticks, release boundaries, packet order, input, mode changes");

    int originalSlowTicks = 0;
    for (int moveTicks = 0; moveTicks <= 6; ++moveTicks) {
        mock::Reset();
        settings = AutoBlockSettings{};
        settings.mode = AutoBlockMode::Grim;
        settings.moveTicks = moveTicks;
        AutoBlock cycle;
        enable(cycle);
        for (int tick = 0; tick < 240; ++tick) {
            cycle.Tick(settings, tick % 2 == 0);
            mock::GameTick();
        }
        Check(mock::attackCount > 10, "Grim recovery must keep attacking");
        if (moveTicks == 0) originalSlowTicks = mock::slowTicks;
        else Check(mock::slowTicks < originalSlowTicks, "movement window must reduce slowed ticks");
        std::printf("Grim move=%d: slowed=%d/240, attacks=%d\n", moveTicks, mock::slowTicks, mock::attackCount);
        cycle.SetEnabled(false);
        Check(!mock::lag && mock::queued.empty() && !mock::key, "disable must flush and release Grim recovery");
    }

    mock::Reset();
    settings.moveTicks = 2;
    AutoBlock recovery;
    enable(recovery);
    recovery.Tick(settings, false); mock::GameTick(); // Block.
    recovery.Tick(settings, false); mock::GameTick(); // Release into Blink.
    for (int i = 0; i < settings.blinkTicks; ++i) {
        recovery.Tick(settings, false); mock::GameTick();
    }
    Check(!mock::lag && !mock::usingItem, "recovery must run with the network unbuffered");
    recovery.Tick(settings, true); mock::GameTick(false);
    const int before = mock::attackCount;
    for (int i = 0; i < 6; ++i) {
        recovery.Tick(settings, true); mock::GameTick(false);
        Check(!mock::key && mock::attackCount == before, "recovery must await the next movement packet before reblocking");
    }
    mock::Packet('M');
    recovery.Tick(settings, true); mock::GameTick();
    Check(mock::usingItem, "recovery must reblock after its movement boundaries");
    mock::screen = true;
    recovery.Tick(settings, true);
    Check(!mock::key && !mock::lag, "opening a screen must reset Grim recovery");
    std::puts("Grim recovery checks passed: 0-6 move ticks, attack progress, packet order, reset and missing movement");

    for (const auto mode : {AutoBlockMode::Vanilla, AutoBlockMode::Grim}) {
        mock::Reset();
        settings = AutoBlockSettings{};
        settings.mode = mode;
        AutoBlock standalone;
        enable(standalone);
        for (int tick = 0; tick < 120; ++tick) {
            // Physical input and AutoClicker both queue vanilla attack-key presses.
            if (tick % 3 == 0) ++mock::attackClicks;
            standalone.Tick(settings, false, true);
            mock::GameTick();
        }
        Check(mock::attackCount > 5, "standalone must attack without KillAura requests");
        standalone.SetEnabled(false);
        Check(!mock::lag && !mock::key, "standalone disable must release block and packet queue");

        mock::Reset();
        AutoBlock idle;
        enable(idle);
        for (int tick = 0; tick < 30; ++tick) {
            idle.Tick(settings, false, true); mock::GameTick();
        }
        Check(mock::attackCount == 0, "standalone must not manufacture attack clicks");
        idle.SetEnabled(false);
    }
    std::puts("Standalone AutoBlock checks passed: native input, no automatic attacks, cleanup");
    for (const auto mode : {AutoBlockMode::Vanilla, AutoBlockMode::Grim}) {
        mock::Reset();
        settings = AutoBlockSettings{};
        settings.mode = mode;
        AutoBlock visual;
        enable(visual);
        int releasedFrames = 0;
        for (int tick = 0; tick < 100; ++tick) {
            visual.Tick(settings, true); mock::GameTick();
            const auto packets = mock::packets;
            const auto queued = mock::queued;
            const bool use = mock::usingItem, key = mock::key;
            visual.Render();
            Check(mock::animation, "block animation must survive release/attack/reblock phases");
            if (!use) ++releasedFrames;
            Check(mock::packets == packets && mock::queued == queued && mock::usingItem == use && mock::key == key,
                  "render animation must not alter packets, use state or movement slowdown");
        }
        Check(releasedFrames > 0 && mock::attackCount > 0, "continuous animation must not hold real blocking");
        settings.animation = false;
        visual.Configure(settings); visual.Render();
        Check(!mock::animation, "turning animation off must clear the render override");
        settings.animation = true;
        visual.Configure(settings); visual.Render();
        Check(mock::animation, "turning animation back on must preserve the live cycle");
        mock::sword = false; visual.Render();
        Check(!mock::animation, "switching away from sword must clear animation");
        mock::sword = true; visual.Render();
        mock::world = false; visual.Render();
        Check(!mock::animation, "missing world must clear animation");
        mock::world = true; visual.Render();
        mock::screen = true; visual.Render();
        Check(!mock::animation, "opening screen must clear animation immediately");
        mock::screen = false; visual.Render();
        mock::target = false;
        visual.Tick(settings, false); mock::GameTick();
        // A release waiting for a movement boundary can finish next tick.
        visual.Tick(settings, false); visual.Render();
        Check(!mock::animation, "losing target must end forced animation");
        mock::target = true;
        visual.Tick(settings, false); mock::GameTick(); visual.Render();
        visual.SetEnabled(false);
        Check(!mock::animation, "disable must clear animation in every phase");
    }
    std::puts("AutoBlock animation: continuous release frames, live toggle, item/world/screen/target cleanup passed");
    for (const auto mode : {AutoBlockMode::Lag, AutoBlockMode::Vanilla, AutoBlockMode::Grim}) {
        mock::Reset();
        mock::teammate = true;
        settings = AutoBlockSettings{};
        settings.mode = mode;
        AutoBlock friendly;
        enable(friendly);
        for (int tick = 0; tick < 20; ++tick) {
            friendly.Tick(settings, true, true); mock::GameTick();
        }
        Check(mock::attackCount == 0 && !mock::lag, "Teams excludes friendly targets and fallback attacks");
        friendly.SetEnabled(false);
    }
}
