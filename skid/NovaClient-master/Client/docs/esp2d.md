# 2DESP

Native C++ port of Rise 6.9.5 `M2DESP`, with Glow/Bloom omitted.
Enable **Render → HUD → 2DESP**; settings are in the module gear.

- Box: Normal (default), CS corners, None.
- Health Bar Mode: Health (default), Standard, Gradient, None.
- Armor Bar: off by default, drawn below the box.
- Targets gear: Player and Player Teammates on; Invisibles, Animals and Mobs off.

The rectangle geometry, quarter-length corners, health/armor placement and
gradient directions follow Rise. Line widths use the game's GUI scale. The
theme palette comes from Nova. Third person includes the local player.
Projection uses the existing camera matrices and interpolated render positions
on the current frame, including Rise's width/height expansion and sneaking offset.
It does not require a visible line of sight to an entity. Armor stands, dead
entities and targets rejected by the enabled AntiBot module are excluded.

The original `Targets` value only controlled setting visibility; Nova replaces
that with its existing gear group. The original unconditional invisible-entity
rejection is removed so Invisibles works. Teammate filtering recognizes the game
scoreboard and Nova Teams. Nova reads native health; Rise's separate optional
HealthBypass module is not part of this port. Invalid health and armor above 20
are handled defensively. Glow is neither registered nor rendered.

Additional native bindings are in the Lunar189, forge1201 and minecraft263
mapping files. `Client/test/render/esp2d/run.ps1` checks these against local game
classes and tests rendering commands, filtering and projection bounds. Builds
and tests do not inject into the game; live visual verification is done by the user.
