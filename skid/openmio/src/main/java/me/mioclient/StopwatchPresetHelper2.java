package me.mioclient;

import java.awt.Color;
import java.util.ArrayList;
import java.util.List;
import me.mioclient.api.Setting;
import me.mioclient.feature.Progress;
import me.mioclient.feature.Stopwatch;
import net.minecraft.client.gui.DrawContext;
import net.minecraft.client.util.math.MatrixStack;
import net.minecraft.util.math.MathHelper;
import org.joml.Matrix4f;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/StopwatchPresetHelper2.class */
public class StopwatchPresetHelper2 extends SettingSearchHelper419<Color> implements PresetHelper_2 {
    public final List<PresetHelper_5> list;
    public final Stopwatch stopwatch;
    public final Stopwatch stopwatch2;
    public final Stopwatch stopwatch3;
    public boolean flag;
    public boolean flag2;
    public boolean flag3;
    public boolean flag4;
    public boolean flag5;
    public float[] floatArr;
    public final HUDHelper_2 hUDHelper_2;
    public final HUDHelper_2 hUDHelper_22;
    public final HUDHelper_2 hUDHelper_23;
    public final HUDHelper_2 hUDHelper_24;
    public final Progress progress;

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/StopwatchPresetHelper2$Inner.class */
    private static class Inner extends ProgressSettingSearchHelper419 {
        public Inner(StopwatchPresetHelper2 stopwatchPresetHelper2) {
            super(stopwatchPresetHelper2.presetEnumSettingHelper, stopwatchPresetHelper2, new BooleanSetting("", Boolean.valueOf(((ColorSetting) stopwatchPresetHelper2.setting).is2862())));
            this.setting.do2339(() -> {
                ((ColorSetting) stopwatchPresetHelper2.setting).do2863(((Boolean) this.setting.getValue()).booleanValue());
            });
            do1399(Mode_13.PADDING_SHIFT);
        }

        @Override // me.mioclient.ProgressSettingSearchHelper419, me.mioclient.SearchHelper4_19, me.mioclient.PresetHelper_5
        public void do19(DrawContext drawContext, MatrixStack matrixStack, double d, double d2) {
            super.do19(drawContext, matrixStack, d, d2);
            String str = "Rainbow";
            do1670(matrixStack, "Rainbow", () -> {
                FontsSearchHelper4.fontsSearchHelper4.do1691(drawContext, str, (this.presetEnumSettingHelper.getX() + (getPresetEnumSettingHelper1394().get1635() * Float.intBitsToFloat(1056964608))) - (FontsSearchHelper4.fontsSearchHelper4.get1316(str) * Float.intBitsToFloat(1056964608)), ((this.presetEnumSettingHelper.getY() + get1742()) - get1396()) + this.num, getUI1744().textColor.getValue());
            });
        }
    }

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/StopwatchPresetHelper2$Inner_2.class */
    private static class Inner_2 extends SearchHelper419_2 {
        public Inner_2(int i, StopwatchPresetHelper2 stopwatchPresetHelper2) {
            super(i, stopwatchPresetHelper2, "Paste", () -> {
                try {
                    Color color828 = MixinMessageIndicatorHelper_2.getColor828(minecraftClient.keyboard.getClipboard().trim());
                    stopwatchPresetHelper2.do2202(color828);
                    stopwatchPresetHelper2.getSetting1668().do2333(color828);
                    stopwatchPresetHelper2.stopwatch2.reset();
                } catch (Exception e) {
                    stopwatchPresetHelper2.stopwatch.reset();
                }
            });
        }

        @Override // me.mioclient.SearchHelper419_2, me.mioclient.SearchHelper4_19, me.mioclient.PresetHelper_5
        public void do19(DrawContext drawContext, MatrixStack matrixStack, double d, double d2) {
            if (this.stopwatchPresetHelper2.is1669()) {
                return;
            }
            CrosshairHelper.do1707(matrixStack, this.presetEnumSettingHelper.getX() + (this.presetEnumSettingHelper.get1635() / Float.intBitsToFloat(1073741824)) + Float.intBitsToFloat(1056964608), (int) (this.presetEnumSettingHelper.getY() + this.num + get124()), (this.presetEnumSettingHelper.getX() + this.presetEnumSettingHelper.get1635()) - Float.intBitsToFloat(1056964608), (((this.presetEnumSettingHelper.getY() + this.num) + get124()) + get93()) - Float.intBitsToFloat(1056964608), getUI1744().color.getValue());
            super.do19(drawContext, matrixStack, d, d2);
        }

