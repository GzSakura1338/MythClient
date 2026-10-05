package me.mioclient;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/AutoCrystalMode_2.class */
public enum AutoCrystalMode_2 implements EnumSettingHelper {
    NONE("None"),
    NORMAL("Normal"),
    SILENT("Silent");

    public final String name;

    AutoCrystalMode_2(String str) {
        this.name = str;
    }

    @Override // me.mioclient.EnumSettingHelper
    public String getName() {
        return this.name;
    }
}
