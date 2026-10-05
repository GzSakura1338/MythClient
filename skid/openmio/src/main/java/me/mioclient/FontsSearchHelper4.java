package me.mioclient;

import java.awt.Color;
import java.util.Objects;
import me.mioclient.module.client.Fonts;
import me.mioclient.module.player.NameProtect;
import net.minecraft.client.font.TextRenderer;
import net.minecraft.client.gui.DrawContext;
import net.minecraft.text.Text;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/FontsSearchHelper4.class */
public class FontsSearchHelper4 implements SearchHelper_4 {
    public static final NameProtect nameprotect = (NameProtect) BaritoneHelper_3.baritoneHelper_4.getModule117(NameProtect.class);
    public static final FontsSearchHelper4 fontsSearchHelper4 = new FontsSearchHelper4();
    public static int num = 0;
    public AdvanceGlyph advanceGlyph;

    public static String getString1684(Object obj) {
        return Character.toUpperCase(obj.toString().charAt(0)) + obj.toString().toLowerCase().substring(1);
    }

    public static boolean is1685(String str, char c) {
        for (int i = 0; i < str.length(); i++) {
            if (str.charAt(i) == c) {
                return true;
            }
        }
        return false;
    }

    public AdvanceGlyph getAdvanceGlyph1686() {
        if (this.advanceGlyph == null) {
            this.advanceGlyph = getAdvanceGlyph1688();
        }
        return this.advanceGlyph;
    }

    public void do1687(AdvanceGlyph advanceGlyph) {
        if (advanceGlyph == null) {
            return;
        }
        this.advanceGlyph = advanceGlyph;
        System.gc();
    }

    public AdvanceGlyph getAdvanceGlyph1688() {
        Fonts fonts = Fonts.fonts;
        return AdvanceGlyph.getAdvanceGlyph2758(fonts.font.getValue(), ((Number) fonts.size.getValue()).intValue(), fonts.style.getValue());
    }

    public void do1689(DrawContext drawContext, String str, float f, float f2, Color color) {
        do1690(drawContext, Text.of(str), f, f2, color);
    }

    public void do1690(DrawContext drawContext, Text text, float f, float f2, Color color) {
        if (Fonts.fonts.isToggled()) {
            getAdvanceGlyph1686().get2765(drawContext.getMatrices(), text.asOrderedText(), f, f2, color.hashCode(), false);
        } else {
            minecraftClient.textRenderer.draw(text, (int) f, (int) f2, color.hashCode(), false, drawContext.getMatrices().peek().getPositionMatrix(), getAdvanceGlyph1686().getVertexConsumerProvider2775(), TextRenderer.TextLayerType.NORMAL, 0, 15728880);
        }
    }

    public void do1691(DrawContext drawContext, String str, float f, float f2, Color color) {
        do1692(drawContext, Text.of(str), f, f2, color);
    }

    public void do1692(DrawContext drawContext, Text text, float f, float f2, Color color) {
        if (Fonts.fonts.isToggled()) {
            getAdvanceGlyph1686().get2765(drawContext.getMatrices(), text.asOrderedText(), f, f2, color.hashCode(), true);
        } else {
            minecraftClient.textRenderer.draw(text, (int) f, (int) f2, color.hashCode(), true, drawContext.getMatrices().peek().getPositionMatrix(), getAdvanceGlyph1686().getVertexConsumerProvider2775(), TextRenderer.TextLayerType.NORMAL, 0, 15728880);
        }
    }

    public void do1693(DrawContext drawContext, String str, float f, float f2, float f3, Color color) {
        do1694(drawContext, Text.of(str), f, f2, f3, color);
    }

    public void do1694(DrawContext drawContext, Text text, float f, float f2, float f3, Color color) {
        drawContext.getMatrices().push();
        drawContext.getMatrices().scale(f3, f3, f3);
        do1690(drawContext, text, f / f3, f2 / f3, color);
        drawContext.getMatrices().scale(1.0f / f3, 1.0f / f3, 1.0f / f3);
        drawContext.getMatrices().pop();
    }

    public void do1695(DrawContext drawContext, String str, float f, float f2, float f3, Color color) {
        do1696(drawContext, Text.of(str), f, f2, f3, color);
    }

    public void do1696(DrawContext drawContext, Text text, float f, float f2, float f3, Color color) {
        drawContext.getMatrices().push();
        drawContext.getMatrices().scale(f3, f3, f3);
        do1692(drawContext, text, f / f3, f2 / f3, color);
        drawContext.getMatrices().scale(1.0f / f3, 1.0f / f3, 1.0f / f3);
        drawContext.getMatrices().pop();
    }

    public void do1597() {
        getAdvanceGlyph1686().do1597();
    }

    public float get1316(String str) {
        if (nameprotect.isToggled() && !is1469()) {
            str = str.replace(minecraftClient.player.getName().getString(), nameprotect.name.getValue());
        }
        return Fonts.fonts.isToggled() ? getAdvanceGlyph1686().get2772(str) : minecraftClient.textRenderer.getWidth(str);
    }

    public int get93() {
        if (Fonts.fonts.isToggled()) {
            return getAdvanceGlyph1686().get2771() - 1;
        }
        if (minecraftClient.textRenderer == null) {
            return 9;
        }
        Objects.requireNonNull(minecraftClient.textRenderer);
        return 9;
    }

    public static int get1697() {
        return Fonts.fonts.shift.getValue().intValue();
    }

    public static float get1698() {
        return Fonts.fonts.shadow.getValue().floatValue();
    }

    public static int get1699() {
        return Fonts.fonts.translate.getValue().intValue();
    }

    public static void do1700(int i) {
        num = i;
    }

    public static int get1701() {
        return num;
    }

    public static String getString1702(String str) {
        return str.replaceAll("[^a-zA-Z0-9.\\-]", "_");
    }
}
