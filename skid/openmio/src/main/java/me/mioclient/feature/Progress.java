package me.mioclient.feature;

import java.util.function.Supplier;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.HUDHelper_2;
import net.minecraft.util.math.MathHelper;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/feature/Progress.class */
public class Progress extends HUDHelper_2 {
    public Progress(float f, boolean z) {
        super(f, z);
    }

    public Progress(float f) {
        super(f);
    }

    public Progress(Supplier<Float> supplier, boolean z) {
        super(supplier, z);
    }

    @Override // me.mioclient.HUDHelper_2
    public float get172() {
        float f = BaritoneHelper_3.hitmarkerSearchHelper4.get3095(Float.intBitsToFloat(1056964608)) * this.supplier.get().floatValue();
        if (this.flag) {
            double longBitsToDouble = Double.longBitsToDouble(4607182418800017408L);
            float intBitsToFloat = f - Float.intBitsToFloat(1065353216);
            f = (float) (longBitsToDouble - (intBitsToFloat * Math.pow(intBitsToFloat, Double.longBitsToDouble(4613937818241073152L))));
        }
        this.val = this.val3 + ((this.val - this.val3) * f);
        if (this.val == this.val2 || Math.abs(this.val2 - this.val) < Double.longBitsToDouble(4576918229304087675L)) {
            do171(this.val2);
        }
        if (BaritoneHelper_3.hitmarkerSearchHelper4.get3094() <= 15) {
            this.val = this.val2;
        }
        return MathHelper.clamp(this.val - Float.intBitsToFloat(1065353216), 0.0f, Float.intBitsToFloat(1065353216));
    }

    @Override // me.mioclient.HUDHelper_2
    public void do1737(float f) {
        super.do1737(f + Float.intBitsToFloat(1065353216));
    }

    public float get2138() {
        return this.val;
    }

    public void do2139(boolean z) {
        do1737(z ? Float.intBitsToFloat(1065353216) : 0.0f);
    }

    public void do2140(boolean z) {
        do171(z ? Float.intBitsToFloat(1073741824) : Float.intBitsToFloat(1065353216));
    }

    public void reset() {
        do1737(0.0f);
    }
}