        @Override // me.mioclient.SearchHelper419_2
        public float get123() {
            return ((getPresetEnumSettingHelper1394().get1635() - (getPresetEnumSettingHelper1394().get1635() / Float.intBitsToFloat(1082130432))) - FontsSearchHelper4.fontsSearchHelper4.get1316(getName())) + (FontsSearchHelper4.fontsSearchHelper4.get1316(getName()) * Float.intBitsToFloat(1056964608));
        }

        @Override // me.mioclient.SearchHelper419_2
        public float get124() {
            return 0.0f;
        }

        @Override // me.mioclient.SearchHelper419_2
        public String getName() {
            return !this.stopwatchPresetHelper2.stopwatch.is419(1000L) ? "Invalid" : !this.stopwatchPresetHelper2.stopwatch2.is419(1000L) ? "Pasted" : super.getName();
        }
    }

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/StopwatchPresetHelper2$Inner_3.class */
    private static class Inner_3 extends ProgressSettingSearchHelper419 {
        public Inner_3(StopwatchPresetHelper2 stopwatchPresetHelper2) {
            super(stopwatchPresetHelper2.presetEnumSettingHelper, stopwatchPresetHelper2, new BooleanSetting("", Boolean.valueOf(((ColorSetting) stopwatchPresetHelper2.setting).is2859())));
            this.setting.do2339(() -> {
                ((ColorSetting) stopwatchPresetHelper2.setting).do2860(((Boolean) this.setting.getValue()).booleanValue());
            });
            do1399(Mode_13.PADDING_SHIFT);
        }

        @Override // me.mioclient.ProgressSettingSearchHelper419, me.mioclient.SearchHelper4_19, me.mioclient.PresetHelper_5
        public void do19(DrawContext drawContext, MatrixStack matrixStack, double d, double d2) {
            super.do19(drawContext, matrixStack, d, d2);
            String str = "Sync";
            do1670(matrixStack, "Sync", () -> {
                FontsSearchHelper4.fontsSearchHelper4.do1691(drawContext, str, (this.presetEnumSettingHelper.getX() + (getPresetEnumSettingHelper1394().get1635() * Float.intBitsToFloat(1056964608))) - (FontsSearchHelper4.fontsSearchHelper4.get1316(str) * Float.intBitsToFloat(1056964608)), ((this.presetEnumSettingHelper.getY() + get1742()) - get1396()) + this.num, getUI1744().textColor.getValue());
            });
        }

        @Override // me.mioclient.PresetHelper_5
        public void init() {
            this.setting.do2333(Boolean.valueOf(((ColorSetting) ((StopwatchPresetHelper2) this.presetHelper_2).getSetting1668()).is2859()));
        }
    }

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/StopwatchPresetHelper2$Inner_4.class */
    private static class Inner_4 extends SearchHelper419_2 {
        public Inner_4(int i, StopwatchPresetHelper2 stopwatchPresetHelper2) {
            super(i, stopwatchPresetHelper2, "Copy", () -> {
                minecraftClient.keyboard.setClipboard(MixinMessageIndicatorHelper_2.getString826(stopwatchPresetHelper2.getColor1125(), true));
                stopwatchPresetHelper2.stopwatch3.reset();
            });
        }

        @Override // me.mioclient.SearchHelper419_2, me.mioclient.SearchHelper4_19, me.mioclient.PresetHelper_5
        public void do19(DrawContext drawContext, MatrixStack matrixStack, double d, double d2) {
            if (this.stopwatchPresetHelper2.is1669()) {
                return;
            }
            CrosshairHelper.do1707(matrixStack, this.presetEnumSettingHelper.getX() + 2, (int) (this.presetEnumSettingHelper.getY() + this.num + get124()), (this.presetEnumSettingHelper.getX() + (this.presetEnumSettingHelper.get1635() / Float.intBitsToFloat(1073741824))) - Float.intBitsToFloat(1056964608), (((this.presetEnumSettingHelper.getY() + this.num) + get124()) + get93()) - Float.intBitsToFloat(1056964608), getUI1744().color.getValue());
            super.do19(drawContext, matrixStack, d, d2);
        }

