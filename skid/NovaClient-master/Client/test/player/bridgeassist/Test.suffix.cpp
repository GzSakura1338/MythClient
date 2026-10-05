
bool Input(float forward=-1,bool jump=false,bool advance=true) {
    if(advance) ++mock::tick;
    mock::forward=forward;
    nova::event::MoveInputEvent e{forward,0,jump,mock::key};
    nova::event::EventBus::Get().input(e); return e.sneak;
}
void Packet(bool block=true) {
    nova::event::PacketEvent e{nova::event::EventType::Outgoing,reinterpret_cast<void*>(std::uintptr_t(block?1:2))};
    nova::event::EventBus::Get().packet(e);
}
void Tick() { nova::event::GameTickEvent e; nova::event::EventBus::Get().tick(e); }
void PrePlace() { nova::event::ClientRotationEvent e; nova::event::EventBus::Get().rotation(e); }
void Interact() { nova::event::PrePlayerInteractEvent e; nova::event::EventBus::Get().interact(e); }
int main() {
    using nova::module::bridgeassist::SneakState;
    auto m=nova::module::MakeBridgeAssistModule(); m->setEnabled(true);
    assert(m->rows.size()==4 && m->rows[2].getSubProps().size()==3 && m->rows[3].getSubProps().size()==4);
    assert(m->rows[0].getSubProps().size()==1);
    auto* fov=static_cast<Properties::NumberProp*>(m->Find("FOV"));
    assert(fov->get()==360 && fov->formatValue(45)=="45 deg");
    assert(!Input()); mock::offset=.01; assert(Input()); // Raven default offset is zero.
    mock::offset=0; assert(!Input()); // Default 50ms releases at first safe tick.
    mock::offset=.1; m->Number("Edge offset",.2); assert(!Input());
    mock::offset=.21; assert(Input());
    m->Number("Unsneak delay",150); mock::offset=0;
    assert(Input()); assert(Input(-1,false,false)); assert(Input()); assert(!Input());
    m->Number("Unsneak delay",50);
    assert(SneakState::DelayTicks(75,.49)==2 && SneakState::DelayTicks(75,.51)==1);
    mock::offset=std::numeric_limits<double>::quiet_NaN();
    assert(Input()); assert(!Input(-1,true)); // No support + jump, with jump-sneak disabled.
    m->Number("Sneak on jump",100);
    assert(Input(-1,true)); mock::ground=false;
    assert(Input(-1,true)); assert(!Input(-1,true));
    mock::ground=true; m->Number("Sneak on jump",0);
    m->Bool("Looking down",true); mock::pitch=69; assert(!Input()); mock::pitch=70; assert(Input());
    m->Bool("Looking down",false); m->Bool("Not moving forward",true); assert(!Input(1));
    m->Bool("Not moving forward",false); assert(Input(1));
    m->Bool("Holding blocks",true); mock::blocks=false; assert(!Input());
    m->Bool("Holding blocks",false); assert(Input()); mock::blocks=true;
    // Placement counts only while the module is sneaking with the required key.
    mock::Reset(); m->Bool("Sneak key pressed",true); mock::manual=mock::key=true;
    Packet(); mock::offset=.3; assert(Input()); mock::offset=0; assert(Input()); assert(mock::key);
    mock::offset=.3; assert(Input()); Packet(false); mock::offset=0; assert(Input()); assert(mock::key);
    mock::offset=.3; assert(Input()); Packet(); mock::offset=0; assert(!Input()); assert(!mock::key);
    assert(!Input()); // Keep the forced key release while moving over safe support.
    assert(Input(0)); assert(mock::key); // Stopping represses the physical key.
    mock::offset=.3; assert(Input()); Packet(); mock::offset=0; assert(!Input());
    m->setEnabled(false); Tick(); assert(mock::key); // Disable restores physical binding even without input events.
    m->setEnabled(true); mock::offset=.3; assert(Input()); Packet(); mock::offset=0; assert(!Input());
    mock::gui=true; Tick(); assert(mock::key); mock::gui=false;
    m->Bool("Sneak key pressed",false); mock::manual=mock::key=false;
    mock::prediction=false; assert(!Input()); mock::prediction=true;
    mock::flying=true; assert(!Input()); mock::flying=false;
    mock::offset=.3; assert(Input()); ++mock::identity; mock::offset=0; assert(!Input());
    mock::offset=.3; assert(Input()); mock::tick=0; mock::offset=0; assert(!Input());
    m->Bool("Pre place",true); mock::ray=true; PrePlace(); assert(mock::rotation);
    mock::ray=false; PrePlace(); assert(!mock::rotation);
    mock::ray=true; PrePlace(); assert(mock::rotation); m->Bool("Pre place",false); PrePlace(); assert(!mock::rotation);
    // FOV is the full cone around the camera, independent of silent/server pitch.
    m->Bool("Pre place",true); m->Number("FOV",30); mock::pitch=20; mock::rayCalls=0;
    PrePlace(); assert(!mock::rotation && mock::rayCalls==0); // Server pitch remains 80.
    mock::pitch=80; PrePlace(); assert(mock::rotation && mock::rayCalls>0);
    mock::pitch=20; PrePlace(); assert(!mock::rotation); // Leaving the cone releases immediately.
    m->Number("FOV",120); PrePlace(); assert(mock::rotation && mock::outPitch<=80.1f);
    mock::pitch=-90; m->Number("FOV",360); PrePlace(); assert(mock::rotation); // Default retains Raven's search.
    m->Number("FOV",0); PrePlace(); assert(!mock::rotation);
    m->Number("FOV",360); mock::pitch=80;
    // Lunar rotates before mouse-over; the later use hook must not advance it a second time.
    m->Bool("Pre place",true); mock::rotationWrites=0; PrePlace(); assert(mock::rotationWrites==1);
    Interact(); assert(mock::rotationWrites==1);
    // Preserve the interaction fallback on profiles without ClientRotationEvent.
    mock::lunar=false; Interact(); assert(mock::rotationWrites==2); mock::lunar=true;
    // Raven only aims at an exposed horizontal face under the player's box.
    mock::raySide=1; PrePlace(); assert(!mock::rotation);
    mock::raySide=0; PrePlace(); assert(!mock::rotation);
    mock::raySide=2; mock::rayX=3; PrePlace(); assert(!mock::rotation); mock::rayX=0;
    for(int side : {2,3,5,4}) { mock::raySide=side; PrePlace(); assert(mock::rotation); }
    mock::support=false; mock::rayCalls=0; PrePlace(); assert(!mock::rotation && mock::rayCalls==0);
    mock::support=true; mock::mining=true; PrePlace(); assert(!mock::rotation && mock::rayCalls==0);
    mock::mining=false; mock::flying=true; PrePlace(); assert(!mock::rotation); mock::flying=false;
    mock::blocks=false; PrePlace(); assert(!mock::rotation); mock::blocks=true;
    mock::screen=true; PrePlace(); assert(!mock::rotation); mock::screen=false;
    m->Bool("Looking down",true); mock::pitch=69; PrePlace(); assert(!mock::rotation);
    mock::pitch=70; PrePlace(); assert(mock::rotation); m->Bool("Looking down",false);
    m->Bool("Not moving forward",true); mock::forward=1; PrePlace(); assert(!mock::rotation);
    mock::forward=-1; m->Bool("Not moving forward",false);
    // Last motion pitch, not the camera/last-reported fallback, selects the target.
    auto& rs=nova::rotation::State(); rs.hasLastRotations=rs.hasAnimation=true;
    rs.lastRotations=rs.animationRotation={0,85}; mock::reportedPitch=5; mock::pitch=20;
    PrePlace(); assert(mock::rotation && std::abs(mock::outPitch-85)<.8f);
    // Every output delta lies on Raven's mouse-sensitivity grid (0.15 at sensitivity .5).
    assert(std::abs((mock::outPitch-85)/.15-std::round((mock::outPitch-85)/.15))<.0001);
    // A large yaw gap advances by the speed-15 randomized, proximity-scaled step.
    rs.animationRotation={0,80}; mock::yaw=180;
    PrePlace(); assert(mock::outYaw>=-48 && mock::outYaw<=-41);
    assert(std::abs(mock::outYaw/.15-std::round(mock::outYaw/.15))<.0001);
    mock::sensitivityOk=false; PrePlace(); assert(!mock::rotation); mock::sensitivityOk=true;
    // A higher-priority direct rotation (e.g. AntiKB) must survive our early-return cleanup.
    mock::yaw=0; PrePlace(); assert(mock::rotation);
    rs.priority=nova::rotation::RotationPriority::Highest; rs.rotations={45,30};
    mock::mining=true; const int releases=mock::releases; PrePlace();
    assert(mock::releases==releases && rs.active && rs.rotations.yaw==45);
    rs={}; mock::mining=false; m->Bool("Pre place",false);
    // Switch uses the Scaffold filter before the Holding blocks condition.
    mock::Reset(); mock::blockSlot=2; mock::blocks=false; mock::offset=.3;
    m->Bool("Holding blocks",true);
    assert(!Input()); assert(mock::selected==0 && mock::slotWrites==0); // Default off.
    assert(m->rows[1].getSubProps().size()==1);
    assert(static_cast<Properties::BoolProperty*>(m->Find("Hold Block"))->get());
    m->Bool("Switch",true); assert(!Input()); assert(mock::selected==0); // Requires a held block by default.
    mock::blocks=true; assert(Input()); assert(mock::selected==2);
    mock::selected=7; mock::blocks=false; assert(!Input()); assert(mock::selected==7);
    m->Bool("Switch",false); Tick(); assert(mock::selected==7); // No stale restoration after manual non-block selection.
    mock::selected=0; m->Bool("Switch",true); m->Bool("Hold Block",false);
    assert(Input()); assert(mock::selected==2); // Opting out restores unrestricted switching.
    const int writes=mock::slotWrites; Input(); assert(mock::slotWrites==writes);
    mock::blockSlot=4; Input(); assert(mock::selected==4); // Replenish from another stack.
    m->setEnabled(false); Tick(); assert(mock::selected==0);
    m->setEnabled(true); Input(); assert(mock::selected==4);
    mock::selected=7; Input(); assert(mock::selected==4);
    m->Bool("Switch",false); Tick(); assert(mock::selected==7); // Respect manual selection.
    m->Bool("Switch",true); Input(); mock::selected=6;
    m->setEnabled(false); Tick(); assert(mock::selected==6); // Manual change before disable.
    m->setEnabled(true); Input(); mock::blockCount=1; Input(); assert(mock::selected==6);
    mock::blockCount=64; Input(); mock::screen=true; Tick(); assert(mock::selected==6);
    mock::screen=false; Input(); mock::gui=true; Tick(); assert(mock::selected==6);
    mock::gui=false; Input(); ++mock::identity; mock::selected=4; mock::blockSlot=-1;
    Input(); assert(mock::selected==4); // No restoration into a different player.
    mock::blockSlot=2; mock::slotWriteOk=false; Input(); assert(mock::selected==4);
    m->setEnabled(false); mock::slotWriteOk=true; Tick(); assert(mock::selected==4);
    std::puts("BridgeAssist: settings, edge decisions, timers, packets, key recovery, pre-place and Switch lifecycle passed");
}
