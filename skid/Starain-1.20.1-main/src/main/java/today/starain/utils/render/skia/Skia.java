package today.starain.utils.render.skia;

import io.github.humbleui.skija.*;
import io.github.humbleui.skija.Canvas;
import io.github.humbleui.skija.Font;
import io.github.humbleui.skija.FontMetrics;
import io.github.humbleui.skija.Paint;
import io.github.humbleui.types.Point;
import io.github.humbleui.types.RRect;
import io.github.humbleui.types.Rect;
import net.minecraft.ChatFormatting;
import net.minecraft.client.Minecraft;
import net.minecraft.client.player.AbstractClientPlayer;
import net.minecraft.client.renderer.texture.AbstractTexture;
import net.minecraft.network.chat.Component;
import net.minecraft.network.chat.Style;
import org.lwjgl.opengl.GL11;
import today.starain.module.impl.client.HUD;
import today.starain.ui.Theme;
import today.starain.utils.render.shader.impl.KawaseBlur;
import today.starain.utils.render.skia.context.SkiaContext;
import today.starain.utils.render.skia.image.ImageHelper;
import today.starain.utils.wrapper.Wrapper;

import java.awt.Color;
import java.io.File;
import java.util.Optional;

public class Skia implements Wrapper {

    private static final ImageHelper imageHelper = new ImageHelper();
    private static final int[] COLOR_CODES = new int[]{
            0xFF000000, 0xFF0000AA, 0xFF00AA00, 0xFF00AAAA, 0xFFAA0000, 0xFFAA00AA, 0xFFAA5500, 0xFFAAAAAA,
            0xFF555555, 0xFF5555FF, 0xFF55FF55, 0xFF55FFFF, 0xFFFF5555, 0xFFFF55FF, 0xFFFFFF55, 0xFFFFFFFF
    };
    public static Path blurPath = new Path();
    public static Path shadowPath = new Path();

    public static void drawRect(float x, float y, float width, float height, Color color) {
        getCanvas().drawRect(Rect.makeXYWH(x, y, width, height), getPaint(color));
    }

    public static void drawCircle(float x, float y, float radius, Color color) {
        Paint paint = getPaint(color);
        getCanvas().drawCircle(x, y, radius, paint);
    }

    public static void drawCircle(float x, float y, float radius, float strokeWidth, Color color) {
        Paint paint = getPaint(color);
        paint.setMode(PaintMode.STROKE);
        paint.setStrokeWidth(strokeWidth);
        getCanvas().drawCircle(x, y, radius, paint);
    }

    public static void drawRoundedRect(float x, float y, float width, float height, float radius, Color color) {
        getCanvas().drawRRect(RRect.makeXYWH(x, y, width, height, radius), getPaint(color));
    }

    public static void drawRect(float x, float y, float width, float height, int argb) {
        drawRect(x, y, width, height, new Color(argb, true));
    }

    public static void drawRoundedRect(float x, float y, float width, float height, float radius, int argb) {
        drawRoundedRect(x, y, width, height, radius, new Color(argb, true));
    }

    public static void clipRect(float x, float y, float width, float height) {
        getCanvas().clipRect(Rect.makeXYWH(x, y, width, height), ClipMode.INTERSECT, true);
    }

    /**
     * 锚点就是基线，不像 drawText 那样按每个字符串自己的包围盒归一化，
     * 所以同一个 baselineY 画出来的文字一定共线。
     */
    public static void drawBaselineText(String text, float x, float baselineY, int argb, Font font) {
        if (text == null || text.isEmpty()) return;
        drawStyledText(getCanvas(), text, x, baselineY, font, getPaint(new Color(argb, true)));
    }

    public static void drawGuiBlur(float x, float y, float width, float height) {
        drawBlurClipped(() -> getCanvas().clipRect(Rect.makeXYWH(x, y, width, height), ClipMode.INTERSECT, true));
    }

    public static void drawGuiRoundedBlur(float x, float y, float width, float height, float radius) {
        drawBlurClipped(() -> getCanvas().clipRRect(RRect.makeXYWH(x, y, width, height, radius), ClipMode.INTERSECT, true));
    }

    public static void drawGuiRoundedBlurVarying(float x, float y, float width, float height, float topLeft,
                                                 float topRight, float bottomRight, float bottomLeft) {
        float[] corners = {topLeft, topLeft, topRight, topRight, bottomRight, bottomRight, bottomLeft, bottomLeft};
        drawBlurClipped(() -> getCanvas().clipRRect(RRect.makeComplexXYWH(x, y, width, height, corners), ClipMode.INTERSECT, true));
    }