        @Override // me.mioclient.SearchHelper419_2
        public float get123() {
            return (getPresetEnumSettingHelper1394().get1635() / Float.intBitsToFloat(1082130432)) - (FontsSearchHelper4.fontsSearchHelper4.get1316(getName()) / Float.intBitsToFloat(1073741824));
        }

        @Override // me.mioclient.SearchHelper419_2
        public float get124() {
            return 0.0f;
        }

        @Override // me.mioclient.SearchHelper419_2
        public String getName() {
            return !this.stopwatchPresetHelper2.stopwatch3.is419(1000L) ? "Copied" : super.getName();
        }
    }

    public StopwatchPresetHelper2(PresetEnumSettingHelper presetEnumSettingHelper, PresetHelper_2 presetHelper_2, Setting<?> setting) {
        super(presetEnumSettingHelper, presetHelper_2, (ColorSetting) setting);
        this.list = new ArrayList(3);
        this.stopwatch = new Stopwatch();
        this.stopwatch2 = new Stopwatch();
        this.stopwatch3 = new Stopwatch();
        this.floatArr = new float[3];
        this.hUDHelper_2 = new HUDHelper_2(Float.intBitsToFloat(1084227584), true);
        this.hUDHelper_22 = new HUDHelper_2(Float.intBitsToFloat(1084227584), true);
        this.hUDHelper_23 = new HUDHelper_2(Float.intBitsToFloat(1084227584), true);
        this.hUDHelper_24 = new HUDHelper_2(Float.intBitsToFloat(1084227584), true);
        this.progress = new Progress(Float.intBitsToFloat(1082130432), true);
        int i = (int) (this.num + get1743() + getPresetEnumSettingHelper1394().get1635() + Double.longBitsToDouble(4619004367821864960L) + get2201());
        this.list.add(new Inner_4(i, this));
        this.list.add(new Inner_2(i, this));
        if (!((ColorSetting) setting).is2861()) {
            this.list.add(new Inner_3(this));
        }
        this.list.add(new Inner(this));
    }

