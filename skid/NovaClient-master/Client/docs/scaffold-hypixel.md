# Scaffold Hypixel rotation

## Native implementation

- `src/minecraft/modules/player/scaffold/Scaffold.cpp`: owns all Scaffold rotation implementation,
  including Hypixel Normal, Telly candidate scoring, independent yaw/pitch steps,
  mode transitions, and the effective-ray placement checks. Internal angle helpers
  are at the top of the file; the module methods below own game state and calls.
- `src/minecraft/modules/player/scaffold/Scaffold.h`: only exposes the shared enabled-state query.
- `test/player/scaffold/run.ps1`: extracts the actual helper and module methods from
  `Scaffold.cpp` for native regression tests, also registered in the offline runner.
  The unused legacy `FindRotation` search and its isolated tests have been removed;
  shared smoothing checks remain in the Telly angle test fixture.

## Normal bridging: selection and placement

Outside Telly with Snap disabled, Hypixel follows Rise Normal: entering the mode
seeds camera yaw minus 180 degrees and pitch 90, including a live Telly-to-Normal
switch. This aim is submitted every normal tick, including supported-ground ticks
and temporarily missing targets, without waiting for the first successful ray.

When placement is needed, a matching effective ray preserves the stored aim.
Otherwise the existing ComputeHypixelRotation performs Rise D(0)'s horizontal
rays at the candidate's fractional height. Rays use camera-relative yaw offsets,
and validated intersection angles are ranked by squared yaw/pitch change from
the effective rotation. Nova supplies an explicit ray origin without moving the
live player. The horizontal scan reaches 5.5 blocks; face validation and actual
placement retain Nova's 4.5-block reach.

When the scan fails, a still-valid stored angle is retained; otherwise the face
center supplies the preparatory angle. A failed validation blocks placement,
but never cancels this turn. The 45 Degrees option filters searched and geometric
angles. Snap remains an explicit separate choice. The old fixed world-yaw,
nine-face-sample search is no longer used by normal Hypixel rotation.

Immediately before placement, the effective smoothed rotation is traced again.
The block, face, and location from that trace are passed to `UseItemOnBlock` together.
This normal-bridge path always uses strict face validation. The Normal/Strict
RayCast selector is available in Telly and the other rotation modes.

## Snap (non-Telly only)

Snap has one switch; `Snap Dist` has been removed. Selecting Telly disables the
Snap path even if Jump Sprint is off. Rotation speed and RayCast controls are
hidden while Snap is active: the mode uses an immediate shared rotation request
and always checks the exact block face. Other Scaffold options remain available.

The ordinary pre-tick search supplies a candidate. Unsupported feet with a usable
stack trigger placement; grounded movement also looks up to two motion ticks
ahead (capped at 0.7 blocks) when approaching an empty column. This lets a required
turn-only preparation happen before support is lost. The search uses that future
foot position, but placement rays always start at the actual current eye. A face
still hidden behind the edge permits only preparation, never an interaction.
Stationary movement, held jump, airborne movement and fully supported forecasts
do not use the grounded forecast. The face center and up to eight interior points are tried with the
shared sensitivity correction and continuous yaw origin. While moving, each point
also supplies a yaw aligned to the nearest of the eight movement directions and
a camera-yaw candidate. All candidates must pass the same face trace (at most 27
search traces plus the final effective-angle trace). After sensitivity correction,
the selector evaluates continuous yaw deltas using the preceding look update.
On Lunar 1.8.9, successful native dispatch and delayed replay supply the actual
C08/C03/C05/C06 sequence. A right-click rejected by Lunar before C08 does not
consume history, even though a local Motion Post still runs. The observer never
changes, sends or cancels packets; a mutex protects its small native snapshot.
World changes clear that snapshot; module toggles do not erase observed history.
Other profiles retain the local Motion Post approximation.
An alternate actual face point must pass the same trace. A potentially repeated
future delta is a selection penalty, not grounds for deleting the sole valid
current ray. An unchanged look does not create a new rotation delta at all.
The current stored-delta guard still defers an unsafe interaction. The selector scores actual
`FixMovement` output against the requested world travel direction, then favors
forward input among almost equally aligned rays. This avoids the old first-valid
face point's unnecessary lateral drift. Every former geometric candidate remains
available, including when the additional rays fail. The effective rotation
must trace the selected block and face, within the existing 4.5-block reach.
The resulting ray hit, rather than a random point, is used for the interaction.

