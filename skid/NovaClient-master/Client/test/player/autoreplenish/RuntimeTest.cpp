namespace nova::compat {bool IsLegacyLunar(){return !mock::supported;}}
namespace nova::java {
// Encode a quantity in the high digits; low digits include type/components.
bool ReadItemIdentity(jobject stack,jobject& out,bool& empty){out=mock::Ptr(intptr_t(stack)%1000);empty=stack==nullptr;return true;}
bool ItemStacksMatch(jobject a,jobject b){return a==b;}
}
// @PRODUCTION_MODULE@
void Tick(){nova::event::GameTickEvent e;nova::event::callback<nova::event::GameTickEvent>(e);}
void Neutral(){
    nova::event::MoveInputEvent e;nova::event::callback<nova::event::MoveInputEvent>(e);
    assert(e.forward==0&&e.strafe==0&&!e.jump&&!e.sneak&&e.suppressSprint);
    nova::event::MotionEvent motion;nova::event::callback<nova::event::MotionEvent>(motion);
}
int main(){
    using namespace mock;
    auto m=nova::module::MakeAutoReplenishModule();m->Enable();
    slots[2]=10011;slots[10]=64011;slots[11]=12;Tick();assert(clicks==0);
    slots[2]=1011;Tick();assert(clicks==0);
    slots[2]=0;Tick();assert(clicks==1&&slots[2]==64011&&slots[10]==0&&source==10);
    // Never fill unrelated initially empty slots, or a slot replaced by another item.
    slots[20]=11;Tick();assert(clicks==1);slots[2]=12;Tick();assert(clicks==1);
    slots[2]=0;Tick();assert(clicks==2&&slots[2]==12&&source==11);
    // Delay begins on depletion, and a failed swap retries without losing identity.
    m->delay.value=100;Tick();slots[2]=0;slots[18]=12;Tick();assert(clicks==2);
    now+=99;Tick();assert(clicks==2);++now;accept=false;Tick();assert(clicks==3&&slots[2]==0);
    now+=249;Tick();assert(clicks==3);++now;accept=true;Tick();assert(clicks==4&&slots[2]==12);
    // GUI/cursor changes clear stale identities; user rearrangement isn't depletion.
    m->delay.value=0;Tick();screen=2;slots[2]=0;slots[19]=12;Tick();screen=0;Tick();assert(clicks==4);
    slots[0]=13;Tick();cursor=false;slots[0]=0;slots[22]=13;Tick();cursor=true;Tick();assert(clicks==4);
    // Grim uses the same input/post-motion acknowledgement as AutoTotem.
    slots[4]=14;slots[23]=14;Tick();m->grim.value=true;slots[4]=0;Tick();
    assert(clicks==4&&lease&&nova::module::autoreplenish::SuppressMovement()&&!sprinting);
    Tick();assert(clicks==4);Neutral();Tick();assert(clicks==5&&slots[4]==14&&lease);
    Tick();assert(!lease&&!nova::module::autoreplenish::SuppressMovement()&&sprintKey);
    Tick();assert(!sprintKey);
    // AutoTotem owns the inventory first; preserve no stale swap through its work.
    slots[23]=14;slots[4]=0;totemBusy=true;Tick();assert(!lease&&clicks==5);
    totemBusy=false;Tick();assert(clicks==5);
    // No hook -> bounded pause, disable/world changes always release movement.
    slots[4]=14;Tick();slots[4]=0;Tick();assert(lease);
    for(int i=0;i<5;++i)Tick();assert(!lease&&!nova::module::autoreplenish::SuppressMovement());
    now+=300;Tick();assert(lease);m->Disable();Tick();assert(!lease&&clicks==5);
    m->Enable();slots[4]=14;Tick();slots[4]=0;Tick();assert(lease);
    ++world;Tick();assert(!lease);Tick();assert(clicks==5);
    // Source mismatch / destination occupied during the neutral tick cancels safely.
    slots[4]=14;Tick();slots[4]=0;Tick();assert(lease);Neutral();slots[4]=15;Tick();assert(clicks==5&&!lease);
    slots[4]=0;slots[24]=15;usingItem=true;Tick();assert(clicks==5&&!lease);
    usingItem=false;otherLease=true;Tick();assert(!lease);otherLease=false;
    Tick();assert(lease);Neutral();Tick();assert(clicks==6&&slots[4]==15);
    Tick();supported=false;Tick();assert(!lease);
    std::puts("PASS: AutoReplenish depletion, matching, original slot, delay/retry, GUI reset, Grim handshake, arbitration and cleanup");
}
