# AutoPot

Combat → General → AutoPot. The module is disabled by default. Its settings gear
contains only **Max Health** (1–20 HP, default 12; two HP equal one heart).

At or below the threshold, AutoPot searches the nine hotbar slots for a splash
potion containing instant healing. It prefers the current slot, then the first
matching slot. It selects the potion, synchronizes the held slot, uses the vanilla
controller once, and restores/synchronizes the original slot immediately. It does
not move items out of the main inventory, use drinkable/lingering/regeneration
potions, or change the player's yaw/pitch. Aim the camera yourself when necessary.

Attempts are spaced at least 500 ms apart to allow the splash/health update.
The threshold uses current health, excluding absorption hearts. Screens, ClickGUI,
background game windows, dead players and spectators prevent automatic use.
Leaving a world or disabling the module resets the state. Slot restoration also runs
if selection, synchronization or item use fails.

The implementation is native C++. `GameItemUse.cpp` uses the shared JNI access
cache and each profile's existing mapping file for Lunar 1.8.9, Forge 1.20.1 and
Minecraft 26.3. Potion effects determine eligibility, not the display name. The
vanilla use method owns stack consumption and packet creation; AutoPot adds no
extra use packet, rotation spoof, packet bypass or network queue.

Run `Client/test/combat/autopot/run.ps1` for the native control-flow regression
and member/descriptor validation against original game archives. Game behavior
still needs injection testing by the user.
