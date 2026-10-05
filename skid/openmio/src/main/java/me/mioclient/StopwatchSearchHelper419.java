package me.mioclient;

import java.awt.Color;
import me.mioclient.feature.Stopwatch;
import net.minecraft.client.gui.DrawContext;
import net.minecraft.client.util.math.MatrixStack;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/StopwatchSearchHelper419.class */
public abstract class StopwatchSearchHelper419 extends SearchHelper4_19 {
    public final int num;
    public final int num2;
    public final Stopwatch stopwatch;

    public StopwatchSearchHelper419(PresetEnumSettingHelper presetEnumSettingHelper, int i, int i2, int i3) {
        super(presetEnumSettingHelper, i);
        this.stopwatch = new Stopwatch();
        this.num = i2;
        this.num2 = i3;
    }

    public StopwatchSearchHelper419(PresetEnumSettingHelper presetEnumSettingHelper, int i) {
        this(presetEnumSettingHelper, i, 22, 22);
    }

    @Override // me.mioclient.SearchHelper4_19, me.mioclient.PresetHelper_5
    public void do19(DrawContext drawContext, MatrixStack matrixStack, double d, double d2) {
        super.do19(drawContext, matrixStack, d, d2);
        if (this.stopwatch.is419(get1639())) {
            do1638();
            this.stopwatch.reset();
        }
    }

    public abstract void do1638();

    public abstract int get1639();

    public void do2518(MatrixStack matrixStack, int i, int i2, Color color) {
        CrosshairHelper.do1707(matrixStack, getPresetEnumSettingHelper1394().getX() + (i * get2519()), getPresetEnumSettingHelper1394().getY() + this.num + (i2 * get2520()), getPresetEnumSettingHelper1394().getX() + (i * get2519()) + get2519(), getPresetEnumSettingHelper1394().getY() + this.num + (i2 * get2520()) + get2520(), color);
    }

    public int get2519() {
        return getPresetEnumSettingHelper1394().get1635() / this.num;
    }

    public int get2520() {
        return get93() / this.num2;
    }
}
