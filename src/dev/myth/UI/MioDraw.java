package dev.myth.UI;

import imgui.ImDrawList;

/** Openmio CrosshairHelper.do1708/do1705, adapted to ImGui image-free 2D quads. */
public final class MioDraw {
    private MioDraw() {}

    /** ARGB to the ABGR word ImGui expects, with the alpha scaled by an opacity. */
    public static int packed(int argb, float opacity) {
        return Math.clamp(Math.round((argb >>> 24) * opacity), 0, 255) << 24
                | (argb & 255) << 16 | (argb & 0xff00) | (argb >> 16 & 255);
    }

    public static void rect(ImDrawList draw, float x, float y, float right, float bottom, int color) {
        if (right > x && bottom > y && (color >>> 24) != 0) draw.addRectFilled(x, y, right, bottom, color);
    }

    public static void outline(ImDrawList draw, float x, float y, float right, float bottom, int color) {
        // Mio places the one-pixel border outside the frame. Use quads so strokes
        // have no anti-aliased fringe, rounded edges, or twice-blended corners.
        rect(draw, x, y, right + 1, y + 1, color);
        rect(draw, x, bottom, right + 1, bottom + 1, color);
        rect(draw, x, y + 1, x + 1, bottom, color);
        rect(draw, right, y + 1, right + 1, bottom, color);
    }

    /** Exterior borders use the same backdrop and alpha layering as panel colors. */
    public static void panelOutline(ImDrawList draw, float x, float y, float right, float bottom, int backdrop, int color) {
        outline(draw, x, y, right, bottom, backdrop);
        outline(draw, x, y, right, bottom, color);
    }
}