    @Override // me.mioclient.SearchHelper4_19, me.mioclient.PresetHelper_5
    public void do19(DrawContext drawContext, MatrixStack matrixStack, double d, double d2) {
        super.do19(drawContext, matrixStack, d, d2);
        if (is1669() || !this.flag) {
            this.hUDHelper_2.do171(0.0f);
            this.hUDHelper_22.do171(0.0f);
            this.hUDHelper_23.do171(0.0f);
            this.hUDHelper_24.do171(0.0f);
        }
        if (is1669()) {
            return;
        }
        if (this.flag && this.flag) {
            FontsSearchHelper4_2.mode_5 = Mode_5.STANDARD;
        }
        do1670(matrixStack, this.setting.getName(), () -> {
            FontsSearchHelper4.fontsSearchHelper4.do1691(drawContext, this.setting.getName(), this.presetEnumSettingHelper.getX() + 4, ((this.presetEnumSettingHelper.getY() + get1742()) - get1396()) + this.num, getUI1744().textColor.getValue());
        });
        CrosshairHelper.do1707(matrixStack, (get123() + this.presetEnumSettingHelper.get1635()) - Float.intBitsToFloat(1095237632), get124() + Float.intBitsToFloat(1065353216), (get123() + this.presetEnumSettingHelper.get1635()) - Float.intBitsToFloat(1080033280), get124() + Float.intBitsToFloat(1092616192), getUI1744().color.getValue());
        CrosshairHelper.do1707(matrixStack, (get123() + this.presetEnumSettingHelper.get1635()) - Float.intBitsToFloat(1094713344), get124() + Float.intBitsToFloat(1069547520), (get123() + this.presetEnumSettingHelper.get1635()) - Float.intBitsToFloat(1082130432), get124() + Float.intBitsToFloat(1092091904), MixinMessageIndicatorHelper_2.getColor816((Color) this.setting.getValue(), 255));
        if (this.flag) {
            int alpha = ((Color) this.setting.getValue()).getAlpha();
            int i = get1743();
            if (this.flag3) {
                FontsSearchHelper4_2.mode_5 = Mode_5.POINTER;
                this.floatArr[1] = (float) MathHelper.clamp(((d - get123()) - Double.longBitsToDouble(4617315517961601024L)) / (getPresetEnumSettingHelper1394().get1635() - 8), 0.0d, Double.longBitsToDouble(4607182418800017408L));
                this.floatArr[2] = Float.intBitsToFloat(1065353216) - ((float) MathHelper.clamp((((d2 - get124()) - i) - Double.longBitsToDouble(4613937818241073152L)) / (getPresetEnumSettingHelper1394().get1635() - 8), 0.0d, Double.longBitsToDouble(4607182418800017408L)));
            } else if (this.flag4) {
                FontsSearchHelper4_2.mode_5 = Mode_5.POINTER;
                this.floatArr[0] = Math.min((((float) Math.min(Math.max(get123(), d - Double.longBitsToDouble(4607182418800017408L)), get123() + (getPresetEnumSettingHelper1394().get1635() - 4))) - get123()) / (getPresetEnumSettingHelper1394().get1635() - 4), Float.intBitsToFloat(1065353216));
            } else if (this.flag5 && !this.setting.flag3) {
                FontsSearchHelper4_2.mode_5 = Mode_5.POINTER;
                alpha = (int) (Math.min(Float.intBitsToFloat(1065353216) - ((((float) Math.min(Math.max(get123(), d - Double.longBitsToDouble(4613937818241073152L)), get123() + (getPresetEnumSettingHelper1394().get1635() - 4))) - get123()) / (getPresetEnumSettingHelper1394().get1635() - 4)), Float.intBitsToFloat(1065353216)) * Float.intBitsToFloat(1132396544));
            }
            int HSBtoRGB = Color.HSBtoRGB(this.floatArr[0], Float.intBitsToFloat(1065353216), Float.intBitsToFloat(1065353216));
            float intBitsToFloat = ((HSBtoRGB >> 16) & 255) / Float.intBitsToFloat(1132396544);
            float intBitsToFloat2 = ((HSBtoRGB >> 8) & 255) / Float.intBitsToFloat(1132396544);
            float intBitsToFloat3 = (HSBtoRGB & 255) / Float.intBitsToFloat(1132396544);
            int hashCode = this.flag3 ? -1 : getUI1744().color.getValue().hashCode();
            CrosshairHelper.do1709(matrixStack.peek().getPositionMatrix(), get123() + Float.intBitsToFloat(1075838976), get124() + i + Float.intBitsToFloat(1056964608), getPresetEnumSettingHelper1394().get1635() - 3, getPresetEnumSettingHelper1394().get1635() - 3, hashCode, hashCode, hashCode, hashCode);
            CrosshairHelper.do1709(matrixStack.peek().getPositionMatrix(), get123() + Float.intBitsToFloat(1077936128), get124() + i + Float.intBitsToFloat(1065353216), getPresetEnumSettingHelper1394().get1635() - 4, getPresetEnumSettingHelper1394().get1635() - 4, -1, HSBtoRGB, -16777216, -16777216);
            Matrix4f positionMatrix = matrixStack.peek().getPositionMatrix();
            float f = get123() + Float.intBitsToFloat(1075838976);
            float f2 = ((get124() + getPresetEnumSettingHelper1394().get1635()) + i) - Float.intBitsToFloat(1056964608);
            float f3 = getPresetEnumSettingHelper1394().get1635() - 3;
            float intBitsToFloat4 = Float.intBitsToFloat(1088421888);
            int[] iArr = new int[2];
            iArr[0] = this.flag4 ? -1 : getColor2200().hashCode();
            iArr[1] = this.flag4 ? -1 : getColor2200().hashCode();
            CrosshairHelper.do1710(positionMatrix, f, f2, f3, intBitsToFloat4, iArr);
            CrosshairHelper.do1710(matrixStack.peek().getPositionMatrix(), get123() + Float.intBitsToFloat(1077936128), get124() + getPresetEnumSettingHelper1394().get1635() + i, getPresetEnumSettingHelper1394().get1635() - 4, Float.intBitsToFloat(1086324736), -65536, -256, -16711936, -16711681, -16776961, -65281, -65536);
            do2203(this.hUDHelper_23, this.floatArr[0] * (getPresetEnumSettingHelper1394().get1635() - 2));
            float clamp = MathHelper.clamp(this.hUDHelper_23.get172(), Float.intBitsToFloat(1077936128), getPresetEnumSettingHelper1394().get1635() - 3);
            CrosshairHelper.do1708(matrixStack, ((int) (get123() + clamp)) - Float.intBitsToFloat(1056964608), ((int) ((get124() + getPresetEnumSettingHelper1394().get1635()) + i)) - Float.intBitsToFloat(1056964608), ((int) (get123() + clamp + Float.intBitsToFloat(1073741824))) + Float.intBitsToFloat(1056964608), (int) (get124() + getPresetEnumSettingHelper1394().get1635() + i + Float.intBitsToFloat(1087373312)), this.flag3 ? -1 : getColor2200().hashCode());
            CrosshairHelper.do1708(matrixStack, (int) (get123() + clamp), (int) (get124() + getPresetEnumSettingHelper1394().get1635() + i), (int) (get123() + clamp + Float.intBitsToFloat(1073741824)), (int) (get124() + getPresetEnumSettingHelper1394().get1635() + i + Float.intBitsToFloat(1087373312)), -1);
            if (!this.setting.flag3) {
                Matrix4f positionMatrix2 = matrixStack.peek().getPositionMatrix();
                float f4 = get123() + Float.intBitsToFloat(1075838976);
                float f5 = ((int) get124()) + getPresetEnumSettingHelper1394().get1635() + i + Float.intBitsToFloat(1089470464);
                float f6 = getPresetEnumSettingHelper1394().get1635() - 3;
                float intBitsToFloat5 = Float.intBitsToFloat(1088421888);
                int[] iArr2 = new int[2];
                iArr2[0] = this.flag5 ? -1 : getColor2200().hashCode();
                iArr2[1] = this.flag5 ? -1 : getColor2200().hashCode();
                CrosshairHelper.do1710(positionMatrix2, f4, f5, f6, intBitsToFloat5, iArr2);
                do2198(matrixStack, ((int) get123()) + 3, ((int) get124()) + getPresetEnumSettingHelper1394().get1635() + i + 8, getPresetEnumSettingHelper1394().get1635() - 4, 6, intBitsToFloat, intBitsToFloat2, intBitsToFloat3, alpha / Float.intBitsToFloat(1132396544));
            }
            do2203(this.hUDHelper_2, MathHelper.clamp(this.floatArr[1], 0.0f, Float.intBitsToFloat(1065353216)) * (getPresetEnumSettingHelper1394().get1635() - 8));
            do2203(this.hUDHelper_22, (Float.intBitsToFloat(1065353216) - MathHelper.clamp(this.floatArr[2], 0.0f, Float.intBitsToFloat(1065353216))) * (getPresetEnumSettingHelper1394().get1635() - 8));
            float f7 = get123() + Float.intBitsToFloat(1084227584) + this.hUDHelper_2.get172();
            float f8 = get124() + i + Float.intBitsToFloat(1077936128) + this.hUDHelper_22.get172();
            this.progress.do2139(this.flag3);
            float intBitsToFloat6 = Float.intBitsToFloat(1073741824) * Float.intBitsToFloat(1065353216) * this.progress.get172();
            Color hSBColor = Color.getHSBColor(this.floatArr[0], this.floatArr[1], this.floatArr[2]);
            CrosshairHelper.do1707(matrixStack, (f7 - Float.intBitsToFloat(1069547520)) - intBitsToFloat6, (f8 - Float.intBitsToFloat(1069547520)) - intBitsToFloat6, f7 + Float.intBitsToFloat(1069547520) + intBitsToFloat6, f8 + Float.intBitsToFloat(1069547520) + intBitsToFloat6, this.flag3 ? Color.white : getColor2200());
            CrosshairHelper.do1707(matrixStack, (f7 - Float.intBitsToFloat(1065353216)) - intBitsToFloat6, (f8 - Float.intBitsToFloat(1065353216)) - intBitsToFloat6, f7 + Float.intBitsToFloat(1065353216) + intBitsToFloat6, f8 + Float.intBitsToFloat(1065353216) + intBitsToFloat6, this.flag3 ? hSBColor : hSBColor.darker());
            this.list.forEach(presetHelper_5 -> {
                presetHelper_5.do19(drawContext, matrixStack, d, d2);
            });
            this.setting.do2333(MixinMessageIndicatorHelper_2.getColor816(new Color(Color.HSBtoRGB(this.floatArr[0], this.floatArr[1], this.floatArr[2])), MathHelper.clamp(alpha, 0, 255)));
            if (((ColorSetting) this.setting).is2862()) {
                this.floatArr[0] = Color.RGBtoHSB(((Color) this.setting.getValue()).getRed(), ((Color) this.setting.getValue()).getGreen(), ((Color) this.setting.getValue()).getBlue(), (float[]) null)[0];
            }
        }
        int i2 = (int) (this.num + get1743() + getPresetEnumSettingHelper1394().get1635() + 6 + get2201());
        float intBitsToFloat7 = Float.intBitsToFloat(1065353216);
        for (PresetHelper_5 presetHelper_52 : this.list) {
            presetHelper_52.do91(d, d2);
            presetHelper_52.do653(i2);
            if (!(presetHelper_52 instanceof SearchHelper419_2)) {
                i2 += (int) (presetHelper_52.get93() + intBitsToFloat7);
                presetHelper_52.do653(i2);
                intBitsToFloat7 = Float.intBitsToFloat(1056964608);
            }
        }
    }

