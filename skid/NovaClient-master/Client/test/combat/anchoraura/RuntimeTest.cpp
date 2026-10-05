void Tick(){
    const auto before=mock::angle;const int uses=mock::placed+mock::charged+mock::detonated;
    mock::used=mock::motion=mock::locked=false;mock::attack.BeginTick();
    nova::event::Post<nova::event::GameTickEvent>();nova::event::Post<nova::event::PrePlayerInteractEvent>();
    if(mock::placed+mock::charged+mock::detonated>uses){
        // Independent server-side pre-move check, not the proposed new angle.
        auto wire=nova::rotation::ReadPlacementWire();
        assert(wire.hasLook&&wire.deltaKnown&&!wire.pendingPlace);
        assert(!wire.rotated||wire.delta<=2.f||(wire.hasPlaced&&std::fabs(wire.delta-wire.placedDelta)>=.0001f));
        nova::rotation::ObservePlacementPacket();
    }
    nova::rotation::ObservePlacementMovement(mock::angle.yaw!=before.yaw||mock::angle.pitch!=before.pitch,mock::angle.yaw,mock::angle.pitch);
}
auto Start(){
    using namespace mock;
    world=2;selected=0;oldIdentity=100;owner=slotOwner=nullptr;spoof=-1;yieldable=physicalLease=false;attack={};
    target=true;team=bot=teammate=wall=rescue=screen=selfDanger=crystal=dropOnAim=extraEnemy=false;stock=supported=defenseOK=syncOK=true;
    targetX=3.4;targetY=64;targetZ=1;selfPosition={0,64,0};selfVelocity=observedVelocity={};
    selfGrounded=targetGrounded=true;charges=-1;placed=charged=detonated=0;explodes=glowstone=accept=true;offhandGlowstone=false;
    baseExists=false;supportExists=true;bases.clear();quantized=false;fallingWorld=false;slowReads=false;
    shields.clear();shieldUses=0;glowCount=64;extra::offhand=extra::creative=false;extra::offhandSwings=0;
    nova::module::explosive::feetWorking=false;
    angle=nova::rotation::Calculate(selfPosition.Add(0,1.62,0),{2.5,63,.5});
    nova::rotation::ResetPlacementWire();
    nova::rotation::ObservePlacementMovement(true,angle.yaw,angle.pitch);
    nova::rotation::ObservePlacementMovement(true,angle.yaw,angle.pitch);
    auto m=nova::module::MakeAnchorAuraModule();m->Enable();return m;
}
void Scan(int count=120){for(int i=0;i<count&&mock::detonated==0;++i)Tick();}
int main(){
    using namespace nova;
    using namespace nova::module;
    using Aura=anchoraura::AnchorAura;
    for(bool silent:{false,true}){
        auto m=Start();m->Bool("Silent",silent);
        for(int i=0;i<200&&mock::detonated<3;++i){Tick();assert(mock::selected==0&&mock::spoof==-1);}
        assert(mock::placed==3&&mock::charged==3&&mock::detonated==3);
        m->Disable();Tick();assert(!explosive::anchorAuraEnabled&&!explosive::anchorWorking&&!mock::slotOwner);
    }
    {auto m=Start();mock::charges=1;mock::baseExists=true;mock::stock=mock::glowstone=false;Scan();
     assert(mock::detonated==1&&mock::placed==0&&mock::charged==0);}
    {auto m=Start();mock::charges=0;mock::baseExists=true;mock::stock=false;Scan();assert(mock::charged==1&&mock::detonated==1);}
    {auto m=Start();mock::stock=false;extra::offhand=true;Scan();assert(mock::placed==1&&mock::detonated==1&&extra::offhandSwings==1);}
    {auto m=Start();m->Bool("Swing",false);int packets=mock::swingPackets,swings=mock::swings;Scan();
     assert(mock::detonated==1&&mock::swings==swings&&mock::swingPackets==packets+3);}
    {auto m=Start();m->Number("Delay",1000);for(int i=0;i<120&&mock::placed==0;++i)Tick();
     for(int i=0;i<10;++i)Tick();assert(mock::placed==1&&mock::charged==0);}
    for(int reason=0;reason<9;++reason){auto m=Start();
        if(reason==0)mock::explodes=false;if(reason==1)mock::glowstone=false;if(reason==2)mock::offhandGlowstone=true;
        if(reason==3)mock::selfDanger=true;if(reason==4)mock::rescue=true;if(reason==5)mock::wall=true;
        if(reason==6)extra::creative=true;if(reason==7)mock::team=true;if(reason==8)mock::defenseOK=false;
        Scan();assert(mock::placed==0&&mock::charged==0&&mock::detonated==0&&mock::selected==0);
    }
    {auto m=Start();mock::wall=true;m->Bool("Target.Through Walls",true);Scan();assert(mock::detonated==1);}
    {auto m=Start();mock::extraEnemy=true;Scan();auto* a=static_cast<Aura*>(m.get());assert(mock::detonated==1&&a->sent_.target==6);}
    {auto m=Start();mock::bases={{{2,63,0},false,.15,0,false},{{1,63,2},false,1,0,false}};
     for(int i=0;i<120&&mock::placed==0;++i)Tick();assert(mock::placed==1&&mock::Same(mock::lastPlaced,{1,63,2}));}
    {auto m=Start();mock::charges=1;mock::baseExists=true;
     mock::bases={{{1,63,2},false,1,0,false},{{2,63,0},true,.2,0,false}};
     Scan();assert(mock::detonated==1&&mock::placed==0&&mock::Same(mock::lastPlaced,{2,63,0}));}
    for(float yaw:{-179.f,-90.f,0.f,90.f,179.f}){
        auto m=Start();mock::angle={yaw,-70};mock::quantized=true;
        rotation::ObservePlacementMovement(true,yaw,-70);Scan(240);assert(mock::detonated==1);
    }
    for(bool silent:{false,true}){auto m=Start();m->Bool("Silent",silent);mock::quantized=true;
        for(int i=0;i<250;++i){mock::selfPosition={.5*std::sin(i*.1),64,.5*std::cos(i*.1)};Tick();}
        assert(mock::detonated>=5);m->Disable();Tick();assert(mock::selected==0);
    }
    {auto m=Start();mock::accept=false;for(int i=0;i<120&&mock::placed==0;++i)Tick();assert(mock::placed==1);
     for(int i=0;i<10;++i)Tick();assert(mock::placed==1&&mock::charged==0);
     auto* a=static_cast<Aura*>(m.get());a->deadline_=0;Tick();assert(!a->pending_&&mock::placed==1);
     a->retryAt_=0;Scan();assert(mock::placed==2);}
    {auto m=Start();mock::accept=false;for(int i=0;i<120&&mock::placed==0;++i)Tick();
     mock::world=8;Tick();assert(!mock::slotOwner&&!explosive::anchorWorking);}
    {auto m=Start();mock::selected=2;m->Disable();Tick();assert(mock::selected==2);}
    {auto m=Start();rotation::ObservePlacementMovement(true,10,0);rotation::ObservePlacementPacket();rotation::ObservePlacementMovement(true,20,0);
     auto before=rotation::ReadPlacementWire();m->Disable();Tick();auto after=rotation::ReadPlacementWire();
     assert(after.hasPlaced==before.hasPlaced&&after.placedDelta==before.placedDelta);}
    {auto m=Start();explosive::ResetAnchor();explosive::OfferAnchor(8,false);
     assert(!explosive::ChooseCrystal(8,false)&&explosive::ChooseAnchor());
     assert(explosive::ChooseCrystal(9,false)&&!explosive::ChooseAnchor());
     explosive::OfferAnchor(8,true);assert(!explosive::ChooseCrystal(9,false));
     assert(explosive::ChooseCrystal(9,true));m->Disable();Tick();}
    {auto m=Start();auto* a=static_cast<Aura*>(m.get());Aura::Plan p;p.pos=mock::base;p.target=3;
     m->Number("Safety.Max Self Damage",1);p.damage=19;assert(!a->Safe(java::PlayerEntity(),p));
     p.damage=20;assert(a->Safe(java::PlayerEntity(),p)); // lethal exemption
     mock::selfDanger=true;a->damageCache_.clear();assert(!a->Safe(java::PlayerEntity(),p));}
    {auto m=Start();auto* a=static_cast<Aura*>(m.get());mock::targetX=8;
     assert(a->Allowed(java::PlayerEntity(),java::EntityById(3))); // target range is not placement range
     mock::targetX=13;assert(!a->Allowed(java::PlayerEntity(),java::EntityById(3)));}
    {auto m=Start();auto legacy=MakeLegitAnchorAuraModule();legacy->Number("Target.FOV",360);legacy->Enable();
     Scan();assert(mock::detonated==1);
     auto* old=static_cast<anchor::LegitAnchorAura*>(legacy.get());assert(!old->have_&&!old->pending_);
     m->Disable();Tick();for(int i=0;i<120&&mock::detonated<2;++i)Tick();assert(mock::detonated==2);
     legacy->Disable();Tick();assert(!mock::slotOwner&&!explosive::anchorWorking);}
    {auto m=Start();mock::slowReads=true;for(int i=0;i<500&&mock::placed==0;++i){
         mock::selfPosition={.4*std::sin(i*.2),64,.4*std::cos(i*.2)};Tick();
     }assert(mock::placed==1);mock::slowReads=false;}
    std::puts("AnchorAura runtime: PASS (cycles, policy, slots, walls, moving/quantized rotation, wire order, confirmation, arbitration)");
}
