#include "minecraft/modules/combat/displace/DisplacePackets.h"
#include "minecraft/compat/lunar189/LunarRuntime.h"
#include "minecraft/java/api/game_entity.h"
#include <mutex>
#include <vector>

namespace nova::module::displace {
namespace {
struct Packet { jobject manager, packet, callbacks; };
std::mutex mutex;
bool blinking=false;
std::vector<Packet> packets;
java::EntityHandle player,world;
void Drain(JNIEnv* env,bool send) {
    blinking=false;
    for(const auto& p:packets) {
        if(send)compat::lunar::SendDisplacePacket(env,p.manager,p.packet,p.callbacks);
        env->DeleteGlobalRef(p.manager);env->DeleteGlobalRef(p.packet);
        if(p.callbacks)env->DeleteGlobalRef(p.callbacks);
    }
    packets.clear();player.Reset();world.Reset();
}
}
void BeginBlink() {
    auto self=java::PlayerEntity(),level=java::LevelEntity();
    if(!self||!level)return;
    std::lock_guard lock(mutex);
    if(blinking)return;
    player=std::move(self);world=std::move(level);blinking=true;
}
void EndBlink(bool send) {
    JniAttach attach;if(!attach)return;
    auto self=java::PlayerEntity(),level=java::LevelEntity();
    std::lock_guard lock(mutex);
    Drain(attach.env,send&&self&&level&&player.IsSame(self)&&world.IsSame(level));
}
bool HoldOutput(JNIEnv* env,jobject manager,jobject packet,jobject callbacks) {
    std::lock_guard lock(mutex);
    if(!blinking)return false;
    if(!packets.empty()&&!env->IsSameObject(manager,packets.front().manager)) {
        Drain(env,false);return false;
    }
    Packet p{env->NewGlobalRef(manager),env->NewGlobalRef(packet),callbacks?env->NewGlobalRef(callbacks):nullptr};
    if(env->ExceptionCheck()||!p.manager||!p.packet||(callbacks&&!p.callbacks)) {
        env->ExceptionClear();
        if(p.manager)env->DeleteGlobalRef(p.manager);
        if(p.packet)env->DeleteGlobalRef(p.packet);
        if(p.callbacks)env->DeleteGlobalRef(p.callbacks);
        Drain(env,true);return false;
    }
    packets.push_back(p);return true;
}
}
