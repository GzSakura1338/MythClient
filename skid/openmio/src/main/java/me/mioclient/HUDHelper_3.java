package me.mioclient;

import net.minecraft.client.gui.DrawContext;
import net.minecraft.client.util.math.MatrixStack;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/HUDHelper_3.class */
public abstract class HUDHelper_3 extends SearchHelper419_2 implements HUDHelper {
    public boolean toggled;

    public HUDHelper_3(int i, StopwatchPresetHelper2 stopwatchPresetHelper2, String str, java.lang.Runnable runnable) {
        super(i, stopwatchPresetHelper2, str, runnable);
        this.runnable = () -> {
            runnable.run();
            this.toggled = !this.toggled;
        };
    }

    @Override // me.mioclient.SearchHelper419_2, me.mioclient.SearchHelper4_19, me.mioclient.PresetHelper_5
    public void do19(DrawContext drawContext, MatrixStack matrixStack, double d, double d2) {
        if (this.stopwatchPresetHelper2.is1669()) {
            return;
        }
        FontsSearchHelper4.fontsSearchHelper4.do1691(drawContext, this.name, this.presetEnumSettingHelper.getX() + get123(), (((this.presetEnumSettingHelper.getY() + this.num) + get124()) + get1742()) - get1396(), this.toggled ? getUI1744().color.getValue() : getUI1744().textColor.getValue());
    }

    @Override // me.mioclient.HUDHelper
    public boolean isToggled() {
        return this.toggled;
    }

    @Override // me.mioclient.HUDHelper
    public void enable() {
    }

    @Override // me.mioclient.HUDHelper
    public void disable() {
    }
}
