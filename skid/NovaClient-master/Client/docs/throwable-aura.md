# ThrowableAura

Native C++ module derived from the local Rise 6.9.5 `ThrowableAura.java`, using
Nova's legacy Lunar adapter. Selection remains source-derived; aiming, movement
correction and item-use scheduling use Nova's shared infrastructure.

## Source behavior

| Setting | Default | Bounds |
| --- | --- | --- |
| Rotation Range | 8 | 1-15, step 0.1 |
| Rotation speed | 60 | 1-180, step 1 |
| Range | 6 | 1-12, step 0.1 |
| Minimum Range | 3 | 1-10, step 0.1 |
| Throw Delay | 5 | 0-100, step 1 |
| FOV | 180 | 30-360, step 1 |

All twelve booleans default on: Player Teammates, Players, Mobs, Auto Switch,
Prediction, Module Check, AntiBot, Snowballs, Eggs, Through Walls, ESP, Trajectory.

- Track on every ClientRotationEvent, after existing rotation users. Delay N
  sets the minimum eligible updates between throws; prediction can wait longer
  for stable motion, alignment or a clear trajectory. Waiting never interrupts tracking.
  Use runs only at PrePlayerInteractEvent, after a natural movement update has
  published a sufficiently aligned smooth step. Skipped movement cannot throw
  and never manufactures an extra C05.
- Choose the nearest alive living entity in loaded-entity order, preserving ties.
  Distance is greater than Minimum Range and at most Rotation Range. Actual use
  additionally requires distance at most Range. Only players and IMob are eligible.
- FOV uses the unpredicted point at half eye height. Prediction samples recent
  target positions once per game tick and solves a moving-target intercept,
  accounting for launch offset, projectile gravity and air drag. Airborne target
  motion includes gravity. Direction reversals, teleports and stale samples
  require fresh stable observations before throwing. Actual projectile speed
  remains vanilla; the module adjusts lead and time between throws.
  A manual crosshair hit is not required: SetRotations uses the same smoothing and sensitivity adjustment
  path as KillAura, with SetMoveInputFix(true). Yaw advances from the last angle
  across full revolutions rather than snapping to an atan2-normalized angle.
  The existing input/strafe/jump/motion bridge applies movement correction and
  packet angles. Tracking continues during cooldown and between throws, and
  within Rotation Range even when the target is outside throw Range. Both the
  reported previous direction and the current direction must pass a moving-box
  projectile sweep with vanilla's 0.3 projectile border. Eight of nine fixed
  angle/motion perturbations must hit; these are robustness probes, not a
  calibrated hit probability. The central trajectory also checks world blocks
  and fails closed if collision data cannot be read. Longer flight time and
  uncertain motion increase the minimum interval. With Prediction disabled,
  the existing direct collision-box aim gate and configured delay remain.
- Through Walls=false excludes targets the game says cannot be seen, including
  a fresh selection check immediately before use. With Prediction disabled it
  also checks the projectile path against blocks; failed traces prevent use.
  Through Walls=true permits hidden target selection, but does not remove
  Prediction's ballistic obstruction gate or make projectiles penetrate blocks.
  Player Teammates=true filters
  players whose teams are non-null and formatted-name charAt(1) matches.
- Search hotbar slots 0-8 for snowballs first, then eggs. Without Auto Switch, a
  throwable must already be held. Before use, recheck the world, target, item,
  rotation ownership, native reported angles and slot-order readiness. Call the
  controller once: it synchronizes the slot and sends C08. Do not append an
  attack swing or duplicate C08.
- Keep the selected slot through that tick's movement. Restore and synchronize
  the previous slot during a later safe ClientRotation phase. Disable also uses
  this deferred cleanup. World changes discard old slots; another slot choice
  supersedes restoration. On target loss, stop submitting rotations and let the
  manager return toward the camera naturally, as for KillAura.
- KillAura targets, AutoBlock cycles/pending attacks, packet buffers, active
  AutoRod use, building/mining and open GUIs take priority. Module Check cannot
  disable these ownership guards. Never temporarily clear KillAura's target.
  A normal rotation merely returning to the camera after an idle module is not
  a fresh owner: ThrowableAura may take over that return. Fresh requests and
  direct/linear owners still take precedence. Scaffold enablement always stops
  projectile aim/use, including when it starts between preparation and use.
- ESP and the predicted projectile path render through ImGui independently of
  whether combat currently owns item use, but only while an enabled projectile
  type is available in the hotbar (or held when Auto Switch is off). Without an
  eligible item, hide both visuals and stop tracking/throwing; replenishing items
  resumes automatically. The path is an approximate central
  trajectory, not a guarantee against random spread or future target movement.
  Network delay, sudden direction changes, terrain landings and water drag can
  invalidate the air-based intercept. No measured 90% hit rate is claimed.

## Integration boundaries

The module reuses Nova's AntiBot service; it does not import all Rise BotManager
producers and AntiBot modes. Module Check looks specifically for AimBacktrack;
Nova currently has Backtrack, which is not treated as that different module.
KillAura's attack algorithm and AutoBlock's mode state machines are unchanged.

This is an adapted module, not a claim that the complete Rise client environment
or behavior on every server is identical.

## Verification

`test/combat/throwableaura/run.ps1` compiles the production module together with
the actual RotationManager, RotationUtils and MovementFix against game fixtures,
and the actual native ThrowHeldItem function against JNI fixtures.
Coverage includes defaults, delay, FOV/range and team/bot filters, prediction,
natural movement before use, rotation handoff, combat ownership, skipped ticks,
deferred restoration, world/disable cleanup, a single controller C08 and JNI
failure handling, continuous cooldown/moving-target tracking, unwrapped yaw and
empty/disabled-item visibility and recovery, full-FOV rear acquisition during
idle rotation return, rising/falling throws and mid-aim Scaffold handoff.
Prediction tests cover 108 distance/sideways/approach/vertical-motion scenarios
with independent fine-grained flight sampling, reversal/teleport recovery,
adaptive intervals, blocked paths and failed world traces. Integration tests
also check that obstruction waits resume after valid clear collision data returns.
DLL builds check reflective-loader static TLS compatibility. In-game targeting,
server response and item consumption still need live validation.