    /**
     * 把 KawaseBlur 那张全屏纹理贴回来，只留裁剪区。纹理是屏幕空间的，
     * 而画布此刻带着缩放/平移，所以要用当前矩阵反推纹理该画在哪、多大。
     */
    private static void drawBlurClipped(Runnable clip) {
        if (!HUD.isBlurEnabled()) return;
        int textureId = KawaseBlur.INSTANCE.getTexture();
        if (textureId == 0) return;

        save();
        clip.run();
        float[] matrix = getCanvas().getLocalToDeviceAsMatrix33().getMat();
        float drawX = -matrix[2] / matrix[0];
        float drawY = -matrix[5] / matrix[4];
        float screenW = Minecraft.getInstance().getWindow().getWidth() / matrix[0];
        float screenH = Minecraft.getInstance().getWindow().getHeight() / matrix[4];
        drawImage(textureId, drawX, drawY, screenW, screenH, 1.0F, SurfaceOrigin.BOTTOM_LEFT);
        restore();
    }

    public static void drawRoundedRectVarying(float x, float y, float width, float height, float topLeft,
                                              float topRight, float bottomRight, float bottomLeft, Color color) {

        float[] corners = new float[]{topLeft, topLeft, topRight, topRight, bottomRight, bottomRight, bottomLeft,
                bottomLeft};

        getCanvas().drawRRect(RRect.makeComplexXYWH(x, y, width, height, corners), getPaint(color));
    }

    public static void drawBlur(float x, float y, float width, float height, float radius) {
        if (HUD.isBlurEnabled()) {
            blurPath.addRRect(RRect.makeXYWH(x, y, width, height, radius));
        }
    }

    public static void drawShadow(float x, float y, float width, float height, float radius) {
        if (HUD.isShadowEnabled()) {
            shadowPath.addRRect(RRect.makeXYWH(x, y, width, height, radius));
        }
    }

    public static void drawShadow(float x, float y, float width, float height, float radius, Color color) {

        Paint paint = getPaint(color);

        paint.setImageFilter(ImageFilter.makeBlur(2.5F, 2.5F, FilterTileMode.DECAL));

        save();
        clip(x, y, width, height, radius, ClipMode.DIFFERENCE);
        getCanvas().drawRRect(RRect.makeXYWH(x, y, width, height, radius), paint);
        restore();
    }

    public static void drawOutline(float x, float y, float width, float height, float radius, float strokeWidth,
                                   Color color) {

        float halfStroke = strokeWidth / 2;

        Path path = new Path();
        path.addRRect(RRect.makeXYWH(x + halfStroke, y + halfStroke, width - strokeWidth, height - strokeWidth,
                radius - halfStroke));

        Paint paint = getPaint(color);
        paint.setStrokeWidth(strokeWidth);
        paint.setMode(PaintMode.STROKE);

        getCanvas().drawPath(path, paint);
    }

    public static void drawImage(String path, float x, float y, float width, float height) {

        path = "/assets/starain/" + path;

        if (imageHelper.load(path)) {
            getCanvas().drawImageRect(imageHelper.get(path), Rect.makeXYWH(x, y, width, height));
        }
    }

    public static void drawImageRegion(int textureId, float x, float y, float width, float height,
                                       float u0, float v0, float u1, float v1, float alpha) {
        drawImageRegion(textureId, x, y, width, height, u0, v0, u1, v1, alpha, 0);
    }

    public static void drawImageRegion(int textureId, float x, float y, float width, float height,
                                       float u0, float v0, float u1, float v1, float alpha, int shadowArgb) {
        int[] size = getGlTextureSize(textureId);
        if (size[0] <= 0 || size[1] <= 0) {
            return;
        }
        if (imageHelper.load(textureId, size[0], size[1], SurfaceOrigin.TOP_LEFT)) {
            Paint paint = new Paint();
            paint.setAntiAlias(true);
            paint.setAlpha((int) (255 * alpha));
            if (shadowArgb != 0) {
                paint.setImageFilter(ImageFilter.makeDropShadow(0.6F, 0.8F, 0.9F, 0.9F, shadowArgb));
            }
            Rect src = Rect.makeLTRB(u0 * size[0], v0 * size[1], u1 * size[0], v1 * size[1]);
            getCanvas().drawImageRect(imageHelper.get(textureId), src, Rect.makeXYWH(x, y, width, height), paint);
        }
    }