Snap holds a shared exclusive rotation through the normal walking update. A
matching Motion Post marks that boundary; the following pre-tick restores the
live camera and releases ownership. A new placement can start on that tick if
needed. Missing walking callbacks time out after two ticks without repeating the
placement. Disable, world changes, AutoMLG handoff and inventory failures release
owned state; another exclusive owner is never overwritten. Movement correction
stays enabled. Snap does not enter the legacy Clutch burst/tick-skip path or send
extra movement packets. Swing and Item Spoof retain their existing behavior.

Relevant local Grim sources under `skid/Grim-2.0/common/src/main/java/ac/grim/grimac/`:

- `checks/impl/scaffolding/RotationPlace.java`: post-flying ray validation uses
  the placement tick's pitch; restoring immediately after clicking is too early.
  After recent failures, its pre-flying check can also reject a placement.
- `checks/impl/misc/Post.java`: interactions and movement/transaction ordering
  matter; no fabricated move-place-return packet sequence is introduced.
- `checks/impl/packetorder/PacketOrderO.java`: modern client actions must respect
  the movement/tick-end boundary.
- `predictionengine/predictions/PredictionEngine.java`, `loopVectors`: outside
  swimming, sprinting predictions require forward input. Modern known inputs
  further constrain that search. A rear-face placement on a straight bridge
  generally cannot preserve forward input while retaining the same world travel
  direction; forcing sprint would contradict that prediction.
- `checks/impl/scaffolding/DuplicateRotPlace.java`: compares the absolute yaw
  deltas of placements, not pitch changes or ray validity. Repeated fixed-yaw
  snaps in the initial movement-alignment optimization triggered this check.
  `CheckManagerListener.handleFlying` first drains queued placements, then calls
  `onRotationUpdate`. Thus this check consumes the PRECEDING look delta. Changing
  the incoming turn alone cannot repair a repeated delta already stored there.
  Snap now completes a normal turn-only tick in that case and retries placement
  on a subsequent tick, preparing before the edge when grounded. Candidate selection prepares a valid face angle for that
  sequence without adding fake movement packets or resetting yaw modulo 360.

Native regressions cover Snap's Telly exclusion, ray rejection, single interaction,
Post matching, live-camera return, timeout, ownership/slot gates and absence of
burst packets, alongside the existing Telly and placement suites. Motion Post is
a local callback, not a server acknowledgement. Actual server acceptance and
movement speed require in-game testing; native tests simulate dispatch/replay,
not the server's actual network and world prediction. The intended speed benefit comes from shorter periods
facing the placement surface, not an increased movement multiplier.

The native cube-ray comparison covers 2,160 valid combinations of faces, offsets,
camera headings, W/A/D input and sensitivity. For the first placement without
prior history in that fixture, 1,536 improve the
heading error by over one degree; mean acceleration projection along intended
travel changes from 0.974485 to 0.997314. These numbers measure direction retention,
not a fraction of full sprint speed. The suite also verifies that a straight
bridge's rear face does not produce a fictitious forward-sprint solution. This
optimization cannot promise near-full sprint in all layouts. Sprint still needs
normal game conditions and sprint input; SafeWalk/sneak still imposes its slowdown.

Geometry-only regressions found alternate points for 1,383 large-delta cases and
exercise another 128 translated bridge positions, including unbounded yaw. These
are candidate-search checks, not server-order validation. The separate lifecycle
test reproduces one flag with the old order, then runs 256 ticks with 232 emitted
placements and zero modeled DuplicateRotPlace flags using the production methods.
It covers local cancellation, position-only packets, camera returns, delayed
dispatch and repeated-look packets (which still update rotation with zero delta).
No local observer can know pre-injection server history or server-side rejection;
live server acceptance still needs manual validation.

The moving bridge regression combines production target search, strict cube
traces, Snap lifecycle and packet history with a 0.6-wide player footprint and
the zero-vertical-motion tick before vanilla falling acceleration. It covers six
headings and four horizontal step lengths over 160 ticks per combination. Starting
with equal prior placement/return deltas reproduced two falls in 24 cases with
the old timing/filter, versus zero with advance preparation; the fixed runs also
produce zero modeled DuplicateRotPlace flags. This is a local deterministic
collision test, not a full Minecraft server or proof against lag/knockback.

