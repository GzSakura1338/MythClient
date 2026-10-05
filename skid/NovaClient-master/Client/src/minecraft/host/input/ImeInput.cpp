#include "minecraft/host/input/ImeInput.h"
#include <imm.h>

namespace nova::minecraft::input {
namespace {
unsigned KeyBit(WPARAM key) {
    switch (key) {
    case VK_RETURN: return 1;
    case VK_SPACE: return 2;
    case VK_ESCAPE: return 4;
    default: return 0;
    }
}
}

void ImeInput::Reset() {
    window_ = nullptr;
    composing_ = committed_ = discarded_ = false;
    heldKeys_ = 0;
}

void ImeInput::InvalidateTarget() {
    // A result delivered after a world/screen/focus change belongs to the OLD
    // target. Keep a tombstone until the next composition or physical input.
    discarded_ = discarded_ || composing_ || committed_;
    composing_ = committed_ = false;
    heldKeys_ = 0;
}

bool ImeInput::Relevant(UINT message) {
    return message == WM_IME_STARTCOMPOSITION || message == WM_IME_COMPOSITION ||
        message == WM_IME_ENDCOMPOSITION || message == WM_IME_CHAR ||
        message == WM_KEYDOWN || message == WM_KEYUP || message == WM_CHAR ||
        message == WM_KILLFOCUS || message == WM_NCDESTROY ||
        message == WM_INPUTLANGCHANGE;
}

bool ImeInput::ValidText(std::u16string_view text) {
    if (text.empty() || text.size() > 32768) return false;
    for (size_t i = 0; i < text.size(); ++i) {
        const auto unit = text[i];
        if (unit < 32 || unit == 127) return false; // Never inject chat-send controls.
        if (unit >= 0xD800 && unit <= 0xDBFF) {
            if (++i == text.size() || text[i] < 0xDC00 || text[i] > 0xDFFF) return false;
        } else if (unit >= 0xDC00 && unit <= 0xDFFF) return false;
    }
    return true;
}

bool ImeInput::ReadCommittedText(HWND window, std::u16string& text) {
    text.clear();
    const HIMC context = ImmGetContext(window);
    if (!context) return false;
    struct Release {
        HWND window; HIMC context;
        ~Release() { ImmReleaseContext(window, context); }
    } release{window, context};
    const LONG bytes = ImmGetCompositionStringW(context, GCS_RESULTSTR, nullptr, 0);
    if (bytes <= 0 || bytes > 65536 || (bytes & 1)) return false;
    static_assert(sizeof(char16_t) == 2);
    text.resize(static_cast<size_t>(bytes) / 2);
    const LONG copied = ImmGetCompositionStringW(context, GCS_RESULTSTR, text.data(), bytes);
    if (copied != bytes || !ValidText(text)) { text.clear(); return false; }
    return true;
}

bool ImeInput::Handle(HWND window, UINT message, WPARAM wp, LPARAM lp, bool eligible,
                      const ReadResult& read, const Commit& commit) {
    if (window_ != window) { Reset(); window_ = window; }
    if (!eligible) {
        InvalidateTarget();
        if (message == WM_IME_STARTCOMPOSITION) { Reset(); return false; }
        if (!discarded_) return false;
    }
    switch (message) {
    case WM_NCDESTROY:
    case WM_INPUTLANGCHANGE:
        Reset();
        return false;
    case WM_KILLFOCUS:
        InvalidateTarget();
        return false;
    case WM_IME_STARTCOMPOSITION:
        composing_ = true;
        committed_ = discarded_ = false;
        return false; // Keep the system composition/candidate UI.
    case WM_IME_COMPOSITION:
        if (lp & GCS_RESULTSTR) {
            if (discarded_) return true;
            std::u16string result;
            if (!read(window, result) || !ValidText(result) || !commit(result)) {
                committed_ = false;
                return false; // No ownership: retain the original input path.
            }
            committed_ = true;
            // Do not pass GCS_RESULTSTR to DefWindowProc: it would generate
            // WM_IME_CHAR -> WM_CHAR and insert this exact commit a second time.
            // START/END and non-result COMPOSITION still go to the system.
            return true;
        }
        // Some IMEs continue composing the next clause without another START.
        if (lp & GCS_COMPSTR) { composing_ = true; committed_ = false; }
        if (!lp) composing_ = false;
        return false;
    case WM_IME_ENDCOMPOSITION:
        composing_ = false;
        return false;
    case WM_IME_CHAR:
        // Covers IMEs which send an IME_CHAR alongside an already-owned result.
        // Never consume ordinary WM_CHAR text or unrelated literal '?'.
        return committed_ || discarded_;
    case WM_KEYDOWN: {
        const auto bit = KeyBit(wp);
        if (bit && (composing_ || (heldKeys_ & bit))) {
            heldKeys_ |= bit;
            return true;
        }
        committed_ = false; // A new physical input starts a new delivery scope.
        discarded_ = false;
        return false;
    }
    case WM_KEYUP: {
        const auto bit = KeyBit(wp);
        const bool claimed = (heldKeys_ & bit) != 0;
        heldKeys_ &= ~bit;
        return claimed;
    }
    case WM_CHAR:
        if (const auto bit = KeyBit(wp); bit && (composing_ || (heldKeys_ & bit)))
            return true;
        return false;
    default:
        return false;
    }
}

}
