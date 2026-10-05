package dev.myth.UI.font;

import imgui.ImDrawList;
import java.awt.Color;
import java.awt.Font;
import java.awt.FontMetrics;
import java.awt.Graphics2D;
import java.awt.RenderingHints;
import java.awt.font.FontRenderContext;
import java.awt.geom.AffineTransform;
import java.awt.geom.Rectangle2D;
import java.awt.image.BufferedImage;
import java.util.HashMap;
import static org.lwjgl.opengl.GL33C.glDeleteTextures;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/FontHelper.class */
public class FontHelper implements AutoCloseable {
    public boolean antiAlias;
    public final Font font;
    public final boolean flag;
    public final boolean flag2;
    public int num;
    public BufferedImage bufferedImage;
    public int texture;
    public final HashMap<Character, Inner> hashMap = new HashMap<>();
    public int num2 = -1;
    public float textTop;
    public float textBottom;

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/FontHelper$Inner.class */
    static class Inner {
        public int x;
        public int y;
        public int num;
        public int num2;
        public float textTop;
        public float textBottom;

        public Inner(int i, int i2, int i3, int i4) {
            this.x = i;
            this.y = i2;
            this.num = i3;
            this.num2 = i4;
        }

        public Inner() {
        }

        public int getX() {
            return this.x;
        }

        public int getY() {
            return this.y;
        }

        public int get1635() {
            return this.num;
        }

        public int get93() {
            return this.num2;
        }
    }

    public FontHelper(Font font, boolean z, boolean z2) {
        this.font = font;
        this.flag = z;
        this.flag2 = z2;
    }

    public void do1() {
        if (this.texture != 0) glDeleteTextures(this.texture);
        this.texture = 0;
        this.hashMap.clear();
    }

