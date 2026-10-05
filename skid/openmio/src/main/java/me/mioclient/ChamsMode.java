package me.mioclient;

import java.awt.Color;
import me.mioclient.module.render.Chams;
import net.minecraft.client.util.math.MatrixStack;
import net.minecraft.entity.Entity;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ChamsMode.class */
public enum ChamsMode implements EnumSettingHelper, ChamsHelper {
    BOTH("Both"),
    FILL("Fill") { // from class: me.mioclient.ChamsMode.Inner_3
        @Override // me.mioclient.ChamsHelper
        public Color[] getColorArray593(Chams chams) {
            return new Color[]{color, chams.fill.getValue()};
        }
    },
    LINE("Line") { // from class: me.mioclient.ChamsMode.Inner_2
        @Override // me.mioclient.ChamsHelper
        public Color[] getColorArray593(Chams chams) {
            return new Color[]{chams.outline.getValue(), color};
        }
    },
    PLAIN("Plain") { // from class: me.mioclient.ChamsMode.Inner
        @Override // me.mioclient.ChamsHelper
        public void do591(Chams chams, Entity entity, MatrixStack matrixStack) {
        }
    },
    OFF("Off");

    public final String name;

    ChamsMode(String str) {
        this.name = str;
    }

    @Override // me.mioclient.EnumSettingHelper
    public String getName() {
        return this.name;
    }

    @Override // me.mioclient.ChamsHelper
    public boolean is594() {
        return this != OFF;
    }
}