    public static void drawSpriteRegion(int textureId, float x, float y, float width, float height,
                                        float u0, float v0, float u1, float v1, float alpha, int shadowArgb) {
        int[] size = getGlTextureSize(textureId);
        if (size[0] <= 0 || size[1] <= 0) {
            return;
        }
        if (!imageHelper.load(textureId, size[0], size[1], SurfaceOrigin.TOP_LEFT)) {
            return;
        }
        Paint paint = new Paint();
        paint.setAntiAlias(true);
        paint.setAlpha((int) (255 * alpha));
        if (shadowArgb != 0) {
            paint.setImageFilter(ImageFilter.makeDropShadow(0.6F, 0.8F, 0.9F, 0.9F, shadowArgb));
        }
        float left = u0 * size[0];
        float top = v0 * size[1];
        float right = u1 * size[0];
        float bottom = v1 * size[1];
        Rect src = Rect.makeLTRB(left, top, right, bottom);
        SamplingMode sampling = width >= (right - left) ? SamplingMode.DEFAULT : SamplingMode.MITCHELL;
        getCanvas().drawImageRect(imageHelper.get(textureId), src, Rect.makeXYWH(x, y, width, height),
                sampling, paint, true);
        paint.close();
    }

    public static void drawImage(int textureId, float x, float y, float width, float height, float alpha,
                                 SurfaceOrigin origin) {

        if (imageHelper.load(textureId, width, height, origin)) {
            Paint paint = new Paint();
            paint.setAntiAlias(true);
            paint.setAlpha((int) (255 * alpha));
            getCanvas().drawImageRect(imageHelper.get(textureId), Rect.makeXYWH(x, y, width, height), paint);
        }
    }

    public static void drawImage(int textureId, float x, float y, float width, float height, float alpha) {
        drawImage(textureId, x, y, width, height, alpha, SurfaceOrigin.TOP_LEFT);
    }

    public static void drawImage(File file, float x, float y, float width, float height) {
        if (imageHelper.load(file)) {
            getCanvas().drawImageRect(imageHelper.get(file.getName()), Rect.makeXYWH(x, y, width, height));
        }
    }

    public static void drawImage(int textureId, float x, float y, float width, float height, SurfaceOrigin origin) {

        if (imageHelper.load(textureId, width, height, origin)) {
            getCanvas().drawImageRect(imageHelper.get(textureId), Rect.makeXYWH(x, y, width, height));
        }
    }

    public static void drawImage(int textureId, float x, float y, float width, float height) {
        drawImage(textureId, x, y, width, height, SurfaceOrigin.TOP_LEFT);
    }

    public static void drawRoundedImage(int textureId, float x, float y, float width, float height, float radius) {

        Path path = new Path();
        path.addRRect(RRect.makeXYWH(x, y, width, height, radius));

        save();
        getCanvas().clipPath(path, ClipMode.INTERSECT, true);
        drawImage(textureId, x, y, width, height);
        restore();
    }

    public static void drawRoundedImage(String filePath, float x, float y, float width, float height, float radius) {

        Path path = new Path();
        path.addRRect(RRect.makeXYWH(x, y, width, height, radius));

        save();
        getCanvas().clipPath(path, ClipMode.INTERSECT, true);
        drawImage(filePath, x, y, width, height);
        restore();
    }

    public static void drawRoundedImage(File file, float x, float y, float width, float height, float radius) {

        Path path = new Path();
        path.addRRect(RRect.makeXYWH(x, y, width, height, radius));

        save();
        getCanvas().clipPath(path, ClipMode.INTERSECT, true);
        drawImage(file, x, y, width, height);
        restore();
    }

    public static void drawRoundedImage(int textureId, float x, float y, float width, float height, float radius,
                                        float alpha, SurfaceOrigin origin) {
        Path path = new Path();
        path.addRRect(RRect.makeXYWH(x, y, width, height, radius));

        save();
        getCanvas().clipPath(path, ClipMode.INTERSECT, true);
        drawImage(textureId, x, y, width, height, alpha, origin);
        restore();
    }

    public static void drawRoundedImage(int textureId, float x, float y, float width, float height, float radius,
                                        float alpha) {
        drawRoundedImage(textureId, x, y, width, height, radius, alpha, SurfaceOrigin.TOP_LEFT);
    }