    public void do2198(MatrixStack matrixStack, int i, int i2, int i3, int i4, float f, float f2, float f3, float f4) {
        boolean z = true;
        int i5 = i4 / 2;
        int i6 = -i5;
        while (true) {
            int i7 = i6;
            if (i7 >= i3) {
                break;
            }
            if (!z) {
                CrosshairHelper.do1708(matrixStack, i + i7, i2, i + i7 + i5, i2 + i4, -1);
                CrosshairHelper.do1708(matrixStack, i + i7, i2 + i5, i + i7 + i5, i2 + i4, -7303024);
                if (i7 < i3 - i5) {
                    int i8 = i + i7 + i5;
                    int min = Math.min(i + i3, i + i7 + (i5 * 2));
                    CrosshairHelper.do1708(matrixStack, i8, i2, min, i2 + i4, -7303024);
                    CrosshairHelper.do1708(matrixStack, i8, i2 + i5, min, i2 + i4, -1);
                }
            }
            z = !z;
            i6 = i7 + i5;
        }
        CrosshairHelper.do1710(matrixStack.peek().getPositionMatrix(), i, i2, i3, i4, new Color(f, f2, f3, Float.intBitsToFloat(1065353216)).hashCode(), 0);
        do2203(this.hUDHelper_24, i3 - ((i3 - 2) * f4));
        float f5 = (i + this.hUDHelper_24.get172()) - Float.intBitsToFloat(1065353216);
        CrosshairHelper.do1707(matrixStack, f5 - Float.intBitsToFloat(1069547520), i2 - Float.intBitsToFloat(1056964608), f5 + Float.intBitsToFloat(1069547520), i2 + i4 + Float.intBitsToFloat(1056964608), this.flag5 ? Color.white : getColor2200());
        CrosshairHelper.do1708(matrixStack, f5 - Float.intBitsToFloat(1065353216), i2, f5 + Float.intBitsToFloat(1065353216), i2 + i4, -1);
    }

