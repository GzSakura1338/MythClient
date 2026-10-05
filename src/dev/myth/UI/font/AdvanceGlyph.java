package dev.myth.UI.font;

import imgui.ImDrawList;
import java.awt.Font;
import java.util.Locale;

/** Mio AdvanceGlyph; Minecraft vertex submission is replaced by ImGui image quads. */
public final class AdvanceGlyph implements AutoCloseable {
    public final FontHelper fontHelper;
    public static final int[] intArr = new int[32];
    public AdvanceGlyph(FontHelper fontHelper) { this.fontHelper = fontHelper; }

    public static AdvanceGlyph create(String name, int size, int style, boolean antiAlias) {
        char[] chars = new char[1503];
        for (int i = 0; i < 1501; i++) chars[i] = (char) i;
        chars[1501] = 8734; chars[1502] = 10054;
        FontHelper helper = new FontHelper(new Font(name, style, size), true, true);
        helper.antiAlias = antiAlias;
        helper.do1778(chars); helper.do1779();
        return new AdvanceGlyph(helper);
    }

    public void draw(ImDrawList draw, String text, float x, float y, int argb, float opacity, float scale) {
        draw(draw, text, x, y, argb, opacity, scale, false);
    }
    public void draw(ImDrawList draw, String text, float x, float y, int argb, float opacity, float scale, boolean shadow) {
        int color = packed(argb, opacity);
        float cursor = x;
        for (int i = 0; i < text.length(); i++) {
            char c = text.charAt(i);
            if (c == 167 && i + 1 < text.length()) {
                int index = "0123456789abcdefklmnor".indexOf(text.toLowerCase(Locale.ROOT).charAt(++i));
                if (index < 16) color = packed((argb & 0xff000000) | intArr[(index < 0 ? 15 : index) + (shadow ? 16 : 0)], opacity);
                else color = packed(argb, opacity);
            } else cursor += fontHelper.get1782(draw, c, cursor, y, color, scale / 2);
        }
    }

    private static int packed(int argb, float opacity) {
        return Math.clamp(Math.round((argb >>> 24) * opacity), 0, 255) << 24
                | (argb & 255) << 16 | argb & 0xff00 | argb >> 16 & 255;
    }
    public int get2771() { return fontHelper.get1784() / 2; }
    public int get2772(String text) {
        int width = 0;
        for (int i = 0; i < text.length(); i++) {
            if (text.charAt(i) == 167 && i + 1 < text.length()) i++;
            else width += (int) fontHelper.get1783(text.charAt(i)) - 8;
        }
        return width / 2 + 1;
    }
    @Override public void close() { fontHelper.close(); }
    static {
        for (int i = 0; i < 32; i++) {
            int extra = ((i >> 3) & 1) * 85;
            int r = ((i >> 2) & 1) * 170 + extra, g = ((i >> 1) & 1) * 170 + extra, b = (i & 1) * 170 + extra;
            if (i == 6) r += 85;
            if (i >= 16) { r /= 4; g /= 4; b /= 4; }
            intArr[i] = (r & 255) << 16 | (g & 255) << 8 | b & 255;
        }
    }
}
