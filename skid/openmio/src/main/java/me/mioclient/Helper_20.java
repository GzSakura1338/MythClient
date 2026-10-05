package me.mioclient;

import java.awt.Color;
import java.util.Objects;
import me.mioclient.module.client.UI;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.font.TextRenderer;
import net.minecraft.client.gui.DrawContext;
import net.minecraft.util.math.MathHelper;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Helper_20.class */
public class Helper_20 {
    public final float val2 = (float) MathHelper.clamp(Math.random(), Double.longBitsToDouble(4589708452267294720L), Double.longBitsToDouble(4595653204011646976L));
    public final float val3 = PingSpoofHelper.get370((float) FreecamHelper.val2, Float.intBitsToFloat(1066611507));
    public final double val4 = Math.random();
    public final double val5 = PingSpoofHelper.get370(Float.intBitsToFloat(-1123603710), Float.intBitsToFloat(1023879938));
    public float val6 = PingSpoofHelper.get370(Float.intBitsToFloat(-1092196762), 0.0f);
    public static final float val = Float.intBitsToFloat(1023879938);
    public static final String string = "❆";

    public void do364(DrawContext drawContext) {
        TextRenderer textRenderer = MinecraftClient.getInstance().textRenderer;
        float scaledWindowWidth = (float) ((this.val4 + (this.val6 * this.val5)) * drawContext.getScaledWindowWidth());
        float scaledWindowHeight = this.val6 * drawContext.getScaledWindowHeight() * Float.intBitsToFloat(1056964608);
        Objects.requireNonNull(textRenderer);
        float intBitsToFloat = (scaledWindowHeight - Float.intBitsToFloat(1091567616)) - Float.intBitsToFloat(1065353216);
        this.val6 += BaritoneHelper_3.hitmarkerSearchHelper4.get3095(Float.intBitsToFloat(1056964608)) * this.val2;
        this.val6 = Math.min(this.val6, Float.intBitsToFloat(1065353216));
        if (!UI.uI.snow.getValue().booleanValue() || this.val6 < 0.0f) {
            return;
        }
        int ceil = MathHelper.ceil(Math.max(Float.intBitsToFloat(1132396544) - (this.val6 * Float.intBitsToFloat(1132396544)), Float.intBitsToFloat(1082130432)));
        drawContext.getMatrices().push();
        drawContext.getMatrices().scale(this.val3, this.val3, Float.intBitsToFloat(1065353216));
        drawContext.getMatrices().translate(scaledWindowWidth / this.val3, intBitsToFloat / this.val3, 0.0f);
        drawContext.drawCenteredTextWithShadow(textRenderer, "❆", 0, 0, new Color(255, 255, 255, ceil).hashCode());
        drawContext.getMatrices().pop();
    }

    public double get515() {
        return this.val4;
    }

    public boolean is2378() {
        return this.val6 >= Float.intBitsToFloat(1065353216);
    }
}
