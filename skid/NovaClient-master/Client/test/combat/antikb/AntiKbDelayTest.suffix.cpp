namespace {
using namespace nova;
using mock::Check;
void Tick() { event::PlayerTickEvent e; event::EventBus::Get().Post(e); }
void StartTick() {
    event::GameTickEvent e(event::EventType::Pre); (void)event::EventBus::Get().Post(e);
    if(mock::syncSlinky)mock::syncSlinky(false);
}
void SettleWatchdog() {
    const bool ground=mock::ground;
    mock::ground=true;
    for(int i=0;i<4;++i)StartTick();
    mock::ground=ground;
    Check(!module::antikb::BlocksAutoBlockBlink(),"Watchdog did not finish spawn recovery");
}
bool Receive(mock::Packet& packet, bool cancelled = false,
             event::EventType type = event::EventType::Incoming) {
    event::PacketEvent e(type, &packet);
    if(type==event::EventType::Incoming && mock::syncSlinky &&
       (packet.kind==mock::Join||packet.kind==mock::Respawn||packet.kind==mock::Disconnect))mock::syncSlinky(true);
    if (cancelled) e.Cancel();
    event::EventBus::Get().Post(e);
    if(type==event::EventType::Incoming && mock::syncSlinky)mock::syncSlinky(false);
    if (!e.IsCancelled() && type == event::EventType::Incoming)
        java::HandleIncomingPacket(&packet);
    if (!e.IsCancelled() && type == event::EventType::Outgoing)
        mock::wire.push_back(packet.kind);
    return e.IsCancelled();
}

}
int main() {
    auto module = nova::module::MakeAntiKbModule();
    auto keepSprint = event::EventBus::Get().Subscribe<event::AttackSlowdownEvent>(
        [](event::AttackSlowdownEvent& e) { e.slowdown = 1.0; e.sprint = true; });
    nova::event::EventBus::Get().Seal();
    Check(!module->bools.at("Force 0.6").value, "Force 0.6 must default off");
    auto coefficient = [&](int hurt) {
        event::AttackSlowdownEvent e(false,hurt);
        (void)event::EventBus::Get().Post(e);
        Check(e.sprint, "Force 0.6 must preserve independent sprint policy");
        return e.slowdown;
    };
    mock::legacy=true;
    module->modes.at("Mode").value=3;module->setEnabled(true);
    Check(coefficient(9)==1.0, "disabled setting must preserve KeepSprint");
    module->bools.at("Force 0.6").value=true;
    Check(coefficient(9)==0.6 && coefficient(1)==0.6,
          "hurt attacks must override KeepSprint exactly once");
    Check(coefficient(0)==1.0, "unhurt attacks must preserve KeepSprint");
    mock::legacy=false;
    Check(coefficient(9)==1.0, "unsupported profiles must not override slowdown");
    mock::legacy=true;
    module->setEnabled(false);
    Check(coefficient(9)==1.0, "disabled module must not override slowdown");
    for(int mode : {0,1,2,4}) {
        module->modes.at("Mode").value=mode;module->setEnabled(true);
        Check(coefficient(9)==1.0, "other AntiKB modes must not override slowdown");
        module->setEnabled(false);
    }
    module->bools.at("Force 0.6").value=false;
    module->modes.at("Mode").value=0;keepSprint.Reset();mock::legacy=false;
    std::puts("PASS: optional Watchdog Force 0.6 overrides KeepSprint only on hurt attacks; defaults, mode and disable isolation");
    Check(module->modes.at("Mode").value == 0 && module->numbers.at("Delay Ticks").value == 3,
          "Delay must remain the default mode with 3 ticks");
    Check(!module->bools.at("Jump Reset").value, "Jump Reset must default off");
    Check(!module->bools.at("Delay Rotation").value, "Delay Rotation must default off");
    Check(module->numbers.at("Turn Ticks").value==1,"turn duration must preserve the previous one-tick default");
    for(int duration:{1,3,10}) {
        module->numbers.at("Delay Ticks").value=1;
        module->numbers.at("Turn Ticks").value=duration;
        module->bools.at("Delay Rotation").value=true;
        module->setEnabled(true);mock::handled.clear();mock::rotations=0;
        mock::Packet kb{89,mock::Motion};Receive(kb);Tick();StartTick();
        Check(mock::rotations==1&&mock::handled==std::vector<int>({89}),"turn duration must not extend knockback delay");
        for(int tick=1;tick<duration;++tick) { StartTick();Check(mock::rotations==tick+1,"rotation window ended early"); }
        StartTick();Check(mock::rotations==duration&&mock::handled.size()==1,"rotation window must stop without replaying packets");
        module->setEnabled(false);StartTick();Check(mock::rotations==duration,"disable must clear turn window");
    }
    module->numbers.at("Turn Ticks").value=1;
    // Observe actual replay: the new request must already exist when the first
    // velocity is applied, with Velocity's source-facing convention and pitch.
    for (const auto& axes : std::vector<std::pair<int,int>>{{100,0},{-100,0},{0,100},{0,-100},{100,100}}) {
        module->numbers.at("Delay Ticks").value=2;
        module->bools.at("Delay Rotation").value=true;
        module->setEnabled(true);mock::handled.clear();mock::rotations=0;
        nova::rotation::applied={13,27};
        mock::Packet kb{90,mock::Motion};kb.x=axes.first;kb.z=axes.second;
        mock::Packet following{91,mock::Ping};
        Check(Receive(kb)&&Receive(following),"rotation must retain incoming FIFO");
        StartTick();Tick();StartTick();Tick();
        Check(mock::rotations==0&&mock::handled.empty(),"must not rotate during delay countdown");
        mock::replayObserver=[&](jobject packet) {
            if(packet!=&kb)return;
            const float expected=nova::rotation::mth::WrapDegrees(static_cast<float>(
                std::atan2(double(kb.z),double(kb.x))*57.29577951308232+90.0));
            Check(mock::rotations==1,"rotation must precede velocity replay exactly once");
            Check(std::abs(nova::rotation::applied.yaw-expected)<.001f&&nova::rotation::applied.pitch==27,
                "release must face source and retain pitch");
        };
        StartTick();mock::replayObserver={};
        Check(mock::handled==std::vector<int>({90,91})&&mock::refs==0,"rotation changed replay order or ownership");
        StartTick();Check(mock::rotations==1,"completed release must not keep submitting rotation");
        module->setEnabled(false);
    }
    module->bools.at("Delay Rotation").value=false;
    module->numbers.at("Delay Ticks").value=3;
    mock::rotations=0;
    for(int gate=0;gate<4;++gate) {
        module->numbers.at("Delay Ticks").value=1;
        module->bools.at("Delay Rotation").value=gate!=0;
        mock::scaffold=gate==1;mock::screen=gate==2;mock::menu=gate==3;
        module->setEnabled(true);mock::handled.clear();
        mock::Packet held{92,mock::Motion};Receive(held);Tick();StartTick();
        Check(mock::rotations==0&&mock::handled==std::vector<int>({92}),
            "disabled/gated rotation must still release knockback on schedule");
        module->setEnabled(false);
    }
    mock::scaffold=mock::screen=mock::menu=false;
    module->bools.at("Delay Rotation").value=false;
    module->numbers.at("Delay Ticks").value=3;
    std::puts("PASS: optional Delay rotation precedes FIFO replay, faces source, preserves pitch and runs once");
    mock::Packet first{1, mock::Motion}, ordinary{2, mock::Ordinary},
        next{3, mock::Motion}, position{4, mock::Position},
        explosion{5, mock::Explosion}, respawn{6, mock::Respawn},
        disconnect{7, mock::Disconnect};

    for (int ticks = 1; ticks <= 5; ++ticks) {
        module->numbers.at("Delay Ticks").value = ticks;
        module->setEnabled(true);
        mock::handled.clear();
        Check(Receive(first) && mock::handled.empty(), "trigger velocity must wait in the queue");
        Check(Receive(ordinary) && Receive(next), "all subsequent incoming packets must queue");
        Check(Receive(position) && Receive(explosion) && Receive(respawn),
              "correction, explosion and respawn must queue without early flush");
        nova::event::GameTickEvent game(nova::event::EventType::Pre);
        nova::event::EventBus::Get().Post(game);
        Check(module->suffix() == std::format("Delay {}t", ticks), "game tick must not spend player tick budget");
        for (int i = 1; i < ticks; ++i) {
            Tick();
            StartTick();
            Check(mock::handled.empty(), "knockback released before configured player tick");
        }
        Tick();
        Check(mock::handled.empty() && nova::module::antikb::OwnsIncomingDelayQueue(),
              "player update must only expire countdown, not replay after attacks");
        nova::event::GameTickEvent post(nova::event::EventType::Post);
        (void)nova::event::EventBus::Get().Post(post);
        Check(mock::handled.empty(), "post tick must not replay mid-frame");
        StartTick();
        Check(mock::handled == std::vector<int>({1,2,3,4,5,6}), "replay must be FIFO and exactly once");
        Check(mock::refs == 0 && module->suffix() == "Delay 0t", "drain must release refs/countdown");
        Check(!Receive(first) && module->suffix() == "Delay 0t", "correction/explosion must inhibit rearming for 100ms");
        std::this_thread::sleep_for(std::chrono::milliseconds(110));
        Check(Receive(first) && module->suffix() != "Delay 0t", "delay must rearm and hold velocity after 100ms guard");
        Receive(ordinary);
        const auto before = mock::handled.size();
        module->setEnabled(false);
        Check(mock::handled.size() == before + 2 && mock::handled[before] == 1 && mock::handled.back() == 2,
              "disable must flush pending packets");
        Check(mock::refs == 0 && mock::wanted == 0, "disable leaked refs or incoming interest");
    }

    // Reproduce the release-frame ordering with a new transaction arriving both
    // after the countdown expires and while the previous batch is being handled.
    module->numbers.at("Delay Ticks").value = 5;
    module->setEnabled(true);
    mock::handled.clear();
    Receive(first);
    mock::Packet transaction{20, mock::Ping}, duringReplay{21, mock::Ping};
    Receive(transaction);
    for (int i = 0; i < 5; ++i) { StartTick(); Tick(); }
    Check(mock::handled.empty(), "all five player updates must finish before velocity applies");
    Check(Receive(ordinary), "arrivals at zero ticks must stay behind the pending velocity");
    bool inTickStart = false;
    double localMotion = 0;
    mock::replayObserver = [&](jobject packet) {
        Check(inTickStart, "delayed packets must replay before the next interaction phase");
        Check(nova::module::antikb::OwnsIncomingDelayQueue(), "ownership released inside replay");
        if (packet == &first) {
            localMotion = 1;
            // Different thread, joined deterministically: the packet lock must be
            // available and this arrival must not overtake the original batch.
            std::thread network([&] { Check(Receive(duringReplay), "arrival overtook replay batch"); });
            network.join();
        }
    };
    inTickStart = true;
    StartTick();
    inTickStart = false;
    // Vanilla interaction reduces the already-applied velocity before physics.
    localMotion *= 0.6;
    Tick();
    Check(std::abs(localMotion - 0.6) < 0.0001, "replay overwrote attack slowdown");
    Check(mock::handled == std::vector<int>({1,20,2,21}), "transaction/new arrival replay order changed");
    Check(mock::refs == 0 && !nova::module::antikb::OwnsIncomingDelayQueue(),
          "completed replay must release queue ownership and references");
    mock::replayObserver = {};
    module->setEnabled(false);

    module->setEnabled(true);
    mock::handled.clear();
    for (auto invalid : {mock::Packet{8,mock::Motion,99}, mock::Packet{9,mock::Motion,7,0,100,0},
                         mock::Packet{10,mock::Motion,7,100,0,0}}) {
        Check(!Receive(invalid) && module->suffix() == "Delay 0t", "non-qualifying motion armed delay");
    }
    Receive(first, true);
    Receive(first, false, nova::event::EventType::Outgoing);
    Check(module->suffix() == "Delay 0t", "cancelled/outgoing packet armed delay");
    mock::failRef = true;
    Check(!Receive(first) && module->suffix() == "Delay 0t" && mock::refs == 0 &&
          !nova::module::antikb::OwnsIncomingDelayQueue(),
          "failed trigger retention must pass velocity without arming delay");
    mock::failRef = false;
    Receive(first);
    mock::failRef = true;
    Check(!Receive(ordinary), "failed retention must not swallow packet");
    mock::failRef = false;
    Receive(ordinary);
    const auto beforeDisconnect = mock::handled.size();
    Check(!Receive(disconnect), "disconnect must pass through");
    Check(mock::handled.size() == beforeDisconnect + 1 && mock::handled.back() == 7 && mock::refs == 0,
          "disconnect must discard queue, not replay it");
    Receive(first);
    Receive(ordinary);
    mock::player = false;
    nova::event::GameTickEvent game(nova::event::EventType::Pre);
    nova::event::EventBus::Get().Post(game);
    Check(mock::refs == 0 && module->suffix() == "Delay 0t", "world exit must discard stale packets");
    mock::player = true;

    for (bool enabled : {false, true}) {
        module->bools.at("Jump Reset").value = enabled;
        for (bool ground : {false, true}) for (int hurt : {0,8,9,10}) {
            mock::ground = ground; mock::hurt = hurt;
            nova::event::MoveInputEvent input(0,0,false,false,0.3);
            nova::event::EventBus::Get().Post(input);
            Check(input.jump == (enabled && ground && hurt == 9), "Jump Reset must use ground/hurtTime==9");
        }
    }
    module->setEnabled(false);
    module->modes.at("Mode").value = 1;
    module->setEnabled(true);
    Check(module->numbers.at("Attack Counts").value == 1 &&
          module->numbers.at("Sprint Ticks").value == 3 &&
          module->numbers.at("Max Delay").value == 1000 &&
          !module->bools.at("Swing Hand").value, "Reduce defaults differ from Epsilon");
    auto clientTick = [&] { (void)nova::event::EventBus::Get().Post(game); };
    auto input = [&] {
        nova::event::MoveInputEvent e(0.25f,0.5f,false,false,0.3);
        (void)nova::event::EventBus::Get().Post(e);
        return e;
    };
    mock::ground = false; mock::target = true;
    mock::handled.clear();
    Check(Receive(first) && nova::module::antikb::IsDelaying(), "Reduce must hold first velocity");
    Check(std::abs(nova::module::antikb::Yaw() - 90) < 0.01f, "velocity yaw mismatch");
    Check(!Receive(ordinary) && !Receive(respawn), "Reduce must pass non-whitelisted packets");
    mock::Packet move{11,mock::Move}, teleport{12,mock::Teleport}, ping{13,mock::Ping}, look{14,mock::LookAt};
    Check(Receive(move) && Receive(teleport) && Receive(ping) && Receive(look) && Receive(next),
          "Reduce must hold all six reference packet families");
    Tick(); clientTick();
    Check(nova::module::antikb::IsDelaying(), "player ticks must not drain Reduce before ground/timeout");
    auto walking = input();
    Check(walking.forward == 1 && walking.strafe == 0 && !walking.jump,
          "delayed velocity + target + zero fall distance must force forward");
    nova::module::antibot::bot = true;
    Check(input().forward == 0.25f, "AntiBot must prevent target-driven forward input");
    nova::module::antibot::bot = false;
    mock::fall = 1;
    Check(input().forward == 0.25f, "falling must suppress delayed forward input");
    mock::fall = 0;
    mock::ground = true;
    clientTick();
    Check(mock::handled == std::vector<int>({2,6,1,11,12,13,14,3}) && mock::refs == 0,
          "Reduce flush must preserve selected packet arrival order");
    Check(nova::module::antikb::AttackQueue() == 1 && mock::rotations == 1,
          "normal flush must arm attack and rotation queues");
    mock::actions.clear();
    Tick();
    Check(mock::actions == "AS" && nova::module::antikb::AttackQueue() == 0,
          "default Reduce must attack then send swing packet, without local hand swing");
    Check(input().jump, "flush must arm grounded jump");
    Check(!input().jump, "jump must be consumed once");
    Check(input().forward == 1 && input().forward == 0.25f, "sprint input budget must last exactly 3 input events");

    // A miss, dead entity, non-player or bot consumes one scheduled attempt.
    module->numbers.at("Attack Counts").value = 4;
    Receive(first); clientTick();
    mock::actions.clear(); mock::target = false;
    Tick(); Check(nova::module::antikb::AttackQueue() == 3, "miss did not consume one attack");
    mock::target = true; mock::alive = false;
    Tick(); Check(nova::module::antikb::AttackQueue() == 2, "dead target did not consume one attack");
    mock::alive = true; mock::targetPlayer = false;
    Tick(); Check(mock::actions.empty() && nova::module::antikb::AttackQueue() == 1, "invalid target attacked");
    mock::targetPlayer = true;
    nova::module::antibot::bot = true;
    Tick(); Check(mock::actions.empty() && nova::module::antikb::AttackQueue() == 0, "bot target attacked");
    nova::module::antibot::bot = false;
    module->bools.at("Swing Hand").value = true;
    Receive(first); clientTick(); Tick();
    Check(mock::actions == "AH", "Swing Hand must use local swing after attack");

    // Protocol-specific packet ordering for both visible and packet-only swings.
    module->numbers.at("Attack Counts").value = 1;
    for (bool legacy : {false, true}) {
        mock::legacy = legacy;
        for (bool visible : {false, true}) {
            module->bools.at("Swing Hand").value = visible;
            mock::actions.clear(); Receive(first); clientTick(); Tick();
            const std::string expected = legacy ? (visible ? "HA" : "SA") : (visible ? "AH" : "AS");
            Check(mock::actions == expected, "Reduce swing/attack order differs from selected protocol");
        }
    }
    mock::legacy = false;
    module->numbers.at("Attack Counts").value = 3;

    // Mode changes discard references and all outstanding movement/attack state.
    mock::ground = false; Receive(first);
    const auto beforeMode = mock::handled.size();
    module->modes.at("Mode").value = 0; clientTick();
    Check(mock::refs == 0 && mock::handled.size() == beforeMode &&
          nova::module::antikb::AttackQueue() == 0 && !nova::module::antikb::IsDelaying(),
          "switching away from Reduce must discard/reset state");
    Receive(first); Receive(ordinary);
    module->modes.at("Mode").value = 1; clientTick();
    Check(mock::refs == 0 && module->suffix() == "Reduce", "switching from Delay must clear its window");

    // Explosion passes immediately; correction queues, both flush next client tick
    // without creating new attacks, and update yaw even during the guard period.
    for (auto* lag : {&explosion, &position}) {
        module->setEnabled(false); module->setEnabled(true);
        mock::ground = false;
        Receive(first);
        Check(Receive(*lag) == (lag == &position), "Reduce lag packet whitelist mismatch");
        next.x = -100;
        Receive(next);
        Check(std::abs(nova::module::antikb::Yaw() + 90) < 0.01f, "guard-period velocity must still update yaw");
        clientTick();
        Check(!nova::module::antikb::IsDelaying() && nova::module::antikb::AttackQueue() == 0,
              "lag flush must not arm attack queue");
        mock::ground = true;
        Check(!input().jump, "lag guard must consume jump without forcing it");
    }
    module->setEnabled(false); module->setEnabled(true);
    mock::ground = false; mock::aura = true;
    module->numbers.at("Max Delay").value = 0;
    const int rotations = mock::rotations;
    Receive(first); clientTick();
    Check(!nova::module::antikb::IsDelaying() && nova::module::antikb::AttackQueue() == 3,
          "zero Max Delay must flush at next client tick even airborne");
    Check(mock::rotations == rotations && input().forward == 0.25f, "Aura target must suppress Reduce free aiming/forward");
    mock::aura = false; mock::scaffold = true;
    clientTick();
    Check(mock::rotations == rotations && input().forward == 0.25f, "Scaffold must suppress Reduce free aiming/forward");
    mock::scaffold = false;
    clientTick();
    Check(mock::rotations == rotations + 1 && input().forward == 1,
          "remaining sprint budget must recover when other aim owners leave");
    Receive(first); Receive(move);
    module->setEnabled(false);
    Check(mock::refs == 0 && nova::module::antikb::AttackQueue() == 0 &&
          mock::handled.back() == 11, "Reduce disable must flush and clear attack queue");
    module->setEnabled(true); Receive(first); Receive(disconnect);
    Check(mock::refs == 0 && !nova::module::antikb::IsDelaying(), "Reduce disconnect must drop queue");
    Receive(first); Receive(move);
    module->setEnabled(false);
    module->modes.at("Mode").value=3;mock::legacy=true;mock::ground=false;
    module->setEnabled(true);SettleWatchdog();mock::handled.clear();
    mock::Packet attack{30,mock::Attack},interaction{31,mock::Interact},other{32,mock::Motion,8};
    auto motion=[&]{event::MotionEvent e(event::EventType::Pre,0.f,0.f);event::EventBus::Get().Post(e);};
    Check(module->bools.at("Cancel On Ground").value && module->bools.at("Cancel On Attack").value &&
        module->bools.at("Cancel Explosions").value,"Watchdog defaults");
    Check(!Receive(ping) && !Receive(teleport),"no whitelist buffering before own S12");
    mock::handled.clear();first.x=0;first.y=0;first.z=0;
    Check(Receive(first),"zero self velocity must be held");
    Check(nova::module::antikb::OwnsIncomingDelayQueue() && !nova::module::antikb::IsDelaying(),
        "Watchdog must block Backtrack without activating Epsilon Aura rotation/attack behavior");
    Check(Receive(ping)&&!Receive(teleport),"hold transaction but pass entity teleport");
    Check(!Receive(other)&&!Receive(move)&&!Receive(explosion),"other S12, relative move, explosion pass");
    mock::handled.clear();
    Receive(interaction,false,event::EventType::Outgoing);motion();
    Check(mock::handled.empty(),"interaction and airborne motion do not release");
    int replayEvents=0;
    auto replayListener=event::EventBus::Get().Subscribe<event::PacketEvent>([&](auto& e) {
        if(e.type!=event::EventType::Incoming)return;
        ++replayEvents;
        Check(nova::module::antikb::OwnsIncomingDelayQueue(),"replay must retain ownership against Backtrack");
        Check(!e.IsCancelled(),"flushing must prevent recapturing replayed packets");
    },event::Priority::Lowest);
    Receive(attack,true,event::EventType::Outgoing);
    Check(mock::handled==std::vector<int>({first.serial,ping.serial})&&mock::refs==0,"attack FIFO replay");
    Check(replayEvents==2,"receive() must republish every held packet event, even on a cancelled attack");
    replayListener.Reset();
    Check(!nova::module::antikb::OwnsIncomingDelayQueue(),"release must clear flushing and ownership");
    mock::handled.clear();Receive(first);Receive(attack,false,event::EventType::Outgoing);StartTick();
    Check(mock::handled==std::vector<int>({first.serial})&&mock::refs==0,"normal attack replay");
    // Rise ignores the original event's cancelled bit. A fresh replay event can
    // still be cancelled by another receiver, just like PacketUtil.receive().
    mock::handled.clear();Receive(first,true);
    Check(mock::refs==1,"Rise must retain self S12 even if a previous receiver cancelled it");
    auto cancelReplay=event::EventBus::Get().Subscribe<event::PacketEvent>([](auto& e) {
        if(e.type==event::EventType::Incoming)e.Cancel();
    },event::Priority::Lowest);
    Receive(attack,false,event::EventType::Outgoing);StartTick();cancelReplay.Reset();
    Check(mock::handled.empty()&&mock::refs==0&&!nova::module::antikb::OwnsIncomingDelayQueue(),
        "replay cancellation must be respected and flushing must finish");
    mock::handled.clear();Receive(first);Receive(ping);Receive(teleport);
    int nestedAttacks=0;
    auto nestedListener=event::EventBus::Get().Subscribe<event::PacketEvent>([&](auto& e) {
        if(e.type!=event::EventType::Incoming)return;
        ++nestedAttacks;Receive(attack,false,event::EventType::Outgoing);
        Check(nova::module::antikb::OwnsIncomingDelayQueue(),"nested release must not clear active batch ownership");
    },event::Priority::Lowest);
    Receive(attack,false,event::EventType::Outgoing);StartTick();nestedListener.Reset();
    Check(nestedAttacks==2&&mock::handled==std::vector<int>({teleport.serial,first.serial,ping.serial})&&mock::refs==0,
        "reentrant attacks must not duplicate or reorder the replay batch");

    // S18 replaces the remote fixed-point server position; S14 adds to it.
    // Holding only S18 previously moved a remote player below ground during
    // capture and snapped it back to an obsolete position on release.
    int remoteY=60*32, remoteUpdates=0;
    mock::Packet remoteAbsolute{60,mock::Teleport,8,0,64*32,0};
    mock::Packet remoteDelta{61,mock::Move,8,0,-32,0};
    mock::replayObserver=[&](auto* p) {
        if(p==&remoteAbsolute){remoteY=p->y;++remoteUpdates;}
        if(p==&remoteDelta){remoteY+=p->y;++remoteUpdates;}
    };
    Receive(first);Receive(ping);
    Check(!Receive(remoteAbsolute)&&!Receive(remoteDelta),"remote position stream must pass during Watchdog capture");
    Check(remoteY==63*32&&remoteUpdates==2,"relative movement used an obsolete absolute baseline");
    Receive(attack,false,event::EventType::Outgoing);
    Check(remoteY==63*32&&remoteUpdates==2&&mock::refs==0,"release replayed stale remote coordinates");
    mock::replayObserver={};
    std::puts("PASS: Watchdog remote S18/S14 position baseline stays ordered during capture and release");
    mock::handled.clear();Receive(first);Receive(ping);
    auto throwingListener=event::EventBus::Get().Subscribe<event::PacketEvent>([](auto& e) {
        if(e.type==event::EventType::Incoming)throw 1;
    },event::Priority::Lowest);
    Receive(attack,false,event::EventType::Outgoing);StartTick();throwingListener.Reset();
    Check(mock::handled==std::vector<int>({first.serial,ping.serial})&&mock::refs==0&&
        !nova::module::antikb::OwnsIncomingDelayQueue(),"listener exception must not strand packets/references/flushing");
    Check(nova::module::antikb::AttackQueue()==0,"no synthetic reduce attacks");
    mock::handled.clear();first.y=-100;Receive(first);mock::ground=true;Tick();
    Check(mock::handled.empty(),"ground release request occurs at PRE motion only");motion();
    Check(mock::handled==std::vector<int>({first.serial}),"negative velocity and ground release");

    // Actual attack ordering: new velocity participates in this attack's vanilla
    // slowdown. A later S12 starts a new batch and is not released by ticking.
    double motionX=0;first.x=8000;first.y=1600;
    mock::replayObserver=[&](auto* p){if(p->kind==mock::Motion)motionX=p->x/8000.0;};
    Receive(first);Receive(attack,false,event::EventType::Outgoing);
    Check(motionX==1,"attack callback did not apply the held S12");
    motionX*=.6;Receive(next);StartTick();
    Check(motionX==.6&&mock::refs==1,"tick merged the new batch or overwrote the same-attack slowdown");
    Receive(attack,false,event::EventType::Outgoing);Check(mock::refs==0,"next attack did not release next batch");
    mock::replayObserver={};
    module->bools.at("Cancel On Ground").value=false;module->bools.at("Cancel On Attack").value=false;
    mock::handled.clear();Receive(first);Receive(ping);motion();Receive(attack,false,event::EventType::Outgoing);StartTick();
    Check(mock::handled.empty()&&mock::refs==2,"both toggles disabled retain packets");
    module->bools.at("Cancel Explosions").value=false;Check(!Receive(explosion),"unused explosion setting parity");
    mock::handled.clear();module->modes.at("Mode").value=0;StartTick();
    Check(mock::handled==std::vector<int>({first.serial,ping.serial})&&mock::refs==0,"mode switch disables and drains Watchdog");
    module->modes.at("Mode").value=3;SettleWatchdog();mock::handled.clear();Receive(first);Receive(respawn);
    Check(mock::handled==std::vector<int>({respawn.serial})&&mock::refs==0,"respawn drops obsolete queue");
    SettleWatchdog();
    mock::handled.clear();Receive(first);Receive(disconnect);
    Check(mock::handled==std::vector<int>({disconnect.serial})&&mock::refs==0,"disconnect drops queue");
    SettleWatchdog();
    mock::handled.clear();Receive(first);mock::failRef=true;
    Check(!Receive(ping),"allocation failure passes current packet");mock::failRef=false;
    Check(mock::handled==std::vector<int>({ping.serial})&&mock::refs==1,"allocation failure must retain preceding FIFO");
    module->bools.at("Cancel On Attack").value=true;Receive(attack,false,event::EventType::Outgoing);
    mock::handled.clear();Receive(first);module->setEnabled(false);
    Check(mock::handled==std::vector<int>({first.serial})&&mock::refs==0,"Watchdog disable replay");
    module->setEnabled(true); SettleWatchdog(); mock::ground = false;
    mock::actions.clear(); mock::target = mock::alive = true;
    Check(!module->bools.at("Auto Attack").value && module->numbers.at("Attack Attempts").value == 3 &&
        module->numbers.at("Attack Interval (ms)").value == 50, "optional attack defaults");
    Receive(first); Receive(attack,false,event::EventType::Outgoing); Tick();
    Check(mock::actions.empty(), "default-off must preserve passive Watchdog behavior");
    module->bools.at("Auto Attack").value = true;
    module->numbers.at("Attack Interval (ms)").value = 200;
    Receive(first); Tick(); Check(mock::actions.empty() && mock::refs == 1, "do not attack/release while holding KB");
    Receive(attack,false,event::EventType::Outgoing); Tick();
    Check(mock::actions == "HA" && module::antikb::AttackQueue() == 0, "release arms first attempt without blocking AutoBlock");
    Tick(); Tick(); Check(mock::actions == "HA", "configured interval prevents tick-rate attacks");
    auto waitAttack = [] { std::this_thread::sleep_for(std::chrono::milliseconds(220)); Tick(); };
    waitAttack(); Check(mock::actions == "HAHA", "second attack after interval");
    waitAttack(); waitAttack(); Check(mock::actions == "HAHAHA", "finite 3-attempt burst, no autonomous rearming");
    module->numbers.at("Attack Attempts").value = 1;
    mock::actions.clear(); mock::target = false;
    Receive(first); Receive(attack,false,event::EventType::Outgoing); Tick();
    mock::target = true; waitAttack(); Check(mock::actions.empty(), "a miss consumes an attempt");
    for (int filter = 0; filter < 3; ++filter) {
        mock::teammate = filter == 0; module::antibot::bot = filter == 1; mock::alive = filter != 2;
        Receive(first); Receive(attack,false,event::EventType::Outgoing); Tick();
        Check(mock::actions.empty(), "team/bot/dead target must not be attacked");
    }
    mock::teammate = module::antibot::bot = false; mock::alive = true;
    mock::blockManaged = true; mock::queuedAttacks = 0;
    Receive(first); Receive(attack,false,event::EventType::Outgoing); Tick();
    Check(mock::actions.empty() && mock::queuedAttacks == 1 && module::antikb::AttackQueue() == 0,
        "AutoBlock receives exactly one request without a circular queue wait");
    mock::blockManaged = false;
    Receive(first); Receive(attack,false,event::EventType::Outgoing);
    module->bools.at("Auto Attack").value = false; Tick();
    module->bools.at("Auto Attack").value = true; Tick(); Check(mock::actions.empty(), "toggle-off discards pending attempts");
    for (int interruption = 0; interruption < 4; ++interruption) {
        Receive(first); Receive(attack,false,event::EventType::Outgoing);
        mock::screen = interruption == 0; mock::menu = interruption == 1;
        mock::scaffold = interruption == 2; mock::alive = interruption != 3;
        Tick(); mock::screen = mock::menu = mock::scaffold = false; mock::alive = true; Tick();
        Check(mock::actions.empty(), "UI/scaffold/death discards attack burst");
    }
    Receive(first); Receive(attack,false,event::EventType::Outgoing); Receive(position); Tick();
    SettleWatchdog(); Tick(); Check(mock::actions.empty(), "S08 must not retain/rearm a burst");
    Receive(first); Receive(attack,false,event::EventType::Outgoing); ++mock::worldIdentity; Tick();
    SettleWatchdog(); Tick(); Check(mock::actions.empty(), "world change discards burst");
    // A disable dispatched by the replay itself must invalidate arming after Finish().
    mock::replayObserver = [&](auto* p) { if (p == &first) module->setEnabled(false); };
    Receive(first); Receive(attack,false,event::EventType::Outgoing); mock::replayObserver = {};
    module->setEnabled(true); SettleWatchdog(); Tick(); Check(mock::actions.empty(), "reentrant disable must invalidate released batch");
    // A newly arriving KB during our final attack must not start an infinite chain.
    Receive(first); Receive(attack,false,event::EventType::Outgoing);
    mock::onAttack = [&] { Receive(first); Receive(attack,false,event::EventType::Outgoing); };
    Tick(); mock::onAttack = {}; waitAttack(); Check(mock::actions == "HA", "own final attack must not refill burst");
    mock::actions.clear();
    Receive(first); Receive(attack,false,event::EventType::Outgoing); module->modes.at("Mode").value = 0; StartTick(); Tick();
    module->modes.at("Mode").value = 3; SettleWatchdog(); Tick(); Check(mock::actions.empty(), "mode switch clears burst");
    Receive(first); Receive(attack,false,event::EventType::Outgoing); module->setEnabled(false);
    module->setEnabled(true); SettleWatchdog(); Tick(); Check(mock::actions.empty(), "disable clears burst");
    module->setEnabled(false);
    std::puts("PASS: optional Watchdog Auto Attack cadence, finite attempts, AutoBlock routing, filters, passive defaults and recovery cleanup");
    Check(module->bools.at("Rotation").value && module->numbers.at("Rotation Ticks").value == 3,
          "Watchdog Rotation must default to Velocity's enabled / 3 tick window");
    module->modes.at("Mode").value = 3;
    module->bools.at("Auto Attack").value = false;
    mock::legacy = true; mock::aura = mock::scaffold = false; mock::alive = true;
    module->setEnabled(true); SettleWatchdog();
    mock::Packet turn{90, mock::Motion, 7, 100, 100, 0};
    auto releaseTurn = [&] {
        Check(Receive(turn), "rotation KB must follow ordinary Watchdog capture");
        Receive(attack, false, event::EventType::Outgoing);
        Check(!module::antikb::WatchdogHoldingPackets(), "rotation must not change attack-triggered release");
    };
    mock::rotations = 0; rotation::applied.pitch = 27;
    Check(Receive(turn), "rotation KB capture failed"); StartTick();
    Check(mock::rotations == 0, "rotation must not start while KB is still held");
    Receive(attack, false, event::EventType::Outgoing);
    for (int i = 0; i < 5; ++i) StartTick();
    Check(mock::rotations == 3 && std::abs(rotation::applied.yaw - 90) < .01f && rotation::applied.pitch == 27,
          "rotation must use Velocity yaw, preserve pitch, last exactly 3 ticks independently of Auto Attack");
    Check(input().forward == .25f && !input().jump, "Rotation must not add Velocity's forced movement/jump");
    module->numbers.at("Rotation Ticks").value = 1;
    for (const auto [x,z,expected] : {std::tuple{0,100,-180.f}, std::tuple{-100,0,-90.f}, std::tuple{0,-100,0.f}}) {
        turn.x = x; turn.z = z; releaseTurn(); StartTick();
        Check(std::abs(rotation::applied.yaw - expected) < .01f, "Watchdog yaw quadrant differs from Velocity");
    }
    int turns = mock::rotations;
    turn.x = turn.z = 0; releaseTurn(); StartTick();
    Check(mock::rotations == turns, "vertical/zero KB must not invent a heading");
    turn.x = 100; Receive(turn); turn.x = 0; Receive(turn); Receive(attack,false,event::EventType::Outgoing); StartTick();
    Check(mock::rotations == turns, "last queued zero horizontal velocity must supersede prior heading");
    turn.x = 100; module->numbers.at("Rotation Ticks").value = 3;
    for (int gate = 0; gate < 2; ++gate) {
        releaseTurn(); mock::aura = gate == 0; mock::scaffold = gate == 1;
        for (int i = 0; i < 4; ++i) StartTick();
        mock::aura = mock::scaffold = false; StartTick();
        Check(mock::rotations == turns, "Aura/Scaffold must suppress rotation and consume the window");
    }
    for (int gate = 0; gate < 3; ++gate) {
        releaseTurn(); mock::screen = gate == 0; mock::menu = gate == 1; mock::alive = gate != 2;
        StartTick(); mock::screen = mock::menu = false; mock::alive = true; StartTick();
        Check(mock::rotations == turns, "GUI/death must cancel pending turns");
    }
    releaseTurn(); module->bools.at("Rotation").value = false; StartTick();
    module->bools.at("Rotation").value = true; StartTick();
    Check(mock::rotations == turns, "toggle off must discard the remaining rotation window");
    module->numbers.at("Rotation Ticks").value = 0; releaseTurn(); StartTick();
    Check(mock::rotations == turns, "zero Rotation Ticks must disable turns");
    module->numbers.at("Rotation Ticks").value = 3;
    releaseTurn(); Receive(position); SettleWatchdog(); StartTick();
    Check(mock::rotations == turns, "S08 recovery must discard pending turns");
    releaseTurn(); ++mock::worldIdentity; SettleWatchdog(); StartTick();
    Check(mock::rotations == turns, "world replacement must discard pending turns");
    releaseTurn(); module->setEnabled(false); module->setEnabled(true); SettleWatchdog(); StartTick();
    Check(mock::rotations == turns, "disable must discard pending turns");
    releaseTurn(); module->modes.at("Mode").value = 0; StartTick();
    module->modes.at("Mode").value = 3; SettleWatchdog(); StartTick();
    Check(mock::rotations == turns, "mode handoff must discard pending turns");
    module->setEnabled(false);
    std::puts("PASS: Watchdog Velocity rotation, direction/pitch, independent finite window, gates and lifecycle");
    module->modes.at("Mode").value = 4; module->setEnabled(true);
    Check(module::mix::enabled && module->suffix() == "Mix", "Mix mode must activate its own runtime");
    Check(!Receive(first) && !module::antikb::OwnsIncomingDelayQueue(), "Mix must not enter legacy Delay/Reduce packet capture");
    Tick(); StartTick();
    module->modes.at("Mode").value = 0; StartTick();
    Check(!module::mix::enabled, "mode switch must disable Mix runtime");
    module->modes.at("Mode").value = 4; StartTick();
    Check(module::mix::enabled, "switching back must reenable Mix runtime");
    module->setEnabled(false);
    Check(!module::mix::enabled, "module disable must disable Mix runtime");
    module.reset();
    Check(mock::refs == 0 && mock::wanted == 0, "destructor must drop references and incoming interest");
    std::puts("AntiKB Epsilon Delay/Reduce: packet ordering, modes, guards, attack/swing, movement, lifecycle passed");
    std::puts("PASS: Watchdog Reduce packet whitelist, zero/negative self motion, attack/ground triggers, inert explosion switch, mode handoff and lifecycle");
    std::puts("PASS: Rise synchronous attack/ground/disable, same-attack slowdown and immediate batch boundary; configured lifecycle protection retained");
}
