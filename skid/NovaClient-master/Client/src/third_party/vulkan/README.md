# Vulkan C headers

Source: https://github.com/KhronosGroup/Vulkan-Headers/tree/v1.3.290

Version pinned to v1.3.290. C headers and video codec dependencies only; no SDK,
loader binary, driver or C++ bindings are required. Nova resolves functions from
the running game's Vulkan instance/device. See LICENSE.md for licensing.

The ImGui Vulkan backend alongside the existing ImGui backends comes from
https://github.com/ocornut/imgui/tree/v1.92.8/backends and matches the bundled
ImGui 1.92.8 core. The backend is built with IMGUI_IMPL_VULKAN_NO_PROTOTYPES.
