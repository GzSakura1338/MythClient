# F11 and render surface recovery

The native swap hook accepts the game render thread's replacement OpenGL context.
It resolves the HWND on each swap rather than binding the UI permanently to the
first HGLRC/HWND. No Java sources or injected Java helpers are involved.

- `src/minecraft/host/render/OverlayHost.cpp` coordinates Minecraft window rebinding and renderer recreation.
- `src/ui/backend/Win32Surface.h` owns the Win32 platform backend and input subclass.
  `WM_NCDESTROY` invalidates the binding even if Windows later reuses the HWND.
  Common-controls subclassing removes only Nova's input callback and preserves
  other window subclasses.
- `src/ui/backend/OpenGLSurface.h` remembers the owning HGLRC and its latest HDC.
  On replacement, GPU objects are deleted only while the owning context is current.
  If that context is gone, the backend abandons identifiers without GL deletion.
  The incoming context is restored even after a failed attempt to bind the old one.
- The OpenGL backend's `Shutdown(context_lost)` extension retains CPU texture data
  and invalidates uploaded IDs. Font atlases, skin textures and music covers are
  uploaded again through ImGui's existing texture lifecycle. Blur buffers and GL
  entry-point caches are also reset when changing contexts.

Ordinary window resizing does not recreate the renderer. Replacing just the HWND
rebinds the platform and input callbacks; the ImGui context, menus, HUD placement
and module/config state survive. Minimized or temporarily empty surfaces skip
drawing. Vulkan's existing swapchain resource lifecycle remains in its backend.

## Verification

Build Release first, then run `Client/test/render/surface/run.ps1`. It uses the
real ImGui/OpenGL backends and private hidden Windows/WGL windows, without
attaching to Minecraft. Both compatibility and core contexts are exercised.

The fixture covers resize, window-only replacement, context-only replacement,
loss of both, a destroyed context whose window is still valid, safe subclass
removal, and twelve repeated context/window switches. Offscreen framebuffer
readback checks geometry, text, skin and cover pixels after every transition;
blur cache recreation and preservation of unrelated new-context textures are
also checked. This does not replace in-game F11 validation by the user.