    public void do1778(char[] cArr) {
        double longBitsToDouble = Double.longBitsToDouble(-4616189618054758400L);
        double longBitsToDouble2 = Double.longBitsToDouble(-4616189618054758400L);
        FontRenderContext fontRenderContext = new FontRenderContext(new AffineTransform(), this.flag, this.flag2);
        for (char c : cArr) {
            Rectangle2D stringBounds = this.font.getStringBounds(Character.toString(c), fontRenderContext);
            if (longBitsToDouble < stringBounds.getWidth()) {
                longBitsToDouble = stringBounds.getWidth();
            }
            if (longBitsToDouble2 < stringBounds.getHeight()) {
                longBitsToDouble2 = stringBounds.getHeight();
            }
        }
        double longBitsToDouble3 = longBitsToDouble + Double.longBitsToDouble(4611686018427387904L);
        double longBitsToDouble4 = longBitsToDouble2 + Double.longBitsToDouble(4611686018427387904L);
        this.num = ((int) Math.ceil(Math.max(Math.ceil(Math.sqrt((longBitsToDouble3 * longBitsToDouble3) * cArr.length) / longBitsToDouble3), Math.ceil(Math.sqrt((longBitsToDouble4 * longBitsToDouble4) * cArr.length) / longBitsToDouble4)) * Math.max(longBitsToDouble3, longBitsToDouble4))) + 1;
        this.bufferedImage = new BufferedImage(this.num, this.num, 2);
        Graphics2D createGraphics = this.bufferedImage.createGraphics();
        createGraphics.setFont(this.font);
        createGraphics.setColor(new Color(255, 255, 255, 0));
        createGraphics.fillRect(0, 0, this.num, this.num);
        createGraphics.setColor(Color.white);
        if (this.antiAlias) {
            createGraphics.setRenderingHint(RenderingHints.KEY_FRACTIONALMETRICS, RenderingHints.VALUE_FRACTIONALMETRICS_ON);
            createGraphics.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
            createGraphics.setRenderingHint(RenderingHints.KEY_TEXT_ANTIALIASING, RenderingHints.VALUE_TEXT_ANTIALIAS_ON);
        } else {
            createGraphics.setRenderingHint(RenderingHints.KEY_FRACTIONALMETRICS, this.flag2 ? RenderingHints.VALUE_FRACTIONALMETRICS_ON : RenderingHints.VALUE_FRACTIONALMETRICS_OFF);
            createGraphics.setRenderingHint(RenderingHints.KEY_ANTIALIASING, this.flag ? RenderingHints.VALUE_ANTIALIAS_OFF : RenderingHints.VALUE_ANTIALIAS_ON);
            createGraphics.setRenderingHint(RenderingHints.KEY_TEXT_ANTIALIASING, this.flag ? RenderingHints.VALUE_TEXT_ANTIALIAS_ON : RenderingHints.VALUE_TEXT_ANTIALIAS_OFF);
        }
        FontMetrics fontMetrics = createGraphics.getFontMetrics();
        int i = 0;
        int i2 = 0;
        int i3 = 1;
        for (char c2 : cArr) {
            Inner inner = new Inner();
            Rectangle2D stringBounds2 = fontMetrics.getStringBounds(Character.toString(c2), createGraphics);
            inner.num = stringBounds2.getBounds().width + 8;
            inner.num2 = stringBounds2.getBounds().height;
            if (i2 + inner.num >= this.num) {
                i2 = 0;
                i3 += i;
                i = 0;
            }
            inner.x = i2;
            inner.y = i3;
            if (inner.num2 > this.num2) {
                this.num2 = inner.num2;
            }
            if (inner.num2 > i) {
                i = inner.num2;
            }
            i2 += inner.num;
            this.hashMap.put(Character.valueOf(c2), inner);
        }
        int requiredSize = Math.max(this.num, i3 + i + 1);
        if (requiredSize > this.num) {
            RenderingHints hints = createGraphics.getRenderingHints();
            createGraphics.dispose();
            this.num = requiredSize;
            this.bufferedImage = new BufferedImage(this.num, this.num, BufferedImage.TYPE_INT_ARGB);
            createGraphics = this.bufferedImage.createGraphics();
            createGraphics.setFont(this.font); createGraphics.setColor(Color.white);
            createGraphics.setRenderingHints(hints);
        }
        for (var entry : this.hashMap.entrySet()) {
            Inner glyph = entry.getValue();
            createGraphics.drawString(Character.toString(entry.getKey()), glyph.x + 2, glyph.y + fontMetrics.getAscent());
        }
        this.bufferedImage.flush();
        createGraphics.dispose();
        int inkTop = this.num2, inkBottom = 0;
        int visibleAlpha = this.antiAlias ? 128 : 180;
        for (var entry : this.hashMap.entrySet()) {
            Inner glyph = entry.getValue();
            int glyphTop = glyph.num2, glyphBottom = 0;
            int fringeTop = glyph.num2, fringeBottom = 0;
            for (int row = 0; row < glyph.num2; row++) for (int col = 0; col < glyph.num; col++) {
                int alpha = this.bufferedImage.getRGB(glyph.x + col, glyph.y + row) >>> 24;
                if (alpha == 0) continue;
                fringeTop = Math.min(fringeTop, row); fringeBottom = Math.max(fringeBottom, row + 1);
                // Faint antialias fringes are not the visible body of the glyph.
                // Including them shifts thin punctuation relative to other text.
                if (alpha <= visibleAlpha) continue;
                glyphTop = Math.min(glyphTop, row); glyphBottom = Math.max(glyphBottom, row + 1);
            }
            if (glyphBottom == 0) { glyphTop = fringeTop; glyphBottom = fringeBottom; }
            glyph.textTop = glyphTop / 2f; glyph.textBottom = glyphBottom / 2f;
            if (Character.isLetterOrDigit(entry.getKey()) && entry.getKey() < 127 && glyphBottom > 0) {
                inkTop = Math.min(inkTop, glyphTop); inkBottom = Math.max(inkBottom, glyphBottom);
            }
        }
        this.textTop = inkBottom > 0 ? inkTop / 2f : 0;
        this.textBottom = inkBottom > 0 ? inkBottom / 2f : this.num2 / 2f;
    }

    public float textTop(String text) { return textEdge(text, true); }
    public float textBottom(String text) { return textEdge(text, false); }
    private float textEdge(String text, boolean top) {
        float edge = top ? Float.POSITIVE_INFINITY : Float.NEGATIVE_INFINITY;
        for (int i = 0; i < text.length(); i++) {
            char c = text.charAt(i);
            if (c == 167 && i + 1 < text.length()) { i++; continue; }
            Inner glyph = this.hashMap.get(c);
            if (glyph == null || glyph.textBottom == 0) continue;
            edge = top ? Math.min(edge, glyph.textTop) : Math.max(edge, glyph.textBottom);
        }
        return Float.isFinite(edge) ? edge : top ? this.textTop : this.textBottom;
    }

    public void do1779() {
        this.texture = FontAtlas.uploadImage(this.bufferedImage, this.antiAlias);
    }

    public float get1782(ImDrawList draw, char c, float x, float y, int color, float scale) {
        Inner inner = this.hashMap.get(c);
        if (inner == null) return 0;
        draw.addImage(this.texture, x, y, x + inner.num * scale, y + inner.num2 * scale,
                inner.x / (float) this.num, inner.y / (float) this.num,
                (inner.x + inner.num) / (float) this.num, (inner.y + inner.num2) / (float) this.num, color);
        return (inner.num - 8) * scale;
    }

    public float get1783(char c) {
        if (!this.hashMap.containsKey(c)) c = ' ';
        return this.hashMap.get(c).num;
    }

    public int get1784() { return this.num2; }
    public boolean is1785() { return this.flag; }
    public boolean is1786() { return this.flag2; }
    @Override public void close() { do1(); }
}