    public static void drawPlayerHead(AbstractClientPlayer player, float x, float y, float width, float height, float radius) {
        if (player == null) return;
        var skin = player.getSkinTextureLocation();
        AbstractTexture texture = mc.getTextureManager().getTexture(skin);
        int textureId = texture != null ? texture.getId() : 0;
        if (textureId == 0) return;

        int[] size = getGlTextureSize(textureId);
        if (size[0] <= 0 || size[1] <= 0) return;

        if (imageHelper.load(textureId, size[0], size[1], SurfaceOrigin.TOP_LEFT)) {
            var image = imageHelper.get(textureId);
            if (image == null) return;

            Path path = new Path();
            path.addRRect(RRect.makeXYWH(x, y, width, height, radius));

            Rect srcRect = Rect.makeXYWH(8, 8, 8, 8);
            Rect srcRect1 = Rect.makeXYWH(40, 8, 8, 8);
            Rect dstRect = Rect.makeXYWH(x, y, width, height);

            save();
            getCanvas().clipPath(path, ClipMode.INTERSECT, true);
            getCanvas().drawImageRect(image, srcRect, dstRect, null, false);
            getCanvas().drawImageRect(image, srcRect1, dstRect, null, false);
            restore();
        }
    }

    public static void drawSkin(AbstractClientPlayer player, float x, float y, float scale) {
        if (player == null) return;
        var skin = player.getSkinTextureLocation();
        AbstractTexture texture = mc.getTextureManager().getTexture(skin);
        int textureId = texture != null ? texture.getId() : 0;
        if (textureId == 0) return;

        int[] size = getGlTextureSize(textureId);
        if (size[0] <= 0 || size[1] <= 0) return;

        if (imageHelper.load(textureId, size[0], size[1], SurfaceOrigin.TOP_LEFT)) {
            var image = imageHelper.get(textureId);
            if (image == null) return;

            Rect head = Rect.makeXYWH(8, 8, 8, 8);
            Rect headLayer = Rect.makeXYWH(40, 8, 8, 8);
            Rect body = Rect.makeXYWH(20, 20, 8, 12);
            Rect bodyLayer = Rect.makeXYWH(20, 36, 8, 12);
            Rect leftArm = Rect.makeXYWH(36, 52, 4, 12);
            Rect leftArmLayer = Rect.makeXYWH(52, 52, 4, 12);
            Rect rightArm = Rect.makeXYWH(44, 20, 4, 12);
            Rect rightArmLayer = Rect.makeXYWH(44, 36, 4, 12);
            Rect leftLeg = Rect.makeXYWH(20, 52, 4, 12);
            Rect leftLegLayer = Rect.makeXYWH(4, 52, 4, 12);
            Rect rightLeg = Rect.makeXYWH(4, 20, 4, 12);
            Rect rightLegLayer = Rect.makeXYWH(4, 36, 4, 12);

            save();
            scale(x, y, scale);
            getCanvas().drawImageRect(image, head,
                    Rect.makeXYWH(x + leftArm.getWidth(), y, head.getWidth(), head.getHeight()), null, false);
            getCanvas().drawImageRect(image, headLayer,
                    Rect.makeXYWH(x + leftArm.getWidth(), y, headLayer.getWidth(), headLayer.getHeight()), null, false);
            getCanvas().drawImageRect(image, body,
                    Rect.makeXYWH(x + leftArm.getWidth(), y + head.getHeight(), body.getWidth(), body.getHeight()),
                    null, false);
            getCanvas().drawImageRect(image, bodyLayer, Rect.makeXYWH(x + leftArm.getWidth(),
                    y + headLayer.getHeight(), bodyLayer.getWidth(), bodyLayer.getHeight()), null, false);
            getCanvas().drawImageRect(image, leftArm,
                    Rect.makeXYWH(x, y + head.getHeight(), leftArm.getWidth(), leftArm.getHeight()), null, false);
            getCanvas().drawImageRect(image, leftArmLayer,
                    Rect.makeXYWH(x, y + headLayer.getHeight(), leftArmLayer.getWidth(), leftArmLayer.getHeight()),
                    null, false);
            getCanvas().drawImageRect(image, rightArm,
                    Rect.makeXYWH(x + leftArm.getWidth() + body.getWidth(), y + head.getHeight(), rightArm.getWidth(),
                            rightArm.getHeight()),
                    null, false);
            getCanvas().drawImageRect(image, rightArmLayer,
                    Rect.makeXYWH(x + leftArmLayer.getWidth() + bodyLayer.getWidth(), y + headLayer.getHeight(),
                            rightArmLayer.getWidth(), rightArmLayer.getHeight()),
                    null, false);
            getCanvas().drawImageRect(
                    image, leftLeg, Rect.makeXYWH(x + leftArm.getWidth(),
                            y + head.getHeight() + body.getHeight(), leftLeg.getWidth(), leftLeg.getHeight()),
                    null, false);
            getCanvas().drawImageRect(image, leftLegLayer,
                    Rect.makeXYWH(x + leftArmLayer.getWidth(), y + headLayer.getHeight() + bodyLayer.getHeight(),
                            leftLegLayer.getWidth(), leftLegLayer.getHeight()),
                    null, false);
            getCanvas()
                    .drawImageRect(image, rightLeg,
                            Rect.makeXYWH(x + leftArm.getWidth() + leftLeg.getWidth(),
                                    y + head.getHeight() + body.getHeight(), rightLeg.getWidth(), rightLeg.getHeight()),
                            null, false);
            getCanvas().drawImageRect(image, rightLegLayer,
                    Rect.makeXYWH(x + leftArmLayer.getWidth() + leftLegLayer.getWidth(),
                            y + headLayer.getHeight() + bodyLayer.getHeight(), rightLegLayer.getWidth(),
                            rightLegLayer.getHeight()),
                    null, false);

            restore();
        }
    }

