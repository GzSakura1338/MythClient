#pragma once
#include <atomic>
namespace nova::bind {
// Shared page-level capture. Releases still reach hold bindings so opening the
// browser while a module hotkey is held cannot leave that module latched.
inline std::atomic<bool> browserCapture{false};
}
