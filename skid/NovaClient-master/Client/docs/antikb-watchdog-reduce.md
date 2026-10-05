# AntiKB: Watchdog Reduce

Native C++ implementation of Rise-6.9.5-main's
`src/main/java/com/alan/clients/module/impl/combat/velocity/WatchdogReduceVelocity.java`.
Mode index 3; packet support targets Lunar 1.8.9.

## Reference behavior

- Cancel On Ground / Cancel On Attack / Cancel Explosions default to true.
- Hold every self S12, including zero, downward and purely vertical velocity.
- While the queue is nonempty, also hold S32 transactions. Remote S18 absolute
  positions and S14 relative updates both pass in arrival order, as do other
  entities' velocity and unrelated packets. Unlike the previous port's S18
  capture, this preserves the remote position baseline during and after replay.
- ATTACK synchronously detaches and replays the FIFO inside its outgoing callback.
  An already-cancelled ATTACK also triggers release; INTERACT does not.
- Grounded PRE Motion synchronously releases. Disable and leaving this mode drain
  the pending FIFO. AntiKB does not move input replay to the next tick; the native
  output adapter below keeps generated confirmations at the correct movement boundary.
- Replay publishes fresh incoming events, with a batch-scoped recapture guard.
  A later S12 starts a new batch. Vanilla's conditional attack slowdown can apply
  to the velocity received during that same attack; this module does not scale
  velocity, create attacks, or override KeepSprint.
- Cancel Explosions is unused in the supplied Rise source and remains inert.
- No queue-length release or ordinary timeout. If both release options are off,
  no hidden timer replaces them. JNI allocation failure passes the current packet
  without cancelling it or altering ownership of earlier queued packets.

## Explicitly retained protections

At the user's request, spawn/correction recovery and S08 handling remain:
player age must reach 20 ticks and three consecutive grounded tick heads before
capture begins. S08 applies the preceding FIFO before the correction, then enters
recovery. Disconnect, respawn, join, missing player and player/world replacement
discard obsolete input. These are deliberate differences from Rise.

## AutoBlock owns coexistence and output ordering

AntiKB exposes read-only input ownership/replay/recovery state. It does not call
AutoBlock, drain an AutoBlock queue or alter AutoBlock state.

`AutoBlockPacketSync.cpp::SynchronizePacketDelays` finishes Slinky/Hypixel's valid
output FIFO when AntiKB owns input or is recovering. It discards obsolete output
on disconnect/respawn/world replacement. Both native delay adapters also check
ownership, including nested sends suppressed by the network recursion guard.

No other AutoBlock owner's queue is drained by this policy. The key/block cycle
remains available; only packet delay yields. Standalone AutoBlock and KillAura
share the same implementation.

`LunarRuntime.cpp::StageWatchdogPacket` groups actions up to the next actual C03
while Watchdog holds input. This prevents a synchronously generated C0F from
splitting C0A swing and C02 attack:

- Attack replay before physics: C0F precedes the staged actions and movement.
- Ground PRE Motion after physics: actions and the current C03 precede C0F;
  newly applied velocity belongs to the next physics step.
- If walking emits no C03, tick HEAD sends confirmations before the next physics
  step and retains actions until a real movement. No synthetic movement is sent.
- Slinky/Hypixel cannot restart delay within the staged group. Switching to another
  delay owner waits for this group to finish; otherwise other modes are unchanged.
- S08 preserves valid confirmations but discards obsolete actions; connection or
  world replacement and shutdown discard both. JNI ownership/capacity failure
  fails open and logs the failure. The temporary output limit is 4096 packets;
  this does not impose a timeout/limit on Rise's input FIFO.

## Verification

- `test/combat/antikb/run_antikb_delay.ps1`: actual AntiKB/EventBus, synchronous
  attack/ground/disable, same-attack slowdown, batch boundary, whitelist, cancelled
  events, reentrancy, settings, recovery and JNI cleanup; Epsilon modes regressions.
- `test/combat/antikb/run_watchdog_slinky.ps1`: actual AntiKB, AutoBlock policy and
  native Blink adapters; both start orders, recovery, S08, identity replacement,
  mode isolation and replayed confirmation delivery. The phase fixture also checks
  swing/attack order, pre/post-physics velocity acknowledgement windows, mixed
  releases, skipped movement and cancelled attacks using mocked game/JNI transport.
- `test/combat/antikb/run_prediction.ps1`: existing Prediction and native AB tests.

JNI/game state is mocked in offline tests. Restoring Rise's synchronous timing
is not evidence of zero setbacks on a server; live injection/testing is left to
the user. Native thread scheduling and reference lifetimes still follow Nova's
client-thread pipeline, without generating Java source.
