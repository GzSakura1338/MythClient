# Slinky AutoBlock

Native port of the supplied `skid/slinkyab (1).java` script, not a claim about
the original Slinky client's implementation. Select `Slinky` in either the
standalone AutoBlock settings or KillAura's AutoBlock gear. Both use one shared
engine; standalone takes precedence when both are enabled. Existing mode indices
remain unchanged; Slinky is appended as index 4. Functional runtime is Lunar 1.8.9,
matching the existing sword-block modes. No Java source is generated or injected.

## Settings

All script controls are in the existing settings pane. A few labels have a
`Slinky` prefix to avoid collisions with existing saved Lag-mode properties.

| Setting | Default | Limits |
| --- | --- | --- |
| Range | 3.5 | 1–6, step 0.1 |
| Max Hurt Time | 200 ms | 50–1000, step 5 |
| Max Hold | 150 ms | 50–500, step 5 |
| Block Immediately | On | Otherwise hurtTime must be >0 and <= floor(ms/50) |
| Lag Chance | 0% | 0–100%, step 5 |
| Lag Max Duration | 100 ms | 0–500, step 10 |
| Queue Size | 3 | 1–10 packets |
| Prevent Delaying Attacks | On | Drain before allowing the attack packet |
| Block Again Immediately | On | Reassert use after attack flush |
| Left Click / Right Click | Off / Off | Physical button conditions, combined with AND |
| Damaged Only | Off | Requires observed damage in the window |
| Damage Window | 2000 ms | 500–5000, step 100 |
| Ignore Mobs | On | Reference searches only the player list regardless |
| Ignore Teammates / Ignore Bots | Off / Off | Source color-prefix / name-shape filters |
| Debug | Off | Throttled diagnostic messages in Nova's log |

Nova's shared Force block animation option also applies. All numeric displays
use explicit integer or two-decimal formatting.

## Timing and integration

The engine runs on PRE Motion, corresponding to the script's onPreMotion.
It changes only the held use key; it does not inject an extra use click or direct
C08. Vanilla right-click delay and sword slowdown still apply. No attack-rate,
rotation or NoSlow algorithm is added. KillAura requests still enter the existing
click path, while standalone leaves native attacks alone.

Target range is position-to-position Euclidean distance with a strict `< range`
comparison, not Nova's ordinary eye-to-hitbox range. The script's first color-code
team test and simple name-length/space bot test are retained. Nova's centralized
Teams and AntiBot exclusions apply as well. The source's `client.isFriend` call
has no counterpart because Nova has no friend-list service; no fictitious friend
list is substituted. This is an explicit host-API limitation.

The interaction latch uses a six-block camera ray and the source's registry-name
substring list (containers, doors, gates, switches, etc.). It uses the actual
ray's block position, avoiding truncation of negative world coordinates. Non-sword
items stop this block cycle; subsequent native bow/rod/food input remains usable.

At the hold deadline, the queue starts before releasing use, allowing vanilla's
release packet to enter the queue. Queue capacity evicts/replays the oldest
packet first. Render drains one expired packet per frame; PRE Motion or the next
outgoing packet drains the rest on expiry. Login/handshake packets pass through;
window click/close and entity interactions drain and pass. Attacks do the same
when Prevent Delaying Attacks is enabled; otherwise they can be queued. Keepalive,
transaction and chat packets follow the source's ordinary buffering behavior.
Queue capacity and attack policy are snapshotted when each lag window starts.

The existing Lunar queue retains JNI global references to the connection, packet
and callbacks, and bypasses packet events on replay. It updates the existing
movement boundary and AutoBlock coverage counters only on dispatch. Prediction
can take ownership of the queue; Slinky suspends itself and cannot flush
Prediction's packets. Packet-thread attack handling publishes a notice, and the
client's next PRE Motion rechecks eligibility before changing the use key.

## Deliberate fixes and limits

Unlike the script, losing the target, releasing required input, changing item,
opening a screen, dying, disabling or switching mode cannot leave lagActive
intercepting packets indefinitely. Corrections and respawns use the existing
connection reset sequence; disconnect/connection replacement discard obsolete
packets. Timers are monotonic milliseconds, with tick/frame scheduling granularity.
There is no server acceptance or no-slowdown guarantee.

With AntiKB Watchdog Reduce, input and output delay windows no longer overlap.
Watchdog finishes an existing Slinky FIFO before retaining/replaying its input;
Slinky cannot start buffering during that ownership or spawn/correction recovery.
Block-key timing still runs. Slinky alone and other AntiKB/AutoBlock owners keep
their behavior. See `antikb-watchdog-reduce.md` for lifecycle and recovery details.

The source's UI timing and buffering algorithm are ported with the lifecycle and
host-integration differences above. Debug output uses Nova's log instead of chat.
This does not establish binary or behavioral equivalence to the Slinky client.

## Verification

- `test/combat/autoblock/run_autoblock.ps1`: actual engine timing, defaults,
  mouse/hurt/damage gates, distance, team/bot filters, interaction latch, attack
  notice, invalid-state cleanup, native non-sword input and shared runtime routing;
  existing Lag/Vanilla/Grim regressions also run.
- `test/combat/autoblock/run_hypixel.ps1`: existing Hypixel behavior plus actual
  Slinky property defaults, visibility and mode serialization.
- `test/combat/antikb/run_prediction.ps1`: actual Lunar queue FIFO/capacity,
  callbacks, attack/window/interaction drains, deadline/render behavior, failed
  allocation cleanup, connection replacement and Prediction owner transfer.
- `verify_slinky_bindings.py`: new JNI names/descriptors and packet classes from
  the actual local Lunar bake.zip, including initialization dependency order.
- `test/combat/antikb/run_watchdog_slinky.ps1`: combined production queues,
  both start orders, corrections, spawn fall, lifecycle and owner isolation.

These are offline native/bytecode checks. Live injection and server testing are
left to the user.
