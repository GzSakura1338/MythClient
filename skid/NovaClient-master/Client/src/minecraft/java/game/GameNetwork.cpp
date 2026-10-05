
#include "minecraft/java/api/game_network.h"
#include "minecraft/java/api/game_freecam.h"
#include "minecraft/java/api/game_backtrack.h"
#include "minecraft/java/api/game_hud.h"
#include "minecraft/java/api/game_teams.h"
#include "minecraft/java/api/game_player_list.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/compat/lunar189/LunarRuntime.h"
#include "minecraft/compat/minecraft263/Minecraft263Game.h"

#include <atomic>
#include <vector>

#include "minecraft/java/api/jni_lookup.h"

namespace nova::java {
namespace {

#define NOVA_FORGE1201_NETWORK
#include "minecraft/compat/mappings/forge1201.inc"
#undef NOVA_FORGE1201_NETWORK

struct Handles {
    jclass    minecraft = nullptr;
    jmethodID getInstance = nullptr;
    jmethodID getConnection = nullptr;
    jmethodID send = nullptr;
    jclass    statusOnly = nullptr;
    jmethodID statusCtor = nullptr;
    jclass    useItem = nullptr;
    jclass    useItemOn = nullptr;
};

Handles g_net;
bool g_netLogged = false;

std::vector<jobject> g_passthrough;
SRWLOCK g_passthroughLock = SRWLOCK_INIT;

struct PassthroughGuard {
    PassthroughGuard() { AcquireSRWLockExclusive(&g_passthroughLock); }
    ~PassthroughGuard() { ReleaseSRWLockExclusive(&g_passthroughLock); }
    PassthroughGuard(const PassthroughGuard&) = delete;
    PassthroughGuard& operator=(const PassthroughGuard&) = delete;
};

bool Resolve(JNIEnv* env) {
    jobject loader = GetMinecraftClassLoader(env);
    jclass minecraft = nova_base::loadOrFind(env, loader, kMinecraftDot, kMinecraftSig);
    jclass listener = nova_base::loadOrFind(env, loader, kListenerDot, kListenerSig);
    jclass useItem = nova_base::loadOrFind(env, loader, kUseItemDot, kUseItemSig);
    jclass useItemOn = nova_base::loadOrFind(env, loader, kUseItemOnDot, kUseItemOnSig);
    jclass statusOnly = nova_base::loadOrFind(env, loader, kStatusOnlyDot, kStatusOnlySig);
    if (loader)
        env->DeleteGlobalRef(loader);

    if (!minecraft || !listener || !useItem || !useItemOn || !statusOnly) {
        if (minecraft) env->DeleteLocalRef(minecraft);
        if (listener) env->DeleteLocalRef(listener);
        if (useItem) env->DeleteLocalRef(useItem);
        if (useItemOn) env->DeleteLocalRef(useItemOn);
        if (statusOnly) env->DeleteLocalRef(statusOnly);
        return false;
    }

    jmethodID getInstance = env->GetStaticMethodID(minecraft, kGetInstance, kGetInstanceDesc);
    jmethodID getConnection = env->GetMethodID(minecraft, kGetConnection, kGetConnectionDesc);
    jmethodID send = env->GetMethodID(listener, kSend, kSendDesc);
    jmethodID statusCtor = env->GetMethodID(statusOnly, kStatusCtor, kStatusCtorDesc);
    if (env->ExceptionCheck())
        env->ExceptionClear();

    if (!getInstance || !getConnection || !send || !statusCtor) {
        if (!g_netLogged) {
            g_netLogged = true;
            LogTo("Base: packets unavailable (getInstance=%d, getConnection=%d, send=%d, "
                  "statusCtor=%d)",
                  getInstance != nullptr, getConnection != nullptr, send != nullptr,
                  statusCtor != nullptr);
        }
        env->DeleteLocalRef(minecraft);
        env->DeleteLocalRef(listener);
        env->DeleteLocalRef(useItem);
        env->DeleteLocalRef(useItemOn);
        env->DeleteLocalRef(statusOnly);
        return false;
    }

    g_net.minecraft = static_cast<jclass>(env->NewGlobalRef(minecraft));
    g_net.statusOnly = static_cast<jclass>(env->NewGlobalRef(statusOnly));
    g_net.useItem = static_cast<jclass>(env->NewGlobalRef(useItem));
    g_net.useItemOn = static_cast<jclass>(env->NewGlobalRef(useItemOn));
    g_net.getInstance = getInstance;
    g_net.getConnection = getConnection;
    g_net.send = send;
    g_net.statusCtor = statusCtor;

    env->DeleteLocalRef(minecraft);
    env->DeleteLocalRef(listener);
    env->DeleteLocalRef(useItem);
    env->DeleteLocalRef(useItemOn);
    env->DeleteLocalRef(statusOnly);
    return g_net.minecraft && g_net.statusOnly && g_net.useItem && g_net.useItemOn;
}

jobject CurrentConnection(JNIEnv* env) {
    jobject instance = env->CallStaticObjectMethod(g_net.minecraft, g_net.getInstance);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
    }
    if (instance == nullptr)
        return nullptr;

