package today.starain.ui.clickgui.opal.render;

import io.github.humbleui.skija.Font;
import io.github.humbleui.skija.Paint;
import io.github.humbleui.skija.Shader;
import io.github.humbleui.types.Point;
import io.github.humbleui.types.RRect;
import today.starain.utils.render.ColorUtil;
import today.starain.utils.render.skia.Skia;
import today.starain.utils.render.skia.font.Fonts;

import java.awt.*;

public final class OpalDraw {

    private OpalDraw() {
    }

    public static Color color(int argb) {
        return new Color(argb, true);
    }

    public static int opacity(int argb, float opacity) {
        return ColorUtil.applyOpacity(new Color(argb, true), clamp01(opacity)).getRGB();
    }

    public static int darker(int argb, float factor) {
        return ColorUtil.darker(new Color(argb, true), factor).getRGB();
    }

    public static int interpolate(int argb1, int argb2, float progress) {
        return ColorUtil.interpolateColorC(new Color(argb1, true), new Color(argb2, true), clamp01(progress)).getRGB();
    }

    public static void gradientRoundedRect(float x, float y, float width, float height, float radius,
                                           int color1, int color2, float angleDegrees) {
        drawGradient(RRect.makeXYWH(x, y, width, height, radius), x, y, width, height, color1, color2, angleDegrees);
    }

    public static void gradientRect(float x, float y, float width, float height, int color1, int color2, float angleDegrees) {
        drawGradient(RRect.makeXYWH(x, y, width, height, 0), x, y, width, height, color1, color2, angleDegrees);
    }

    public static void gradientRoundedRectVarying(float x, float y, float width, float height,
                                                  float topLeft, float topRight, float bottomRight, float bottomLeft,
                                                  int color1, int color2, float angleDegrees) {
        float[] corners = {topLeft, topLeft, topRight, topRight, bottomRight, bottomRight, bottomLeft, bottomLeft};
        drawGradient(RRect.makeComplexXYWH(x, y, width, height, corners), x, y, width, height, color1, color2, angleDegrees);
    }

    private static void drawGradient(RRect rrect, float x, float y, float width, float height,
                                     int color1, int color2, float angleDegrees) {
        float angleRadians = (float) Math.toRadians(angleDegrees);
        float dx = (float) Math.cos(angleRadians);
        float dy = (float) Math.sin(angleRadians);

        float startX = x + width * 0.5f - dx * width * 0.5f;
        float startY = y + height * 0.5f - dy * height * 0.5f;
        float endX = x + width * 0.5f + dx * width * 0.5f;
        float endY = y + height * 0.5f + dy * height * 0.5f;

        try (Shader shader = Shader.makeLinearGradient(new Point(startX, startY), new Point(endX, endY),
                new int[]{color1, color2});
             Paint paint = new Paint().setAntiAlias(true)) {
            paint.setShader(shader);
            Skia.getCanvas().drawRRect(rrect, paint);
        }
    }

    public static void rainbowRect(float x, float y, float width, float height) {
        for (float i = y; i < y + height; i += 0.5f) {
            float hue = (i - y) / height;
            float segmentHeight = Math.min(0.5f, y + height - i);
            Skia.drawRect(x, i, width, segmentHeight, Color.HSBtoRGB(hue, 1, 1) | 0xFF000000);
        }
    }

    public static void text(String str, float x, float baselineY, float size, int argb, boolean bold) {
        Skia.drawBaselineText(str, x, baselineY, argb, font(str, size, bold));
    }

    public static void centeredText(String str, float centerX, float baselineY, float size, int argb, boolean bold) {
        Font font = font(str, size, bold);
        float width = Skia.getTextBounds(str, font).getWidth();
        Skia.drawBaselineText(str, centerX - width / 2f, baselineY, argb, font);
    }

    public static float textWidth(String str, float size, boolean bold) {
        return Skia.getTextBounds(str, font(str, size, bold)).getWidth();
    }

    public static void icon(String glyph, float x, float baselineY, float size, int argb, boolean outlined) {
        Font font = Fonts.getIcon(size);
        Skia.drawBaselineText(glyph, x, baselineY, argb, font);
    }

    public static void centeredIcon(String glyph, float centerX, float centerY, float size, int argb) {
        Skia.drawFullCenteredTextPlain(glyph, centerX, centerY, new Color(argb, true), Fonts.getIcon(size));
    }

    public static float iconWidth(String glyph, float size) {
        return Skia.getTextBounds(glyph, Fonts.getIcon(size)).getWidth();
    }

    private static Font font(String str, float size, boolean bold) {
        if (containsNonLatin(str)) {
            return bold ? Fonts.getMedium(size) : Fonts.getNormal(size);
        }
        return bold ? Fonts.getMedium(size) : Fonts.getNormal(size);
    }

    private static boolean containsNonLatin(String str) {
        for (int i = 0; i < str.length(); i++) {
            if (str.charAt(i) >= 0x2E80) {
                return true;
            }
        }
        return false;
    }

    public static void scissor(float x, float y, float width, float height, Runnable content) {
        Skia.save();
        Skia.clipRect(x, y, width, height);
        content.run();
        Skia.restore();
    }

    public static void rotate(float pivotX, float pivotY, float degrees, Runnable content) {
        Skia.save();
        Skia.rotate(pivotX - 0.5f, pivotY - 0.5f, 1, 1, degrees);
        content.run();
        Skia.restore();
    }

    public static void alpha(float alpha, Runnable content) {
        if (alpha >= 1f) {
            content.run();
            return;
        }
        Skia.setAlpha((int) (clamp01(alpha) * 255));
        content.run();
        Skia.restore();
    }

    private static float clamp01(float value) {
        return Math.max(0f, Math.min(1f, value));
    }
}
