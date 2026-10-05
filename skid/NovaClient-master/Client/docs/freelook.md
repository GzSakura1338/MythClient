# FreeLook

Native C++ port of Raven BS `module/impl/render/Freelook.java` and its camera,
mouse, terrain, ActiveRenderInfo and RenderManager integration. No Java source
or helper class is added. Registered in Render → General → FreeLook; settings
use the existing module gear and configuration serializer.

Enable the module, then hold Left Alt by default. The **Key** picker accepts
keyboard/mouse bindings and its existing **Hold / Click** control selects hold
or toggle behavior. The separate module **Bind** still enables/disables the
module. Invert Pitch defaults off; Lock Pitch defaults on (−90°..90°). Custom
FOV defaults off, with 10..150 and default 90 when enabled.

The camera enters rear third person and starts at the player's rotation. Mouse
movement updates private camera yaw/pitch, without rewriting player rotations,
movement input, targeting rays or outgoing rotation packets. Camera-dependent
terrain visibility, billboards and particles use the same camera orientation.

## Version integration

- Lunar 1.8.9: intercept the player-turning focus read in `updateCameraAndRender`
  (the earlier Mac cursor workaround is untouched). Consume MouseHelper deltas
  with Raven's `(sensitivity * .6 + .2)^3 * 8 * .15` scaling. Redirect current
  and previous camera angles in `orientCamera`, `setupTerrain` and its view
  vector. Final RenderManager/ActiveRenderInfo callbacks update billboards and
  particle axes; Lunar's existing mixins have moved the original particle field
  reads into generated wrappers, so vanilla-only redirects would miss them.
- Forge 1.20.1: redirect Camera.setup's view-angle reads and MouseHandler's
  local-player turn call. Native camera collision/third-person positioning and
  render consumers continue using the normal Camera object.
- Minecraft 26.3: equivalent integration in Camera.alignWithEntity and
  MouseHandler.turnPlayer; it is independent of OpenGL/Vulkan presentation.
  Modern versions retain their native mouse smoothing/scaling. Game inversion
  is removed before applying FreeLook's independent Invert Pitch option, and
  their Y delta convention is translated to LWJGL2's direction.

All names/descriptors reside in each version's existing mapping file. Modern
FOV temporarily sets the option's raw value, since its vanilla setter restricts
the range more than Raven's slider. The captured value is restored on exit.
Scaffold's existing final FOV override remains intact and can take precedence.

## Lifecycle differences from the reference

Leaving FreeLook restores the prior perspective, including when disabling the
module (Raven forces first person specifically on disable). Turning Custom FOV
off restores its old value immediately. Hold exits on menus; toggle can remain
active through a menu, with mouse turning suppressed. Focus loss, world/player
replacement, another camera entity, disabling and unload restore owned state.
Key edges are synchronized through menus and rebinds to avoid stuck holds or
phantom reactivation. The source's separate Hold option is represented by Nova's
existing Hold/Click key picker, avoiding two conflicting controls.

## Verification

`test/render/freelook/run.ps1` checks the native state machine, player-rotation
isolation, hold/toggle, inversion/clamping, FOV and cleanup. It verifies every
JNI mapping against local Lunar/Forge/26.3 class bytes and applies the actual
redirects/appends to the original classes, auditing branch/stack-map structure
and native binding descriptors. Shared FOV and NameTags regressions also run.
These are offline checks; injection and visual acceptance are left to the user.
