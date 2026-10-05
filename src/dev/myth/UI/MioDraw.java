package dev.myth.UI;

import imgui.ImDrawList;

/** Openmio CrosshairHelper.do1708/do1705, adapted to ImGui image-free 2D quads. */
public final class MioDraw {
    private MioDraw() {}

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
}
