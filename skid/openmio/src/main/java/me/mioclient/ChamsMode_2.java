package me.mioclient;

import java.awt.Color;
import me.mioclient.module.render.Chams;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ChamsMode_2.class */
public enum ChamsMode_2 implements EnumSettingHelper, ChamsHelper {
    BOTH("Both") { // from class: me.mioclient.ChamsMode_2.Inner
        @Override // me.mioclient.ChamsHelper
        public Color[] getColorArray593(Chams chams) {
            return new Color[]{chams.popLine.getValue(), chams.popFill.getValue()};
        }
    },
    FILL("Fill") { // from class: me.mioclient.ChamsMode_2.Inner_2
        @Override // me.mioclient.ChamsHelper
        public Color[] getColorArray593(Chams chams) {
            return new Color[]{color, chams.popFill.getValue()};
        }
    },
    LINE("Line") { // from class: me.mioclient.ChamsMode_2.Inner_3
        @Override // me.mioclient.ChamsHelper
        public Color[] getColorArray593(Chams chams) {
            return new Color[]{chams.popLine.getValue(), color};
        }
    };

    public final String name;

    ChamsMode_2(String str) {
        this.name = str;
    }

    @Override // me.mioclient.EnumSettingHelper
    public String getName() {
        return this.name;
    }
}
