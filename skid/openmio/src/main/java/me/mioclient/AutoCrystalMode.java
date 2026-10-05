package me.mioclient;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/AutoCrystalMode.class */
public enum AutoCrystalMode implements EnumSettingHelper {
    ALWAYS("Always"),
    STRICT("Strict"),
    NONE("None");

    public final String name;

    AutoCrystalMode(String str) {
        this.name = str;
    }

    @Override // me.mioclient.EnumSettingHelper
    public String getName() {
        return this.name;
    }
}
