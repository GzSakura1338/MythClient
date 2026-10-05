package me.mioclient;

import me.mioclient.api.Setting;
import net.minecraft.client.util.math.MatrixStack;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/SettingSearchHelper419.class */
public abstract class SettingSearchHelper419<T> extends SearchHelper4_19 {
    public final PresetHelper_2 presetHelper_2;
    public Setting<T> setting;
    public final ZoomHelper zoomHelper;
    public boolean flag;
    public long scrollTime;
    public boolean flag2;

    public SettingSearchHelper419(PresetEnumSettingHelper presetEnumSettingHelper, Setting<T> setting) {
        this(presetEnumSettingHelper, () -> {
            return false;
        }, setting);
    }

    public SettingSearchHelper419(PresetEnumSettingHelper presetEnumSettingHelper, PresetHelper_2 presetHelper_2, Setting<T> setting) {
        super(presetEnumSettingHelper, 0);
        this.zoomHelper = new ZoomHelper();
        this.presetHelper_2 = presetHelper_2;
        this.setting = setting;
    }

    public int get93() {
        if (this.setting.is2349()) {
            return get1743();
        }
        return 0;
    }

    public PresetHelper_2 getPresetHelper_21667() {
        return this.presetHelper_2;
    }

    public Setting<T> getSetting1668() {
        return this.setting;
    }

    public boolean is1669() {
        boolean z = !this.setting.is2349() || this.presetHelper_2.isClosed();
        if (z) {
            this.setting.flag = false;
        }
        return z;
    }

    public void do1670(MatrixStack matrixStack, String str, java.lang.Runnable runnable) {
        matrixStack.push();
        do1671(FontsSearchHelper4.fontsSearchHelper4.get1316(str));
        float f = -this.zoomHelper.get172();
        boolean z = (f == 0.0f || (this instanceof PresetSettingSearchHelper419)) ? false : true;
        if (z) {
            ArrayListPresetHelper2 arrayListPresetHelper2 = (ArrayListPresetHelper2) this.presetHelper_2;
            CrosshairHelper.do1597();
            SearchHelper4_17.do1106(this.presetEnumSettingHelper.getX() + get1397(), this.presetEnumSettingHelper.getY() + arrayListPresetHelper2.get1395(), this.presetEnumSettingHelper.get1635() - get1397(), arrayListPresetHelper2.get93());
        }
        matrixStack.translate(f, 0.0f, 0.0f);
        runnable.run();
        if (z) {
            CrosshairHelper.do1597();
            SearchHelper4_17.do1107();
        }
        matrixStack.pop();
    }

    public void do1671(float f) {
        if (f <= this.presetEnumSettingHelper.get1635() - 4) {
            this.zoomHelper.do171(0.0f);
            return;
        }
        if (this.flag) {
            if (!this.flag) {
                this.scrollTime = System.currentTimeMillis();
            }
            if (System.currentTimeMillis() > this.scrollTime + 750) {
                this.zoomHelper.do169(f - (this.presetEnumSettingHelper.get1635() - 4), 500L);
            }
        } else {
            this.zoomHelper.do169(0.0f, 500L);
        }
        this.flag = this.flag;
    }

    @Override // me.mioclient.SearchHelper4_19, me.mioclient.PresetHelper_5
    public void do20(double d, double d2, int i) {
        if (is1669()) {
            return;
        }
        super.do20(d, d2, i);
    }

    public MixinTitleScreenSearchHelper4 getMixinTitleScreenSearchHelper41672() {
        return BaritoneHelper_3.getMixinTitleScreenSearchHelper42216();
    }
}