    jobject connection = env->CallObjectMethod(instance, g_net.getConnection);
    env->DeleteLocalRef(instance);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
    }
    return connection;
}

bool TakePassthrough(JNIEnv* env, jobject packet) {
    if (!env || packet == nullptr)
        return false;

    PassthroughGuard guard;
    for (auto it = g_passthrough.begin(); it != g_passthrough.end(); ++it) {
        if (env->IsSameObject(*it, packet)) {
            env->DeleteGlobalRef(*it);
            g_passthrough.erase(it);
            return true;
        }
    }
    return false;
}

// --- inbound ---------------------------------------------------------------

// ClientboundSetEntityMotionPacket's four int accessors. The numbers are not in
// field order -- SRG assigns them by name -- so these were resolved through the
// official named table rather than guessed from the declaration order.

struct IncomingHandles {
    jclass    connection = nullptr;
    jmethodID genericsFtw = nullptr;
    jclass    packet = nullptr;
    jmethodID handle = nullptr;

    jclass    motion = nullptr;
    jmethodID motionId = nullptr;
    jmethodID motionXa = nullptr;
    jmethodID motionYa = nullptr;
    jmethodID motionZa = nullptr;

    jclass    moveEntity = nullptr;
    jclass    teleport = nullptr;
    jclass    ping = nullptr;
    jclass    lookAt = nullptr;
    jclass    playerPosition = nullptr;
    jclass    explode = nullptr;
    jclass    disconnect = nullptr;
    jclass    respawn = nullptr;
};

IncomingHandles g_in;
bool g_inLogged = false;

// A count, not a flag: more than one module can want the inbound stream, and a
// plain bool lets whichever disables first switch the hook off under the other.
std::atomic<int> g_wantIncoming{0};

jclass LoadGamePacket(JNIEnv* env, jobject loader, const char* name) {
    const std::string dot = std::string(kGamePacketDir) + name;
    const std::string sig = std::string(kGamePacketSigPrefix) + name + ";";
    return nova_base::loadOrFind(env, loader, dot.c_str(), sig.c_str());
}

