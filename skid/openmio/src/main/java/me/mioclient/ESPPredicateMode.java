package me.mioclient;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ESPPredicateMode.class */
public enum ESPPredicateMode implements EnumSettingHelper {
    BOX("Box"),
    TEXT("Text"),
    BOTH("Both");

    public final String name;

    ESPPredicateMode(String str) {
        this.name = str;
    }

    @Override // me.mioclient.EnumSettingHelper
    public String getName() {
        return this.name;
    }
}