    private static int[] getGlTextureSize(int textureId) {
        try {
            int prev = GL11.glGetInteger(GL11.GL_TEXTURE_BINDING_2D);
            GL11.glBindTexture(GL11.GL_TEXTURE_2D, textureId);
            int w = GL11.glGetTexLevelParameteri(GL11.GL_TEXTURE_2D, 0, GL11.GL_TEXTURE_WIDTH);
            int h = GL11.glGetTexLevelParameteri(GL11.GL_TEXTURE_2D, 0, GL11.GL_TEXTURE_HEIGHT);
            GL11.glBindTexture(GL11.GL_TEXTURE_2D, prev);
            return new int[]{w, h};
        } catch (Throwable t) {
            return new int[]{0, 0};
        }
    }

    public static void drawArc(float x, float y, float radius, float startAngle, float endAngle, float strokeWidth,
                               Color color) {

        Paint paint = getPaint(color);
        paint.setStrokeWidth(strokeWidth);
        paint.setMode(PaintMode.STROKE);

        getCanvas().drawArc(x - radius, y - radius, x + radius, y + radius, startAngle - 90, endAngle, false, paint);
    }

    public static void drawLine(float x, float y, float endX, float endY, float width, Color color) {

        Paint paint = getPaint(color);

        paint.setStroke(true);
        paint.setStrokeWidth(width);
        paint.setAntiAlias(true);

        getCanvas().drawLine(x, y, endX, endY, paint);
    }

    public static void drawGradientRoundedRect(float x, float y, float width, float height, float radius, Color color1,
                                               Color color2) {

        long currentTime = System.nanoTime();
        double speed = 0.0000000006;
        double tick = (currentTime * speed) % (2 * Math.PI);
        float max = Math.max(width, height);

        Path path = new Path();

        path.addRRect(RRect.makeXYWH(x, y, width, height, radius));

        float startX = x + width / 2 - (max / 2) * (float) Math.cos(tick);
        float startY = y + height / 2 - (max / 2) * (float) Math.sin(tick);
        float endX = x + width / 2 + (max / 2) * (float) Math.cos(tick);
        float endY = y + height / 2 + (max / 2) * (float) Math.sin(tick);

        int skColor1 = io.github.humbleui.skija.Color.makeARGB(color1.getAlpha(), color1.getRed(), color1.getGreen(),
                color1.getBlue());
        int skColor2 = io.github.humbleui.skija.Color.makeARGB(color2.getAlpha(), color2.getRed(), color2.getGreen(),
                color2.getBlue());

        int skColorMid = io.github.humbleui.skija.Color.makeARGB(color1.getAlpha(),
                (color1.getRed() + color2.getRed()) / 2, (color1.getGreen() + color2.getGreen()) / 2,
                (color1.getBlue() + color2.getBlue()) / 2);

        Paint paint = new Paint();
        paint.setAntiAlias(true);

        paint.setShader(Shader.makeLinearGradient(new Point(startX, startY), new Point(endX, endY),
                new int[]{skColor1, skColorMid, skColor2}, new float[]{0, 0.5f, 1}));

        getCanvas().drawPath(path, paint);
    }

