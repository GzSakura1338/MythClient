package me.mioclient;

import net.minecraft.util.math.Vec3d;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ChorusControlMode.class */
public enum ChorusControlMode implements SearchHelper_4, EnumSettingHelper {
    KEEP("Keep"),
    INVERT("Invert"),
    POS("Pos");

    public final String name;

    ChorusControlMode(String str) {
        this.name = str;
    }

    @Override // me.mioclient.EnumSettingHelper
    public String getName() {
        return this.name;
    }

    public float get607(Vec3d vec3d) {
        if (this == KEEP) {
            return minecraftClient.player.getYaw();
        }
        float f = SearchHelper4_8.getFloatArray2484(vec3d)[0];
        if (this == INVERT) {
            f += 180.0f;
        }
        return f;
    }
}
