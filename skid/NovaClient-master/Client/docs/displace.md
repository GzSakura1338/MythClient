# KBDisplace

Native C++ port of `skid/raven-bs/src/main/java/keystrokesmod/module/impl/combat/Displace.java`.
The module is under Combat. Runtime adapters currently target Lunar 1.8.9; the
settings remain visible on other profiles but no legacy game logic runs there.
No Java source is added. The reference arrow, fade animation and Show direction
setting are intentionally omitted at the user's request.

## Settings

| Setting | Default / range |
| --- | --- |
| Dynamic angle | Static; Static / Dynamic |
| Yaw offset | 90; 0–180, step 1; Static only |
| Delay | 0 ms; 0–500, step 50 |
| Direction | Left; Left / Right; Static only |
| Find void | Off; Static only |
| Blink | Off |
| Ignore teammates | On |
| Has knockback | Off |
| Item whitelist | Off |
| Whitelisted items | Empty; visible when Item whitelist is on |

The whitelist uses Nova's text setting instead of Raven's item picker. Separate
Raven storage IDs with spaces, commas or semicolons, for example
`minecraft:stick, minecraft:wool:14, minecraft:wool:*, @category:sword, @fist`.
Supported categories: sword, bow, tool, axe, pickaxe, shovel, hoe, shears.
Damage values are part of an ID only for items with subtypes that are not damageable.

The source's item-condition expression is preserved verbatim in meaning:
`(!HasKnockback || enchanted) || (!ItemWhitelist || matches)`.
Enabling only one of the two conditions therefore does not filter anything;
enabling both accepts either an enchanted item or a whitelist match.

## Behavioral correspondence

- `onClientRotation`: native redirect before `EntityRenderer.getMouseOver` inside
  `Minecraft.runTick`, guarded once per tick, with player-update fallback.
  Attack binding state or KillAura's selected target enables target searching.
- `CombatTargeting.findClosestTarget(9.0, ...)`: 9 is squared distance from the
  eye to the collision-border-expanded AABB, not a nine-block range. No extra
  FOV, line-of-sight, hurt-time, ground or sword-only gate is added.
- Dead/deathTime players are rejected. The teammate check follows Raven's
  scoreboard / first two UTF-16 display-name characters / formatted network-name
  rule, independent of Nova's Teams toggle. AntiBot uses Nova's existing service;
  Nova has no friend list (equivalent to an empty Raven friend list). Those global
  service/configuration differences are not a claim of full-client equivalence.
- `shouldDisplaceInCurrentWindow`: independent ten-tick windows per entity ID.
  The first displacement in a new window is immediate; remaining attempts wait
  `ceil(delay/50)` ticks. Dead/unloaded targets are pruned.
- `findStaticVoidYaw`: 32 directions, 12 rings at 0.5 blocks, first qualifying
  ring, source score/tie order and near-target aim radius retained.
- `findDynamicVoidYaw`: all 32 paths, 6-block reach, 0.5-block steps, three lanes
  at 0.45 spacing, center weight 1.4 and consecutive-center bonus. Collision checks
  sample every 0.25 blocks with a 0.03 horizontal inset. They invoke each block's
  actual collision-list method; unloaded columns and Y outside 0–255 obstruct.
- `isVoidColumn`: air-only scan from floor(Y)-1 through start-10 inclusive,
  stopping at Y=0. Air results are cached only within one rotation computation.
- Rotation alternates each eligible tick. The non-displacement tick after one
  displacement queues `KeyBinding.onTick`, including custom attack bindings.
  Yaw unwrap and sensitivity quantization use the reference formulas; no extra
  smoothing speed or randomized yaw is introduced (Raven's global random factor
  defaults to zero). Pitch from other rotation logic is retained and quantized.
  Mouse-over yaw/pitch overrides are scoped to `getMouseOver`, including render
  calls, and do not replace ordinary item-use rays or local attack yaw.
- `onPostInput`: reference movement-fix search executes before the module's input
  adjustment. Unenchanted moving players receive raw forward=1 on displacement,
  then raw strafe=-1/+1 on the compensation tick, even while sneaking. Existing
  rotation correction is skipped only for this module's already-corrected input.
- `onSendPacket` / `onGameTick`: the first C03 in a displacement tick arms an
  outbound-only FIFO. It includes that movement and subsequent output, including
  chat/transactions, and releases at the next tick HEAD or module disable. It does
  not release another module's queue. Already-delayed output from other native
  owners passes through the same final dispatch gate when those owners release it.

Nova's lifecycle protections discard the queue on correction, disconnect,
respawn, player/world/manager replacement or unload. JNI allocation failure
releases owned references and fails open. These are intentional native safety
adaptations, rather than additional configurable gameplay behavior.

## Layout and validation

All feature implementation is in `src/minecraft/modules/combat/displace/`: `DisplaceLogic.h`
holds the reference algorithm, `Displace.cpp` owns settings/events/JNI adapters,
and `DisplacePackets.*` owns outbound lifetime. Lunar members remain consolidated
in `src/minecraft/compat/mappings/Lunar189.inc`.

Run `test/combat/displace/run.ps1`. It checks 4,400 state transitions against an
independent reference transcription, geometry/void/wall boundaries, movement and
quantization, the actual outbound FIFO and partial JNI reference failures, original
Lunar member descriptors, and the original Minecraft class rewritten with all
runTick redirects/appends. Existing Watchdog/Slinky/Hypixel and Prediction tests
also cover output dispatch with Displace disabled. These are offline checks;
live injection and gameplay testing are left to the user.
