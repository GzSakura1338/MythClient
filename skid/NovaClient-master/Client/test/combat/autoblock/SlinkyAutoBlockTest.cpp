int main(){
    using namespace nova;
    using namespace nova::module;
    mock::Reset();
    AutoBlock ab; AutoBlockSettings s; s.mode=AutoBlockMode::Slinky;
    ab.Configure(s);ab.SetEnabled(true);
    auto tick=[&](int delta=0){mock::now+=delta;ab.Tick(s);};
    // Source default: no mouse required, key-only start, no lag, 150ms hold.
    tick();mock::Check(ab.Blocking()&&mock::key&&mock::useClicks==0,"Slinky key-only immediate block");
    tick(149);mock::Check(ab.Blocking(),"hold before deadline");
    tick(1);mock::Check(!ab.Blocking()&&!mock::key&&!mock::lag,"default releases without lag");
    tick(50);mock::Check(ab.Blocking(),"default reblocks next motion");
    s.slinky.immediately=false;ab.Reset();mock::hurt=5;
    tick();mock::Check(!ab.Blocking(),"hurt above floor(ms/50)");
    mock::hurt=4;tick();mock::Check(ab.Blocking(),"hurt threshold inclusive");
    s.slinky.hurtMs=95;ab.Reset();mock::hurt=2;tick();mock::Check(!ab.Blocking(),"hurt uses integer truncation");
    mock::hurt=1;tick();mock::Check(ab.Blocking(),"hurt one tick allowed");
    s.slinky.immediately=true;s.slinky.damagedOnly=true;mock::hurt=0;ab.Reset();
    tick();mock::Check(!ab.Blocking(),"requires observed damage");
    mock::hurt=10;tick();mock::Check(ab.Blocking(),"damage window opens");
    mock::hurt=5;tick(2001);mock::Check(!ab.Blocking(),"damage window expires");
    s.slinky.damagedOnly=false;s.slinky.requireLeft=true;
    tick();mock::Check(!ab.Blocking(),"left condition uses physical button only");
    mock::left=true;s.slinky.requireRight=true;tick();mock::Check(!ab.Blocking(),"both buttons required");
    mock::right=true;tick();mock::Check(ab.Blocking(),"both buttons satisfied");
    s.slinky.requireLeft=s.slinky.requireRight=false;mock::targetX=3.5;
    tick();mock::Check(!ab.Blocking(),"strict position-distance range boundary");
    mock::targetX=3.49;tick();mock::Check(ab.Blocking(),"inside range");
    s.slinky.ignoreBots=true;mock::targetName="a";tick();mock::Check(!ab.Blocking(),"script name bot filter");
    mock::targetName="Enemy";s.slinky.ignoreBots=false;s.slinky.ignoreTeammates=true;
    mock::ownDisplay="\xc2\xa7" "aSelf";mock::targetDisplay="\xc2\xa7" "aEnemy";
    tick();mock::Check(!ab.Blocking(),"script first-color team filter");
    mock::targetDisplay="\xc2\xa7" "bEnemy";tick();mock::Check(ab.Blocking(),"different color allowed");
    s.slinky.ignoreTeammates=false;mock::teammate=true;tick();mock::Check(!ab.Blocking(),"central Teams respected");
    mock::teammate=false;antibot::bot=true;tick();mock::Check(!ab.Blocking(),"central AntiBot respected");
    antibot::bot=false;tick();
    mock::blockHit=true;mock::blockName="minecraft:oak_door";tick();
    mock::Check(!ab.Blocking()&&mock::key&&!mock::inputManaged,"interactive block returns physical use key");
    tick();mock::Check(!ab.Blocking(),"interaction latch");
    mock::blockHit=false;tick();mock::Check(ab.Blocking(),"interaction latch clears");
    s.slinky.lagChance=100;s.slinky.lagMs=100;
    tick(150);mock::Check(mock::lag&&mock::slinkyLag&&!mock::key,"lag starts before key release");
    tick(99);mock::Check(mock::lag&&!ab.Blocking(),"lag before deadline");
    tick(1);mock::Check(!mock::lag&&!ab.Blocking(),"expiry drains but defers reblock");
    tick(50);mock::Check(ab.Blocking(),"reblock after lag");
    tick(150);mock::slinkyLag=mock::lag=false;mock::slinkyAttack=true;
    tick();mock::Check(ab.Blocking()&&mock::key,"attack flush reblock notice");
    // Lost eligibility never leaves lagActive intercepting the connection.
    for(int reason=0;reason<5;++reason){
        ab.Reset();mock::sword=mock::target=true;mock::screen=mock::dead=false;
        tick();tick(150);mock::Check(mock::slinkyLag,"start cleanup case");
        if(reason==0)mock::sword=false;
        if(reason==1)mock::target=false;
        if(reason==2)mock::screen=true;
        if(reason==3)mock::dead=true;
        if(reason==4)++mock::resetSequence;
        tick();mock::Check(!mock::lag&&!ab.Blocking()&&!mock::inputManaged,"cleanup releases own queue and input ownership");
        mock::Check(mock::key==(reason==1||reason==4),"cleanup restores manual use only in valid gameplay");
    }
    mock::sword=mock::target=true;mock::screen=mock::dead=false;
    tick();tick(150);mock::slinkyLag=false; // Prediction takes the queue.
    tick();ab.Reset();mock::Check(mock::lag,"reset must preserve another queue owner");
    mock::lag=false;tick();tick(150);ab.SetEnabled(false);
    mock::Check(!mock::lag&&mock::key&&!mock::animation&&!mock::inputManaged,"disable returns held manual use");
    ab.SetEnabled(true);mock::sword=false;mock::key=true;tick();
    mock::Check(mock::key,"idle non-sword mode must not cancel native bow/rod use");
    ab.SetEnabled(false);
    for (int idle=0;idle<4;++idle) {
        mock::Reset(); mock::right=true;
        AutoBlock manual; AutoBlockSettings settings; settings.mode=AutoBlockMode::Slinky;
        if(idle==0) mock::target=false;
        if(idle==1) settings.slinky.requireLeft=true;
        if(idle==2) settings.slinky.damagedOnly=true;
        if(idle==3) settings.slinky.immediately=false;
        manual.Configure(settings); manual.SetEnabled(true);
        for(int i=0;i<8;++i) {
            manual.Tick(settings); mock::GameTick();
            mock::Check(mock::usingItem&&mock::key&&!mock::inputManaged&&!manual.Blocking(),
                "idle Slinky permits sustained vanilla manual blocking");
        }
        mock::right=false; manual.Tick(settings); mock::GameTick();
        mock::Check(!mock::usingItem&&!mock::key,"idle Slinky releases manual block with physical key");
        mock::target=true; settings.slinky.requireLeft=settings.slinky.damagedOnly=false;
        settings.slinky.immediately=true; settings.animation=false;
        manual.Tick(settings);
        mock::Check(mock::inputManaged&&manual.Blocking()&&!mock::animation,
            "active input ownership remains independent of animation setting");
        mock::right=true; mock::target=false; manual.Tick(settings);
        mock::Check(mock::key&&!mock::inputManaged,"target loss hands a held key back to vanilla");
        manual.SetEnabled(false);
    }
    // A held physical RMB must not overwrite Slinky's release during render.
    // Drive actual vanilla input processing between motion ticks, not just the internal blocking flag.
    for(int lagChance : {0,100}) {
        mock::Reset(); mock::right=true;
        AutoBlock guarded; AutoBlockSettings cfg; cfg.mode=AutoBlockMode::Slinky;
        cfg.disableManualBlock=true; cfg.slinky.lagChance=lagChance; cfg.slinky.lagMs=100;
        guarded.Configure(cfg); guarded.SetEnabled(true);
        int releases=0;
        for(int i=0;i<80;++i) {
            mock::now+=50; guarded.Tick(cfg);
            for(int frame=0;frame<8;++frame) guarded.Render();
            const bool hadUse=mock::usingItem;
            mock::GameTick();
            if(hadUse && !mock::usingItem) ++releases;
        }
        mock::Check(releases>=8,"Disable Manual Block must preserve repeated Slinky releases with RMB held");
        // Losing a target finishes the held block; press-count clicks remain untouched.
        mock::target=false; guarded.Tick(cfg); guarded.Render(); mock::GameTick();
        mock::Check(!mock::usingItem,"suppression must end lingering block after target loss");
        guarded.Render();
        mock::useClicks=1; guarded.Render();
        mock::Check(!mock::key && mock::manualSuppressed && mock::useClicks==1,
                    "idle Slinky keeps physical clicks for containers without holding the use key");
        guarded.SetEnabled(false);
    }
    // A pending native right click may outlive a short hold window. Simulate
    // several cooldown phases before vanilla acknowledges use, including stalls.
    for(int wait : {50,100,200,500}) {
        mock::Reset();
        AutoBlock pending; AutoBlockSettings cfg; cfg.mode=AutoBlockMode::Slinky;
        cfg.disableManualBlock=true; cfg.slinky.holdMs=50;
        pending.Configure(cfg); pending.SetEnabled(true); pending.Tick(cfg);
        mock::now+=wait; pending.Tick(cfg);
        mock::Check(mock::key && pending.Blocking() && !mock::manualSuppressed,
                    "pending automatic use survives native cooldown beyond hold duration");
        mock::usingItem=true; pending.Tick(cfg);
        mock::now+=49; pending.Tick(cfg); mock::Check(mock::key,"hold starts at actual use");
        ++mock::now; pending.Tick(cfg); mock::Check(!mock::key,"confirmed block releases at deadline");
        mock::usingItem=false; pending.Tick(cfg); mock::target=false; pending.Tick(cfg);
        mock::Check(!mock::key && !pending.Blocking(),"target loss cancels pending use");
        pending.SetEnabled(false);
    }
    std::puts("PASS: Slinky source timings, physical gates, filters, interaction, attack reblock, lost-target cleanup and owner isolation");
}
