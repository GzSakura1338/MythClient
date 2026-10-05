# GhostHand

`Mode` selects two independent behaviors. Missing mode in an old configuration
defaults to `Chest`.

- **Chest** keeps the existing Starain behavior: aim at a loaded chest within
  4.5 blocks and right-click through obstructing blocks.
- **Player** ports Raven BS `GhostHand`: after vanilla cursor picking, select
  the block behind the nearest eligible entity. Normal Minecraft left/right
  click handling then acts on that block; this mode does not emit its own
  attack/use packets or change rotation.

Player settings are grouped behind four gear buttons:

| Group | Defaults |
| --- | --- |
| Interact through | Non-player entities and enemies enabled; bots and friendlies disabled |
| Preferred targets | Everything enabled; Bed and Next to bed disabled |
| Allow while using | Sword disabled; tools, fists, buckets, flint and steel, cobweb and other enabled |
| Conditions | Both mouse-button requirements and Not holding a sword disabled |

Bed/Next to bed scan the entire ray up to vanilla reach, giving the first
matching preferred block priority over the first ordinary block and entity
filter. Without a preferred hit, only the **nearest** entity is checked; a
disallowed entity cannot be skipped to reach an allowed entity farther away.
This matches Raven's selection order and defaults.

Nova uses its shared AntiBot and Teams modules for classification. It has no
Raven friend-list service, so Friendlies currently means entities recognized by
Teams. FreeCam, a game screen, or ClickGUI suspends the Player override.

Native adapters cover Lunar 1.7.10/1.8.9, Forge 1.20.1, the supported GLFW
modern profiles including Lunar 1.21.11, and 26.3. Legacy uses block collision
ray tracing; modern versions use their outline VoxelShape. 26.3's removed
HoeItem subclass is handled through the untranslated item key. No Java source
or helper class is added.

Verification:

- `Client/test/misc/ghosthand/run.ps1`: Chest regression and click hook audit.
- `Client/test/misc/ghosthand/player.ps1`: real class-file mapping audit, ray
  boundary/diagonal tests, and cursor-tail bytecode/stack-map checks.
- `Client/test/lunar12111/run.ps1`: translated Lunar JNI and hook audit.

Game verification after reinjection: select Player, hold an allowed item
(empty hand is allowed by default), and aim at a block behind an enemy or
non-player entity. Toggle Enemies off to confirm the entity blocks the cursor
again. Enable Bed to test priority through a covering block. Switching back
to Chest restores the original chest interaction behavior.
