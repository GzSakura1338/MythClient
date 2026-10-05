package me.mioclient.module.render;

import java.awt.Color;
import me.mioclient.EnumSettingHelper;
import me.mioclient.PhaseESPHelper;
import me.mioclient.SearchHelper4_7;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.module.Module;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/render/SkyColor.class */
public class SkyColor extends Module {
    public Setting<MixinClientWorldMode> type;
    public Setting<Boolean> dense;
    public Setting<Color> fog;
    public Setting<Color> sky;
    public Setting<Boolean> dimensions;
    public Setting<Boolean> overworld;
    public Setting<Boolean> nether;
    public Setting<Boolean> end;

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/module/render/SkyColor$MixinClientWorldMode.class */
    public enum MixinClientWorldMode implements EnumSettingHelper {
        NONE("None"),
        FLAT("Flat"),
        END("End");

        public final String name;

        MixinClientWorldMode(String str) {
            this.name = str;
        }

        @Override // me.mioclient.EnumSettingHelper
        public String getName() {
            return this.name;
        }
    }

    public SkyColor() {
        super("SkyColor", "Changes the fog color.", Category.RENDER, new String[0]);
        PhaseESPHelper.do1351(this);
        setDrawn(false);
        this.fog.do2343(color -> {
            return this.type.getValue() != MixinClientWorldMode.END;
        });
        this.sky.do2343(color2 -> {
            return this.type.getValue() != MixinClientWorldMode.END;
        });
        this.dense.do2343(bool -> {
            return this.type.getValue() == MixinClientWorldMode.FLAT || ((this.nether.getValue().booleanValue() || this.end.getValue().booleanValue()) && this.type.getValue() == MixinClientWorldMode.NONE);
        });
    }

    public boolean is3136() {
        switch (SearchHelper4_7.getStashFinderMode2438()) {
            case OVERWORLD:
                return this.overworld.getValue().booleanValue();
            case THE_NETHER:
                return this.nether.getValue().booleanValue();
            default:
                return this.end.getValue().booleanValue();
        }
    }
}