    @Override // me.mioclient.SettingSearchHelper419, me.mioclient.SearchHelper4_19, me.mioclient.PresetHelper_5
    public void do20(double d, double d2, int i) {
        if (is1669()) {
            return;
        }
        int i2 = get1743();
        if (is2199(d, d2) && i == 1) {
            this.flag = !this.flag;
        }
        if (this.flag) {
            this.list.forEach(presetHelper_5 -> {
                presetHelper_5.do20(d, d2, i);
            });
        }
        if (this.flag && i == 0) {
            if (d > get123() + Float.intBitsToFloat(1073741824) && d < (get123() + getPresetEnumSettingHelper1394().get1635()) - Float.intBitsToFloat(1073741824) && d2 > get124() + i2 && d2 < ((get124() + i2) + getPresetEnumSettingHelper1394().get1635()) - Float.intBitsToFloat(1073741824)) {
                this.flag3 = true;
            }
            if (d > get123() + Float.intBitsToFloat(1073741824) && d < (get123() + getPresetEnumSettingHelper1394().get1635()) - Float.intBitsToFloat(1073741824) && d2 > get124() + i2 + getPresetEnumSettingHelper1394().get1635() && d2 < get124() + i2 + getPresetEnumSettingHelper1394().get1635() + Float.intBitsToFloat(1086324736)) {
                this.flag4 = true;
            }
            if (d > get123() + Float.intBitsToFloat(1073741824) && d < (get123() + getPresetEnumSettingHelper1394().get1635()) - Float.intBitsToFloat(1073741824) && d2 > get124() + i2 + getPresetEnumSettingHelper1394().get1635() + Float.intBitsToFloat(1090519040) && d2 < get124() + i2 + getPresetEnumSettingHelper1394().get1635() + Float.intBitsToFloat(1096810496) && !this.setting.flag3) {
                this.flag5 = true;
            }
            super.do20(d, d2, i);
        }
    }

