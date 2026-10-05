package me.mioclient;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/SpeedMineMode_4.class */
public enum SpeedMineMode_4 implements EnumSettingHelper {
    OFF("Off"),
    TICK("Tick"),
    SLOW("Slow");

    public final String name;

    SpeedMineMode_4(String str) {
        this.name = str;
    }

    @Override // me.mioclient.EnumSettingHelper
    public String getName() {
        return this.name;
    }
}
