#pragma once

#include "base.h"

namespace nova::java {

bool IsUseItemPacket(jobject packet);

bool IsUseItemOnlyPacket(jobject packet);

bool SendPacketNoEvent(jobject packet);

// A send that still walks the outbound hook, for modules that want their packets
// to look like the game's own.
bool SendPacket(jobject packet);

bool SendStatusOnlyPacket(bool onGround);

bool ConsumePassthrough(JNIEnv* env, jobject packet);
void ReleaseNetwork(JNIEnv* env);

// --- inbound ----------------------------------------------------------------
//
// The packetIn hook runs on the netty I/O thread rather than the game thread.
// The event bus cannot tell inbound interest from outbound interest, so a module
// that wants inbound packets raises this flag while it is enabled and the hook
// stays inert otherwise -- that keeps an uninteresting client from publishing
// anything across threads at all.
//
// It counts rather than latches: several modules can want the stream at once, and
// the count is what stops the first one to disable from cutting off the rest.
void SetIncomingPacketsWanted(bool wanted);
bool IncomingPacketsWanted();

// Hands a packet the hook swallowed on to the game. Called from wherever the
// packet was parked, so it may be a different thread than the hook ran on.
bool ReplayIncoming(JNIEnv* env, jobject packet, jobject listener);

// packet.handle(mc.getConnection()) -- the route a packet takes once it is let
// through, used to replay one that was held back.
bool HandleIncomingPacket(jobject packet);

// The families a velocity-style module parks while it holds a knockback.
bool IsVelocityDelayPacket(jobject packet);
// Epsilon Reduce's exact whitelist (kept separate from legacy Velocity).
bool IsReduceDelayPacket(jobject packet);

bool IsMotionPacket(jobject packet);

bool IsPositionOrExplodePacket(jobject packet);

// The family above, told apart. A module that holds some of these back has to know
// which is which -- a teleport and an explosion both mean "something moved us", but
// only one of them can be held.
bool IsPingPacket(jobject packet);
bool IsExplodePacket(jobject packet);
bool IsPlayerPositionPacket(jobject packet);
bool IsDisconnectPacket(jobject packet);
bool IsRespawnPacket(jobject packet);

// ClientboundSetEntityMotionPacket's id/xa/ya/za.
bool ReadMotionPacket(jobject packet, int& id, int& xa, int& ya, int& za);

}
