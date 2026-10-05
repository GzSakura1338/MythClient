package me.mioclient;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Mode_8.class */
public enum Mode_8 implements EnumSettingHelper {
    PLAIN("Plain"),
    BOLD("Bold"),
    ITALIC("Italic"),
    BOLDITALIC("BoldItalic");

    public final String name;

    Mode_8(String str) {
        this.name = str;
    }

    @Override // me.mioclient.EnumSettingHelper
    public String getName() {
        return this.name;
    }
}
