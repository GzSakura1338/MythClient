# Nova embedded browser

Native C++ browser service shared by Minecraft and CS2. The game DLL links
`NovaChromeClient`; the bundled **WebView2 fixed runtime** runs in its own process
tree. No installed Chrome/Edge is required or used as a fallback.

## Build

1. Run `tools/get-webview2.ps1` for the pinned Microsoft x64 runtime and SDK.
2. Build `NovaBrowserHost`, `NovaBrowserSmoke`, and `NovaClientBase` in Release.
3. `tools/build-injector.cmd` packages `build-vs2026/browser-webview2/Release`.
   Only one runtime is embedded; CEF and development files are excluded.

Fixed runtime: `154.0.4258.37`; SDK: `1.0.4191.47`. Acquisition verifies pinned
SHA-256 hashes and the runtime's Microsoft signature. The full runtime takes
about 668 MiB unpacked; its official compressed CAB is about 294 MiB.

## Design

`host/WebViewHost.cpp` hosts CompositionController in a separate native EXE.
Windows Composition and Graphics Capture produce complete BGRA frames for the
existing IPC mailbox. Capture is limited to the host's own visual tree. The
game's OpenGL/DX11/Vulkan upload paths and ImGui browser UI remain shared.
The offscreen tool window supplies native composition/input without a taskbar
window. Browser children keep their normal sandbox and remain in the owning job.

The fixed runtime directory is explicitly supplied to WebView2 initialization.
Missing bundled files cause an error; there is no dependency on installed Edge.
`RuntimeVersion` selects a separate runtime namespace from the old CEF install.
The injector uses immutable package-hash directories and an atomic current pointer.

ClickGUI's Chrome page starts automatically. The same service supports tabs,
navigation, zoom, FPS limit, background audio and the detached stage window.
Only HTTP(S) and about:blank are accepted as top-level navigation. There is no
web-to-game bridge, remote debugging port, automatic download, or extension API.

## Tests

Configure `Client/test/chrome/kernel` for a standalone browser build. Serve
`Client/test/chrome` on loopback port 18795. Run `NovaBrowserSmoke` with the test
URL first and `--data-root <project-local-directory>` for isolated data.

`--h264` validates actual video playback/seeking and AAC sample decoding;
`--cloud-h264` validates H.264 WebRTC frames and data-channel delivery. Both
passed with the fixed runtime. The UI smoke also requires nonblack pixels before
accepting input/rendering, and after hidden-window recovery. Game injection is
performed by the user. Cloud-service pointer lock, relative input, login and
actual sessions are not implied by local codec tests.

See `Client/docs/browser-media-cloud-compatibility.md` for deployment details.