### Recaf evidence: installed Lunar 1.8.9

The 2026-09-28 `profiles/1.8/logs/ichor-boot.log` identifies the active baked game
resource as `.lunarclient/offline/multiver/cache/72c13d03/484384a2/bake.zip`
(18,872 classes). Recaf analysis used this processed Lunar resource, not only
`lunar.jar` (which does not contain the transformed Minecraft game classes).

- `Minecraft.runTick`: right-click handling precedes `WorldClient.updateEntities`.
- `EntityPlayerSP.onUpdate`: ordinary player update precedes walking update.
- `EntityPlayerSP.onUpdateWalkingPlayer`: compares yaw/pitch to lastReported;
  chooses C06 for position+look, C04 for position, C05 for look, plain C03 for
  neither; writes lastReported angles after sending and only when look changes.
  Recaf bytecode confirms these comparisons and write order.
- `PlayerControllerMP.onPlayerRightClick`: Lunar's cancellable HEAD handler can
  return without a C08; otherwise slot synchronization and validation precede
  C08. Local item use follows C08, so a false result can also occur after sending.
- `C03PacketPlayer` nested classes: C05/C06 set `rotating`; C03/C04 do not.
- `EntityPlayerSP.updateEntityActionState`: arm yaw/pitch approach camera angles
  by half of their difference, retaining previous arm endpoints. This confirms
  the shared whole-turn camera/arm rebase fix must remain intact.

## Telly and rescue

Hypixel Telly exposes `RotSpeed` and `BackSpeed`. `RotSpeed` scales both the
initial 122-128 degree placement step and subsequent 35-38 degree steps relative
to its default 127, preserving the existing cadence. `BackSpeed` independently
controls the return toward the travel direction. Rescue and stopping retain
their existing minimum/immediate turn speeds. `RotSpeed2` remains Heypixel-only.

Select Hypixel with both Telly and Jump Sprint enabled. This uses a native
adaptation of Rise 6.9.5 Watchdog Telly; no Java implementation is generated.

Ground movement returns toward the input direction at pitch 85 using BackSpeed,
including diagonal movement while holding jump. Nova applies its rotation to both
movement-input correction and the sprint-jump impulse: skipping ground return can
convert forward input into side/back input and point the jump boost away from
travel. Ground ticks arm the pending return boost; placement aiming resumes in
the air. Rise's Block Diagonal Ascend ground-branch exclusion is therefore not
used with Nova's current movement bridge.

Before a forward jump, the highest-priority movement-input callback refreshes the
takeoff angle from the newly sampled input, before the rotation bridge corrects
movement and vanilla evaluates sprint/jump. PreTick only had the previous input:
starting from idle or changing direction on landing could otherwise leave a stale
rearward angle and turn forward into side/back input for one tick. The correction
invalidates that tick's cached smoothing and submits the travel yaw at pitch 85
with budget 360, retaining shared sensitivity correction. BackSpeed still controls
the ground preparation turn, but cannot delay the direction of an actual takeoff.
This refresh requires a placeable block, Hypixel Telly, grounded forward jump and
no sneak; higher-priority rotations keep ownership. Airborne aiming is unchanged.

Missing targets do not cancel preparation: yaw -135 relative to the camera and
pitch 82 seed the initial target, and preparation can continue at step 127. The
pending boost survives until the effective airborne rotation actually hits the
placement target. A valid target angle alone does not consume it: if the first
122-128 degree step falls short, or shared smoothing retains an earlier angle,
the next tick keeps the fast step. Subsequent steps use 35-38. A higher
priority rotation owner prevents submission and does not consume the boost.

The search retains a still-valid target angle. Otherwise it tests four relative
yaw candidates (camera +45, camera +90, camera, movement +45), snaps to 45 degrees,
and removes duplicates. Each yaw tests seven pitches around 82, then candidates
are ranked by squared yaw/pitch change with Rise's small camera-yaw tie breaker.
There are at most 28 candidate rays plus retained-angle and placement checks.
The geometric face angle remains a preparatory fallback; Place independently
validates the effective rotation before clicking. The existing 45 Degrees filter
applies to retained and scored candidates, as with the legacy geometric fallback.

