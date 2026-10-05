#include <atomic>
#include <cassert>
#include <cstdio>

#define JNICALL
using jobject=int; using jclass=int; using jint=int; using jboolean=bool;
constexpr bool JNI_FALSE=false;
enum { RightClick=1, StartUse, KeyBinding, KeyState };
struct { int RightClickMouse=RightClick, StartUseItem=StartUse, KeyBinding=::KeyBinding, KeyState=::KeyState; } h;
std::atomic<bool> autoBlockManualSuppressed{false}, autoBlockManaged{false}, hypixelBlockManaged{false}, blockLagActive{false};
bool grimReblockPending=false, circuit=false, sword=true, interactive=false, cancelled=false;
int opened=0, used=0, keyWrites=0;
bool keyDown=false;
bool IsCircuitOpen() { return circuit; }
bool HoldingSword() { return sword; }
struct BlockLagLock {};
namespace event { struct ClickEvent {}; }
namespace module::blockin { bool active=false; bool IsPlacing() {return active;} }
bool Dispatch(event::ClickEvent&) { return cancelled; }
struct JNIEnv;
void DrainBlockPackets(JNIEnv*,bool) {}
jboolean BlockManualSwordUseHook(JNIEnv*,jclass,jobject,jobject,jobject,jobject);
struct JNIEnv {
    void CallVoidMethod(jobject,int method) {
        assert(method==RightClick);
        // Vanilla first tries the block/entity; only an unhandled click falls back to item use.
        if(interactive) ++opened;
        else BlockManualSwordUseHook(this,0,1,2,3,4);
    }
    bool CallBooleanMethod(jobject,int method,jobject,jobject,jobject) {
        assert(method==StartUse); ++used; return true;
    }
    void CallStaticVoidMethod(int,int,jint,jboolean pressed) { ++keyWrites; keyDown=pressed; }
    bool ExceptionCheck() { return false; }
};

// @PRODUCTION_MANUAL_BLOCK_HOOKS@

int main() {
    JNIEnv env;
    autoBlockManualSuppressed=true;
    BlockMouseKeyHook(&env,0,-99,true); assert(keyWrites==1 && keyDown);
    interactive=true; BlockVanillaUseHook(&env,0,1); assert(opened==1 && used==0);
    interactive=false; BlockVanillaUseHook(&env,0,1); assert(used==0); // Sword stays unblocked.
    sword=false; BlockVanillaUseHook(&env,0,1); assert(used==1); // Food, potion, rod etc.
    sword=true; autoBlockManualSuppressed=false;
    BlockVanillaUseHook(&env,0,1); assert(used==2);
    autoBlockManualSuppressed=true; circuit=true;
    BlockVanillaUseHook(&env,0,1); assert(used==3); // Circuit bypass remains vanilla.
    circuit=false; interactive=true; autoBlockManaged=true; blockLagActive=true;
    BlockVanillaUseHook(&env,0,1); assert(opened==1); // Existing active blink ownership unchanged.
    BlockMouseKeyHook(&env,0,-99,true); assert(keyWrites==1);
    autoBlockManaged=false; blockLagActive=false; cancelled=true;
    BlockVanillaUseHook(&env,0,1); assert(opened==1);
    cancelled=false; BlockMouseKeyHook(&env,0,-99,false); assert(keyWrites==2 && !keyDown);
    module::blockin::active=true;
    BlockVanillaUseHook(&env,0,1); assert(opened==1);
    BlockMouseKeyHook(&env,0,-100,true); assert(keyWrites==2);
    BlockMouseKeyHook(&env,0,-99,true); assert(keyWrites==2);
    BlockMouseKeyHook(&env,0,17,true); assert(keyWrites==3 && keyDown); // Movement remains usable.
    circuit=true;
    BlockVanillaUseHook(&env,0,1); assert(opened==2);
    BlockMouseKeyHook(&env,0,-100,true); assert(keyWrites==4);
    std::puts("Manual block hooks: container interaction, sword fallback, other items, input and active ownership passed");
}