    public static void clipPath(Path path, ClipMode mode, boolean arg) {
        getCanvas().clipPath(path, mode, arg);
    }

    public static void clipPath(Path path) {
        getCanvas().clipPath(path, ClipMode.INTERSECT, true);
    }

    public static void clip(float x, float y, float width, float height, float radius, ClipMode mode) {

        Path path = new Path();

        path.addRRect(RRect.makeXYWH(x, y, width, height, radius));
        clipPath(path, mode, true);
    }

    public static void clip(float x, float y, float width, float height, float topLeft, float topRight,
                            float bottomRight, float bottomLeft) {

        float[] corners = new float[]{topLeft, topLeft, topRight, topRight, bottomRight, bottomRight, bottomLeft,
                bottomLeft};

        Path path = new Path();

        path.addRRect(RRect.makeComplexXYWH(x, y, width, height, corners));
        clipPath(path, ClipMode.INTERSECT, true);
    }

    public static void clip(float x, float y, float width, float height, float radius) {
        clip(x, y, width, height, radius, ClipMode.INTERSECT);
    }

    private static int getColorIndex(char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    }

    private static void drawStyledText(Canvas canvas, String text, float x, float y, Font font, Paint basePaint) {
        if (text == null || text.isEmpty()) return;
        float currentX = x;
        int originalColor = basePaint.getColor();

        for (int i = 0; i < text.length(); i++) {
            char c = text.charAt(i);

            if (c == '§' && i + 1 < text.length()) {
                char colorCode = text.charAt(i + 1);
                int colorIndex = getColorIndex(colorCode);

                if (colorIndex != -1) {
                    basePaint.setColor(COLOR_CODES[colorIndex]);
                } else if (colorCode == 'r') {
                    basePaint.setColor(originalColor);
                }

                i++;
                continue;
            }

            int nextSection = i;
            StringBuilder sb = new StringBuilder();
            while (nextSection < text.length()) {
                char nc = text.charAt(nextSection);
                if (nc == '§' && nextSection + 1 < text.length()) {
                    break;
                }
                sb.append(nc);
                nextSection++;
            }

            String segment = sb.toString();

            canvas.drawString(segment, currentX, y, font, basePaint);

            currentX += font.measureTextWidth(segment);

            i = nextSection - 1;
        }

        basePaint.setColor(originalColor);
    }

    public static void drawText(String text, float x, float y, Color color, Font font) {
        if (text == null || text.isEmpty()) return;
        Paint paint = getPaint(color);
        Rect bounds = font.measureText(text);
        float bx = x - bounds.getLeft();
        float by = y - bounds.getTop();
        drawStyledText(getCanvas(), text, bx, by, font, paint);
    }

    /**
     * 和 {@link #drawText} 一样，但字体颜色是一条从头到尾的渐变，颜色取自 HUD 的全局主题。
     *
     * <p>色标按字符索引从 {@link Theme#accent(int)} 上采样，所以 Theme Mode / Speed / Spread 都会自动生效：
     * Solid 下所有色标同色，渐变退化成纯色；Gradient / Rainbow 下每个字符的相位依次递增，再由 shader
     * 在色标之间插值出连续的过渡。渐变锚在画布绝对坐标上，因此 §颜色码切出的分段不会打断它——
     * 但段内的 §颜色会被渐变盖掉。
     */
    public static void drawGradientText(String text, float x, float y, Font font) {
        if (text == null || text.isEmpty()) return;
        Rect bounds = font.measureText(text);
        float bx = x - bounds.getLeft();
        float by = y - bounds.getTop();

        String visible = stripFormatting(text);
        Paint paint = new Paint();
        paint.setAntiAlias(true);
        paint.setShader(themeGradient(bx, bx + font.measureTextWidth(visible), visible.length()));
        drawStyledText(getCanvas(), text, bx, by, font, paint);
    }

    public static void drawThemeGradientRect(float x, float y, float width, float height, float gradientStartX,
                                             float gradientEndX, int stops) {
        if (width <= 0 || height <= 0) return;
        Paint paint = new Paint();
        paint.setAntiAlias(true);
        paint.setShader(themeGradient(gradientStartX, gradientEndX, stops));
        getCanvas().drawRect(Rect.makeXYWH(x, y, width, height), paint);
    }

