# AutoRod

Combat → General → AutoRod. Enable the module, assign its **Key**, then hold that
key to use snowballs, eggs or a fishing rod repeatedly. Like FreeLook, it has a separate
Key; AutoRod enforces **Hold** even if an older configuration saved Toggle.
No key is assigned by default. The module's ordinary Bind still toggles its enable
switch; Key controls item use.

While AutoRod is enabled, ThrowableAura pauses its automatic projectile throws,
even when AutoRod's Key is released or ThrowableAura's Module Check is off.
AutoRod itself has no target-detection trigger. Disable AutoRod to allow
ThrowableAura to resume its independently enabled behavior.

**Delay** sets the attempt interval (50–2000 ms, default 500 ms). Release stops
further uses. Screens, ClickGUI, loss of focus, dead/spectator players and leaving
the world stop activation; press the key again afterward. Failed use attempts are
also rate limited. Missing items cause no slot change or use packet.

**Switch Delay** is the wait after selecting a different slot and before using
its item (0–1000 ms, default 0). No wait is needed when already holding the chosen
item. Releasing the key or manually switching slots during the wait cancels use.

**Rod Return Delay** keeps the rod held after a cast before using it again to reel
in (50–2000 ms, default 500 ms). Only continued holding of Key permits this second
use; releasing cancels further use. The ordinary Delay starts again after reeling.
The original slot is restored afterward. Snowballs and eggs do not use this timer.

Only hotbar slots 0–8 are searched, strictly from left to right. The leftmost
eligible slot wins even when the current hand holds another eligible item.
Potions are excluded; item identity and
positive stack count determine eligibility. Both supported items follow their
normal use behavior, including reeling in an already cast rod.

AutoRod shares AutoPot's native controller use. It selects, synchronizes and uses
once, then restores/synchronizes in a later action interval. On Lunar, a passive
outbound observer requires an actually dispatched movement boundary after other
gameplay actions; merely reaching the next game tick is insufficient. This also
observes delayed packet replay. It never switches back immediately after
use in the same tick. Restoration also runs after failures, key release, module
disable or opening menus; a manually changed slot is preserved. World changes
discard the old pending slot. It does not aim, select targets, move inventory items or create extra
use packets. The shared controller mappings cover Lunar 1.8.9, Forge 1.20.1 and
Minecraft 26.3. All implementation code is native C++.

Run `Client/test/combat/autorod/run.ps1` for control-flow tests and
`Client/test/combat/autopot/run.ps1` for the shared transaction/mapping regressions.
Actual in-game behavior requires the user's injection test.
