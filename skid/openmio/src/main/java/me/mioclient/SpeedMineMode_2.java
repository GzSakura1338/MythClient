package me.mioclient;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/SpeedMineMode_2.class */
public enum SpeedMineMode_2 implements EnumSettingHelper {
    NONE("None"),
    FAST("Fast"),
    INSTANT("Instant");

    public final String name;

    SpeedMineMode_2(String str) {
        this.name = str;
    }

    @Override // me.mioclient.EnumSettingHelper
    public String getName() {
        return this.name;
    }
}