bool ResolveIncoming(JNIEnv* env) {
    if (!env)
        return false;
    if (g_in.connection && g_in.motion)
        return true;

    jobject loader = GetMinecraftClassLoader(env);
    jclass connection = nova_base::loadOrFind(env, loader, kConnectionDot, kConnectionSig);
    jclass packet = nova_base::loadOrFind(env, loader, kPacketDot, kPacketSig);
    jclass motion = LoadGamePacket(env, loader, kMotionName);
    jclass moveEntity = LoadGamePacket(env, loader, kMoveEntityName);
    jclass teleport = LoadGamePacket(env, loader, kTeleportName);
    jclass ping = LoadGamePacket(env, loader, kPingName);
    jclass lookAt = LoadGamePacket(env, loader, kLookAtName);
    jclass playerPosition = LoadGamePacket(env, loader, kPositionName);
    jclass explode = LoadGamePacket(env, loader, kExplodeName);
    jclass disconnect = LoadGamePacket(env, loader, kDisconnectName);
    jclass respawn = LoadGamePacket(env, loader, kRespawnName);
    if (loader)
        env->DeleteGlobalRef(loader);

    jmethodID genericsFtw =
        connection ? env->GetStaticMethodID(connection, kGenericsFtw, kGenericsFtwDesc) : nullptr;
    jmethodID handle = packet ? env->GetMethodID(packet, kPacketHandle, kPacketHandleDesc) : nullptr;
    jmethodID motionId = motion ? env->GetMethodID(motion, kMotionId, kIntGetterDesc) : nullptr;
    jmethodID motionXa = motion ? env->GetMethodID(motion, kMotionXa, kIntGetterDesc) : nullptr;
    jmethodID motionYa = motion ? env->GetMethodID(motion, kMotionYa, kIntGetterDesc) : nullptr;
    jmethodID motionZa = motion ? env->GetMethodID(motion, kMotionZa, kIntGetterDesc) : nullptr;
    if (env->ExceptionCheck())
        env->ExceptionClear();

    const bool ok = connection && packet && genericsFtw && handle && motion && moveEntity &&
                    teleport && ping && lookAt && playerPosition && explode && disconnect &&
                    respawn && motionId && motionXa && motionYa && motionZa;
    if (!ok) {
        if (!g_inLogged) {
            g_inLogged = true;
            LogTo("Base: inbound packets unavailable (conn=%d packet=%d fwd=%d handle=%d "
                  "motion=%d/%d/%d/%d/%d move=%d tp=%d ping=%d look=%d pos=%d explode=%d)",
                  connection != nullptr, packet != nullptr, genericsFtw != nullptr,
                  handle != nullptr, motion != nullptr, motionId != nullptr, motionXa != nullptr,
                  motionYa != nullptr, motionZa != nullptr, moveEntity != nullptr,
                  teleport != nullptr, ping != nullptr, lookAt != nullptr,
                  playerPosition != nullptr, explode != nullptr);
        }
    } else {
        g_in.connection = static_cast<jclass>(env->NewGlobalRef(connection));
        g_in.packet = static_cast<jclass>(env->NewGlobalRef(packet));
        g_in.motion = static_cast<jclass>(env->NewGlobalRef(motion));
        g_in.moveEntity = static_cast<jclass>(env->NewGlobalRef(moveEntity));
        g_in.teleport = static_cast<jclass>(env->NewGlobalRef(teleport));
        g_in.ping = static_cast<jclass>(env->NewGlobalRef(ping));
        g_in.lookAt = static_cast<jclass>(env->NewGlobalRef(lookAt));
        g_in.playerPosition = static_cast<jclass>(env->NewGlobalRef(playerPosition));
        g_in.explode = static_cast<jclass>(env->NewGlobalRef(explode));
        g_in.disconnect = static_cast<jclass>(env->NewGlobalRef(disconnect));
        g_in.respawn = static_cast<jclass>(env->NewGlobalRef(respawn));
        g_in.genericsFtw = genericsFtw;
        g_in.handle = handle;
        g_in.motionId = motionId;
        g_in.motionXa = motionXa;
        g_in.motionYa = motionYa;
        g_in.motionZa = motionZa;
    }

    if (connection) env->DeleteLocalRef(connection);
    if (packet) env->DeleteLocalRef(packet);
    if (motion) env->DeleteLocalRef(motion);
    if (moveEntity) env->DeleteLocalRef(moveEntity);
    if (teleport) env->DeleteLocalRef(teleport);
    if (ping) env->DeleteLocalRef(ping);
    if (lookAt) env->DeleteLocalRef(lookAt);
    if (playerPosition) env->DeleteLocalRef(playerPosition);
    if (explode) env->DeleteLocalRef(explode);
    if (disconnect) env->DeleteLocalRef(disconnect);
    if (respawn) env->DeleteLocalRef(respawn);
    return ok && g_in.connection && g_in.motion;
}

bool IsPacketType(JNIEnv* env, jobject packet, jclass type) {
    if (!env || !packet || !type)
        return false;
    const jboolean result = env->IsInstanceOf(packet, type);
    if (env->ExceptionCheck())
        env->ExceptionClear();
    return result == JNI_TRUE;
}

}

bool ConsumePassthrough(JNIEnv* env, jobject packet) {
    return TakePassthrough(env, packet);
}

