# Camera yaw rebasing and hand sway

Silent rotation release uses `CorrectDisabledRotations` to move the camera's yaw
by whole revolutions while preserving its view. Previously this updated camera
yaw and previous yaw, but left the hand's smoothed yaw endpoints unchanged.
Vanilla computes hand sway using the difference between interpolated camera yaw
and interpolated hand yaw. A 360-degree representation change therefore created
a visible transient even though the camera had not physically turned.

`SetPlayerCameraRotation` now recognizes pure whole-turn yaw changes with
unchanged pitch, and shifts both hand yaw endpoints by the same amount. Ordinary
aim/mouse turns retain vanilla lag. The shared fix covers callers such as Scaffold
and AutoMLG rather than suppressing their swing animations.

Mappings: legacy `renderArmYaw` / `prevRenderArmYaw`; modern `yBob` / `yBobO`;
Forge 1.20.1 `f_108585_` / `f_108587_`; Lunar 1.21.11 `field_3932` / `field_3931`.
The legacy update formula and modern renderer interpolation are present in local
inspection outputs. Forge names were checked at mappings.dev/1.20.1 and Lunar
fields confirmed in the loaded Recaf class_746. Mapping resolution still follows
the existing per-profile translation path.

Native continuity tests check both signs and multiple whole turns at five render
interpolation points, plus ordinary turns, pitch changes and exclusive MLG-style
rotation return. In-game visual acceptance remains manual; this fixes a confirmed
whole-turn discontinuity and does not establish that every possible equip/swing
animation issue has the same cause.
