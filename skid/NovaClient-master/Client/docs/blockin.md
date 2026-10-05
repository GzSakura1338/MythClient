# BlockIn

Native C++ port of `skid/raven-bs`'s `Block In`, under **Player**. The current
game adapter targets Lunar 1.8.9; other profiles display the module but do not
run its placement logic. No Java source or helper JAR is added.

Enable the module, then hold **Select Keybind** with building blocks in the
hotbar. Release the key to stop and restore the original slot. The module
prioritizes the roof, then the eight surrounding positions at feet/head height.
The ring at the crosshair counts these nine protected positions.

Settings match the reference defaults:

- Speed: 10 (1–30).
- Randomization: 10% (0–100%).
- Rotation Tolerance: 25° (20–100°).
- Select Keybind: unbound; physical hold required.
- Ignore blocks: off. Its gear contains **Items**, a comma/space separated
  list of registry identifiers, e.g. `minecraft:tnt, minecraft:wool:*`.
  A metadata suffix selects one variant, e.g. `minecraft:wool:14`.

The search preserves Raven's roof-first order, nearest-enemy side preference,
five expansion layers, support face order, grid jitter, 4.5-block ray reach,
angular-cost sorting, strong adjacent/weak support block selection, smooth
rotation and placement tolerance. Mouse input is reserved while placing;
movement keys remain usable and rotation movement correction is enabled.

Nova uses its existing native place/swing functions and sensitivity correction.
The ignore list uses Nova's text editor instead of Raven's item picker, and the
progress ring uses Nova HUD colors/opacity. World changes, GUI opening, release,
failed reads and loss of rotation ownership invalidate queued placement. The
per-selection caches avoid repeating JNI reads during the expanding search.

Validation: `Client/test/player/blockin/run.ps1` tests the real C++ planner with
voxel ray traces, verifies mappings against the locally cached Lunar classes,
and exercises the production runtime's hold gates, queued placement consumption,
world/slot changes, rotation ownership and cleanup with mocked game access.
Live placement is tested by the user.
