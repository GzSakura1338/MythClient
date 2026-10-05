package me.mioclient;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Mode_6.class */
public enum Mode_6 implements EnumSettingHelper {
    NCP("NCP"),
    GRIM("Grim");

    public final String name;

    Mode_6(String str) {
        this.name = str;
    }

    @Override // me.mioclient.EnumSettingHelper
    public String getName() {
        return this.name;
    }
}
