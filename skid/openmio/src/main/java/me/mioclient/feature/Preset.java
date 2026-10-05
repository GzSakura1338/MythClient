package me.mioclient.feature;

import com.google.gson.JsonElement;
import java.io.IOException;
import me.mioclient.ArgumentTypeHelper;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.EnumSetting;
import me.mioclient.PresetEnumSettingHelper;
import me.mioclient.PresetFontsSearchHelper42;
import me.mioclient.PresetHelperMode;
import me.mioclient.PresetHelperSearchHelper4_2;
import me.mioclient.PresetHelper_2;
import me.mioclient.PresetHelper_5;
import me.mioclient.PresetMode;
import me.mioclient.PresetSearchHelper4;
import me.mioclient.PresetSearchHelper419;
import me.mioclient.PresetSearchHelper419_2;
import me.mioclient.PresetSettingSearchHelper419;
import me.mioclient.PresetSettingSearchHelper419_2;
import me.mioclient.StringSetting;
import me.mioclient.api.Setting;
import net.minecraft.client.gui.DrawContext;
import net.minecraft.client.util.math.MatrixStack;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/feature/Preset.class */
public class Preset extends PresetEnumSettingHelper {
    public Setting<String> setting;
    public Setting<PresetHelperMode> setting2;
    public final PresetFontsSearchHelper42 presetFontsSearchHelper42;
    public PresetMode presetMode;
    public PresetSearchHelper4 presetSearchHelper4;
    public boolean flag;

    public Preset(PresetFontsSearchHelper42 presetFontsSearchHelper42) {
        super("Preset");
        this.setting = new StringSetting("Save", "");
        this.setting2 = new EnumSetting("Category", PresetHelperMode.MODULES);
        this.presetMode = PresetMode.NEW;
        this.presetFontsSearchHelper42 = presetFontsSearchHelper42;
        this.setting.do2339(() -> {
            try {
                getPresetHelperSearchHelper4_21031().do39(this.setting.getValue());
                presetFontsSearchHelper42.getList2318().do34();
            } catch (Exception e) {
                e.printStackTrace();
            }
            this.setting.do2333("");
        });
        this.setting2.do2339(() -> {
            presetFontsSearchHelper42.do2321(this.setting2.getValue());
            presetFontsSearchHelper42.getList2318().do34();
        });
    }

    @Override // me.mioclient.PresetEnumSettingHelper, me.mioclient.PresetHelper_5
    public void init() {
        do34();
    }

    @Override // me.mioclient.PresetEnumSettingHelper, me.mioclient.PresetHelper_5
    public void do19(DrawContext drawContext, MatrixStack matrixStack, double d, double d2) {
        if (this.flag) {
            do34();
            this.flag = false;
        }
        super.do19(drawContext, matrixStack, d, d2);
    }

    public void do34() {
        this.registry.clear();
        switch (this.presetMode) {
            case NEW:
                do1030(new PresetSettingSearchHelper419(this, PresetHelper_2.presetHelper_2, this.setting2));
                do1030(new PresetSettingSearchHelper419_2(this, PresetHelper_2.presetHelper_2, this.setting));
                do1030(new PresetSearchHelper419_2(this, "Refresh", () -> {
                    getPresetHelperSearchHelper4_21031().do34();
                    this.presetFontsSearchHelper42.getList2318().do34();
                }));
                do1030(new PresetSearchHelper419_2(this, "Restore", () -> {
                    JsonElement jsonElement1434 = BaritoneHelper_3.presetHelper.getPresetHelper_374().getJsonElement1434();
                    if (jsonElement1434 != null) {
                        PresetHelperMode.ALL.fromJson(jsonElement1434);
                    }
                }));
                break;
            case EDIT:
                do1030(new PresetSearchHelper419(this, new ArgumentTypeHelper().getArgumentTypeHelper2919(this.presetSearchHelper4.getName()).getString2921("Preset \u0001")));
                do1030(new PresetSearchHelper419_2(this, "Load", () -> {
                    BaritoneHelper_3.presetHelper.getPresetHelper_374().do41();
                    getPresetHelperSearchHelper4_21031().is35(this.presetSearchHelper4.getName());
                }));
                do1030(new PresetSearchHelper419_2(this, "Save", () -> {
                    try {
                        getPresetHelperSearchHelper4_21031().do39(this.presetSearchHelper4.getName());
                        this.presetFontsSearchHelper42.getList2318().do34();
                    } catch (Exception e) {
                    }
                }));
                do1030(new PresetSearchHelper419_2(this, "Delete", () -> {
                    getPresetHelperSearchHelper4_21031().is36(this.presetSearchHelper4.getName());
                    do1034(PresetMode.NEW);
                }));
                do1030(new PresetSearchHelper419_2(this, "Back", () -> {
                    do1034(PresetMode.NEW);
                }));
                break;
        }
        this.presetFontsSearchHelper42.getList2318().do34();
    }

    public void do1030(PresetHelper_5 presetHelper_5) {
        this.registry.add(presetHelper_5);
    }

    public PresetHelperSearchHelper4_2 getPresetHelperSearchHelper4_21031() {
        return BaritoneHelper_3.presetHelper.getPresetHelperSearchHelper4_273(this.presetFontsSearchHelper42.getPresetHelperMode2320());
    }

    public void do1032(PresetSearchHelper4 presetSearchHelper4) {
        this.presetSearchHelper4 = presetSearchHelper4;
        do1034(PresetMode.EDIT);
    }

    public PresetSearchHelper4 getPresetSearchHelper41033() {
        if (this.presetMode != PresetMode.EDIT) {
            return null;
        }
        return this.presetSearchHelper4;
    }

    public void do1034(PresetMode presetMode) {
        this.presetMode = presetMode;
        this.flag = true;
    }
}