void ReleaseNetwork(JNIEnv* env) {
    if (!env) return;
    ReleaseBacktrackAccess(env);
    ReleaseHudAccess(env);
    ReleaseTeamsAccess(env);
    ReleasePlayerListAccess(env);
    {
        PassthroughGuard guard;
        for (jobject packet : g_passthrough) env->DeleteGlobalRef(packet);
        g_passthrough.clear();
    }
    const jclass classes[] = {g_net.minecraft, g_net.statusOnly, g_net.useItem, g_net.useItemOn,
        g_in.connection, g_in.packet, g_in.motion, g_in.moveEntity, g_in.teleport, g_in.ping,
        g_in.lookAt, g_in.playerPosition, g_in.explode, g_in.disconnect, g_in.respawn};
    for (jclass type : classes) if (type) env->DeleteGlobalRef(type);
    g_net = {};
    g_in = {};
    g_wantIncoming.store(0, std::memory_order_release);
}

bool IsUseItemPacket(jobject packet) {
    if (compat::UsesModernRuntime()) return compat::mc263::IsPacket(packet, compat::mc263::PacketKind::UseItem);
    if (compat::IsLegacyLunar()) return compat::lunar::IsPacket(packet, compat::lunar::PacketKind::UseItem);
    if (packet == nullptr)
        return false;

    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!g_net.useItem && !Resolve(env))
        return false;

    if (env->IsInstanceOf(packet, g_net.useItem) == JNI_TRUE)
        return true;
    if (env->ExceptionCheck())
        env->ExceptionClear();
    const jboolean onBlock = env->IsInstanceOf(packet, g_net.useItemOn);
    if (env->ExceptionCheck())
        env->ExceptionClear();
    return onBlock == JNI_TRUE;
}

bool IsUseItemOnlyPacket(jobject packet) {
    if (compat::UsesModernRuntime()) return compat::mc263::IsPacket(packet, compat::mc263::PacketKind::UseItemOnly);
    if (compat::IsLegacyLunar()) return compat::lunar::IsPacket(packet, compat::lunar::PacketKind::UseItemOnly);
    if (packet == nullptr)
        return false;

    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!g_net.useItem && !Resolve(env))
        return false;

    const jboolean result = env->IsInstanceOf(packet, g_net.useItem);
    if (env->ExceptionCheck())
        env->ExceptionClear();
    return result == JNI_TRUE;
}

bool SendPacketNoEvent(jobject packet) {
    if (FreeCamActive()) {
        JniAttach attach;
        if (!attach || FreeCamBlocksPacket(attach.env, packet)) return false;
    }
    if (compat::UsesModernRuntime()) {
        JniAttach attach;
        if (!attach || !packet) return false;
        jobject kept = attach.env->NewGlobalRef(packet);
        if (!kept) { if (attach.env->ExceptionCheck()) attach.env->ExceptionClear(); return false; }
        try {
            PassthroughGuard guard;
            g_passthrough.push_back(kept);
        } catch (...) {
            attach.env->DeleteGlobalRef(kept);
            return false;
        }
        const bool sent = compat::mc263::SendPacket(packet);
        if (!sent) TakePassthrough(attach.env, packet);
        return sent;
    }
    if (compat::IsLegacyLunar()) {
        JniAttach attach;
        if (!attach || !packet) return false;
        jobject kept = attach.env->NewGlobalRef(packet);
        if (!kept) { if (attach.env->ExceptionCheck()) attach.env->ExceptionClear(); return false; }
        try {
            PassthroughGuard guard;
            g_passthrough.push_back(kept);
        } catch (...) {
            attach.env->DeleteGlobalRef(kept);
            return false;
        }
        const bool sent = compat::lunar::SendPacket(packet);
        if (!sent) TakePassthrough(attach.env, packet);
        return sent;
    }
    if (packet == nullptr)
        return false;

    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!g_net.send && !Resolve(env))
        return false;

    jobject connection = CurrentConnection(env);
    if (connection == nullptr)
        return false;

    jobject kept = env->NewGlobalRef(packet);
    if (kept == nullptr) {
        env->DeleteLocalRef(connection);
        return false;
    }
    {
        PassthroughGuard guard;
        g_passthrough.push_back(kept);
    }

    env->CallVoidMethod(connection, g_net.send, packet);
    env->DeleteLocalRef(connection);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        TakePassthrough(env, packet);
        return false;
    }
    return true;
}

