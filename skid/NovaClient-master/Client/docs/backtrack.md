# Backtrack

The previous Epsilon port has been replaced. The compact settings are:

- **Delay**: 0–200 ms in 10 ms steps, default 100 ms. Zero drains the queue and stops buffering.
- **Min Range / Max Range**: 0-10 blocks in 0.1-block steps, defaults 0 / 6. Max Range is nested under Min Range. Distances are measured from the local player's eyes to the target box. Both client and tracked server positions must stay inside the inclusive interval. Reversed bounds are normalized; changing the interval releases an excluded target at the next tick head. This does not change attack reach.
- **ESP**: show the tracked server-position box while packets are actually queued, default on.

Old probability, range mode, min/max delay, target filters, tracking buffer, random cooldown and color settings are no longer registered. Existing configuration values for removed settings are ignored; Delay uses its new default until saved. Enabled and Bind remain standard module controls.

## Selection and lifetime

The controller attack hook records the player actually attacked, whether by a physical click or a module. Selection is processed at the next game-tick head; if several attacks occur before that, the latest target wins. There is no nearby-entity scan. Self, nonplayers, removed/dead players, the first ten local-player ticks, and eye-to-box distances outside the configured interval do not qualify.

The attack hook must never replay packets: on 1.8 it runs between the outgoing animation and attack. Replaying a queued transaction there can produce `animation -> confirmation -> attack`, clearing Grim PacketOrderB's swing state and allowing Post to observe an animation before the next movement. Target switching, invalid-target cleanup, zero-delay cleanup and ownership changes requested by an attack now run at the next tick head instead. Regression tests cover each path.

Selection only arms the module. Buffering starts when a subsequent target position update increases its eye-to-box distance. Static targets and transaction packets alone cannot start a window. After activation the whole inbound stream is queued, including transaction/ping, chat and unrelated entities, retaining arrival order. Packets expire individually by age; packet arrivals and game ticks both service expiration. Tick-driven release is quantized by the game tick and can be later under a stalled client.

A window lasts at most 500 ms, even with repeated attacks, then requires a new attack after a fixed 150 ms recovery. No attack for 750 ms, target death/removal, target approaching, or a client or tracked box outside the configured interval ends the window. A target change drains the previous stream before selecting the new player. The queue is bounded to 256 references; reaching the limit flushes and ends the window.

## Grim source findings

The design was checked against the local source under `skid/Grim-2.0/common/src/main/java/ac/grim/grimac/`:

- `events/packets/PacketEntityReplication.java`: position changes are attached to the first and second surrounding transaction confirmations. Delaying only entity movement while allowing later confirmations through gives the client and compensated entity state different timelines.
- `events/packets/PacketPingListener.java` and `player/GrimPlayer.java:addTransactionResponse`: confirmation IDs advance an ordered sent-transaction queue; skipped confirmations feed TransactionOrder.
- `checks/impl/combat/Reach.java`: attacks are checked against compensated entity bounds and viewing direction. A client displaying an older position is not itself permission for a longer hit.
- `checks/impl/timer/Timer.java`: movement timing is checked against the transaction-derived clock. This module does not alter outgoing movement timing, manufacture acknowledgements or send historical-position attacks.

These findings guide bounded ordered buffering; they do not establish that every Grim version/configuration will accept it. Server validation is still required.

## Release and integration

Position corrections, own velocity, explosions and zero-health updates flush the old stream before the trigger passes through. These barriers are also inspected inside modern bundle packets; bundles themselves remain indivisible. Disconnect/respawn/world replacement discard old-world references. Disable replays pending packets; destruction frees remaining references.

Backtrack's receive handler runs before AntiKB. Own velocity ends Backtrack first, allowing AntiKB to process the actual velocity event. Existing AntiKB and Velocity delay/attack ownership blocks new Backtrack buffering. Outbound AutoBlock/Blink logic is unchanged.

Queue mutation and replay submission are serialized, with a reentrancy guard. JNI/game lookups for target geometry run on the game thread; packet decoding uses the sampled target ID, player ID and box. ESP uses that snapshot and the existing shared camera projection. Replay retains Nova's existing vanilla packet-handler scheduling; the tests validate submission order, not live JVM executor timing.

Lunar 1.8.9, Forge 1.20.1 and Minecraft 26.3 retain their existing attack hook and version-specific position decoder. Removed filter mappings/helpers were deleted. No other JAR was opened in Recaf.

## Validation

- `Client/test/combat/backtrack/run.ps1`: actual module and EventBus with mock JNI/clock. Checks custom range limits (including reversed bounds, equality and edits during buffering), attacked-player selection, receding-only activation, FIFO deadlines including chat/confirmations, window/attack timeout, recovery, switching, critical barriers, ownership, world cleanup, zero delay, allocation failure and queue bounds.
- `Client/test/combat/backtrack/audit_mapping.py`: remaining Forge and 26.3 bindings verified against the installed JARs.
- `Client/test/combat/antikb/run_antikb_delay.ps1`: existing Delay/Reduce regression suite.
- Release DLL built at `Client/build-vs2026/Release/NovaClientBase.dll`. Injection and in-game validation are performed by the user.
