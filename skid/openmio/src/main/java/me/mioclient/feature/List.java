package me.mioclient.feature;

import java.util.Iterator;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.PresetEnumSettingHelper;
import me.mioclient.PresetFontsSearchHelper42;
import me.mioclient.PresetHelper_5;
import me.mioclient.PresetSearchHelper4;
import me.mioclient.PresetSearchHelper419_2;
import net.minecraft.client.gui.DrawContext;
import net.minecraft.client.util.math.MatrixStack;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/feature/List.class */
public class List extends PresetEnumSettingHelper {
    public final PresetFontsSearchHelper42 presetFontsSearchHelper42;
    public boolean flag;

    /* JADX INFO: Access modifiers changed from: private */
    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/feature/List$Inner.class */
    public static final class Inner extends PresetSearchHelper419_2 {
        public final PresetFontsSearchHelper42 presetFontsSearchHelper42;
        public final PresetSearchHelper4 presetSearchHelper4;

        public Inner(PresetFontsSearchHelper42 presetFontsSearchHelper42, PresetEnumSettingHelper presetEnumSettingHelper, PresetSearchHelper4 presetSearchHelper4) {
            super(presetEnumSettingHelper, presetSearchHelper4.getName(), () -> {
                presetFontsSearchHelper42.getPreset2319().do1032(presetSearchHelper4);
                presetFontsSearchHelper42.getPreset2319().do466();
            });
            this.presetSearchHelper4 = presetSearchHelper4;
            this.presetFontsSearchHelper42 = presetFontsSearchHelper42;
        }

        @Override // me.mioclient.PresetSearchHelper419_2
        public boolean is960() {
            return this.presetSearchHelper4.equals(this.presetFontsSearchHelper42.getPreset2319().getPresetSearchHelper41033());
        }
    }

    public List(PresetFontsSearchHelper42 presetFontsSearchHelper42) {
        super("List");
        this.presetFontsSearchHelper42 = presetFontsSearchHelper42;
    }

    @Override // me.mioclient.PresetEnumSettingHelper, me.mioclient.PresetHelper_5
    public void do19(DrawContext drawContext, MatrixStack matrixStack, double d, double d2) {
        if (this.flag) {
            Iterator<PresetHelper_5> it = this.registry.iterator();
            while (it.hasNext()) {
                it.next().do91(d, d2);
            }
            do466();
            this.flag = false;
        }
        super.do19(drawContext, matrixStack, d, d2);
    }

    public void do34() {
        this.flag = true;
        this.registry.clear();
        Iterator it = BaritoneHelper_3.presetHelper.getPresetHelperSearchHelper4_273(this.presetFontsSearchHelper42.getPresetHelperMode2320()).getRegistry().iterator();
        while (it.hasNext()) {
            this.registry.add(new Inner(this.presetFontsSearchHelper42, this, (PresetSearchHelper4) it.next()));
        }
    }
}
