#include "minecraft/modules/render/nametags/ClassicNameTags.h"
#include "ui/clickgui/Fonts.h"
#include <cassert>
#include <iostream>
#include <vector>

using namespace nova::module::nametags;
constexpr auto section = "\xC2\xA7";

int main() {
    assert(Listed("Alex,STEVE; Bob\n", "steve"));
    assert(!Listed("Alexander; Steve", "Alex"));
    assert(!Listed("  , ;\n", ""));
    std::string plain;
    std::vector<TextStyle> styles;
    auto label = ClassicLabel("Alex", 19.5f, true, true, true, false);
    WalkText(label, 1.f, [](auto) { return 6.f; }, [&](auto glyph, auto, auto, auto style) {
        plain += glyph; styles.push_back(style);
    });
    assert(plain == "[FRIEND] [TARGET] [TEAM] Alex [\xE2\x9D\xA4" "20]");
    assert(styles[0].color == 11 && styles[0].bold);
    assert(styles[9].color == 4 && styles[9].bold);
    assert(styles[18].color == 10 && styles[18].bold);
    assert(styles[25].color == -1 && !styles[25].bold);
    assert(ClassicLabel("A", 19.49f, false, false, false, false).find("19") != std::string::npos);
    assert(ClassicLabel("A", 0, true, true, true, true).find("[TM]") != std::string::npos);
    assert(ClassicLabel("A", NAN, false, false, false, false).find("0") != std::string::npos);
    const auto healthColor = [](float health, float maximum) {
        int result = -1;
        WalkText(ClassicLabel("A", health, false, false, false, false, maximum), 1.f,
            [](auto) { return 6.f; }, [&](auto glyph, auto, auto, auto style) {
                if (glyph == "\xE2\x9D\xA4") result = style.color;
            });
        return result;
    };
    assert(healthColor(20, 20) == 10);
    assert(healthColor(14, 20) == 10);
    assert(healthColor(10, 20) == 14);
    assert(healthColor(6, 20) == 6);
    assert(healthColor(5, 20) == 12);
    assert(healthColor(20, 40) == 14);
    assert(healthColor(20, 0) == 10);

    const auto formatting = std::string(section) + "lAB" + section + "cC" + section + "rD";
    float width = WalkText(formatting, 2, [](auto) { return 12.f; }, [](auto, auto, auto, auto) {});
    assert(width == 52); // Bold advance per glyph; color resets bold.
    bool rgbSeen = false;
    WalkText(std::string(section) + "#12Ab34X" + section + "rY", 1, [](auto) { return 6.f; },
        [&](auto glyph, auto, auto, auto style) {
            if (glyph == "X") { assert(style.rgb == 0x12ab34); rgbSeen = true; }
            if (glyph == "Y") assert(style.rgb == -1);
        });
    assert(rgbSeen);

    int corners = 0;
    auto project = [&](nova::rotation::Vec3 p, float& x, float& y) {
        ++corners; x = float(p.x * 10); y = float(100 - p.y * 10); return true;
    };
    auto anchor = ProjectClassic({2, 4, 6}, {0, 0, 0}, .6, 1.8, false, .5f, project);
    assert(corners == 8 && anchor);
    assert(std::abs(anchor->x - 10) < .001f && std::abs(anchor->y - 59.9f) < .001f);
    auto sneaking = ProjectClassic({2, 4, 6}, {0, 0, 0}, .6, 1.8, true, .5f, project);
    assert(std::abs(sneaking->y - anchor->y - 3) < .001f);
    assert(!ProjectClassic({}, {}, .6, 1.8, false, .5f, [](auto, auto&, auto&) { return false; }));
    assert(!ProjectClassic({}, {}, NAN, 1.8, false, .5f, project));

    // Exercise the production atlas, merge fallback and full context recreation.
    for (int cycle = 0; cycle < 2; ++cycle) {
        ImGui::CreateContext();
        ImGui::GetIO().BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
        InitializeFatalMenuFonts();
        assert(Classic9 && Classic9 != Medium12);
        auto* baked = Classic9->GetFontBaked(9);
        assert(baked->FindGlyphNoFallback('A'));
        assert(std::abs(baked->FindGlyph('A')->AdvanceX - 6.f) < .01f);
        assert(std::abs(baked->FindGlyph(' ')->AdvanceX - 4.f) < .01f);
        auto* heart = baked->FindGlyphNoFallback(0x2764);
        assert(heart && heart->Visible);
        assert(baked->FindGlyphNoFallback(0x00e9)); // Existing HUD font fallback.
        ImGui::DestroyContext();
        ResetFatalMenuFonts();
        assert(!Classic9 && !Medium12);
    }
    std::cout << "PASS: Classic labels, styles, projection, font metrics/fallback and context recreation\n";
}
