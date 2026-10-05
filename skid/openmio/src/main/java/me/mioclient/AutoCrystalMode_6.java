package me.mioclient;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/AutoCrystalMode_6.class */
public enum AutoCrystalMode_6 implements EnumSettingHelper {
    MOTION("Motion"),
    SILENT("Silent");

    public final String name;

    AutoCrystalMode_6(String str) {
        this.name = str;
    }

    @Override // me.mioclient.EnumSettingHelper
    public String getName() {
        return this.name;
    }
}
