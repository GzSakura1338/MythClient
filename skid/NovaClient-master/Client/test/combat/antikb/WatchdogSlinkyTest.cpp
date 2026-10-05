// Combines actual AntiKB/EventBus and actual Lunar Blink functions. Only game,
// JNI and clock are mocked. No live injection or Java source is involved.
int main() {
    auto module=nova::module::MakeAntiKbModule();
    nova::event::EventBus::Get().Seal();
    mock::legacy=true;mock::ground=true;
    module->modes.at("Mode").value=3;
    blink::watchdogBlocksBlink=&nova::module::antikb::BlocksAutoBlockBlink;
    mock::endSlinky=[](bool send){blink::EndSlinkyBlockLag(send);};
    mock::slinkyLagging=[] { return blink::SlinkyBlockLagging(); };
    mock::hypixelLagging=[] { return blink::HypixelBlockLagging(); };
    mock::endHypixel=[](bool send) { blink::EndHypixelBlockLag(send); };
    mock::syncSlinky=[](bool discard) { nova::module::autoblock::SynchronizePacketDelays(discard); };
    blink::Object manager,move{blink::Move,10},ack{blink::Transaction,11};
    mock::Packet velocity{1,mock::Motion,7,8000,1600,0},ping{2,mock::Ping},correction{3,mock::Position};
    mock::Packet join{4,mock::Join},respawn{5,mock::Respawn},disconnect{6,mock::Disconnect};
    double motionX=0;
    auto send=[&](blink::Object& packet) {
        if(!blink::HoldBlockPacket(&blink::environment,&manager,&packet,nullptr,false))
            blink::environment.CallNonvirtualVoidMethod(&manager,0,0,&packet,nullptr);
    };
    mock::replayObserver=[&](auto* packet) {
        if(packet->kind==mock::Motion)motionX=packet->x/8000.0;
        if(packet->kind==mock::Position)motionX=0; // vanilla absolute-axis S08
        if(packet->kind==mock::Ping)send(ack);
    };
    auto motion=[] {
        nova::event::MotionEvent e(nova::event::EventType::Pre,0.f,0.f);
        (void)nova::event::EventBus::Get().Post(e);
    };
    auto begin=[] {
        const bool started=blink::BeginSlinkyBlockLag(100,3,true);
        nova::module::autoblock::SynchronizePacketDelays(false);
        return started;
    };
    auto clear=[&]{mock::handled.clear();blink::sent.clear();motionX=0;};

    // Enable in an existing world: valid pending output is sent, not discarded.
    Check(begin(),"Slinky alone cannot start");send(move);
    module->setEnabled(true);SettleWatchdog();
    Check(blink::sent==std::vector<int>({10}),"enable discarded valid output");
    clear();Receive(velocity);Receive(ping);motion();StartTick();
    Check(motionX==1 && blink::sent==std::vector<int>({11}),"baseline replay failed");

    // Slinky observes input ownership before replay and yields its own output.
    clear();Receive(velocity);Receive(ping);motion();
    Check(motionX==1 && blink::sent==std::vector<int>({11}) && mock::refs==0,
          "Rise ground release was not synchronous");
    send(move);StartTick();
    Check(motionX==1 && blink::sent==std::vector<int>({11,10}) && blink::refs==0,
          "replayed C0F was delayed a second time");

    // Reverse start order: existing output must drain before input is replayed.
    clear();Check(begin(),"Slinky did not recover after replay");send(move);
    Check(Receive(velocity),"Watchdog failed to retain self velocity");
    Check(blink::sent==std::vector<int>({10}) && !blink::SlinkyBlockLagging(),
          "Watchdog did not finish existing Slinky FIFO");
    Check(!begin(),"Slinky rearmed while input owned");
    Receive(ping);motion();StartTick();
    Check(blink::sent==std::vector<int>({10,11}) && mock::refs==0 && blink::refs==0,
          "reverse start order lost FIFO or references");
    std::puts("PASS: both Watchdog/Slinky start orders preserve FIFO and deliver replayed C0F without a second delay");

    // Nested native sends can skip outgoing events. Even if the normal
    // post-input policy did not run yet, Slinky's native adapter must yield.
    clear();Check(begin(),"Slinky setup for nested reply failed");send(move);
    auto synchronize=mock::syncSlinky;mock::syncSlinky={};
    Receive(velocity);Receive(ping);
    Check(blink::SlinkyBlockLagging(),"AntiKB directly ended Slinky instead of leaving policy to Slinky");
    mock::Packet attack{7,mock::Attack};
    Receive(attack,false,nova::event::EventType::Outgoing);
    Check(motionX==1 && blink::sent==std::vector<int>({10,11}) && blink::refs==0 && mock::refs==0,
          "native Slinky adapter did not yield before synchronous replay acknowledgement");
    mock::syncSlinky=synchronize;mock::syncSlinky(false);
    std::puts("PASS: AntiKB does not drive Slinky; native Slinky adapter handles nested confirmation replay");

    // S08 must win over all older S12, with S32 acknowledgements preserved.
    clear();Receive(velocity);Receive(ping);
    blink::DrainBlockPackets(&blink::environment,true); // ProcessClientPacket
    Check(!Receive(correction),"correction cancelled");
    Check(mock::handled==std::vector<int>({1,2,3}) && motionX==0 && mock::refs==0,
          "old velocity survived correction, or old confirmations were lost");
    Check(blink::sent==std::vector<int>({11}),"correction lost transaction acknowledgement");
    motion();StartTick();Check(motionX==0,"next tick undid S08 with stale motion");
    mock::ground=false;
    for(int i=0;i<40;++i) {
        StartTick();Check(!begin(),"Slinky began during airborne correction recovery");
        Check(!Receive(velocity)&&!Receive(ping),"setback recovery captured server data");
    }
    mock::ground=true;StartTick();StartTick();
    Check(!begin(),"single ground flicker finished recovery too early");
    mock::ground=false;StartTick();mock::ground=true;StartTick();StartTick();
    Check(!begin(),"ground counter did not reset after airborne tick");
    StartTick();Check(begin(),"stable correction recovery did not rearm Slinky");
    blink::EndSlinkyBlockLag();
    std::puts("PASS: correction applies after old FIFO, retains confirmations, and cannot replay stale velocity later");

    // Join/respawn/disconnect can occur between ticks with an old player still
    // visible. Same numeric entity id in the next world must not preserve state.
    for(auto* boundary:{&join,&respawn,&disconnect}) {
        clear();Receive(velocity);Receive(ping);
        Check(!Receive(*boundary),"world boundary was cancelled");
        Check(mock::handled==std::vector<int>({boundary->serial}) && mock::refs==0,
              "obsolete world queue replayed across boundary");
        ++mock::worldIdentity;++mock::playerIdentity;mock::ticks=0;
        StartTick();
        // Default onGround during initialization is insufficient.
        for(int i=0;i<8;++i) {StartTick();Check(!Receive(velocity),"initial world state captured S12");}
        mock::ticks=100;mock::ground=false;
        for(int i=0;i<50;++i) {StartTick();Check(!Receive(velocity)&&!Receive(ping),"spawn fall captured input");}
        Check(!begin() && mock::refs==0,"spawn fall allowed nested output delay");
        mock::ground=true;StartTick();StartTick();
        Check(!Receive(velocity),"spawn recovered before three grounded ticks");
        StartTick();Check(Receive(velocity),"landed player never resumed Watchdog");
        motion();StartTick();
    }
    // Identity-only transitions cover missed lifecycle packets/null ticks.
    for(bool changePlayer:{false,true}) {
        clear();Receive(velocity);Receive(ping);
        if(changePlayer)++mock::playerIdentity;else ++mock::worldIdentity;
        StartTick();
        Check(mock::handled.empty()&&mock::refs==0,"identity transition replayed old world queue");
        SettleWatchdog();
    }
    clear();Receive(velocity);mock::player=false;StartTick();
    Check(mock::refs==0,"missing player retained old packets");
    mock::player=true;SettleWatchdog();
    clear();Check(begin(),"Slinky start before world loss failed");send(move);
    mock::player=false;StartTick();
    Check(blink::sent.empty()&&blink::refs==0&&!blink::SlinkyBlockLagging(),
          "missing player retained or sent obsolete Slinky output");
    mock::player=true;SettleWatchdog();
    Check(begin(),"Slinky start before respawn event failed");send(move);
    nova::event::RespawnEvent respawnEvent;
    (void)nova::event::EventBus::Get().Post(respawnEvent);
    nova::module::autoblock::SynchronizePacketDelays(true);
    Check(blink::sent.empty()&&blink::refs==0,"native respawn event sent stale Slinky output");
    SettleWatchdog();
    std::puts("PASS: join, respawn, disconnect, spawn fall, player/world replacement and missing-player lifecycle");

    // Changing mode/disable releases this ownership. Other AB queue owners
    // are never drained by Watchdog's Slinky-specific coordination.
    for(bool prediction:{false,true}) {
        if(prediction)Check(blink::BeginPredictionBlink(),"Prediction start failed");
        else blink::BeginHypixelBlockLag();
        send(move);Receive(velocity);
        Check(prediction ? (blink::blockLagActive && blink::blockPackets.size()==1) : !blink::blockLagActive,
              "Hypixel did not yield, or the policy drained Prediction");
        motion();StartTick();
        Check(prediction ? blink::blockLagActive.load() : !blink::blockLagActive.load(),"AB owner isolation changed");
        if(prediction)blink::EndPredictionBlink(false);else blink::EndHypixelBlockLag();
    }
    for(int mode:{0,1,2}) {
        module->modes.at("Mode").value=mode;StartTick();
        Check(begin(),"another AntiKB mode inherited Slinky restriction");blink::EndSlinkyBlockLag();
    }
    module->modes.at("Mode").value=3;StartTick();
    Check(!begin(),"reentered Watchdog skipped lifecycle guard");
    module->setEnabled(false);Check(begin(),"disabled Watchdog blocked Slinky");blink::EndSlinkyBlockLag();
    module->setEnabled(true);module.reset();
    Check(begin(),"destroyed Watchdog blocked Slinky");blink::EndSlinkyBlockLag();
    mock::replayObserver={};mock::endSlinky={};
    mock::syncSlinky={};mock::slinkyLagging={};mock::hypixelLagging={};mock::endHypixel={};
    Check(mock::refs==0 && blink::refs==0 && mock::wanted==0,"reference/interest leak");
    std::puts("PASS: Slinky alone, other AntiKB/AB owners, mode switch, disable and destruction remain isolated");
}
