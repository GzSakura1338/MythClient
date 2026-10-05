// Compiled with actual AntiKB, EventBus, native output queues and AB policy.
// Game/JNI transport is mocked; no injection or Java source is used.
int main() {
    auto module=nova::module::MakeAntiKbModule();
    nova::event::EventBus::Get().Seal();
    mock::legacy=true;mock::ground=false;
    module->modes.at("Mode").value=3;
    blink::watchdogBlocksBlink=&nova::module::antikb::BlocksAutoBlockBlink;
    blink::watchdogHoldingPackets=&nova::module::antikb::WatchdogHoldingPackets;
    blink::watchdogReplayingPackets=&nova::module::antikb::WatchdogReplayingPackets;
    mock::endSlinky=[](bool send){blink::EndSlinkyBlockLag(send);};
    mock::endHypixel=[](bool send){blink::EndHypixelBlockLag(send);};
    mock::slinkyLagging=[] {return blink::SlinkyBlockLagging();};
    mock::hypixelLagging=[] {return blink::HypixelBlockLagging();};
    mock::syncSlinky=[](bool discard){nova::module::autoblock::SynchronizePacketDelays(discard);};
    blink::Object manager,otherManager;
    blink::Object move{blink::Move,10},swing{blink::Ordinary,20},hit{blink::Interact,3};
    blink::Object ack1{blink::Transaction,31},ack2{blink::Transaction,32},chat{blink::Chat,40},ka{blink::KeepAlive,41};
    mock::Packet v1{1,mock::Motion,7,8000,1600,0},v2{2,mock::Motion,7,16000,1600,0};
    mock::Packet tx1{3,mock::Ping},tx2{4,mock::Ping},attack{5,mock::Attack},correction{6,mock::Position};
    double clientVelocity=0,serverVelocity=0;
    bool swingSeen=false,haveMovement=false,actionAfterMovement=false;
    int noSwing=0,post=0,velocityMismatch=0;
    auto send=[&](blink::Object& p) {
        if(blink::StageWatchdogPacket(&blink::environment,&manager,&p,nullptr,false))return;
        if(blink::HoldBlockPacket(&blink::environment,&manager,&p,nullptr,false))return;
        blink::environment.CallNonvirtualVoidMethod(&manager,0,0,&p,nullptr);
        if(p.kind==blink::Move)blink::watchdogBeforeMovement=false;
    };
    auto preMotion=[&] {
        blink::watchdogBeforeMovement=true;
        nova::event::MotionEvent e(nova::event::EventType::Pre,0.f,0.f);
        (void)nova::event::EventBus::Get().Post(e);
    };
    auto tickHead=[&] {
        blink::watchdogBeforeMovement=false;
        blink::MaintainWatchdogPhase(&blink::environment,false);
        StartTick();
    };
    auto observe=[&](blink::jobject p) {
        // Relevant 1.8 PacketOrderB/Post branches and a transaction-delimited
        // velocity model. Each movement carries the velocity used by its physics.
        if(p->serial==20){swingSeen=true;if(haveMovement)actionAfterMovement=true;}
        else if(p->serial==3){
            if(!swingSeen)++noSwing;
            swingSeen=false;serverVelocity*=.6;
            if(haveMovement)actionAfterMovement=true;
        } else if(p->kind==blink::Transaction) {
            if(haveMovement&&actionAfterMovement)++post;
            haveMovement=actionAfterMovement=swingSeen=false;
            serverVelocity=p->serial==31?1:2;
        } else if(p->kind==blink::Move) {
            if(std::abs(p->x-serverVelocity)>1e-9)++velocityMismatch;
            haveMovement=true;actionAfterMovement=swingSeen=false;
        }
    };
    blink::sendObserver=observe;
    mock::replayObserver=[&](auto* p){
        if(p->kind==mock::Motion)clientVelocity=p->x/8000.0;
        if(p==&tx1)send(ack1);
        if(p==&tx2)send(ack2);
    };
    module->setEnabled(true);SettleWatchdog();
    auto reset=[&] {
        blink::MaintainWatchdogPhase(&blink::environment,true);
        blink::EndSlinkyBlockLag(false);blink::EndHypixelBlockLag(false);
        blink::watchdogBeforeMovement=false;
        mock::syncSlinky(false);mock::ground=false;
        clientVelocity=serverVelocity=0;noSwing=post=velocityMismatch=0;
        swingSeen=haveMovement=actionAfterMovement=false;
        blink::sent.clear();move.x=0;send(move);
    };
    auto startOwner=[&](int mode) {
        if(mode==1)Check(blink::BeginSlinkyBlockLag(100,3,true),"Slinky start failed");
        if(mode==2)blink::BeginHypixelBlockLag();
        mock::syncSlinky(false);
    };
    // First reproduce the original bug independently of the repaired adapter.
    reset();
    observe(&swing);observe(&ack1);observe(&hit);move.x=.6;observe(&move);
    Check(noSwing==1&&post==1,"packet-order model failed to detect the old C0A/C0F/C02 order");
    reset();move.x=1.2;observe(&move);
    Check(velocityMismatch==1,"velocity model did not detect movement before acknowledgement");

    for(int mode=0;mode<3;++mode) {
        // Release BEFORE physics: confirmations precede the entire action group.
        reset();startOwner(mode);module->bools.at("Cancel On Attack").value=true;
        Receive(v1);Receive(tx1);Receive(v2);Receive(tx2);
        Check(!blink::blockLagActive,"AutoBlock failed to yield at input capture");
        send(swing);Receive(attack,false,nova::event::EventType::Outgoing);send(hit);
        Check(clientVelocity==2&&mock::refs==0,"Rise attack replay ceased to be synchronous");
        clientVelocity*=.6;move.x=clientVelocity;
        Check(blink::sent==std::vector<int>({10,31,32}),"confirmations did not precede staged actions");
        // Releasing inbound ownership must not reopen AB lag before C03.
        Check(!blink::BeginSlinkyBlockLag(100,3,true),"Slinky rearmed inside transaction phase");
        blink::BeginHypixelBlockLag();Check(!blink::HypixelBlockLagging(),"Hypixel rearmed inside transaction phase");
        blink::BeginBlockLag();Check(!blink::blockLagActive,"Lag took an incomplete action group");
        Check(!blink::BeginPredictionBlink(),"Prediction took an incomplete action group");
        send(move);
        Check(blink::sent==std::vector<int>({10,31,32,20,3,10})&&noSwing==0&&post==0&&velocityMismatch==0,
              "pre-physics release violated packet order or velocity transaction window");
        Check(!blink::watchdogPhasePending&&blink::refs==0,"pre-physics phase leaked references");

        // Release AFTER physics: this C03 still represents OLD velocity.
        reset();startOwner(mode);module->bools.at("Cancel On Attack").value=false;
        Receive(v1);Receive(tx1);send(swing);Receive(attack,false,nova::event::EventType::Outgoing);send(hit);
        move.x=clientVelocity;mock::ground=true;preMotion();
        Check(clientVelocity==1&&blink::sent==std::vector<int>({10}),"ground replay split state/confirmations before old movement");
        send(move);tickHead();move.x=clientVelocity;send(move);
        Check(blink::sent==std::vector<int>({10,20,3,10,31,10})&&noSwing==0&&post==0&&velocityMismatch==0,
              "post-physics confirmation was attached to the wrong movement");
        Check(!blink::watchdogPhasePending&&blink::refs==0,"post-physics phase leaked references");
    }
    std::puts("PASS: Watchdog alone, Slinky and Hypixel; pre/post-physics release, multiple S12, PacketOrderB/Post and velocity windows");

    // Both release stages in one tick: velocity A drives this movement, B next.
    reset();module->bools.at("Cancel On Attack").value=true;
    Receive(v1);Receive(tx1);send(swing);Receive(attack,false,nova::event::EventType::Outgoing);send(hit);
    clientVelocity*=.6;move.x=clientVelocity;
    Receive(v2);Receive(tx2);mock::ground=true;preMotion();send(move);
    tickHead();move.x=clientVelocity;send(move);
    Check(noSwing==0&&post==0&&velocityMismatch==0&&clientVelocity==2,"mixed release stages conflated velocity epochs");

    // If walking emits no C03, tick HEAD acknowledges before the NEXT physics;
    // actions remain grouped until a real movement rather than fabricating one.
    reset();module->bools.at("Cancel On Attack").value=false;
    Receive(v1);Receive(tx1);send(swing);mock::ground=true;preMotion();tickHead();
    Check(blink::sent==std::vector<int>({10,31})&&blink::watchdogPhasePending,"missing movement split action/confirmation order");
    move.x=clientVelocity;send(move);
    Check(noSwing==0&&post==0&&velocityMismatch==0&&blink::refs==0,"missing movement fallback used the wrong velocity");

    // Ordinary traffic stays untouched; cancelled attacks do not create C02.
    reset();module->bools.at("Cancel On Attack").value=true;
    Receive(v1);Receive(tx1);send(swing);send(chat);send(ka);
    Receive(attack,true,nova::event::EventType::Outgoing);move.x=clientVelocity;send(move);
    Check(blink::sent==std::vector<int>({10,40,41,31,20,10})&&noSwing==0&&post==0&&velocityMismatch==0,
          "cancelled attack or async traffic was replayed incorrectly");

    // S08 preserves valid confirmations but drops obsolete staged actions.
    reset();Receive(v1);Receive(tx1);send(swing);mock::ground=true;preMotion();
    blink::watchdogBeforeMovement=false;
    blink::MaintainWatchdogPhase(&blink::environment,false,true);
    Receive(correction);tickHead();SettleWatchdog();
    Check(blink::sent==std::vector<int>({10,31})&&blink::refs==0&&!blink::watchdogPhasePending,
          "correction sent obsolete actions or lost confirmations");

    // New connection/world, shutdown, and mode switch retain no dangling refs.
    reset();Receive(v1);send(swing);
    Check(!blink::StageWatchdogPacket(&blink::environment,&otherManager,&move,nullptr,false)&&blink::refs==0,
          "old manager phase crossed reconnect");
    Receive(attack,false,nova::event::EventType::Outgoing);
    reset();Receive(v1);send(swing);blink::phaseLevel=&otherManager;
    blink::MaintainWatchdogPhase(&blink::environment,false);
    blink::phaseLevel=&blink::phaseWorld;
    Check(blink::refs==0&&!blink::watchdogPhasePending,"old world phase survived identity replacement");
    Receive(attack,false,nova::event::EventType::Outgoing);

    // A JNI allocation failure must release existing callback and identity refs.
    reset();Receive(v1);
    Check(blink::StageWatchdogPacket(&blink::environment,&manager,&swing,&chat,false),"callback packet was not staged");
    blink::failRef=true;send(hit);blink::failRef=false;
    Check(blink::sent==std::vector<int>({10,20,3})&&blink::refs==0&&!blink::watchdogPhasePending,
          "JNI failure stranded callbacks or changed queued action FIFO");
    Receive(attack,false,nova::event::EventType::Outgoing);
    reset();Receive(v1);send(swing);module->setEnabled(false);
    move.x=clientVelocity;send(move);
    Check(blink::refs==0&&!blink::watchdogPhasePending,"disable stranded the final action group");
    module.reset();blink::MaintainWatchdogPhase(&blink::environment,true);
    mock::syncSlinky(true);blink::sendObserver={};mock::replayObserver={};
    Check(mock::refs==0&&blink::refs==0&&mock::wanted==0,"phase/module lifetime leaked JNI ownership");
    std::puts("PASS: mixed stages, skipped movement, cancellation, async traffic, S08, reconnect, world identity and disable cleanup");
}
