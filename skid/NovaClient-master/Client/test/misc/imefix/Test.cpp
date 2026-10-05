#include "minecraft/host/input/ImeInput.h"
#include <imm.h>
#include <cassert>
#include <iostream>
#include <vector>

using nova::minecraft::input::ImeInput;

struct Fixture {
    ImeInput input;
    HWND window = nullptr;
    bool eligible = true, readOk = true, accept = true;
    std::u16string result = u"中文测试";
    std::vector<std::u16string> inserted;
    unsigned passed = 0;
    bool Dispatch(HWND handle, UINT message, WPARAM wp, LPARAM lp) {
        return input.Handle(handle, message, wp, lp, eligible,
            [&](HWND, std::u16string& out) { out = result; return readOk; },
            [&](std::u16string_view text) {
                if (!accept) return false;
                inserted.emplace_back(text);
                return true;
            });
    }
    static LRESULT CALLBACK Proc(HWND window, UINT message, WPARAM wp, LPARAM lp) {
        auto* self = reinterpret_cast<Fixture*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (self && ImeInput::Relevant(message)) {
            if (self->Dispatch(window, message, wp, lp)) return 1;
            ++self->passed;
            // Stand-in for the downstream game/default handler. Track whether
            // result messages could reach it and generate duplicate characters.
            return 0;
        }
        return DefWindowProcW(window, message, wp, lp);
    }
    Fixture() {
        static const auto atom = [] {
            WNDCLASSW wc{};
            wc.lpfnWndProc = Proc;
            wc.hInstance = GetModuleHandleW(nullptr);
            wc.lpszClassName = L"Nova.ImeFix.NativeTest";
            return RegisterClassW(&wc);
        }();
        assert(atom);
        window = CreateWindowExW(0, L"Nova.ImeFix.NativeTest", L"", 0, 0, 0, 0, 0,
                                 HWND_MESSAGE, nullptr, GetModuleHandleW(nullptr), nullptr);
        assert(window);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    }
    ~Fixture() { DestroyWindow(window); }
    bool Send(UINT message, WPARAM wp = 0, LPARAM lp = 0) {
        return SendMessageW(window, message, wp, lp) != 0;
    }
};

int main() {
    {
        Fixture f;
        assert(!f.Send(WM_IME_STARTCOMPOSITION));
        const auto passed = f.passed;
        assert(f.Send(WM_IME_COMPOSITION, 0, GCS_RESULTSTR));
        assert(f.passed == passed); // Original handler never sees the owned result.
        assert(f.inserted == std::vector<std::u16string>{u"中文测试"});
        assert(!f.Send(WM_IME_ENDCOMPOSITION));
        assert(f.Send(WM_IME_CHAR, L'中')); // No second default WM_CHAR generation.
        assert(!f.Send(WM_CHAR, L'?')); // A literal question mark is not filtered.
        assert(!f.Send(WM_KEYDOWN, 'A'));
        assert(!f.Send(WM_IME_CHAR, L'文'));
    }
    {
        Fixture f;
        f.Send(WM_IME_STARTCOMPOSITION);
        assert(f.Send(WM_KEYDOWN, VK_RETURN));
        assert(f.Send(WM_IME_COMPOSITION, 0, GCS_RESULTSTR));
        f.Send(WM_IME_ENDCOMPOSITION);
        assert(f.Send(WM_CHAR, VK_RETURN));
        assert(f.Send(WM_KEYUP, VK_RETURN));
        assert(!f.Send(WM_KEYDOWN, VK_RETURN)); // Deliberate second Enter sends.
        assert(!f.Send(WM_CHAR, VK_RETURN));
    }
    {
        Fixture f;
        f.result = u"中文\U0001F600混合abc";
        assert(f.Send(WM_IME_COMPOSITION, 0, GCS_RESULTSTR));
        assert(f.inserted.front() == f.result); // Preserve complete UTF-16 surrogate pair.
        f.readOk = false;
        assert(!f.Send(WM_IME_COMPOSITION, 0, GCS_RESULTSTR));
        assert(!f.Send(WM_IME_CHAR, L'中')); // Failed read leaves fallback intact.
        f.readOk = true; f.accept = false;
        assert(!f.Send(WM_IME_COMPOSITION, 0, GCS_RESULTSTR));
        assert(f.inserted.size() == 1);
    }
    {
        Fixture f;
        for (const auto& invalid : {std::u16string{}, std::u16string{char16_t(0xD800)},
             std::u16string{char16_t(0xDC00)}, std::u16string(u"a\nb"), std::u16string(32769, u'中')}) {
            f.result = invalid;
            assert(!f.Send(WM_IME_COMPOSITION, 0, GCS_RESULTSTR));
        }
        assert(f.inserted.empty());
    }
    {
        Fixture f;
        f.Send(WM_IME_STARTCOMPOSITION);
        f.input.InvalidateTarget(); // Same HWND, different chat/world/widget.
        assert(f.Send(WM_IME_COMPOSITION, 0, GCS_RESULTSTR));
        assert(f.inserted.empty());
        f.Send(WM_IME_ENDCOMPOSITION);
        assert(f.Send(WM_IME_CHAR, L'中'));
        f.Send(WM_IME_STARTCOMPOSITION);
        assert(f.Send(WM_IME_COMPOSITION, 0, GCS_RESULTSTR));
        assert(f.inserted.size() == 1);
    }
    {
        Fixture f;
        f.Send(WM_IME_STARTCOMPOSITION);
        f.Send(WM_KILLFOCUS);
        assert(f.Send(WM_IME_COMPOSITION, 0, GCS_RESULTSTR));
        assert(f.inserted.empty());
        f.input.Reset(); // Disable/unload removes all ownership.
        assert(!f.Send(WM_IME_CHAR, L'中'));
    }
    {
        Fixture f;
        f.eligible = false; // Browser / ClickGUI / non-chat input must work normally.
        assert(!f.Send(WM_IME_STARTCOMPOSITION));
        assert(!f.Send(WM_IME_COMPOSITION, 0, GCS_COMPSTR));
        assert(!f.Send(WM_IME_COMPOSITION, 0, GCS_RESULTSTR));
        assert(!f.Send(WM_IME_CHAR, L'中'));
        assert(!f.Send(WM_CHAR, L'中'));
        assert(!f.Send(WM_KEYDOWN, VK_RETURN));
        assert(f.inserted.empty());
        f.eligible = true;
        f.Send(WM_IME_STARTCOMPOSITION);
        f.eligible = false;
        assert(f.Send(WM_IME_COMPOSITION, 0, GCS_RESULTSTR)); // Old game commit discarded.
        f.Send(WM_IME_STARTCOMPOSITION); // New browser composition.
        assert(!f.Send(WM_IME_COMPOSITION, 0, GCS_RESULTSTR));
        assert(f.inserted.empty());
    }
    {
        Fixture f, other;
        f.Send(WM_IME_STARTCOMPOSITION);
        f.input.InvalidateTarget();
        assert(f.Dispatch(other.window, WM_IME_COMPOSITION, 0, GCS_RESULTSTR));
        assert(f.inserted.size() == 1); // New HWND has no stale cancellation state.
    }
    std::cout << "IME Fix: native Unicode, ownership, key and lifecycle checks passed\n";
}
