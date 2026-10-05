package me.mioclient;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Mode_12.class */
public enum Mode_12 implements EnumSettingHelper {
    PLAIN("Plain"),
    GRIM("Grim"),
    GRIMV3("NoVelocity");

    public final String name;

    Mode_12(String str) {
        this.name = str;
    }

    @Override // me.mioclient.EnumSettingHelper
    public String getName() {
        return this.name;
    }
}