bool SendStatusOnlyPacket(bool onGround) {
    if (compat::UsesModernRuntime()) return compat::mc263::SendStatusOnlyPacket(onGround);
    if (compat::IsLegacyLunar()) return compat::lunar::SendStatusOnlyPacket(onGround);
    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!g_net.statusOnly && !Resolve(env))
        return false;

    jobject packet = env->NewObject(g_net.statusOnly, g_net.statusCtor,
                                    onGround ? JNI_TRUE : JNI_FALSE);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (packet == nullptr)
        return false;

    const bool sent = SendPacketNoEvent(packet);
    env->DeleteLocalRef(packet);
    return sent;
}

// A plain send: it goes out through the listener exactly as the game would, so it
// still passes through the outbound hook and anything listening to it. The
// no-event variant above exists for the opposite case -- a packet that module
// machinery itself is re-injecting and must not see again.
bool SendPacket(jobject packet) {
    if (FreeCamActive()) {
        JniAttach attach;
        if (!attach || FreeCamBlocksPacket(attach.env, packet)) return false;
    }
    if (compat::UsesModernRuntime()) return compat::mc263::SendPacket(packet);
    if (compat::IsLegacyLunar()) return compat::lunar::SendPacket(packet);
    if (packet == nullptr)
        return false;

    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!g_net.minecraft && !Resolve(env))
        return false;

    jobject listener = CurrentConnection(env);
    if (listener == nullptr)
        return false;

    env->CallVoidMethod(listener, g_net.send, packet);
    env->DeleteLocalRef(listener);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return true;
}

void SetIncomingPacketsWanted(bool wanted) {
    if (wanted) {
        g_wantIncoming.fetch_add(1, std::memory_order_acq_rel);
        return;
    }
    // Never below zero: a module that disables twice, or disables without having
    // enabled, must not be able to cancel somebody else's request.
    int current = g_wantIncoming.load(std::memory_order_acquire);
    while (current > 0 &&
           !g_wantIncoming.compare_exchange_weak(current, current - 1,
                                                 std::memory_order_acq_rel)) {
    }
}

bool IncomingPacketsWanted() {
    return g_wantIncoming.load(std::memory_order_acquire) > 0;
}

bool ReplayIncoming(JNIEnv* env, jobject packet, jobject listener) {
    if (compat::UsesModernRuntime()) return compat::mc263::ReplayIncoming(env, packet, listener);
    if (compat::IsLegacyLunar()) return compat::lunar::ReplayIncoming(env, packet, listener);
    if (env == nullptr || packet == nullptr || listener == nullptr)
        return false;
    if (!g_in.genericsFtw && !ResolveIncoming(env))
        return false;

    env->CallStaticVoidMethod(g_in.connection, g_in.genericsFtw, packet, listener);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return true;
}

bool HandleIncomingPacket(jobject packet) {
    if (compat::UsesModernRuntime()) return compat::mc263::HandleIncomingPacket(packet);
    if (compat::IsLegacyLunar()) return compat::lunar::HandleIncomingPacket(packet);
    if (packet == nullptr)
        return false;

    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!g_in.handle && !ResolveIncoming(env))
        return false;
    if (!g_net.minecraft && !Resolve(env))
        return false;

    jobject listener = CurrentConnection(env);
    if (listener == nullptr)
        return false;

    env->CallVoidMethod(packet, g_in.handle, listener);
    env->DeleteLocalRef(listener);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return true;
}

bool IsVelocityDelayPacket(jobject packet) {
    if (compat::UsesModernRuntime()) return compat::mc263::IsPacket(packet, compat::mc263::PacketKind::VelocityDelay);
    if (compat::IsLegacyLunar()) return compat::lunar::IsPacket(packet, compat::lunar::PacketKind::VelocityDelay);
    if (packet == nullptr)
        return false;

    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!g_in.motion && !ResolveIncoming(env))
        return false;

    return IsPacketType(env, packet, g_in.motion) ||
           IsPacketType(env, packet, g_in.moveEntity) ||
           IsPacketType(env, packet, g_in.teleport) ||
           IsPacketType(env, packet, g_in.ping) ||
           IsPacketType(env, packet, g_in.lookAt) ||
           IsPacketType(env, packet, g_in.playerPosition);
}

bool IsReduceDelayPacket(jobject packet) {
    if (compat::UsesModernRuntime())
        return compat::mc263::IsPacket(packet, compat::mc263::PacketKind::ReduceDelay);
    return IsVelocityDelayPacket(packet);
}

