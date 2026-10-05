int main() {
    using namespace nova;
    using namespace nova::module::autocrystal;
    using nova::module::explosive::autoCrystalWorking;
    assert(Better(PlacementStage::Direct,4,PlacementStage::Obsidian,30));
    assert(!Better(PlacementStage::Obsidian,30,PlacementStage::Direct,4));
    assert(Better(PlacementStage::Direct,4.2,PlacementStage::Direct,4.4,true));
    assert(!Better(PlacementStage::Direct,4.9,PlacementStage::Direct,4.6,true));
    assert(Minimum(6,2,1)==3);
    { // Regression: current proposed angle must not mask the previous look delta.
        rotation::PlacementWireHistory wire;
        assert(!wire.Ready());wire.Move(true,0,0);assert(!wire.Ready());
        wire.Move(true,1,0);assert(wire.Ready());
        wire.Place();wire.Move(true,11,0); // placed with delta 1, now delta 10
        assert(wire.Ready());wire.Place();wire.Move(true,21,0);
        assert(!wire.Ready()); // server compares 10 with 10, even if NEXT look is 1
        wire.Move(false);assert(!wire.Ready()); // status-only cannot clear rotated
        wire.Move(true,22,0);assert(wire.Ready());
        wire.Place();wire.Move(false);assert(wire.Ready());
        wire.Move(true,32,0);assert(wire.Ready());
        wire.Place();wire.Move(true,42,0);assert(!wire.Ready());
        wire.Move(true,42,0);assert(wire.Ready()); // explicit unchanged look IS a new update
        wire.Move(true,52,0);wire.UnknownUse();assert(!wire.Ready());
        wire.Move(true,53,0);assert(wire.Ready()); // bucket/rocket ambiguity handled conservatively
    }
    { // Weak blocks are ignored, resistant blocks stop exposure, errors fail closed.
        bool clear=false;int clips=0;
        auto clip=[&](auto from,auto,java::BlockHitResultInfo& hit){++clips;
            if(from.x>2){hit={};return true;}
            hit.kind=java::HitKind::Block;hit.hasBlockPos=true;hit.blockPos={from.x<1?0:1,0,0};return true;};
        assert(ResistantRay({.5,.5,.5},{3,.5,.5},clip,[](auto,double& r){r=5;return true;},clear)&&clear&&clips==3);
        clips=0;assert(ResistantRay({.5,.5,.5},{3,.5,.5},clip,[](auto p,double& r){r=p.x==1?600:5;return true;},clear)&&!clear&&clips==2);
        assert(!ResistantRay({.5,.5,.5},{3,.5,.5},clip,[](auto,double&){return false;},clear));
    }
    {auto m=Start();Scan();assert(mock::uses==1&&mock::selected==4);
     Tick();assert(mock::uses==1); // no duplicate use before spawn
     mock::crystal=true;Tick();assert(mock::attacks==1);
     Tick();assert(mock::attacks==1); // no duplicate attack before removal
     m->Disable();Tick();assert(mock::selected==0&&!mock::slotOwner&&!autoCrystalWorking);}
    {auto m=Start();m->Bool("Place.Silent",true);Scan();
     assert(mock::uses==1&&mock::selected==0&&mock::spoof==4);
     m->Disable();Tick();assert(mock::selected==0&&mock::spoof==-1&&!mock::owner);}
    {auto m=Start();mock::selfDanger=true;Scan();assert(mock::uses==0&&mock::attacks==0);
     m->Disable();Tick();}
    {auto m=Start();extra::offhand=true;mock::stock=false;Scan();
     assert(mock::uses==1&&mock::selected==0&&!mock::slotOwner);m->Disable();Tick();extra::offhand=false;}
    {auto m=Start();extra::creative=true;Scan();assert(mock::uses==0);m->Disable();Tick();extra::creative=false;}
    {auto m=Start();mock::team=true;Scan();assert(mock::uses==0&&!autoCrystalWorking);
     m->Disable();Tick();}
    {auto m=Start();mock::wall=true;Scan();assert(mock::uses==0);
     m->Bool("Target.Through Walls",true);Scan();assert(mock::uses==1);
     m->Disable();Tick();}
    {auto m=Start();mock::extraEnemy=true;Scan();
     auto* aura=dynamic_cast<AutoCrystal*>(m.get());
     assert(mock::uses==1&&aura->targetId_==6); // damage wins over first/nearest player
     m->Disable();Tick();mock::extraEnemy=false;}
    {auto m=Start();mock::bases={{{2,63,0},true,.15,0,false},{{1,63,2},true,1,0,false}};
     Scan();assert(mock::uses==1&&mock::Same(mock::lastPlaced,{1,63,2}));
     m->Disable();Tick();}
    {auto m=Start();m->Bool("Place.Auto Base",true);mock::baseExists=false;ScanBase();
     assert(mock::baseUses==1&&mock::uses==0);
     mock::baseExists=true;Scan();assert(mock::uses==1);
     m->Disable();Tick();assert(mock::selected==0);}
    {auto m=Start();Scan();mock::selected=2;m->Disable();Tick();assert(mock::selected==2);}
    {auto m=Start();Scan();mock::rescue=true;Tick();
     assert(!mock::owner&&!mock::slotOwner&&!autoCrystalWorking);m->Disable();Tick();}
    {auto m=Start();Scan();mock::world=9;Tick();
     assert(!mock::owner&&!mock::slotOwner);m->Disable();Tick();}
    {auto m=Start();const auto a=mock::angle;
     // A manual/other-module placement must survive AutoCrystal's enable reset.
     rotation::ObservePlacementMovement(true,a.yaw+10,a.pitch);
     rotation::ObservePlacementPacket();
     rotation::ObservePlacementMovement(true,a.yaw+20,a.pitch);
     assert(!rotation::ReadPlacementWire().Ready());
     m->Disable();Tick();m->Enable();Tick();assert(mock::uses==0);
     Scan();assert(mock::uses==1);m->Disable();Tick();}
    for(bool silent:{false,true})for(bool building:{false,true}) {
        auto m=Start();m->Bool("Place.Silent",silent);m->Bool("Place.Auto Base",building);
        m->Number("Rotation.Speed",45);mock::baseExists=!building;mock::quantized=true;
        // Independent packet-order oracle: a placement consumes the last look,
        // not the angle which will be sent at the end of this client tick.
        float previousYaw=mock::angle.yaw,previousPitch=mock::angle.pitch,delta=0,last=0;
        bool rotated=true;
        for(int tick=0;tick<150;++tick) {
            const auto old=mock::selfPosition;const double a=3.141592653589793+tick*.075;
            mock::selfPosition={2.5+2.7*std::cos(a),64,.5+2.7*std::sin(a)};
            mock::selfVelocity=mock::selfPosition.Subtract(old);
            const int uses=mock::uses,bases=mock::baseUses,attacks=mock::attacks;Tick();
            if(mock::uses>uses||mock::baseUses>bases) {
                if(rotated){assert(delta<=2||std::fabs(delta-last)>=.0001f);last=delta;rotated=false;}
            }
            if(mock::angle.yaw!=previousYaw||mock::angle.pitch!=previousPitch) {
                delta=std::fabs(mock::angle.yaw-previousYaw);rotated=true;
            }
            previousYaw=mock::angle.yaw;previousPitch=mock::angle.pitch;
            if(mock::baseUses>bases)mock::baseExists=true;
            if(mock::uses>uses)mock::crystal=true;
            if(mock::attacks>attacks){mock::crystal=false;if(building)mock::baseExists=false;}
        }
        assert(mock::uses>=5&&mock::attacks>=5);
        m->Disable();Tick();
    }
    std::puts("AutoCrystal: policy, exposure traversal, lifecycle, arbitration and multi-target regressions passed");
}
