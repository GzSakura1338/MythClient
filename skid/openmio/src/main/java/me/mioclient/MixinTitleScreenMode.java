package me.mioclient;

import java.awt.Color;
import me.mioclient.module.client.HUD;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/MixinTitleScreenMode.class */
public enum MixinTitleScreenMode implements EnumSettingHelper {
    NONE("None") { // from class: me.mioclient.MixinTitleScreenMode.Inner_3
        @Override // me.mioclient.MixinTitleScreenMode
        public Color getColor1911(HUD hud, float f) {
            if (!((ColorSetting) hud.setting7).is2862()) {
                return hud.setting7.getValue();
            }
            return ((ColorSetting) hud.setting7).getColor2857(hud.setting7.getValue(), (int) (f * hud.setting9.getValue().intValue() * MixinTitleScreenMode.get1912()));
        }
    },
    PULSE("Pulse") { // from class: me.mioclient.MixinTitleScreenMode.Inner_2
        @Override // me.mioclient.MixinTitleScreenMode
        public Color getColor1911(HUD hud, float f) {
            return MixinMessageIndicatorHelper_2.getColor814(hud.setting7.getValue(), hud.setting10.getValue(), 2000.0d, f * hud.setting9.getValue().intValue() * MixinTitleScreenMode.get1912());
        }
    },
    RAW("Raw") { // from class: me.mioclient.MixinTitleScreenMode.Inner
        @Override // me.mioclient.MixinTitleScreenMode
        public Color getColor1911(HUD hud, float f) {
            int intValue = hud.setting11.getValue().intValue();
            float intValue2 = 1.0f / hud.setting9.getValue().intValue();
            int currentTimeMillis = (int) (((float) ((System.currentTimeMillis() / hud.setting12.getValue().intValue()) % (intValue * hud.setting9.getValue().intValue()))) + (f / MixinTitleScreenMode.get1912()));
            int floor = (int) Math.floor(currentTimeMillis * intValue2);
            if (floor < 0) {
                floor = 0;
            }
            if (!hud.setting13.getValue().booleanValue()) {
                return hud.list.get(floor % intValue).getValue();
            }
            return MixinMessageIndicatorHelper_2.getColor815(hud.list.get(floor % intValue).getValue(), hud.list.get((floor + 1) % intValue).getValue(), 1.0f - ((currentTimeMillis * intValue2) - floor));
        }
    };

    public final String name;

    MixinTitleScreenMode(String str) {
        this.name = str;
    }

    @Override // me.mioclient.EnumSettingHelper
    public String getName() {
        return this.name;
    }

    public abstract Color getColor1911(HUD hud, float f);

    public static float get1912() {
        return 10.0f / (FontsSearchHelper4.fontsSearchHelper4.get93() + 2);
    }
}