bool IsMotionPacket(jobject packet) {
    if (compat::UsesModernRuntime()) return compat::mc263::IsPacket(packet, compat::mc263::PacketKind::Motion);
    if (compat::IsLegacyLunar()) return compat::lunar::IsPacket(packet, compat::lunar::PacketKind::Motion);
    if (packet == nullptr)
        return false;

    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!g_in.motion && !ResolveIncoming(env))
        return false;

    return IsPacketType(env, packet, g_in.motion);
}

bool IsPositionOrExplodePacket(jobject packet) {
    if (compat::UsesModernRuntime()) return compat::mc263::IsPacket(packet, compat::mc263::PacketKind::PositionOrExplode);
    if (compat::IsLegacyLunar()) return compat::lunar::IsPacket(packet, compat::lunar::PacketKind::PositionOrExplode);
    if (packet == nullptr)
        return false;

    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!g_in.motion && !ResolveIncoming(env))
        return false;

    return IsPacketType(env, packet, g_in.playerPosition) ||
           IsPacketType(env, packet, g_in.explode);
}

// The individual members of the family above, for a caller that has to tell them
// apart rather than treat them all as "something that moves us".
bool IsPingPacket(jobject packet) {
    if (compat::UsesModernRuntime()) return compat::mc263::IsPacket(packet, compat::mc263::PacketKind::Ping);
    if (compat::IsLegacyLunar()) return compat::lunar::IsPacket(packet, compat::lunar::PacketKind::Ping);
    if (packet == nullptr)
        return false;
    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;
    if (!g_in.motion && !ResolveIncoming(env))
        return false;
    return IsPacketType(env, packet, g_in.ping);
}

bool IsExplodePacket(jobject packet) {
    if (compat::UsesModernRuntime()) return compat::mc263::IsPacket(packet, compat::mc263::PacketKind::Explode);
    if (compat::IsLegacyLunar()) return compat::lunar::IsPacket(packet, compat::lunar::PacketKind::Explode);
    if (packet == nullptr)
        return false;
    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;
    if (!g_in.motion && !ResolveIncoming(env))
        return false;
    return IsPacketType(env, packet, g_in.explode);
}

bool IsPlayerPositionPacket(jobject packet) {
    if (compat::UsesModernRuntime()) return compat::mc263::IsPacket(packet, compat::mc263::PacketKind::Position);
    if (compat::IsLegacyLunar()) return compat::lunar::IsPacket(packet, compat::lunar::PacketKind::Position);
    if (packet == nullptr)
        return false;
    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;
    if (!g_in.motion && !ResolveIncoming(env))
        return false;
    return IsPacketType(env, packet, g_in.playerPosition);
}

bool IsDisconnectPacket(jobject packet) {
    if (compat::UsesModernRuntime()) return compat::mc263::IsPacket(packet, compat::mc263::PacketKind::Disconnect);
    if (compat::IsLegacyLunar()) return compat::lunar::IsPacket(packet, compat::lunar::PacketKind::Disconnect);
    if (packet == nullptr)
        return false;
    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;
    if (!g_in.motion && !ResolveIncoming(env))
        return false;
    return IsPacketType(env, packet, g_in.disconnect);
}

bool IsRespawnPacket(jobject packet) {
    if (compat::UsesModernRuntime()) return compat::mc263::IsPacket(packet, compat::mc263::PacketKind::Respawn);
    if (compat::IsLegacyLunar()) return compat::lunar::IsPacket(packet, compat::lunar::PacketKind::Respawn);
    if (packet == nullptr)
        return false;
    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;
    if (!g_in.motion && !ResolveIncoming(env))
        return false;
    return IsPacketType(env, packet, g_in.respawn);
}

bool ReadMotionPacket(jobject packet, int& id, int& xa, int& ya, int& za) {
    if (compat::UsesModernRuntime()) return compat::mc263::ReadMotionPacket(packet, id, xa, ya, za);
    if (compat::IsLegacyLunar()) return compat::lunar::ReadMotionPacket(packet, id, xa, ya, za);
    if (packet == nullptr)
        return false;

    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!g_in.motionId && !ResolveIncoming(env))
        return false;

    id = env->CallIntMethod(packet, g_in.motionId);
    xa = env->CallIntMethod(packet, g_in.motionXa);
    ya = env->CallIntMethod(packet, g_in.motionYa);
    za = env->CallIntMethod(packet, g_in.motionZa);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return true;
}

}