Rise applies independent yaw/pitch steps, followed by a RotationComponent whose
speed argument is multiplied by 36. Nova computes the axis steps explicitly and
submits that intermediate target with budget 360 for shared sensitivity correction;
it does not apply the old shared 35-degree vector budget again.

With Clutch disabled, downward motion below -0.1 after airborne tick 3 raises
the ordinary turn step to at least 127. Falling also bypasses the TellyTicks wait.
There is still only one ordinary placement attempt and the effective-ray check
remains mandatory. This does not enable tick skipping, recursive burst placement,
or extra movement packets. Rise's automatic RayCast Off switching and optional
falling raycast bypass are not used. Rise/Heypixel Telly keep their prior flow.

Telly Clutch retains the existing burst rescue path when explicitly enabled.

With Clutch disabled, Hypixel Telly keeps its takeoff-layer restriction while
rising or level. Once airborne vertical velocity is below -0.1, it uses the same
candidate selection, full yaw/pitch search, RotSpeed smoothing and placement body
as the existing Rise mode. There is no additional takeoff-height threshold,
bounding-box dependency or predicted-footprint filter during this falling phase.
Hypixel candidate search uses Rise's default Same Y = Off. Telly alone does not
restrict candidates to the takeoff layer. Nova's Rise/Heypixel height policy and
the existing placement/rotation timing remain unchanged.

Falling refreshes cached preparation smoothing and bypasses TellyTicks, including
enabling the module in mid-fall. It still requires the effective Normal/Strict
placement ray and makes one ordinary placement attempt, with no tick skipping or
Clutch rotation packets. Higher-priority rotation owners still take precedence.
Ground takeoff and rising preparation retain Hypixel's movement and rotation
logic. Explicit Clutch and the other rotation modes retain their existing flow.

Target-block selection now replaces the old column/spiral and eye-distance
heuristics directly inside Scaffold.cpp::GetBlockInfo with Rise 6.9.5's
PlayerUtil.a algorithm. Ordinary placement uses its zero offset: scan an 11-cube
around the player, generate six neighbours of each non-replaceable permitted
origin, retain replaceable candidates within five blocks, and stable-sort by
distance to (playerX, playerY - 1, playerZ). Fractional coordinates and generation
order are retained, including equal-distance ties. Per-search block reads are
cached; there is no alternative legacy selector or separate Rise class.

Candidate origins exclude Rise's chest, trapped chest, anvil, crafting table,
furnace/lit furnace, hopper, dropper, dispenser, note block and jukebox. Native
adapters compare the registry ID in the game's Block/BlockState description;
unknown/failed descriptions are excluded. The filter is only applied when
creating candidates, exactly as in Rise, not repeated during support selection.
Hypixel leaves Rise's optional fixed-layer argument unset, matching Same Y = Off,
including moving Telly and held-jump searches. Other rotation modes retain their
existing fixed-layer policy. Nova does not expose Rise's Expand or Downwards
settings; neither mode is added here. Forecast offsets keep the player's original
scan origin and five-block distance filter, and only change the desired foot and
scan radius, as in Rise.

Horizontal supports are considered in z-, z+, x-, x+ order and stable-sorted
using the source's literal MathUtil.n/yaw expression. Only when none exist are
floor and ceiling supports used, followed by the prior-facing fallback. A failed
ray check does not initiate a second candidate search. The rotation algorithms,
movement controls and final placement ray gates remain those described above.

HitVector now samples the whole selected face uniformly and prefers the actual
current-rotation block ray intersection when both support and face match. Normal
and burst paths both use this method; bounded Hypixel placement already passes
the validated intersection directly. Native placement reach remains 4.5 blocks.

Outside Telly, Hypixel Clutch makes one rescue attempt per tick using the effective rotation and
the same face/hit validation as ordinary placement. It sends that rotation before
the placement. It no longer recursively advances tick logic or skips player ticks.
Its speed parameter is 180. This trades the previous multi-attempt burst for
consistent tick timing in normal bridging only.

## Silent slot / local hand

