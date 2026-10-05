#pragma once

// Minecraft's GLFW/LWJGL/SDL entry points and game input restoration.

namespace nova::glhook {

bool Install();
// Queue one LWJGL mouse pulse; consumed by the game's normal input loop.
bool QueueAttackMousePulse();
void CancelAttackMousePulse();
bool WaitForFirstFrame(unsigned timeoutMs);
void VulkanFrame(void* hwnd);
void VulkanRendererLost();
void VulkanFrameSubmitted();

bool WaitForTeardown(unsigned timeoutMs);

void FinalizeHooks();

}
