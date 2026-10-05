#include <atomic>
#include <mutex>
#include <cassert>
#include <iostream>
using jint=int;
using jclass=void*;
using jobject=void*;
#define JNICALL
struct JNIEnv { bool IsSameObject(jobject a,jobject b){return a==b;} } env;
int localInventory, otherInventory, playerObject, items[9];
int physicalSlot=1, serverSlot=1, syncs=0;
bool syncOk=true;
std::atomic<int> spoofed{-1};
std::mutex slotMutex;const void* slotOwner=nullptr;const int scaffoldOwner=0;
bool physicalLease=false,yieldable=false;
using DWORD=unsigned long;
DWORD physicalWriter=0;
DWORD GetCurrentThreadId(){return 1;}
bool SetSelectedSlot(int value){physicalSlot=value;return true;}
namespace compat { jobject LocalPlayer(JNIEnv*){return &playerObject;} }
bool SelectedSlot(int& out){out=physicalSlot;return true;}
bool Synchronize(){++syncs; if(!syncOk)return false; serverSlot=spoofed.load()<0?physicalSlot:spoofed.load();return true;}
struct Frame {
    Frame(JNIEnv*){}
    int physical(jobject inv){return inv==&localInventory?physicalSlot:7;}
    jobject inventory(jobject){return &localInventory;}
    bool check(){return true;}
    jobject item(jobject,int slot){return &items[slot];}
    jobject keep(jobject value){return value;}
};
// @SLOT_METHODS@
int main(){
    int selected=-1;
    assert(EffectiveSelectedSlot(selected)&&selected==1);
    assert(SetScaffoldSlot(4)&&physicalSlot==1&&serverSlot==4);
    assert(EffectiveSelectedSlot(selected)&&selected==4);
    assert(SpoofSlotHook(&env,nullptr,&localInventory)==4);
    assert(SpoofSlotHook(&env,nullptr,&otherInventory)==7);
    assert(VisualInventoryHandHook(&env,nullptr,&localInventory)==&items[1]);
    assert(VisualPlayerHandHook(&env,nullptr,&playerObject)==&items[1]);
    // Scrolling changes the physical item and its rendering, not the active
    // placement slot. Release must restore the new choice, never the enable-time slot.
    physicalSlot=6;
    assert(VisualPlayerHandHook(&env,nullptr,&playerObject)==&items[6]);
    assert(SpoofSlotHook(&env,nullptr,&localInventory)==4);
    assert(SetScaffoldSlot(8)&&physicalSlot==6&&serverSlot==8);
    const int releaseSyncs=syncs;
    ReleaseScaffoldSlot();
    assert(syncs==releaseSyncs && serverSlot==8);
    Synchronize(); // next vanilla controller tick
    assert(serverSlot==6&&spoofed.load()==-1&&EffectiveSelectedSlot(selected)&&selected==6);
    const int before=syncs;
    ReleaseScaffoldSlot();assert(syncs==before);
    assert(!SetScaffoldSlot(-1)&&!SetScaffoldSlot(9)&&syncs==before);
    syncOk=false;
    assert(!SetScaffoldSlot(3)&&spoofed.load()==-1&&physicalSlot==6);
    syncOk=true;int mlgOwner=0;
    assert(SetSilentSlot(&mlgOwner,4));
    ReleaseScaffoldSlot();assert(EffectiveSelectedSlot(selected)&&selected==4);
    assert(!SetScaffoldSlot(2)&&physicalSlot==6);
    assert(VisualPlayerHandHook(&env,nullptr,&playerObject)==&items[6]);
    ReleaseSilentSlot(&mlgOwner);assert(EffectiveSelectedSlot(selected)&&selected==6);
    assert(SetScaffoldSlot(2));ReleaseSilentSlot(&mlgOwner);
    assert(EffectiveSelectedSlot(selected)&&selected==2);ReleaseScaffoldSlot();
    int crystalOwner=0,urgentOwner=0;
    assert(SetSilentSlot(&crystalOwner,4));
    assert(!AcquirePhysicalSlot(&urgentOwner));
    const int beforeYield=syncs;
    YieldSlot(&crystalOwner);
    assert(syncs==beforeYield&&serverSlot==4&&OwnsSlot(&crystalOwner));
    assert(AcquirePhysicalSlot(&urgentOwner)&&OwnsPhysicalSlot(&urgentOwner));
    assert(spoofed.load()==-1&&!OwnsSlot(&crystalOwner));
    ReleaseSilentSlot(&crystalOwner);assert(OwnsPhysicalSlot(&urgentOwner));
    assert(SetOwnedPhysicalSlot(&urgentOwner,2)&&physicalSlot==2);
    ReleaseSilentSlot(&urgentOwner);
    assert(AcquirePhysicalSlot(&crystalOwner));YieldSlot(&crystalOwner);
    assert(SetSilentSlot(&urgentOwner,8)&&serverSlot==8&&physicalSlot==2);
    ReleaseSilentSlot(&crystalOwner);assert(OwnsSlot(&urgentOwner));
    assert(!AcquirePhysicalSlot(&crystalOwner));
    ReleaseSilentSlot(&urgentOwner);
    std::cout<<"PASS: production silent-slot ownership, physical rendering, scrolling, block-slot replacement, restoration and sync failure\n";
}
