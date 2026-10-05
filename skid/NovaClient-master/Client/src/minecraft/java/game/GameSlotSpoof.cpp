#include "minecraft/java/api/game_slotspoof.h"
#include "minecraft/java/api/game_place.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/compat/minecraft263/Minecraft263Access.h"
#include <atomic>
#include <mutex>

namespace nova::java {
namespace {
struct Mapping {
    const char *inventory, *inventoryDesc, *selected, *item, *itemDesc;
    const char *controller, *controllerDesc, *sync;
};
#define NOVA_SLOT_SPOOF_MAPPING
constexpr Mapping lunar{
#include "minecraft/compat/mappings/Lunar189.inc"
};
constexpr Mapping forge{
#include "minecraft/compat/mappings/forge1201.inc"
};
constexpr Mapping modern{
#include "minecraft/compat/mappings/minecraft263.inc"
};
#undef NOVA_SLOT_SPOOF_MAPPING
std::atomic<int> spoofed{-1};
std::mutex slotMutex;
const void* slotOwner=nullptr;
bool physicalLease=false;
bool yieldable=false;
DWORD physicalWriter=0;
const int scaffoldOwner=0;
const Mapping& Names() { return compat::IsLegacyLunar() ? lunar : compat::UsesModernRuntime() ? modern : forge; }

// No persistent JNI object / class / JNIEnv cache and no static TLS. Each call
// remains safe across player replacement and only affects the local inventory.
struct Frame {
    JniAttach attach;
    JNIEnv* env;
    bool valid;
    explicit Frame(JNIEnv* supplied=nullptr) : env(supplied ? supplied : attach.env),
        valid(env && env->PushLocalFrame(24)==JNI_OK) {}
    ~Frame() { if(valid) { check();env->PopLocalFrame(nullptr); } }
    bool check() { if(!valid)return false; if(env->ExceptionCheck()){env->ExceptionClear();return false;}return true; }
    jobject field(jobject object,const char* name,const char* desc) {
        if(!object || !valid)return nullptr;
        auto cls=env->GetObjectClass(object);
        auto id=cls?env->GetFieldID(cls,name,desc):nullptr;
        return check()&&id?env->GetObjectField(object,id):nullptr;
    }
    jobject inventory(jobject player) {
        if(compat::IsLunar12111()){compat::mc263::access::Frame f(env);return f.keep(f.call(player,compat::mc263::names::Player_getInventory));}
        const auto& n=Names();
        if(compat::IsLegacyLunar())return field(player,n.inventory,n.inventoryDesc);
        if(!player || !valid)return nullptr;
        auto cls=env->GetObjectClass(player);
        auto method=cls?env->GetMethodID(cls,n.inventory,n.inventoryDesc):nullptr;
        return check()&&method?env->CallObjectMethod(player,method):nullptr;
    }
    int physical(jobject inventory) {
        if(compat::IsLunar12111()){compat::mc263::access::Frame f(env);const int slot=f.get<int>(inventory,{"net.minecraft.world.entity.player.Inventory","selected","I"});return f.check()?slot:-1;}
        if(!inventory || !valid)return -1;
        auto cls=env->GetObjectClass(inventory);
        auto id=cls?env->GetFieldID(cls,Names().selected,"I"):nullptr;
        if(!check()||!id)return -1;
        const int slot=env->GetIntField(inventory,id);
        return check()?slot:-1;
    }
    jobject item(jobject inventory,int slot) {
        if(compat::IsLunar12111()){compat::mc263::access::Frame f(env);return slot>=0&&slot<=8?f.keep(f.call(inventory,{"net.minecraft.world.entity.player.Inventory","getItem","(I)Lnet/minecraft/world/item/ItemStack;"},slot)):nullptr;}
        if(!inventory || slot<0 || slot>8 || !valid)return nullptr;
        auto cls=env->GetObjectClass(inventory);
        auto method=cls?env->GetMethodID(cls,Names().item,Names().itemDesc):nullptr;
        return check()&&method?env->CallObjectMethod(inventory,method,slot):nullptr;
    }
    jobject keep(jobject value) {
        if(!check())return nullptr;
        valid=false;return env->PopLocalFrame(value);
    }
};
bool Synchronize() {
    if(compat::IsLunar12111()){
        compat::mc263::access::Frame f;
        return f.invoke(f.get(f.client(),compat::mc263::names::Minecraft_Field_gameMode),{"net.minecraft.client.multiplayer.MultiPlayerGameMode","ensureHasSentCarriedItem","()V"});
    }
    Frame f;
    if(!f.valid)return false;
    auto mc=compat::Minecraft(f.env);
    auto controller=f.field(mc,Names().controller,Names().controllerDesc);
    if(!controller || !f.check())return false;
    auto cls=f.env->GetObjectClass(controller);
    auto method=cls?f.env->GetMethodID(cls,Names().sync,"()V"):nullptr;
    if(!f.check()||!method)return false;
    f.env->CallVoidMethod(controller,method);
    return f.check();
}
}

bool SetSilentSlot(const void* owner,int slot) {
    if(!owner || slot<0 || slot>8)return false;
    {
        std::lock_guard lock(slotMutex);
        if(slotOwner && slotOwner!=owner) {
            if(!yieldable||physicalWriter)return false;
            physicalLease=false;
        }
        if(physicalLease)return false;
        slotOwner=owner;yieldable=false;spoofed.store(slot);
    }
    // Synchronization dispatches game events; never hold the ownership lock there.
    if(Synchronize())return true;
    ReleaseSilentSlot(owner);
    return false;
}
void ReleaseSilentSlot(const void* owner) {
    std::lock_guard lock(slotMutex);
    if(!owner || slotOwner!=owner)return;
    // Release can run from the UI or during respawn, outside the player tick.
    // Let the vanilla controller sync the physical slot in its normal tick path.
    spoofed.store(-1);slotOwner=nullptr;physicalLease=false;physicalWriter=0;yieldable=false;
}
bool AcquirePhysicalSlot(const void* owner) {
    std::lock_guard lock(slotMutex);
    if(!owner)return false;
    if(slotOwner&&slotOwner!=owner) {
        if(!yieldable||physicalWriter)return false;
        spoofed.store(-1);
    }
    if(spoofed.load()>=0)return false;
    slotOwner=owner;physicalLease=true;yieldable=false;return true;
}
bool OwnsSlot(const void* owner) {
    std::lock_guard lock(slotMutex);return owner&&slotOwner==owner;
}
void YieldSlot(const void* owner) {
    std::lock_guard lock(slotMutex);if(owner&&slotOwner==owner&&!physicalWriter)yieldable=true;
}
bool OwnsPhysicalSlot(const void* owner) {
    std::lock_guard lock(slotMutex);return owner&&slotOwner==owner&&physicalLease;
}
bool CanChangePhysicalSlot() {
    std::lock_guard lock(slotMutex);
    return !physicalLease||physicalWriter==GetCurrentThreadId();
}
bool SetOwnedPhysicalSlot(const void* owner,int slot) {
    {
        std::lock_guard lock(slotMutex);
        if(!owner||slotOwner!=owner||!physicalLease||physicalWriter)return false;
        physicalWriter=GetCurrentThreadId();
    }
    struct Reset {~Reset(){std::lock_guard lock(slotMutex);physicalWriter=0;}} reset;
    return SetSelectedSlot(slot);
}
bool SetScaffoldSlot(int slot) {return SetSilentSlot(&scaffoldOwner,slot);}
void ReleaseScaffoldSlot() {ReleaseSilentSlot(&scaffoldOwner);}
bool EffectiveSelectedSlot(int& out) {
    const int slot=spoofed.load();
    if(slot>=0){out=slot;return true;}
    return SelectedSlot(out);
}
jint JNICALL SpoofSlotHook(JNIEnv* env,jclass,jobject inventory) {
    Frame f(env);
    const int physical=f.physical(inventory), slot=spoofed.load();
    if(slot<0 || physical<0)return physical;
    auto player=compat::LocalPlayer(env);
    auto local=f.inventory(player);
    return f.check() && local && env->IsSameObject(local,inventory) ? slot : physical;
}
jobject JNICALL VisualInventoryHandHook(JNIEnv* env,jclass,jobject inventory) {
    Frame f(env);
    return f.keep(f.item(inventory,f.physical(inventory)));
}
jobject JNICALL VisualPlayerHandHook(JNIEnv* env,jclass,jobject player) {
    Frame f(env);
    auto inventory=f.inventory(player);
    return f.keep(f.item(inventory,f.physical(inventory)));
}
}