    @Override // me.mioclient.PresetHelper_5
    public void do88(double d, double d2, int i) {
        if (is1669()) {
            return;
        }
        this.list.forEach(presetHelper_5 -> {
            presetHelper_5.do88(d, d2, i);
        });
        super.do88(d, d2, i);
        this.flag5 = false;
        this.flag4 = false;
        this.flag3 = false;
    }

    @Override // me.mioclient.PresetHelper_5
    public void do90(char c) {
        if (is1669()) {
            return;
        }
        this.list.forEach(presetHelper_5 -> {
            presetHelper_5.do90(c);
        });
        super.do90(c);
    }

    @Override // me.mioclient.PresetHelper_5
    public void do89(int i) {
        if (is1669()) {
            return;
        }
        this.list.forEach(presetHelper_5 -> {
            presetHelper_5.do89(i);
        });
        super.do89(i);
    }

    @Override // me.mioclient.SettingSearchHelper419, me.mioclient.PresetHelper_5
    public int get93() {
        if (!this.flag || is1669()) {
            return super.get93();
        }
        int i = (int) (getPresetEnumSettingHelper1394().get1635() + Float.intBitsToFloat(1088421888) + (get1743() * 2) + get2201());
        if (this.list.size() > 2) {
            i = (int) (i + (get1743() * (this.list.size() - 2)) + Float.intBitsToFloat(1056964608));
        }
        return i;
    }

    @Override // me.mioclient.PresetHelper_5
    public void init() {
        do2202((Color) this.setting.getValue());
        this.list.forEach((v0) -> {
            v0.init();
        });
    }

    @Override // me.mioclient.PresetHelper_2
    public boolean isClosed() {
        return super.presetHelper_2.isClosed() || !this.flag;
    }

    public boolean is2199(double d, double d2) {
        return d > ((double) this.presetEnumSettingHelper.getX()) && d < ((double) (this.presetEnumSettingHelper.getX() + this.presetEnumSettingHelper.get1635())) && d2 > ((double) (this.presetEnumSettingHelper.getY() + this.num)) && d2 < ((double) ((this.presetEnumSettingHelper.getY() + get1743()) + this.num));
    }

    public float get123() {
        return this.presetEnumSettingHelper.getX();
    }

    public float get124() {
        return this.presetEnumSettingHelper.getY() + this.num;
    }

    public Color getColor2200() {
        return MixinMessageIndicatorHelper_2.getColor817(getUI1744().color.getValue(), Float.intBitsToFloat(1065353216));
    }

    public float get2201() {
        return this.setting.flag3 ? Float.intBitsToFloat(1073741824) : Float.intBitsToFloat(1092616192);
    }

    @Override // me.mioclient.SearchHelper4_19, me.mioclient.PresetHelper_5
    public boolean is92(double d, double d2) {
        return super.is92(d, d2) || this.flag4 || this.flag3 || this.flag5;
    }

    public void do2202(Color color) {
        float[] RGBtoHSB = Color.RGBtoHSB(color.getRed(), color.getGreen(), color.getBlue(), (float[]) null);
        this.floatArr[0] = RGBtoHSB[0];
        this.floatArr[1] = RGBtoHSB[1];
        this.floatArr[2] = RGBtoHSB[2];
    }

    public Color getColor1125() {
        return MixinMessageIndicatorHelper_2.getColor816(Color.getHSBColor(this.floatArr[0], this.floatArr[1], this.floatArr[2]), ((Color) this.setting.getValue()).getAlpha());
    }

    public void do2203(HUDHelper_2 hUDHelper_2, float f) {
        if (this.flag2) {
            hUDHelper_2.do171(f);
        } else {
            hUDHelper_2.do1737(f);
        }
    }
}
