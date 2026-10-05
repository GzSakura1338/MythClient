package me.mioclient;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/AutoCrystalMode_5.class */
public enum AutoCrystalMode_5 implements EnumSettingHelper {
    NONE("None"),
    STRICT("Strict"),
    STRONG("Strong");

    public final String name;

    AutoCrystalMode_5(String str) {
        this.name = str;
    }

    @Override // me.mioclient.EnumSettingHelper
    public String getName() {
        return this.name;
    }

    public boolean is1102() {
        return this != NONE;
    }
}
