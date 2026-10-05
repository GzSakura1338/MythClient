#pragma once
#include "chrome/BrowserService.h"
#include "ui/backend/blur/Blur.h"

namespace nova::ui::browser {
// Small, cached CPU surface; ImGui uploads it through the active render backend.
// Work is bounded to 256 pixels per side and performed only while a popup is open.
class BrowserBackdrop {
public:
    const chrome::Frame& Update(const chrome::Frame& source, blur::Options options);
private:
    chrome::Frame frame_;
    blur::Refresh refresh_;
    uint64_t sourceSequence_{};
    std::vector<unsigned char> scratch_;
};
}