    // 水平方向的主题渐变，一个字符一个色标。stops 至少要有两个，makeLinearGradient 不接受单色标。
    private static Shader themeGradient(float startX, float endX, int stops) {
        int count = Math.max(stops, 2);
        int[] colors = new int[count];
        float[] positions = new float[count];

        for (int i = 0; i < count; i++) {
            Color color = Theme.accent(-i);
            colors[i] = io.github.humbleui.skija.Color.makeARGB(color.getAlpha(), color.getRed(), color.getGreen(),
                    color.getBlue());
            positions[i] = i / (float) (count - 1);
        }

        return Shader.makeLinearGradient(new Point(startX, 0), new Point(endX, 0), colors, positions);
    }

    private static String stripFormatting(String text) {
        StringBuilder stripped = new StringBuilder(text.length());
        for (int i = 0; i < text.length(); i++) {
            char c = text.charAt(i);
            if (c == '§' && i + 1 < text.length()) {
                i++;
            } else {
                stripped.append(c);
            }
        }
        return stripped.toString();
    }

    public static void drawCenteredText(String text, float x, float y, Color color, Font font) {
        if (text == null || text.isEmpty()) return;
        Paint paint = getPaint(color);
        Rect bounds = font.measureText(text);
        float bx = x - bounds.getLeft();
        float by = y - bounds.getTop();
        drawStyledText(getCanvas(), text, bx, by, font, paint);
    }

    public static void drawHeightCenteredText(String text, float x, float y, Color color, Font font) {
        if (text == null || text.isEmpty()) return;

        FontMetrics metrics = font.getMetrics();
        Paint paint = getPaint(color);
        Rect bounds = font.measureText(text);
        float bx = x - bounds.getLeft();
        float by = y - bounds.getTop();
        drawStyledText(getCanvas(), text, bx, by, font, paint);
    }

    public static void drawTextPlain(String text, float x, float y, Color color, Font font) {
        if (text == null || text.isEmpty()) return;
        Rect bounds = font.measureText(text);
        drawStyledText(getCanvas(), text, x - bounds.getLeft(), y - bounds.getTop(), font, getPaint(color));
    }

    public static void drawHeightCenteredTextPlain(String text, float x, float y, Color color, Font font) {
        if (text == null || text.isEmpty()) return;
        FontMetrics metrics = font.getMetrics();
        Rect bounds = font.measureText(text);
        float textCenterY = y + (metrics.getAscent() - metrics.getDescent()) / 2 - metrics.getAscent();
        drawStyledText(getCanvas(), text, x - bounds.getLeft(), textCenterY, font, getPaint(color));
    }

    public static void drawFullCenteredTextPlain(String text, float x, float y, Color color, Font font) {
        if (text == null || text.isEmpty()) return;
        FontMetrics metrics = font.getMetrics();
        Rect bounds = font.measureText(text);
        float textCenterX = x - bounds.getLeft() - (bounds.getWidth() / 2);
        float textCenterY = y + (metrics.getAscent() - metrics.getDescent()) / 2 - metrics.getAscent();
        drawStyledText(getCanvas(), text, textCenterX, textCenterY, font, getPaint(color));
    }

    /**
     * 只画模糊的那一层，不画清晰的文字——调用方自己在上面叠一层
     * {@link #drawHeightCenteredTextPlain} 才能得到发光效果。
     * 基线算法与 Plain 版完全一致，两者用同一个 y 才能重合。
     */
    public static void drawHeightCenteredTextGlow(String text, float x, float y, Color color, Font font, float radius) {
        if (text == null || text.isEmpty()) return;
        FontMetrics metrics = font.getMetrics();
        Rect bounds = font.measureText(text);
        Paint paint = getPaint(color);
        paint.setImageFilter(ImageFilter.makeBlur(radius, radius, FilterTileMode.DECAL));
        float textCenterY = y + (metrics.getAscent() - metrics.getDescent()) / 2 - metrics.getAscent();
        drawStyledText(getCanvas(), text, x - bounds.getLeft(), textCenterY, font, paint);
    }

    public static void drawFullCenteredText(String text, float x, float y, Color color, Font font) {
        if (text == null || text.isEmpty()) return;

        Paint paint = getPaint(color);
        paint.setImageFilter(ImageFilter.makeBlur(1.0F, 1.0F, FilterTileMode.DECAL));
        Rect bounds = font.measureText(text);
        float bx = x - bounds.getLeft();
        float by = y - bounds.getTop();
        drawStyledText(getCanvas(), text, bx, by, font, paint);

        FontMetrics metrics = font.getMetrics();

        float textCenterX = x - bounds.getLeft() - (bounds.getWidth() / 2);
        float textCenterY = y + (metrics.getAscent() - metrics.getDescent()) / 2 - metrics.getAscent();

        drawStyledText(getCanvas(), text, textCenterX, textCenterY, font, getPaint(color));
    }

