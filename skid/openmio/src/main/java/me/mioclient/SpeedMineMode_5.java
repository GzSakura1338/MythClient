package me.mioclient;

import java.awt.Color;
import me.mioclient.module.player.SpeedMine;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/SpeedMineMode_5.class */
public enum SpeedMineMode_5 implements EnumSettingHelper {
    PROGRESS("Progress") { // from class: me.mioclient.SpeedMineMode_5.Inner
        @Override // me.mioclient.SpeedMineMode_5
        public Color[] getColorArray1773(SpeedMine speedMine, float f) {
            Color[] colorArray1773 = CUSTOM.getColorArray1773(speedMine, f);
            Color[] colorArr = {new Color(0.8f, 0.0f, 0.0f, colorArray1773[0].getAlpha() / 255.0f), new Color(0.8f, 0.0f, 0.0f, colorArray1773[1].getAlpha() / 255.0f)};
            if (f <= 0.8f) {
                return colorArr;
            }
            float f2 = (0.2f - (1.0f - f)) / 0.2f;
            float max = Math.max(0.8f - f2, 0.0f);
            return new Color[]{new Color(max, f2 * 0.8f, 0.0f, colorArray1773[0].getAlpha() / 255.0f), new Color(max, f2 * 0.8f, 0.0f, colorArray1773[1].getAlpha() / 255.0f)};
        }
    },
    CUSTOM("Custom") { // from class: me.mioclient.SpeedMineMode_5.Inner_2
        @Override // me.mioclient.SpeedMineMode_5
        public Color[] getColorArray1773(SpeedMine speedMine, float f) {
            return new Color[]{speedMine.fill.getValue(), speedMine.outline.getValue()};
        }
    };

    public final String name;

    SpeedMineMode_5(String str) {
        this.name = str;
    }

    @Override // me.mioclient.EnumSettingHelper
    public String getName() {
        return this.name;
    }

    public Color[] getColorArray1773(SpeedMine speedMine, float f) {
        return null;
    }
}
