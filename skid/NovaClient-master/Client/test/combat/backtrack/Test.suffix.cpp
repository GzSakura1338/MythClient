namespace {
using namespace nova;
void Tick(){event::GameTickEvent e(event::EventType::Pre);(void)event::EventBus::Get().Post(e);}
void AttackHead(java::Entity& target=java::first){event::AttackEntityEvent e(&target);(void)event::EventBus::Get().Post(e);}
void Attack(java::Entity& target=java::first){AttackHead(target);Tick();}
bool Receive(java::Packet& p){event::PacketEvent e(event::EventType::Incoming,&p);(void)event::EventBus::Get().Post(e);if(!e.IsCancelled())java::HandleIncomingPacket(&p);return e.IsCancelled();}
}
int main(){
 using mock::Check;
 auto m=nova::module::MakeBacktrackModule();event::EventBus::Get().Seal();
 Check(m->props.size()==4&&m->props.contains("Delay")&&m->props.contains("ESP")&&m->props.contains("Min Range")&&m->props.contains("Max Range"),"expected compact delay/range/ESP settings");
 auto reset=[&]{
  m->setEnabled(false);mock::now+=1000;mock::played.clear();mock::player=true;mock::world=1;
  mock::ticks=50;mock::owns=mock::blocks=mock::failRef=false;
  java::first.pos=java::first.base={3,0,0};java::first.alive=true;java::first.kind=java::BacktrackKind::Player;
  java::second.pos=java::second.base={3.1,0,0};java::second.alive=true;
  m->n("Delay",100);m->n("Min Range",0);m->n("Max Range",6);m->setEnabled(true);Tick();
 };
 java::Packet normal{1},move{2,false,false,true},ping{3},chat{4,true},closer{5,false,false,true};
 move.pos={3.5,0,0};closer.pos={2.8,0,0};
 auto start=[&]{Attack();Check(Receive(move),"receding target should start delay");};
 reset();Check(!Receive(normal),"must not queue before attacking");Attack();
 Check(!Receive(ping),"arming alone must not freeze transaction replies");
 java::Packet still=move;still.pos={3,0,0};Check(!Receive(still),"stationary movement should not start delay");
 mock::played.clear();Check(Receive(move),"receding movement must be held");
 mock::now+=20;Check(Receive(ping)&&Receive(chat),"transactions and chat must stay in same FIFO");
 mock::now+=79;Tick();Check(mock::played.empty(),"released before deadline");
 mock::now+=1;Tick();Check(mock::played==std::vector<int>{2}&&mock::refs==2,"per-packet age must be honored");
 mock::now+=20;Tick();Check(mock::played==std::vector<int>({2,3,4})&&mock::refs==0,"FIFO deadline release failed");
 Check(!module::backtrack::IsLagging(),"empty queue still reports lagging");
 reset();start();Receive(ping);Receive(closer);
 Check(mock::played==std::vector<int>({2,3,5})&&mock::refs==0,"closer movement must follow flushed history");
 Attack();Check(!Receive(move),"recovery window must prevent immediate rearming");
 reset();start();Attack(java::second);Check(mock::refs==0,"switching target must release previous stream");
 java::Packet move2=move;move2.target=3;move2.pos={3.6,0,0};Check(Receive(move2),"new target not tracked");
 // A queued transaction must never replay from the 1.8 controller head,
 // which vanilla invokes after animation and before the actual attack packet.
 for(int reason=0;reason<4;++reason){
  reset();start();Receive(ping);mock::wire="MS";
  if(reason==1)m->n("Delay",0);
  if(reason==2)mock::blocks=true;
  if(reason==3)java::second.kind=java::BacktrackKind::Mob;
  AttackHead(java::second);
  Check(mock::wire=="MS"&&mock::refs==2,"attack head replayed a transaction between swing and attack");
  mock::wire+="AM";Tick();
  Check(mock::wire=="MSAMT"&&mock::refs==0,"transaction must follow next movement at the safe tick boundary");
  java::second.kind=java::BacktrackKind::Player;
 }
 // Last attack in a tick wins; disconnect clears intent before it can rearm.
 reset();AttackHead();AttackHead(java::second);Tick();
 java::Packet selected=move;selected.target=3;selected.pos={3.7,0,0};
 Check(Receive(selected),"pending selection must retain the latest target");
 reset();AttackHead();java::Packet pendingDisconnect{99};pendingDisconnect.disconnect=true;Receive(pendingDisconnect);Tick();
 Check(!Receive(move),"disconnect must clear pending attack intent");
 reset();m->n("Max Range",3.2);java::first.pos=java::first.base={5,0,0};Attack();
 java::Packet farther=move;farther.pos={5.5,0,0};Check(!Receive(farther),"attack above Max Range must not arm");
 reset();m->n("Min Range",2.8);Attack();Check(!Receive(move),"attack below Min Range must not arm");
 reset();m->n("Min Range",2.7);m->n("Max Range",3.2);start();
 Check(mock::refs==1,"inclusive min/max boundaries must allow tracking");
 java::Packet outside=move;outside.pos={3.6,0,0};Check(!Receive(outside)&&mock::refs==0,"server position beyond maximum must flush");
 reset();m->n("Min Range",4);m->n("Max Range",2);start();Check(mock::refs==1,"reversed bounds must normalize");
 reset();start();m->n("Max Range",2);Tick();Check(mock::refs==0&&!Receive(ping),"editing range must drain an excluded target at tick head");
 reset();start();java::self.pos={2,0,0};m->n("Min Range",2);Tick();
 Check(mock::refs==0,"moving inside Min Range must end tracking");java::self.pos={0,0,0};
 reset();java::first.kind=java::BacktrackKind::Mob;Attack();Check(!Receive(move),"only attacked players are tracked");
 reset();mock::ticks=10;Attack();Check(!Receive(move),"spawn grace missing");
 reset();start();mock::now+=751;Tick();Check(mock::refs==0&&!Receive(ping),"attack timeout failed");
 reset();start();mock::now+=450;Attack();mock::now+=50;Tick();
 Check(mock::refs==0&&!Receive(ping),"repeated attacks must not extend max hold window");
 reset();start();java::first.alive=false;Tick();Check(mock::refs==0&&!Receive(ping),"dead target retained");
 reset();start();mock::world=2;mock::played.clear();Tick();Check(mock::refs==0&&mock::played.empty(),"old world packets must be discarded");
 reset();start();mock::player=false;mock::played.clear();Tick();Check(mock::refs==0&&mock::played.empty(),"world exit must discard references");
 for(int kind=0;kind<5;++kind){
  reset();start();Receive(ping);java::Packet barrier{10+kind};
  barrier.correction=kind==0;barrier.velocity=kind==1;barrier.target=1;
  barrier.explosion=kind==2;barrier.dead=kind==3;barrier.decodeFail=kind==4;
  Check(!Receive(barrier)&&mock::refs==0,"barrier must flush and pass through");
  Check(mock::played==std::vector<int>({2,3,10+kind}),"barrier overtook queued packets");
 }
 for(int kind=0;kind<2;++kind){
  reset();start();java::Packet terminal{20+kind};terminal.disconnect=kind==0;terminal.respawn=kind==1;
  Check(!Receive(terminal)&&mock::refs==0&&mock::played==std::vector<int>{20+kind},"terminal event replayed old packets");
 }
 reset();start();mock::owns=true;Tick();Check(mock::refs==0&&!Receive(ping),"AntiKB ownership did not release stream");
 reset();start();mock::blocks=true;Receive(ping);Check(mock::refs==0,"AntiKB attack queue must prevent double buffering");
 reset();start();m->n("Delay",0);Tick();Check(mock::refs==0&&!Receive(ping),"Delay zero must drain immediately");
 reset();start();mock::failRef=true;Receive(ping);Check(mock::refs==0&&!module::backtrack::IsLagging(),"failed global ref must fail open");
 reset();start();for(int i=0;i<255;++i)Check(Receive(ping),"queue capacity too small");
 Check(!Receive(normal)&&mock::refs==0,"queue limit must release rather than grow unbounded");
 reset();start();m->setEnabled(false);Check(mock::refs==0&&mock::wanted==0&&mock::played==std::vector<int>{2},"disable must replay and release interest");
 reset();start();m.reset();Check(mock::refs==0&&mock::wanted==0,"destructor leaked JNI refs or incoming interest");
 std::puts("Backtrack redesign: attack-head transaction ordering, custom range bounds, attacked-player selection, receding-only activation, ordered transactions/chat, deadlines, recovery, barriers, AntiKB, world reset and queue bounds passed");
}