    public static Rect getTextBounds(String text, Font font) {
        return font.measureText(text);
    }

    public static void drawTextShadow(String text, float x, float y, Color color, Font font) {
        Paint paint = getPaint(color);
        paint.setImageFilter(ImageFilter.makeBlur(2.5F, 2.5F, FilterTileMode.DECAL));
        Rect bounds = font.measureText(text);
        float bx = x - bounds.getLeft();
        float by = y - bounds.getTop();
        getCanvas().drawString(text, bx, by, font, paint);
    }

    public static String getLimitText(String text, Font font, float width) {

        boolean isInRange = false;
        boolean isRemoved = false;

        while (!isInRange) {

            if (getTextBounds(text, font).getWidth() > width - getTextBounds("...", font).getWidth()) {
                text = text.substring(0, text.length() - 1);
                isRemoved = true;
            } else {
                isInRange = true;
            }
        }

        return text + (isRemoved ? "..." : "");
    }

    private static Paint getPaint(Color color) {
        Paint paint = new Paint();
        paint.setARGB(color.getAlpha(), color.getRed(), color.getGreen(), color.getBlue());
        paint.setAntiAlias(true);
        return paint;
    }

    public static void save() {
        getCanvas().save();
    }

    public static void restore() {
        getCanvas().restore();
    }

    public static void scale(float scale) {
        getCanvas().scale(scale, scale);
    }

    public static void scale(float x, float y, float scale) {
        getCanvas().translate(x, y);
        getCanvas().scale(scale, scale);
        getCanvas().translate(-x, -y);
    }

    public static void scale(float x, float y, float width, float height, float scale) {

        float centerX = x + width / 2;
        float centerY = y + height / 2;

        getCanvas().translate(centerX, centerY);
        getCanvas().scale(scale, scale);
        getCanvas().translate(-centerX, -centerY);
    }

    public static void translate(float x, float y) {
        getCanvas().translate(x, y);
    }

    public static void rotate(float x, float y, float width, float height, float rotate) {

        float centerX = x + width / 2;
        float centerY = y + height / 2;

        getCanvas().translate(centerX, centerY);
        getCanvas().rotate(rotate);
        getCanvas().translate(-centerX, -centerY);
    }

    public static void setAlpha(int alpha) {

        Paint paint = new Paint();
        paint.setAlpha(alpha);
        paint.setAntiAlias(true);

        getCanvas().saveLayer(null, paint);
    }

    public static Canvas getCanvas() {
        return SkiaContext.getCanvas();
    }

    public static ImageHelper getImageHelper() {
        return imageHelper;
    }


    public static float getTextWidth(String text, Font font) {
        if (text == null || text.isEmpty()) return 0;
        StringBuilder cleanText = new StringBuilder();

        for (int i = 0; i < text.length(); i++) {
            char c = text.charAt(i);
            if (c == '§' && i + 1 < text.length()) {
                i++;
            } else {
                cleanText.append(c);
            }
        }

        return font.measureTextWidth(cleanText.toString());
    }

    public static String getLegacyString(Component component) {
        StringBuilder builder = new StringBuilder();
        component.visit((style, text) -> {
            if (style.getColor() != null) {
                int textColorRgb = style.getColor().getValue();
                for (ChatFormatting format : ChatFormatting.values()) {
                    if (format.isColor() && format.getColor() != null && format.getColor() == textColorRgb) {
                        builder.append(format);
                        break;
                    }
                }
            }
            if (style.isBold()) builder.append(ChatFormatting.BOLD);
            if (style.isItalic()) builder.append(ChatFormatting.ITALIC);
            if (style.isUnderlined()) builder.append(ChatFormatting.UNDERLINE);
            if (style.isStrikethrough()) builder.append(ChatFormatting.STRIKETHROUGH);
            if (style.isObfuscated()) builder.append(ChatFormatting.OBFUSCATED);

            builder.append(text);
            builder.append(ChatFormatting.RESET);
            return Optional.empty();
        }, Style.EMPTY);

        String result = builder.toString();
        while (result.endsWith("§r")) {
            result = result.substring(0, result.length() - 2);
        }
        return result;
    }
}
