package dev.myth.UI.hud;

import dev.myth.Myth;
import dev.myth.UI.MioDraw;
import imgui.ImDrawList;

final class HudDraw {
    private HudDraw() {}
    static int packed(int argb, float opacity) {
        return Math.clamp(Math.round((argb >>> 24) * opacity), 0, 255) << 24
                | (argb & 255) << 16 | (argb & 0xff00) | (argb >> 16 & 255);
    }
    static void rect(ImDrawList draw, float x, float y, float w, float h, int color, float opacity) {
        MioDraw.rect(draw, x, y, x + w, y + h, packed(color, opacity));
    }
    static void outline(ImDrawList draw, float x, float y, float w, float h, int color, float opacity) {
        MioDraw.outline(draw, x, y, x + w, y + h, packed(color, opacity));
    }
    static int theme() { return Myth.UI_MANAGER.getThemeColor(); }
}
