#pragma once
#include <cstdint>
struct ImDrawData;
namespace nova::vkoverlay {
struct Handles {
    std::uintptr_t instance{}, physicalDevice{}, device{}, queue{};
    std::uint64_t surface{}, swapchain{};
    std::uint32_t queueFamily{};
};
struct Callbacks {
    void (*frame)(void* window)=nullptr;
    void (*rendererLost)()=nullptr;
    void (*submitted)()=nullptr;
};
// Native HWND and render-thread callbacks supplied by the host.
bool Install(const Handles& handles, void* hwnd,Callbacks callbacks);
bool InitializeImGui();
void NewFrame();
void Render(ImDrawData* data);
void ShutdownImGui();
void DisableHooks();
void Finalize();
}