Scaffold now requests an effective interaction slot through
`src/minecraft/java/game/GameSlotSpoof.cpp`, following Rise's SlotComponent separation.
The `Item Spoof` toggle in Scaffold's settings defaults to on. With it on,
the physical inventory selection is never overwritten. Turning it off uses visible
slot selection; toggling it back on restores the prior physical slot. Inventory held-stack reads
and controller slot synchronization use the requested block slot for the local
player only; first-person item rendering explicitly reads the physical selection.
Scrolling changes the visible item, while placement keeps its block slot. Releasing
Scaffold lets the vanilla controller restore the server to the current physical
selection on its normal tick, without sending a slot packet from the UI thread.
No-block ticks, invalid-player ticks, disable and respawn release the request.
Shared controller synchronization avoids redundant held-slot packets.

Lunar uses InventoryPlayer.getCurrentItem and the existing controller sync path.
Forge 1.20.1 also redirects the MAINHAND inventory write used when an item is
replaced, keeping depleted-stack updates off the physical slot. Minecraft 26.3
redirects both getSelectedItem and setSelectedItem; FirstPersonHandsAndItems reads
the physical item for its tick and hand-selection logic. These mappings are in the
three existing `.inc` files; no Java source or additional HUD is introduced.
Swing controls the local arm animation; successful placements still notify the
server when the animation is disabled.

`test/player/scaffold/run_slotspoof.ps1` exercises the production request/release,
effective-slot and visual-slot callbacks with a native fixture. It patches original
local Forge and 26.3 class bytes to verify actual field/call sites and resulting
bytecode structure. Lunar's injection sites still require validation on the user's
live Lunar distribution; its selected classes can vary with installed patches.

## Verification scope

The native tests cover retained angles, failed search, candidate bounds, wrapped
yaw ranking, geometric fallback, the cardinal-angle filter, and invalid input.
They also exercise production smoothing at zero angular distance, which must stay
finite. DLL builds additionally check reflective-loader static TLS compatibility.
The Telly fixture compiles production routing, preparation, and placement methods
to cover delayed-target boosts, effective-ray misses across multiple ticks and
cached smoothing, takeoff-layer lifecycle through the production tick method,
diagonal sprint takeoff using production movement
correction and jump-yaw handling, falling catches with
Clutch off, higher-priority rotation ownership, and the Normal/Strict distinction.
The real movement-input method is also exercised after PreTick with stale input
and cached smoothing, covering idle starts, W+A/D, automatic/manual jumps, low
BackSpeed and landing direction changes, plus the takeoff correction's guards.
Watchdog tests exercise candidate scoring, yaw wrap, search limits, independent
axis steps and their composition with production sensitivity smoothing.
The placement fixture compiles the production block-search and hit-vector methods
and covers Rise nearest-distance selection, stable ties, horizontal/floor/ceiling
priority, blacklist stage, negative coordinates, fixed height, Clutch-off ordinary
jump height limits, descending selection parity with Rise, upward
candidates in four diagonal quadrants, and actual ray-hit
replacement. These deliberately replace the old footprint/spiral expectations.
The same fixture also executes production target selection, angle search,
OverBlock, shared rotation smoothing and Place against ray/cube intersections:
240 Rise/Hypixel falling comparisons cover side supports, lower floors, view
angles, jump input and both ray modes. It separately covers enabling in mid-fall
without a stored rotation. This verifies local control flow; server acceptance
and real-world continuity still require manual testing.
In-game placement continuity and Clutch recovery require manual testing; the finite
face samples are not an exhaustive search of all possible visible angles.

## Keep FOV and block reserve

`Keep FOV` is a Scaffold sub-setting, off by default. Its `FOV` slider appears
when enabled (30-120, default 90). Native camera return hooks fix the world FOV
while Scaffold is enabled, without changing saved game options or hand FOV.
Enabling, changing the target and disabling use a time-based smooth zoom.
Disabling either switch smoothly returns to vanilla FOV computation. Lunar 1.8.9, Forge
1.20.1 and Minecraft 26.3 have profile-specific camera mappings.

Scaffold keeps one block in every hotbar stack. Selection skips stacks at one,
and placement checks the reserve again, including burst/Clutch paths. If no
usable stack has more than one block, Scaffold releases its slot and stops
placing. This reduces empty-stack desync; it cannot guarantee server acceptance
or eliminate network lag.

`test/player/scaffold/run_fov.ps1` validates production FOV callbacks and patches
original Forge/26.3 camera bytes for structural validation. `run.ps1` checks
reserve selection and the last-block placement guard.
