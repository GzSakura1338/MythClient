using namespace nova;
using namespace nova::module;
namespace lunar=compat::lunar;

// Exercise the real manager, smoother and movement correction in runTick order.
void Movement() {
    rotation::UpdateGlobalYaw();
    float yaw=java::yaw,pitch=java::pitch;
    rotation::OnMotionPre(yaw,pitch);
    java::serverYaw=yaw;java::serverPitch=pitch;
    ++lunar::movement;lunar::slotReady=true;
    ++java::gameTick;
}
void Tick(ThrowableAuraModule& m,bool move=true) {
    m.OnPreUpdate();m.OnInteract();if(move)Movement();
}
void Reset(ThrowableAuraModule& m) {
    m.enabled=false;m.onDisable();Movement();m.OnPreUpdate();
    rotation::State()={};
    m.enabled=true;m.onEnable();
}
void ThrowOnce(ThrowableAuraModule& m) {
    const int before=lunar::uses;
    for(int i=0;i<30;++i) {
        m.OnPreUpdate();m.OnInteract();
        if(lunar::uses!=before)return;
        Movement();
    }
    assert(false && "expected a naturally synchronized throw");
}
int main(){
    blocker.enabled=false;rod.enabled=true;
    java::world={{&java::self},{&java::enemy}};
    java::hotbar[7]=java::Snowball;java::hotbar[1]=java::Egg;java::yaw=60;
    ThrowableAuraModule m;m.onEnable();
    assert(m.ns.size()==6 && m.bs.size()==11);
    java::enemy.team=true;
    assert(m.GetTarget(java::PlayerEntity()));
    teams::enabled=true;
    assert(!m.GetTarget(java::PlayerEntity()));
    java::enemy.team=false;
    assert(m.GetTarget(java::PlayerEntity()));
    teams::enabled=false;
    assert(m.rotationRange_->formatter(4.20000000001)=="4.2");
    assert(m.rotationSpeed_->formatter(10.00000001)=="10");
    m.rotationSpeed_->value=10;m.throwDelay_->value=0;
    Tick(m);
    assert(lunar::uses==0 && java::throwOrder.empty() && java::selected==0);
    assert(!rotation::State().direct && rotation::MoveInputFix());
    assert(java::serverYaw>45 && java::serverYaw<60 && java::yaw==60);
    rotation::MoveInput input{1,0};rotation::OnMoveInput(input);
    float strafeYaw=java::yaw;rotation::OnStrafe(strafeYaw);
    assert(strafeYaw==rotation::GetRotation().yaw);
    float previous=java::serverYaw;
    for(int i=0;i<3;++i) {
        Tick(m);assert(lunar::uses==0);
        assert(std::abs(java::serverYaw-previous)<12);previous=java::serverYaw;
    }
    ThrowOnce(m);
    assert(java::throwOrder=="U" && java::selected==7 && m.target_);
    assert(!m.RestoreSlot());m.OnInteract();assert(lunar::uses==1);
    m.throwDelay_->value=20;Movement();
    for(int i=0;i<10;++i) {
        Tick(m);assert(lunar::uses==1 && m.target_ && rotation::IsActive());
        assert(std::abs(java::serverYaw)<4 && java::selected==0); // launch offset is compensated
    }
    // Moving targets are followed during the cooldown without resetting to camera.
    java::enemy.pos.x=1;Tick(m);
    assert(java::serverYaw< -5 && m.target_ && lunar::uses==1);
    java::enemy.pos.x=0;

    // Full FOV must acquire targets behind the physical camera.
    Reset(m);m.fov_->value=360;m.throwDelay_->value=0;m.rotationSpeed_->value=60;
    java::yaw=0;java::enemy.pos.z=-4;
    // KillAura is enabled but has dropped its target; the shared manager is
    // still active, returning its last angle toward the physical camera.
    killaura::active=true;killaura::id=0;
    rotation::UpdateGlobalYaw();rotation::SetRotations({30,10},60);Movement();
    assert(rotation::IsActive() && !rotation::IsSmoothed());
    Tick(m);assert(m.target_ && std::abs(java::serverYaw)>40);
    ThrowOnce(m);assert(std::abs(std::remainder(java::serverYaw-180.f,360.f))<4);
    Movement();java::enemy.pos.z=4;killaura::active=false;
    // Warm up aim, then change eye height between natural updates as in a jump.
    Reset(m);java::yaw=60;Tick(m);Tick(m);Movement();
    const int airborne=lunar::uses;
    for(double height : {.42,.7532,1.0013}) {
        java::self.pos.y=height;Tick(m);
    }
    assert(lunar::uses>airborne);
    const int descending=lunar::uses;
    for(double height : {1.05,.83,.53,.13}) {
        java::self.pos.y=height;Tick(m);
    }
    assert(lunar::uses>descending);
    java::self.pos.y=0;m.fov_->value=180;

    // Through Walls OFF must filter hidden targets with either prediction mode.
    // Also check the launch path when vanilla eye-to-eye visibility says clear.
    m.throughWalls_->value=false;
    for(bool prediction : {false,true}) {
        Reset(m);m.prediction_->value=prediction;java::enemy.visible=false;
        const int blocked=lunar::uses;
        for(int i=0;i<12;++i)Tick(m);
        assert(lunar::uses==blocked && !m.target_ && !m.visuals_.active);
        java::enemy.visible=true;lunar::wall=true;
        for(int i=0;i<12;++i)Tick(m);
        assert(lunar::uses==blocked && m.target_);
        lunar::wall=false;lunar::worldTraceOk=false;
        for(int i=0;i<4;++i)Tick(m);assert(lunar::uses==blocked);
        lunar::worldTraceOk=true;
        // A target can go behind cover between aim preparation and item use.
        m.OnPreUpdate();assert(m.reportedAimReady_);
        java::enemy.visible=false;m.OnInteract();
        assert(lunar::uses==blocked && !m.target_);
        java::enemy.visible=true;
        ThrowOnce(m);assert(lunar::uses==blocked+1);Movement();
    }
    // ON bypasses the visibility filter; it cannot make projectiles cross walls.
    Reset(m);m.throughWalls_->value=true;java::enemy.visible=false;
    Tick(m);assert(m.target_);ThrowOnce(m);Movement();
    java::enemy.visible=true;m.throughWalls_->value=false;
    puts("PASS: Through Walls polarity, hidden target/visual rejection, pre-use visibility recheck, blocked/failed traces with Prediction on and off");

    // A high, unwrapped camera yaw must remain in the same turn neighborhood.
    Reset(m);java::yaw=780;m.rotationSpeed_->value=10;m.throwDelay_->value=0;
    Tick(m);assert(java::serverYaw>765 && java::serverYaw<780);
    ThrowOnce(m);assert(std::abs(java::serverYaw-720)<10 && java::yaw==780);
    Movement();Reset(m);java::yaw=60;m.rotationSpeed_->value=60;
    // No movement => no throw, even if preparation callbacks repeat.
    const int skipped=lunar::uses;for(int i=0;i<7;++i)Tick(m,false);
    assert(lunar::uses==skipped && java::selected==0);
    ThrowOnce(m);Movement();

    // No enabled projectile type means no targeting, ESP, or trajectory.
    java::hotbar[7]=java::hotbar[1]=java::Nothing;
    const int empty=lunar::uses;Tick(m);
    assert(!m.target_ && !m.ownsRotation_ && !m.visuals_.active && !m.visuals_.trajectory);
    for(int i=0;i<8;++i)Tick(m);assert(lunar::uses==empty);
    java::hotbar[1]=java::Egg;
    for(int i=0;i<20 && lunar::uses==empty;++i)Tick(m);
    assert(lunar::uses>empty && m.visuals_.active);
    m.eggs_->value=false;Tick(m);assert(!m.target_ && !m.visuals_.active);
    m.eggs_->value=true;java::hotbar[7]=java::Snowball;
    Reset(m);m.autoSwitch_->value=false;java::selected=0;Tick(m);
    assert(!m.target_ && !m.visuals_.active);
    java::selected=1;ThrowOnce(m);assert(lunar::usedSlot==1);Movement();
    m.autoSwitch_->value=true;java::selected=0;

    m.moduleCheck_->value=false;
    for(bool* busy : {&autorod::owns,&autoblock::owns,&java::usingItem,&lunar::blockLag,
                     &lunar::hypixelLag,&lunar::slinkyLag,&scaffold::enabled,&blockin::placing,
                     &breaker::mining,&java::screen,&clickgui::open}) {
        Reset(m);Tick(m);*busy=true;const int count=lunar::uses;m.OnInteract();
        assert(lunar::uses==count && !m.target_ && java::selected==0);
        Tick(m);assert(lunar::uses==count && m.visuals_.active);*busy=false;
    }
    Reset(m);Tick(m);killaura::active=true;killaura::id=1;
    const int melee=lunar::uses;m.OnInteract();assert(lunar::uses==melee && killaura::id==1);
    killaura::id=0;Reset(m);ThrowOnce(m);Movement();killaura::active=false;
    Reset(m);Tick(m);
    rotation::SetRotations({120,10},100,nullptr,rotation::RotationPriority::Highest);
    const int other=lunar::uses;m.OnInteract();
    assert(lunar::uses==other && rotation::State().priority==rotation::RotationPriority::Highest);
    assert(rotation::State().targetRotations.yaw==120);
    // Scaffold starts between aim preparation and use, taking the same shared
    // manager. Projectile cleanup must never clear or overwrite the new owner.
    Reset(m);Tick(m);scaffold::enabled=true;
    rotation::SetRotations({135,70},80,nullptr,rotation::RotationPriority::Highest);
    const auto scaffoldRotation=rotation::GetRotation();
    const int building=lunar::uses;m.OnPreUpdate();m.OnInteract();
    assert(lunar::uses==building && !m.target_ && !m.ownsRotation_);
    assert(rotation::GetRotation().yaw==scaffoldRotation.yaw &&
        rotation::State().targetRotations.yaw==135 && rotation::State().targetRotations.pitch==70);
    scaffold::enabled=false;
    for(bool* guard : {&lunar::attackClick,&lunar::left,&lunar::right}) {
        Reset(m);Tick(m);m.OnPreUpdate();*guard=true;const int count=lunar::uses;m.OnInteract();
        assert(lunar::uses==count && java::selected==0);*guard=false;
    }
    Reset(m);Tick(m);m.OnPreUpdate();java::serverYaw=140;
    const int wrong=lunar::uses;m.OnInteract();assert(lunar::uses==wrong && java::selected==0);
    Reset(m);Tick(m);m.OnPreUpdate();lunar::slotReady=false;m.OnInteract();assert(lunar::uses==wrong);
    lunar::slotReady=true;

    teams::enabled=true;
    for(int change : {0,1,2,3,4,5}) {
        Reset(m);
        if(change==0)java::yaw=180;
        if(change==1)java::enemy.pos.z=3;
        if(change==2)java::enemy.pos.z=7;
        if(change==3)java::enemy.team=true;
        if(change==4)java::enemy.bot=true;
        if(change==5)java::enemy.visible=false;
        const int count=lunar::uses;for(int i=0;i<4;++i)Tick(m);assert(lunar::uses==count);
        if(change==2)assert(m.target_); // Rotation Range can exceed throw Range.
        java::yaw=60;java::enemy.pos.z=4;java::enemy.team=java::enemy.bot=false;java::enemy.visible=true;
    }
    teams::enabled=false;
    Reset(m);java::enemy.motion={.1,.02,0};rotation::Vec3 point;
    assert(m.AimPosition({&java::enemy},{&java::self},true,point));
    assert(std::abs(point.x-4*.1/1.5)<1e-9);java::enemy.motion={};
    ThrowOnce(m);assert(java::selected==7);
    m.enabled=false;m.onDisable();m.OnPreUpdate();assert(java::selected==7);
    Movement();java::syncOk=false;m.OnPreUpdate();assert(m.previousSlot_==0);
    java::syncOk=true;m.OnPreUpdate();assert(m.previousSlot_==-1 && java::selected==0);
    Reset(m);ThrowOnce(m);java::selected=2;Movement();m.enabled=false;m.onDisable();m.OnPreUpdate();
    assert(java::selected==2);
    java::selected=0;Reset(m);ThrowOnce(m);java::currentWorld=&java::otherLevel;java::selected=3;
    Movement();m.OnPreUpdate();assert(java::selected==3 && m.previousSlot_==-1);
    m.enabled=false;m.onDisable();m.OnPreUpdate();assert(!m.ownsRotation_ && !m.visuals_.active);
    assert(java::throwOrder.find('R')==std::string::npos);
    std::cout<<"PASS: real smooth manager/movement fix, continuous cooldown tracking, moving target, unwrapped yaw, natural movement before use\n";
    std::cout<<"PASS: empty/disabled/held-only items hide visuals and stop tracking; refill resumes automatically\n";
    std::cout<<"PASS: combat ownership, packet boundaries, filters, delayed restore, disable/world/manual-slot cleanup\n";
    std::cout<<"PASS: 360 rear acquisition after idle KillAura return, rising/falling throws, and mid-aim Scaffold handoff\n";
}
