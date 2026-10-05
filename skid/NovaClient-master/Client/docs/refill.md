# Refill

Player → General → Refill is a one-shot module. Enable it (or use its standard
module key binding) to fill empty hotbar slots with instant-healing splash potions
from main inventory slots 9–35. It does not replace occupied hotbar slots.

Refill uses InvManager's `inventory::Snapshot`, `FirstEmptyHotbarSlot`, `Timer`,
Gaussian delay policy and `ClickMenuSlot(menuId, source, hotbarIndex, Swap)` path.
Each delayed action reads a fresh snapshot and rechecks both live stacks before
clicking. A no-op/failed click stops the session instead of retrying indefinitely.
At most one stack moves per game tick, even when delay is zero.

The settings gear contains:

- **Min Delay**: lower bound between moves, default 125 ms.
- **Delay**: base interval, default 150 ms; sampled with the same 50 ms Gaussian
  variation as InvManager, clamped to Min Delay. The first move waits this interval
  after opening the inventory as well.
- **Close Delay**: wait before finishing after the next scan finds no more work,
  default 100 ms.

The module opens the real inventory screen only if both a potion and a free hotbar
slot exist. Once full or out of potions it closes the inventory it opened and
disables itself. An inventory already opened by the user stays open. ClickGUI
defers starting; other screens, a held cursor stack, focus loss during the session,
death/world replacement, or manually closing/replacing the inventory cancel it.
Refill only closes the exact screen it opened, with the original player/world
still present and the cursor empty.

InvManager gives Refill priority: its arrangement and packet-driven inventory
closing pause during the session, then resume afterward. Its discard pass now
keeps healing splash potions so the newly refilled stacks are not thrown away.
AutoPot's existing screen guard suspends throwing while the inventory is open.

Potion detection is shared with AutoPot and reads effects, not display names.
All code is native C++; the screen JNI mappings remain in each existing version
mapping file for Lunar 1.8.9, Forge 1.20.1 and Minecraft 26.3.

Run `Client/test/player/refill/run.ps1` for native session tests and verification
of the new JNI members against original game archives. In-game testing belongs
to the user; a valid click response is not proof of server acceptance.
