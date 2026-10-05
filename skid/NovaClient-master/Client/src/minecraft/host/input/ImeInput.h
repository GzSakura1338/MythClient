#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <functional>
#include <string>
#include <string_view>

namespace nova::minecraft::input {

// Runs only on the HWND owner thread. The host must reset when the focused
// target changes. Returning true means the original window procedure must NOT
// receive this message (in particular, no second default IME commit).
class ImeInput {
public:
    using ReadResult = std::function<bool(HWND, std::u16string&)>;
    // True once the target has accepted ownership, including a failed responder
    // after insertion. Never replay a possibly partially applied commit.
    using Commit = std::function<bool(std::u16string_view)>;
    bool Handle(HWND window, UINT message, WPARAM wp, LPARAM lp, bool eligible,
                const ReadResult& read, const Commit& commit);
    void Reset();
    void InvalidateTarget();
    bool Composing() const { return composing_; }
    static bool Relevant(UINT message);
    static bool ReadCommittedText(HWND window, std::u16string& text);
    static bool ValidText(std::u16string_view text);
private:
    HWND window_ = nullptr;
    bool composing_ = false;
    bool committed_ = false;
    bool discarded_ = false;
    unsigned heldKeys_ = 0;
};

}
